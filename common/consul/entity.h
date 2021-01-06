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

#pragma once

#include "folly/dynamic.h"
#include "proxygen/lib/utils/Base64.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace service_framework {
namespace consul {


class Value {
 public:
  Value() = default;
  ~Value() = default;

  int64_t getCreateIndex() const { return create_index_; } 

  void setCreateIndex(int64_t create_index) { create_index_ = create_index; } 

  int64_t getModifyIndex() const { return modify_index_; } 

  void setModifyIndex(int64_t modify_index) { modify_index_ = modify_index; } 

  int64_t getLockIndex() const { return lock_index_; } 

  void setLockIndex(int64_t lock_index) { lock_index_ = lock_index; } 

  int64_t getFlags() const { return flags_; } 

  void setFlags(int64_t flags) { flags_ = flags; } 

  const std::string& getSession() const { return session_; } 

  void setSession(const std::string& session) { session_ = session; } 

  const std::string& getKey() const { return key_; } 

  void setKey(const std::string& key) { key_ = key; } 

  const std::string& getValue() const { return value_; } 

  void setValue(const std::string& value) { value_ = value; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  int64_t create_index_{0};
  int64_t modify_index_{0};
  int64_t lock_index_{0};
  int64_t flags_{0};
  std::string session_;
  std::string key_;
  std::string value_;
};

std::ostream& operator<<(std::ostream& os, const Value& value);

enum class AclType {
  CLIENT,
  MANAGEMENT
};

folly::dynamic serializeAclType(const AclType& value);

bool deserializeAclType(const folly::dynamic& data, AclType* value);

std::ostream& operator<<(std::ostream& os, const AclType& value);

folly::Optional<AclType> stringToAclType(const std::string& name);

const std::string toStringAclType(const AclType& value);

enum class CheckStatus {
  UNKNOWN,
  PASSING,
  WARNING,
  CRITICAL
};

folly::dynamic serializeCheckStatus(const CheckStatus& value);

bool deserializeCheckStatus(const folly::dynamic& data, CheckStatus* value);

std::ostream& operator<<(std::ostream& os, const CheckStatus& value);

folly::Optional<CheckStatus> stringToCheckStatus(const std::string& name);

const std::string toStringCheckStatus(const CheckStatus& value);

enum class SelfLogLevel {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERR
};

folly::dynamic serializeSelfLogLevel(const SelfLogLevel& value);

bool deserializeSelfLogLevel(const folly::dynamic& data, SelfLogLevel* value);

std::ostream& operator<<(std::ostream& os, const SelfLogLevel& value);


class NewAcl {
 public:
  NewAcl() = default;
  ~NewAcl() = default;

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const AclType& getType() const { return type_; } 

  void setType(const AclType& type) { type_ = type; } 

  const std::string& getRules() const { return rules_; } 

  void setRules(const std::string& rules) { rules_ = rules; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string name_;
  AclType type_;
  std::string rules_;
};

std::ostream& operator<<(std::ostream& os, const NewAcl& value);

class UpdateAcl {
 public:
  UpdateAcl() = default;
  ~UpdateAcl() = default;

  const std::string& getId() const { return id_; } 

  void setId(const std::string& id) { id_ = id; } 

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const AclType& getType() const { return type_; } 

  void setType(const AclType& type) { type_ = type; } 

  const std::string& getRules() const { return rules_; } 

  void setRules(const std::string& rules) { rules_ = rules; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string id_;
  std::string name_;
  AclType type_;
  std::string rules_;
};

std::ostream& operator<<(std::ostream& os, const UpdateAcl& value);

class AclInfo {
 public:
  AclInfo() = default;
  ~AclInfo() = default;

  int64_t getCreateIndex() const { return create_index_; } 

  void setCreateIndex(int64_t create_index) { create_index_ = create_index; } 

  int64_t getModifyIndex() const { return modify_index_; } 

  void setModifyIndex(int64_t modify_index) { modify_index_ = modify_index; } 

  const std::string& getId() const { return id_; } 

  void setId(const std::string& id) { id_ = id; } 

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const AclType& getType() const { return type_; } 

  void setType(const AclType& type) { type_ = type; } 

  const std::string& getRules() const { return rules_; } 

  void setRules(const std::string& rules) { rules_ = rules; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  int64_t create_index_{0};
  int64_t modify_index_{0};
  std::string id_;
  std::string name_;
  AclType type_;
  std::string rules_;
};

std::ostream& operator<<(std::ostream& os, const AclInfo& value);

class Check {
 public:
  Check() = default;
  ~Check() = default;

