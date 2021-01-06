/*
 * Copyright 2020 Weibo Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author ZhongXiu Hao <nmred.hao@gmail.com>
 */

#include "folly/init/Init.h"
#include "folly/ScopeGuard.h"
#include "folly/Random.h"
#include "folly/executors/IOThreadPoolExecutor.h"
#include "folly/stats/Histogram.h"

#include "common/service_router/thrift.h"
#include "common/service_router/http.h"

#include "geo/if/gen-cpp2/GeoService.h"

DEFINE_string(host, "127.0.0.1", "current service host address");
DEFINE_int32(port, 0, "current service port");
DEFINE_string(target_service_name, "geo", "Search geo service name");
DEFINE_string(service_name, "geo_client", "Current geo client service name");
DEFINE_bool(block, true, "Is block runing..");
DEFINE_int32(numbers, 10, "Send geo server numbers.");
DEFINE_string(search_ip, "192.168.105.102", "Search geo ip address.");
DEFINE_int32(num_clients, 16, "Number of clients to use. (Default: 1 per core)");
DEFINE_int32(io_threads, 0, "Number of io perform thread to use. (Default: cup cores)");
DEFINE_bool(use_router, true, "Use service router");
DEFINE_bool(print, false, "Print result to stdout");
DEFINE_bool(future, false, "Send data by future");
DEFINE_string(target_server_addresses, "127.0.0.1:7777", "Direct specified client address list");
DEFINE_bool(is_kv, true, "Test kv store");
DEFINE_string(key_prefix, "key_test_", "rocksdb test key prefix");
DEFINE_int32(key_numbers, 100000, "key items size");
DEFINE_int32(sleep_ms, 0, "each request sleep time");
DEFINE_int32(request_timeout, 10, "each request send timeout");
DEFINE_int32(rpc_request_timeout, 10, "each request recv timeout");
DEFINE_int32(diff_range, 256, "Address diff range");
DEFINE_string(load_balance_method, "random",
              "request load balance method `random/roundrobin/localfirst/configurable_weight`");
DEFINE_int32(max_conn_per_server, 0, "Max connection pre server.");

class GeoClient {
 public:
  GeoClient(const std::string& target_server_addresses, bool use_router, const std::string& target_service_name,
            bool print)
      : target_server_addresses_(target_server_addresses),
        use_router_(use_router),
        service_name_(target_service_name),
        print_(print) {}

  GeoClient(const GeoClient&) = delete;
  GeoClient& operator=(const GeoClient&) = delete;
  GeoClient(GeoClient&&) = default;
  ~GeoClient() = default;

