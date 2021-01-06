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

#include "http_server_manager.h"
#include "common/metrics/system_metrics.h"
#include "folly/Singleton.h"
#include "folly/portability/GFlags.h"
#include "glog/vlog_is_on.h"
#include "common/service_router/router.h"

DEFINE_int32(http_requests_bucket_size, 1, "Http server request time stat bucket size");
DEFINE_int32(http_requests_min, 0, "Http server request time stat min value");
DEFINE_int32(http_requests_max, 1000, "Http server request time stat max value");
DECLARE_string(vmodule);

namespace service_framework {
namespace http {

constexpr static char HTTP_STAT_MODULE_NAME[] = "service_framework.http";
constexpr static char HTTP_STAT_METRIC_NAME[] = "requests";
constexpr static char HTTP_STATUS_200[] = "OK";
constexpr static char HTTP_STATUS_503[] = "Bad Gateway";
constexpr static char HTTP_STATUS_404[] = "Not Found";
constexpr static char HTTP_SERVER_MODULE_SPLIT[] = ",";
constexpr static char HTTP_SERVER_MODULE_BLANK[] = " ";
constexpr static char HTTP_SERVER_STATUS[] = "/server/status";
constexpr static char HTTP_SERVER_PROMETHEUS[] = "/server/prometheus";
constexpr static char HTTP_SERVER_DISCOVER_SWITCH[] = "/server/discover_switch";
constexpr static char HTTP_SERVER_GLOGV_SET[] = "/glogv/set";
constexpr static char HTTP_SERVER_GLOGV_GET[] = "/glogv/get";
constexpr static char HTTP_SERVER_GLOGV_MODULE_GET[] = "/glogv/module/get";
constexpr static char HTTP_SERVER_GLOGV_MODULE_SET[] = "/glogv/module/set";
constexpr static char HTTP_SERVER_SUBSCRIBED[] = "/router/subscribed";
constexpr static char HTTP_SERVER_ROUTER_STATIC_SWITCH[] = "/router/static_switch";
constexpr static char HTTP_SERVER_ROUTER_PUT_NODES[] = "/router/put_nodes";
constexpr static char SERVER_STATE_AVAILABLE[] = "available";
constexpr static char SERVER_STATE_UNAVAILABLE[] = "unavailable";
constexpr static char ROUTER_USE_STATIC[] = "1";

folly::Singleton<HttpServerManager> globalHttpServerManager = folly::Singleton<HttpServerManager>().shouldEagerInit();
std::shared_ptr<HttpServerManager> HttpServerManager::getInstance() { return globalHttpServerManager.try_get(); }

void HttpServerManager::registerLocation(const std::string& location, LocationCallback callback,
                                         const proxygen::HTTPMethod& method) {
  std::shared_ptr<Location> location_info = std::make_shared<Location>(location, std::move(callback), method);
  locations_.wlock()->insert({location, location_info});
}

void HttpServerManager::unregisterLocation(const std::string& location) {
  locations_.withWLock([location](auto& locations) {
    if (locations.find(location) != locations.end()) {
      return;
    }

    locations.erase(location);
  });
}

folly::Optional<std::shared_ptr<Location>> HttpServerManager::getLocation(const std::string& location) {
  return locations_.withRLock([location](auto& locations) -> folly::Optional<std::shared_ptr<Location>> {
    if (locations.find(location) == locations.end()) {
      return folly::none;
    }

    return locations.at(location);
  });
}

void HttpHandlerFactory::onServerStart(folly::EventBase* /*evb*/) noexcept {
  timers_ = metrics::Metrics::getInstance()->buildTimers(HTTP_STAT_MODULE_NAME, HTTP_STAT_METRIC_NAME,
                                                         FLAGS_http_requests_bucket_size, FLAGS_http_requests_min,
                                                         FLAGS_http_requests_max);
}

void HttpHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
  header_ = std::move(headers);
  std::string location = header_->getPath();
  auto server_manager = HttpServerManager::getInstance();
  auto location_info = server_manager->getLocation(location);
  if (!location_info || (*location_info)->getMethod() != header_->getMethod()) {
    return processErrorResponse(404, HTTP_STATUS_404, header_->getPath());
  }
  is_valid_ = true;
}

void HttpHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
  if (body_) {
    body_->prependChain(std::move(body));
  } else {
    body_ = std::move(body);
  }
}

void HttpHandler::onEOM() noexcept {
  if (!is_valid_) {
    return;
  }

  proxygen::ResponseBuilder builder(downstream_);
  builder.status(200, HTTP_STATUS_200);
  ServerResponse response(builder);
  std::string url = header_->getURL();
  auto server_manager = HttpServerManager::getInstance();
  auto location_info = server_manager->getLocation(header_->getPath());
  if (!location_info) {
    return processErrorResponse(404, HTTP_STATUS_404, header_->getPath());
  }
  try {
    (*location_info)->process(&response, std::move(header_), std::move(body_));
  } catch (...) {
    return processErrorResponse(503, HTTP_STATUS_503, url);
  }

  builder.sendWithEOM();
}