  const std::string& getNode() const { return node_; } 

  void setNode(const std::string& node) { node_ = node; } 

  const std::string& getCheckId() const { return check_id_; } 

  void setCheckId(const std::string& check_id) { check_id_ = check_id; } 

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const CheckStatus& getStatus() const { return status_; } 

  void setStatus(const CheckStatus& status) { status_ = status; } 

  const std::string& getNotes() const { return notes_; } 

  void setNotes(const std::string& notes) { notes_ = notes; } 

  const std::string& getOutput() const { return output_; } 

  void setOutput(const std::string& output) { output_ = output; } 

  const std::string& getServiceId() const { return service_id_; } 

  void setServiceId(const std::string& service_id) { service_id_ = service_id; } 

  const std::string& getServiceName() const { return service_name_; } 

  void setServiceName(const std::string& service_name) { service_name_ = service_name; } 

  const std::vector<std::string>& getServiceTags() const { return service_tags_; } 

  void setServiceTags(const std::vector<std::string>& service_tags) { service_tags_ = service_tags; } 

  int64_t getCreateIndex() const { return create_index_; } 

  void setCreateIndex(int64_t create_index) { create_index_ = create_index; } 

  int64_t getModifyIndex() const { return modify_index_; } 

  void setModifyIndex(int64_t modify_index) { modify_index_ = modify_index; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string node_;
  std::string check_id_;
  std::string name_;
  CheckStatus status_;
  std::string notes_;
  std::string output_;
  std::string service_id_;
  std::string service_name_;
  std::vector<std::string> service_tags_;
  int64_t create_index_{0};
  int64_t modify_index_{0};
};

std::ostream& operator<<(std::ostream& os, const Check& value);

class Service {
 public:
  Service() = default;
  ~Service() = default;

  const std::string& getId() const { return id_; } 

  void setId(const std::string& id) { id_ = id; } 

  const std::string& getService() const { return service_; } 

  void setService(const std::string& service) { service_ = service; } 

  const std::vector<std::string>& getTags() const { return tags_; } 

  void setTags(const std::vector<std::string>& tags) { tags_ = tags; } 

  const std::string& getAddress() const { return address_; } 

  void setAddress(const std::string& address) { address_ = address; } 

  int getPort() const { return port_; } 

  void setPort(int port) { port_ = port; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string id_;
  std::string service_;
  std::vector<std::string> tags_;
  std::string address_;
  int port_{0};
};

std::ostream& operator<<(std::ostream& os, const Service& value);

class Member {
 public:
  Member() = default;
  ~Member() = default;

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const std::string& getAddress() const { return address_; } 

  void setAddress(const std::string& address) { address_ = address; } 

  int getPort() const { return port_; } 

  void setPort(int port) { port_ = port; } 

  const std::unordered_map<std::string, std::string>& getTags() const { return tags_; } 

  void setTags(const std::unordered_map<std::string, std::string>& tags) { tags_ = tags; } 

  int getStatus() const { return status_; } 

  void setStatus(int status) { status_ = status; } 

  int getProtocolMin() const { return protocol_min_; } 

  void setProtocolMin(int protocol_min) { protocol_min_ = protocol_min; } 

  int getProtocolMax() const { return protocol_max_; } 

  void setProtocolMax(int protocol_max) { protocol_max_ = protocol_max; } 

  int getProtocolCur() const { return protocol_cur_; } 

  void setProtocolCur(int protocol_cur) { protocol_cur_ = protocol_cur; } 

  int getDelegateMin() const { return delegate_min_; } 

  void setDelegateMin(int delegate_min) { delegate_min_ = delegate_min; } 

  int getDelegateMax() const { return delegate_max_; } 

  void setDelegateMax(int delegate_max) { delegate_max_ = delegate_max; } 

  int getDelegateCur() const { return delegate_cur_; } 

  void setDelegateCur(int delegate_cur) { delegate_cur_ = delegate_cur; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string name_;
  std::string address_;
  int port_{0};
  std::unordered_map<std::string, std::string> tags_;
  int status_{0};
  int protocol_min_{0};
  int protocol_max_{0};
  int protocol_cur_{0};
  int delegate_min_{0};
  int delegate_max_{0};
  int delegate_cur_{0};
};

std::ostream& operator<<(std::ostream& os, const Member& value);

class SelfConfig {
 public:
  SelfConfig() = default;
  ~SelfConfig() = default;

  const std::string& getDatacenter() const { return datacenter_; } 