  void request(const geo::IpSearchRequest& req) {
    for (int i = 0; i < FLAGS_numbers; i++) {
      if (FLAGS_sleep_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_sleep_ms));
      }
      //syncRequest(req);
      asyncRequest(req);
    }
    LOG(INFO) << "Complete..";
  }

  void print(geo::IpSearchResponse result) {
    if (print_) {
      std::cout << result.get_code() << std::endl;
      std::cout << result.get_city() << std::endl;
    }
  }

 private:
  std::string target_server_addresses_;
  bool use_router_;
  std::string service_name_;
  bool print_;

  folly::Optional<service_router::ClientOption> getDirectConnectionOption() {
    service_router::ClientOption option;
    service_router::ServerAddress address;
    std::vector<std::string> addresses;
    folly::split(',', target_server_addresses_, addresses);
    if (addresses.empty()) {
      VLOG(4) << "Direct specified client address list is invalid, addresses:" << target_server_addresses_;
      return folly::none;
    }
    std::random_shuffle(addresses.begin(), addresses.end());
    std::string pick_address = addresses[0];
    std::string host;
    uint16_t port = 0;
    folly::split(':', pick_address, host, port);
    if (port <= 0) {
      return folly::none;
    }
    address.setHost(host);
    address.setPort(port);
    option.setTargetServerAddress(address);
    option.setMaxConnPerServer(FLAGS_max_conn_per_server);
    return option;
  }

  const service_router::ClientOption getClientOption() {
    service_router::ClientOption option;
    if (!use_router_) {
      auto conn_option = getDirectConnectionOption();
      if (conn_option) {
        option = *conn_option;
      }
    } else {
      option.setServiceName(service_name_);
      option.setProtocol(service_router::ServerProtocol::THRIFT);
      option.setTimeoutMs(FLAGS_request_timeout);
      option.setMaxConnPerServer(FLAGS_max_conn_per_server);
      auto method = service_router::stringToLoadBalanceMethod(FLAGS_load_balance_method);
      service_router::LoadBalanceMethod load_method = service_router::LoadBalanceMethod::RANDOM;
      if (!method) {
        FB_LOG_EVERY_MS(ERROR, 1000) << "Specified load balance method is invalid, default is random";
      } else {
        load_method = *method;
      }
      option.setLoadBalance(load_method);
      service_router::BalanceLocalFirstConfig local_first;
      local_first.setLocalIp(FLAGS_host);
      local_first.setDiffRange(FLAGS_diff_range);
      option.setLocalFirstConfig(local_first);
    }
    return option;
  }

  void syncRequest(const geo::IpSearchRequest& req) {
    service_router::thriftServiceCall<geo::GeoServiceAsyncClient>(getClientOption(), [&req, this](auto client) {
      geo::IpSearchResponse result;
      apache::thrift::RpcOptions rpc_options;
      rpc_options.setTimeout(std::chrono::milliseconds(FLAGS_rpc_request_timeout));
      client->sync_ipSearch(rpc_options, result, req);
      print(result);
    });
  }

  void asyncRequest(const geo::IpSearchRequest& req) {
    service_router::thriftServiceCall<geo::GeoServiceAsyncClient>(getClientOption(), [&req, this](auto client) {
      apache::thrift::RpcOptions rpc_options;
      rpc_options.setTimeout(std::chrono::milliseconds(FLAGS_rpc_request_timeout));
      auto f = client->future_ipSearch(rpc_options, req);
      std::move(f).then([this](folly::Try<::geo::IpSearchResponse>&& t) {
        if (t.hasException()) {
          try {
            t.exception().throw_exception();
          }
          catch (apache::thrift::transport::TTransportException& ex) {
          }
          catch (apache::thrift::TApplicationException& ex) {
          }
          catch (...) {
          }
        } else {
        }
        print(t.value());
      }).get();
    });
  }
};

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;
  folly::Init init(&argc, &argv);

  if (FLAGS_io_threads == 0) {
    FLAGS_io_threads = std::thread::hardware_concurrency();
  }
  auto io_pool = std::make_shared<folly::IOThreadPoolExecutor>(
      FLAGS_io_threads, std::make_shared<folly::NamedThreadFactory>("IOThreadPool"));
  folly::setIOExecutor(io_pool);

  SCOPE_EXIT {
    service_router::unregisterAll();
    service_framework::http::stop();
    service_router::stop_connection_pool();
    io_pool->stop();
  };

  GeoClient geo_client(FLAGS_target_server_addresses, FLAGS_use_router, FLAGS_target_service_name, FLAGS_print);
  std::vector<std::thread> threads;
  std::vector<geo::IpSearchRequest> requests(FLAGS_num_clients);
  for (int i = 0; i < FLAGS_num_clients; ++i) {
    requests[i].set_ip(FLAGS_search_ip);
    threads.push_back(std::thread(std::bind(&GeoClient::request, &geo_client, requests[i])));
  }

  std::thread http_server_thread([FLAGS_service_name, FLAGS_host, FLAGS_port]() {
    service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_port);
  });
  http_server_thread.join();
  for (auto& thr : threads) {
    thr.join();
  }
  return 0;
}