void HttpHandler::processErrorResponse(uint16_t code, const std::string& reason, const std::string& path) {
  proxygen::ResponseBuilder(downstream_)
      .status(code, reason)
      .body(folly::to<std::string>("Could not parse server from URL: ", path))
      .sendWithEOM();
}

void HttpHandler::onUpgrade(proxygen::UpgradeProtocol /*protocol*/) noexcept {
  // handler doesn't support upgrades
}

void HttpHandler::requestComplete() noexcept { delete this; }

void HttpHandler::onError(proxygen::ProxygenError /*err*/) noexcept { delete this; }

const std::vector<std::string> getModulesName(const std::string& modules_pattern) {
  std::vector<std::string> ret;
  std::vector<std::string> each_pattern;
  folly::split(HTTP_SERVER_MODULE_SPLIT, modules_pattern, each_pattern, true);
  for (auto& each : each_pattern) {
    std::vector<std::string> module_level_pair;
    folly::split('=', each, module_level_pair, true);
    if (module_level_pair.size() == 2) {
      ret.emplace_back(module_level_pair[0]);
    }
    module_level_pair.clear();
  }
  return ret;
}

void initGlogvs(const std::string& host, uint16_t port, const std::string& service_name) {
  auto server_manager = service_framework::http::HttpServerManager::getInstance();
  server_manager->registerLocation(
      HTTP_SERVER_GLOGV_SET,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        folly::dynamic result = folly::dynamic::object;
        std::string response_result;
        auto param_level = folly::tryTo<int>(request->getQueryParam("vlog_level"));
        if (param_level.hasValue()) {
          FLAGS_v = param_level.value();
          result["vlog_level"] = FLAGS_v;
          result["msg"] = "glogv/set succ";
        } else {
          result["msg"] = "glogv/set invalid param";
        }
        common::toJson(&response_result, result);
        response->status(200, "OK").body(response_result).send();
      });

  server_manager->registerLocation(
      HTTP_SERVER_GLOGV_GET,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        folly::dynamic result = folly::dynamic::object;
        std::string response_result;
        result["vlog_level"] = FLAGS_v;
        result["msg"] = "/glogv/get";
        common::toJson(&response_result, result);
        response->status(200, "OK").body(response_result).send();
      });

  server_manager->registerLocation(
      HTTP_SERVER_GLOGV_MODULE_SET,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        folly::dynamic result = folly::dynamic::object;
        std::string response_result;
        const std::string& module_str = request->getQueryParam("module_pattern");
        auto log_level = folly::tryTo<int>(request->getQueryParam("level"));
        if (!module_str.empty() && log_level.hasValue()) {
          std::vector<std::string> modules_name = getModulesName(FLAGS_vmodule);
          bool found = false;
          for (auto& name : modules_name) {
            if (module_str == name) {
              found = true;
              break;
            }
          }
          if (found) {
            google::SetVLOGLevel(module_str.c_str(), log_level.value());
            result["msg"] = "module/set succ";
          } else {
            result["msg"] = "module name not exist";
          }
        } else {
          result["msg"] = "module/set invalid param";
        }
        common::toJson(&response_result, result);
        response->status(200, "OK").body(response_result).send();
      });

  server_manager->registerLocation(
      HTTP_SERVER_GLOGV_MODULE_GET,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        folly::dynamic result = folly::dynamic::object;
        std::string response_result;
        std::vector<std::string> modules_name = getModulesName(FLAGS_vmodule);
        std::string modules;
        for (auto& each : modules_name) {
          modules = folly::to<std::string>(modules, each, HTTP_SERVER_MODULE_BLANK);
        }
        result["module"] = folly::toJson(modules);
        result["msg"] = "module/get";
        common::toJson(&response_result, result);
        response->status(200, "OK").body(response_result).send();
      });
}