  void setDatacenter(const std::string& datacenter) { datacenter_ = datacenter; } 

  const std::string& getNodeName() const { return node_name_; } 

  void setNodeName(const std::string& node_name) { node_name_ = node_name; } 

  const std::string& getRevision() const { return revision_; } 

  void setRevision(const std::string& revision) { revision_ = revision; } 

  bool getServer() const { return server_; } 

  void setServer(bool server) { server_ = server; } 

  const std::string& getVersion() const { return version_; } 

  void setVersion(const std::string& version) { version_ = version; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string datacenter_;
  std::string node_name_;
  std::string revision_;
  bool server_;
  std::string version_;
};

std::ostream& operator<<(std::ostream& os, const SelfConfig& value);

class SelfDebugConfig {
 public:
  SelfDebugConfig() = default;
  ~SelfDebugConfig() = default;

  bool getBootstrap() const { return bootstrap_; } 

  void setBootstrap(bool bootstrap) { bootstrap_ = bootstrap; } 

  const std::string& getDataDir() const { return data_dir_; } 

  void setDataDir(const std::string& data_dir) { data_dir_ = data_dir; } 

  const std::string& getDnsRecursor() const { return dns_recursor_; } 

  void setDnsRecursor(const std::string& dns_recursor) { dns_recursor_ = dns_recursor; } 

  const std::string& getDnsDomain() const { return dns_domain_; } 

  void setDnsDomain(const std::string& dns_domain) { dns_domain_ = dns_domain; } 

  const SelfLogLevel& getLogLevel() const { return log_level_; } 

  void setLogLevel(const SelfLogLevel& log_level) { log_level_ = log_level; } 

  const std::string& getNodeId() const { return node_id_; } 

  void setNodeId(const std::string& node_id) { node_id_ = node_id; } 

  const std::vector<std::string>& getClientAddresses() const { return client_addresses_; } 

  void setClientAddresses(const std::vector<std::string>& client_addresses) { client_addresses_ = client_addresses; } 

  const std::string& getBindAddress() const { return bind_address_; } 

  void setBindAddress(const std::string& bind_address) { bind_address_ = bind_address; } 

  bool getLeaveOnTerm() const { return leave_on_term_; } 

  void setLeaveOnTerm(bool leave_on_term) { leave_on_term_ = leave_on_term; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  bool bootstrap_;
  std::string data_dir_;
  std::string dns_recursor_;
  std::string dns_domain_;
  SelfLogLevel log_level_;
  std::string node_id_;
  std::vector<std::string> client_addresses_;
  std::string bind_address_;
  bool leave_on_term_;
};

std::ostream& operator<<(std::ostream& os, const SelfDebugConfig& value);

class Self {
 public:
  Self() = default;
  ~Self() = default;

  const SelfConfig& getConfig() const { return config_; } 

  void setConfig(const SelfConfig& config) { config_ = config; } 

  const SelfDebugConfig& getDebugConfig() const { return debug_config_; } 

  void setDebugConfig(const SelfDebugConfig& debug_config) { debug_config_ = debug_config; } 

  const Member& getMember() const { return member_; } 

  void setMember(const Member& member) { member_ = member; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  SelfConfig config_;
  SelfDebugConfig debug_config_;
  Member member_;
};

std::ostream& operator<<(std::ostream& os, const Self& value);

class NewCheck {
 public:
  NewCheck() = default;
  ~NewCheck() = default;

  const std::string& getId() const { return id_; } 

  void setId(const std::string& id) { id_ = id; } 

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const std::string& getServiceId() const { return service_id_; } 

  void setServiceId(const std::string& service_id) { service_id_ = service_id; } 

  const std::string& getNotes() const { return notes_; } 

  void setNotes(const std::string& notes) { notes_ = notes; } 

  const std::string& getScript() const { return script_; } 

  void setScript(const std::string& script) { script_ = script; } 

  const std::string& getHttp() const { return http_; } 

  void setHttp(const std::string& http) { http_ = http; } 

  const std::string& getTcp() const { return tcp_; } 

  void setTcp(const std::string& tcp) { tcp_ = tcp; } 

  const std::string& getDockerContainerId() const { return docker_container_id_; } 

  void setDockerContainerId(const std::string& docker_container_id) { docker_container_id_ = docker_container_id; } 

  const std::string& getShell() const { return shell_; } 

  void setShell(const std::string& shell) { shell_ = shell; } 

  const std::string& getInterval() const { return interval_; } 

  void setInterval(const std::string& interval) { interval_ = interval; } 

  const std::string& getTimeout() const { return timeout_; } 

  void setTimeout(const std::string& timeout) { timeout_ = timeout; } 

  const std::string& getTtl() const { return ttl_; } 

  void setTtl(const std::string& ttl) { ttl_ = ttl; } 

  const std::string& getDeregisterCriticalServiceAfter() const { return deregister_critical_service_after_; } 

  void setDeregisterCriticalServiceAfter(const std::string& deregister_critical_service_after) { deregister_critical_service_after_ = deregister_critical_service_after; } 

  const std::string& getTlsSkipVerify() const { return tls_skip_verify_; } 

  void setTlsSkipVerify(const std::string& tls_skip_verify) { tls_skip_verify_ = tls_skip_verify; } 

  const std::string& getStatus() const { return status_; } 

  void setStatus(const std::string& status) { status_ = status; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string id_;
  std::string name_;
  std::string service_id_;
  std::string notes_;
  std::string script_;
  std::string http_;
  std::string tcp_;
  std::string docker_container_id_;
  std::string shell_;
  std::string interval_;
  std::string timeout_;
  std::string ttl_;
  std::string deregister_critical_service_after_;
  std::string tls_skip_verify_;
  std::string status_;
};

std::ostream& operator<<(std::ostream& os, const NewCheck& value);

class NewServiceCheck {
 public:
  NewServiceCheck() = default;
  ~NewServiceCheck() = default;

  const std::string& getScript() const { return script_; } 

  void setScript(const std::string& script) { script_ = script; } 

  const std::string& getInterval() const { return interval_; } 

  void setInterval(const std::string& interval) { interval_ = interval; } 

  const std::string& getTtl() const { return ttl_; } 

  void setTtl(const std::string& ttl) { ttl_ = ttl; } 

  const std::string& getHttp() const { return http_; } 

  void setHttp(const std::string& http) { http_ = http; } 

  const std::string& getTcp() const { return tcp_; } 

  void setTcp(const std::string& tcp) { tcp_ = tcp; } 

  const std::string& getTimeout() const { return timeout_; } 

  void setTimeout(const std::string& timeout) { timeout_ = timeout; } 

  const std::string& getDeregisterCriticalServiceAfter() const { return deregister_critical_service_after_; } 

  void setDeregisterCriticalServiceAfter(const std::string& deregister_critical_service_after) { deregister_critical_service_after_ = deregister_critical_service_after; } 

  const std::string& getTlsSkipVerify() const { return tls_skip_verify_; } 

  void setTlsSkipVerify(const std::string& tls_skip_verify) { tls_skip_verify_ = tls_skip_verify; } 

  const std::string& getStatus() const { return status_; } 

  void setStatus(const std::string& status) { status_ = status; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string script_;
  std::string interval_;
  std::string ttl_;
  std::string http_;
  std::string tcp_;
  std::string timeout_;
  std::string deregister_critical_service_after_;
  std::string tls_skip_verify_;
  std::string status_;
};

std::ostream& operator<<(std::ostream& os, const NewServiceCheck& value);

class NewService {
 public:
  NewService() = default;
  ~NewService() = default;

  const std::string& getId() const { return id_; } 

  void setId(const std::string& id) { id_ = id; } 

  const std::string& getName() const { return name_; } 

  void setName(const std::string& name) { name_ = name; } 

  const std::vector<std::string>& getTags() const { return tags_; } 

  void setTags(const std::vector<std::string>& tags) { tags_ = tags; } 

  const std::string& getAddress() const { return address_; } 

  void setAddress(const std::string& address) { address_ = address; } 

  int getPort() const { return port_; } 

  void setPort(int port) { port_ = port; } 

  bool getEnableTagOverride() const { return enable_tag_override_; } 

  void setEnableTagOverride(bool enable_tag_override) { enable_tag_override_ = enable_tag_override; } 

  const NewServiceCheck& getCheck() const { return check_; } 

  void setCheck(const NewServiceCheck& check) { check_ = check; } 

  const std::vector<NewServiceCheck>& getChecks() const { return checks_; } 

  void setChecks(const std::vector<NewServiceCheck>& checks) { checks_ = checks; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string id_;
  std::string name_;
  std::vector<std::string> tags_;
  std::string address_;
  int port_{0};
  bool enable_tag_override_;
  NewServiceCheck check_;
  std::vector<NewServiceCheck> checks_;
};

std::ostream& operator<<(std::ostream& os, const NewService& value);


}  // namespace consul
}  // namespace service_framework