void initMetrics(const std::string& host, uint16_t port, const std::string& service_name) {
  auto server_manager = service_framework::http::HttpServerManager::getInstance();

  // 初始化系统指标统计
  metrics::SystemMetrics::getInstance()->init();

  server_manager->registerLocation(
      HTTP_SERVER_STATUS,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        auto status = service_router::Router::getInstance()->getStatus(service_name);
        std::string service_status = status.hasValue() ? service_router::toStringServerStatus(status.value()) : "";
        metrics::JsonTransformer json_export(host, port, service_name, service_status);
        response->body(json_export.transform()).send();
      });

  server_manager->registerLocation(
      HTTP_SERVER_PROMETHEUS,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        metrics::PrometheusTransformer prometheus_export(host, port, service_name);
        response->body(prometheus_export.transform()).send();
      });
  server_manager->registerLocation(HTTP_SERVER_DISCOVER_SWITCH,
                                   [](service_framework::http::ServerResponse* response,
                                      std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
                                     const std::string& service_name = request->getQueryParam("service_name");
                                     const std::string& state = request->getQueryParam("state");

                                     folly::dynamic result = folly::dynamic::object;
                                     std::string response_result;
                                     if (service_name.empty()) {
                                       result["code"] = 10001;
                                       result["msg"] = "Service name is empty.";
                                       common::toJson(&response_result, result);
                                       response->status(200, "OK").body(response_result).send();
                                       return;
                                     }
                                     service_router::ServerStatus status;
                                     if (state == SERVER_STATE_AVAILABLE) {
                                       status = service_router::ServerStatus::AVAILABLE;
                                     } else if (state == SERVER_STATE_UNAVAILABLE) {
                                       status = service_router::ServerStatus::UNAVAILABLE;
                                     } else {
                                       result["code"] = 10002;
                                       result["msg"] = "State must in (unavailable/available).";
                                       common::toJson(&response_result, result);
                                       response->status(200, "OK").body(response_result).send();
                                       return;
                                     }
                                     service_router::Router::getInstance()->setStatus(service_name, status);
                                     result["code"] = 0;
                                     result["msg"] = "Change state is success";
                                     result["state"] = service_router::serializeServerStatus(status);
                                     common::toJson(&response_result, result);
                                     response->status(200, "OK").body(response_result).send();
                                   });

  server_manager->registerLocation(
      HTTP_SERVER_SUBSCRIBED,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        auto router = service_router::Router::getInstance();
        std::vector<std::string> registries;
        router->getAllRegistryName(&registries);
        folly::dynamic dy_service_names = folly::dynamic::array;
        for (auto& registry : registries) {
          dy_service_names.push_back(registry);
        }
        resultJson(response, dy_service_names);
      });

  server_manager->registerLocation(
      HTTP_SERVER_ROUTER_STATIC_SWITCH,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request, std::unique_ptr<folly::IOBuf>) {
        auto router = service_router::Router::getInstance();
        const std::string& registry_name = request->getQueryParam("service_name");
        const std::string& use_static = request->getQueryParam("use_static");
        std::shared_ptr<service_router::RegistryInterface> registry = router->getRegistry(registry_name);
        if (registry == nullptr) {
          return errorJson(response, 10001,
                           folly::to<std::string>("Current node dose not registed in ", registry_name));
        }

        if (use_static == ROUTER_USE_STATIC) {
          registry->enableStatic();
        } else {
          registry->disableStatic();
        }

        folly::dynamic null;
        resultJson(response, null);
      });

  server_manager->registerLocation(
      HTTP_SERVER_ROUTER_PUT_NODES,
      [host, port, service_name](service_framework::http::ServerResponse* response,
                                 std::unique_ptr<proxygen::HTTPMessage> request,
                                 std::unique_ptr<folly::IOBuf> request_body) {
        auto router = service_router::Router::getInstance();
        const std::string& service_name = request->getQueryParam("service_name");
        std::shared_ptr<service_router::RegistryInterface> registry = router->getRegistry(service_name);
        if (service_name.empty()) {
          return errorJson(response, 10002, "Parameter error.");
        }
        if (registry == nullptr) {
          return errorJson(response, 10001, folly::to<std::string>("Current node dose not registed in ", service_name));
        }
        const folly::IOBuf* p = request_body.get();
        std::string body;
        if (p == nullptr) {
          return errorJson(response, 10002, "Post data is empty.");
        }
        do {
          body.append(reinterpret_cast<const char*>(p->data()), p->length());
          p = p->next();
        } while (p != request_body.get());

        folly::dynamic dy_servers;
        std::vector<service_router::Server> servers;

        if (!common::fromJson(&dy_servers, body) || !dy_servers.isArray()) {
          return errorJson(response, 10003, "Post data invalid.");
        }

        for (size_t i = 0; i < dy_servers.size(); i++) {
          service_router::Server server;
          if (!server.deserialize(dy_servers[i])) {
            return errorJson(response, 10003, "Json to object is fail.");
          }
          servers.emplace_back(std::move(server));
        }

        registry->pushStaticNodes(service_name, servers);
        folly::dynamic null;
        resultJson(response, null);
      },
      proxygen::HTTPMethod::POST);
}

void resultJson(ServerResponse* response, const folly::dynamic& data) { sendResponse(response, 0, "", data); }

void errorJson(ServerResponse* response, uint32_t code, const std::string& error) {
  folly::dynamic data = folly::dynamic::object;
  sendResponse(response, code, error, data);
}

void sendResponse(ServerResponse* response, uint32_t code, const std::string& error, const folly::dynamic& data) {
  folly::dynamic result = folly::dynamic::object;
  result.insert("Code", code);
  result.insert("Error", error);
  result.insert("Data", data);

  std::string response_body;
  if (common::toJson(&response_body, result)) {
    response->status(200, "OK").body(response_body).send();
  } else {
    response->status(500, "Server error").body("result object to json fail.").send();
  }
  return;
}

}  // namespace http
}  // namespace service_framework
