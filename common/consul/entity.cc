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

#include "entity.h"

namespace service_framework {
namespace consul {


void Value::describe(std::ostream& os) const {
  os << "Value{"
     << "CreateIndex=" << create_index_
     << ", " << "ModifyIndex=" << modify_index_
     << ", " << "LockIndex=" << lock_index_
     << ", " << "Flags=" << flags_
     << ", " << "Session='" << session_ << "'"
     << ", " << "Key='" << key_ << "'"
     << ", " << "Value='" << value_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Value& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Value::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("CreateIndex", create_index_);
  result.insert("ModifyIndex", modify_index_);
  result.insert("LockIndex", lock_index_);
  result.insert("Flags", flags_);
  result.insert("Session", session_);
  result.insert("Key", key_);
  result.insert("Value", value_);

  return result;
}

bool Value::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* create_index = data.get_ptr("CreateIndex");
  if (create_index == nullptr || !create_index->isInt()) {
    return false;
  }
  setCreateIndex(create_index->asInt());
  auto* modify_index = data.get_ptr("ModifyIndex");
  if (modify_index == nullptr || !modify_index->isInt()) {
    return false;
  }
  setModifyIndex(modify_index->asInt());
  auto* lock_index = data.get_ptr("LockIndex");
  if (lock_index == nullptr || !lock_index->isInt()) {
    return false;
  }
  setLockIndex(lock_index->asInt());
  auto* flags = data.get_ptr("Flags");
  if (flags == nullptr || !flags->isInt()) {
    return false;
  }
  setFlags(flags->asInt());
  auto* session = data.get_ptr("Session");
  if (session && session->isString()) {
    setSession(session->asString());
  }
  auto* key = data.get_ptr("Key");
  if (key == nullptr || !key->isString()) {
    return false;
  }
  setKey(key->asString());
  auto* value = data.get_ptr("Value");
  if (value == nullptr || !value->isString()) {
    return false;
  }
  setValue(value->asString());

  return true;
}

std::ostream& operator<<(std::ostream& os, const AclType& value) {
  switch(value) {
    case AclType::CLIENT:
      os << "client";
      break;
    case AclType::MANAGEMENT:
      os << "management";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeAclType(const AclType& value) {
  folly::dynamic result = toStringAclType(value);
  return result;
}

bool deserializeAclType(const folly::dynamic& data, AclType* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToAclType(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<AclType> stringToAclType(const std::string& name) {
  if (name == "client") {
    return AclType::CLIENT;
  }
  if (name == "management") {
    return AclType::MANAGEMENT;
  }
     
  return folly::none;
}

const std::string toStringAclType(const AclType& value) {
  std::string result;
  switch(value) {
    case AclType::CLIENT:
      result = "client";
      break;
    case AclType::MANAGEMENT:
      result = "management";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const CheckStatus& value) {
  switch(value) {
    case CheckStatus::UNKNOWN:
      os << "unknown";
      break;
    case CheckStatus::PASSING:
      os << "passing";
      break;
    case CheckStatus::WARNING:
      os << "warning";
      break;
    case CheckStatus::CRITICAL:
      os << "critical";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeCheckStatus(const CheckStatus& value) {
  folly::dynamic result = toStringCheckStatus(value);
  return result;
}

bool deserializeCheckStatus(const folly::dynamic& data, CheckStatus* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToCheckStatus(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<CheckStatus> stringToCheckStatus(const std::string& name) {
  if (name == "unknown") {
    return CheckStatus::UNKNOWN;
  }
  if (name == "passing") {
    return CheckStatus::PASSING;
  }
  if (name == "warning") {
    return CheckStatus::WARNING;
  }
  if (name == "critical") {
    return CheckStatus::CRITICAL;
  }
     
  return folly::none;
}

const std::string toStringCheckStatus(const CheckStatus& value) {
  std::string result;
  switch(value) {
    case CheckStatus::UNKNOWN:
      result = "unknown";
      break;
    case CheckStatus::PASSING:
      result = "passing";
      break;
    case CheckStatus::WARNING:
      result = "warning";
      break;
    case CheckStatus::CRITICAL:
      result = "critical";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const SelfLogLevel& value) {
  os << static_cast<int>(value);
  return os;
}

folly::dynamic serializeSelfLogLevel(const SelfLogLevel& value) {
  folly::dynamic result = static_cast<int>(value);
  return result;
}

bool deserializeSelfLogLevel(const folly::dynamic& data, SelfLogLevel* result) {
  if (!data.isInt()) {
    return false;
  }

  int value = data.asInt();
  switch(value) {
    case 0:
      *result = SelfLogLevel::TRACE;
      break;
    case 1:
      *result = SelfLogLevel::DEBUG;
      break;
    case 2:
      *result = SelfLogLevel::INFO;
      break;
    case 3:
      *result = SelfLogLevel::WARN;
      break;
    case 4:
      *result = SelfLogLevel::ERR;
      break;
    default:
      return false;
  }

  return true;
}

void NewAcl::describe(std::ostream& os) const {
  os << "NewAcl{"
     << "Name='" << name_ << "'"
     << ", " << "Type=" << type_
     << ", " << "Rules='" << rules_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NewAcl& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NewAcl::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Name", name_);
  folly::dynamic type = serializeAclType(type_);
  result.insert("Type", type);
  result.insert("Rules", rules_);

  return result;
}

bool NewAcl::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* type = data.get_ptr("Type");
  if (type == nullptr) {
    return false;
  }

  AclType itemtype;
  if (!deserializeAclType(*type, &itemtype)) {
    return false;
  }
  setType(itemtype);
  auto* rules = data.get_ptr("Rules");
  if (rules == nullptr || !rules->isString()) {
    return false;
  }
  setRules(rules->asString());

  return true;
}

void UpdateAcl::describe(std::ostream& os) const {
  os << "UpdateAcl{"
     << "Id='" << id_ << "'"
     << ", " << "Name='" << name_ << "'"
     << ", " << "Type=" << type_
     << ", " << "Rules='" << rules_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const UpdateAcl& value) {
  value.describe(os);
  return os;
}

const folly::dynamic UpdateAcl::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Id", id_);
  result.insert("Name", name_);
  folly::dynamic type = serializeAclType(type_);
  result.insert("Type", type);
  result.insert("Rules", rules_);

  return result;
}

bool UpdateAcl::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* id = data.get_ptr("Id");
  if (id == nullptr || !id->isString()) {
    return false;
  }
  setId(id->asString());
  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* type = data.get_ptr("Type");
  if (type == nullptr) {
    return false;
  }

  AclType itemtype;
  if (!deserializeAclType(*type, &itemtype)) {
    return false;
  }
  setType(itemtype);
  auto* rules = data.get_ptr("Rules");
  if (rules == nullptr || !rules->isString()) {
    return false;
  }
  setRules(rules->asString());

  return true;
}

void AclInfo::describe(std::ostream& os) const {
  os << "AclInfo{"
     << "CreateIndex=" << create_index_
     << ", " << "ModifyIndex=" << modify_index_
     << ", " << "Id='" << id_ << "'"
     << ", " << "Name='" << name_ << "'"
     << ", " << "Type=" << type_
     << ", " << "Rules='" << rules_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const AclInfo& value) {
  value.describe(os);
  return os;
}

const folly::dynamic AclInfo::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("CreateIndex", create_index_);
  result.insert("ModifyIndex", modify_index_);
  result.insert("Id", id_);
  result.insert("Name", name_);
  folly::dynamic type = serializeAclType(type_);
  result.insert("Type", type);
  result.insert("Rules", rules_);

  return result;
}

bool AclInfo::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* create_index = data.get_ptr("CreateIndex");
  if (create_index == nullptr || !create_index->isInt()) {
    return false;
  }
  setCreateIndex(create_index->asInt());
  auto* modify_index = data.get_ptr("ModifyIndex");
  if (modify_index == nullptr || !modify_index->isInt()) {
    return false;
  }
  setModifyIndex(modify_index->asInt());
  auto* id = data.get_ptr("Id");
  if (id == nullptr || !id->isString()) {
    return false;
  }
  setId(id->asString());
  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* type = data.get_ptr("Type");
  if (type == nullptr) {
    return false;
  }

  AclType itemtype;
  if (!deserializeAclType(*type, &itemtype)) {
    return false;
  }
  setType(itemtype);
  auto* rules = data.get_ptr("Rules");
  if (rules == nullptr || !rules->isString()) {
    return false;
  }
  setRules(rules->asString());

  return true;
}

void Check::describe(std::ostream& os) const {
  os << "Check{"
     << "Node='" << node_ << "'"
     << ", " << "CheckId='" << check_id_ << "'"
     << ", " << "Name='" << name_ << "'"
     << ", " << "Status=" << status_
     << ", " << "Notes='" << notes_ << "'"
     << ", " << "Output='" << output_ << "'"
     << ", " << "ServiceId='" << service_id_ << "'"
     << ", " << "ServiceName='" << service_name_ << "'"
     << ", " << "ServiceTags=[";
  for (auto& t : service_tags_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "CreateIndex=" << create_index_
     << ", " << "ModifyIndex=" << modify_index_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Check& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Check::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Node", node_);
  result.insert("CheckId", check_id_);
  result.insert("Name", name_);
  folly::dynamic status = serializeCheckStatus(status_);
  result.insert("Status", status);
  result.insert("Notes", notes_);
  result.insert("Output", output_);
  result.insert("ServiceId", service_id_);
  result.insert("ServiceName", service_name_);
  folly::dynamic service_tags = folly::dynamic::array;
  for (auto& t : service_tags_) {
    service_tags.push_back(t);
  }
  result.insert("ServiceTags", service_tags);
  result.insert("CreateIndex", create_index_);
  result.insert("ModifyIndex", modify_index_);

  return result;
}

bool Check::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* node = data.get_ptr("Node");
  if (node == nullptr || !node->isString()) {
    return false;
  }
  setNode(node->asString());
  auto* check_id = data.get_ptr("CheckId");
  if (check_id == nullptr || !check_id->isString()) {
    return false;
  }
  setCheckId(check_id->asString());
  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* status = data.get_ptr("Status");
  if (status == nullptr) {
    return false;
  }

  CheckStatus itemstatus;
  if (!deserializeCheckStatus(*status, &itemstatus)) {
    return false;
  }
  setStatus(itemstatus);
  auto* notes = data.get_ptr("Notes");
  if (notes == nullptr || !notes->isString()) {
    return false;
  }
  setNotes(notes->asString());
  auto* output = data.get_ptr("Output");
  if (output == nullptr || !output->isString()) {
    return false;
  }
  setOutput(output->asString());
  auto* service_id = data.get_ptr("ServiceId");
  if (service_id == nullptr || !service_id->isString()) {
    return false;
  }
  setServiceId(service_id->asString());
  auto* service_name = data.get_ptr("ServiceName");
  if (service_name == nullptr || !service_name->isString()) {
    return false;
  }
  setServiceName(service_name->asString());
  auto* service_tags = data.get_ptr("ServiceTags");
  if (service_tags == nullptr || !service_tags->isArray()) {
    return false;
  }

  std::vector<std::string> vec_service_tags;
  for (size_t i = 0; i < service_tags->size(); i++) {
    if (!service_tags->at(i).isString()) {
      return false;
    }
    vec_service_tags.push_back(service_tags->at(i).asString());
  }
  setServiceTags(vec_service_tags);
  auto* create_index = data.get_ptr("CreateIndex");
  if (create_index == nullptr || !create_index->isInt()) {
    return false;
  }
  setCreateIndex(create_index->asInt());
  auto* modify_index = data.get_ptr("ModifyIndex");
  if (modify_index == nullptr || !modify_index->isInt()) {
    return false;
  }
  setModifyIndex(modify_index->asInt());

  return true;
}

void Service::describe(std::ostream& os) const {
  os << "Service{"
     << "Id='" << id_ << "'"
     << ", " << "Service='" << service_ << "'"
     << ", " << "Tags=[";
  for (auto& t : tags_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "Address='" << address_ << "'"
     << ", " << "Port=" << port_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Service& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Service::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Id", id_);
  result.insert("Service", service_);
  folly::dynamic tags = folly::dynamic::array;
  for (auto& t : tags_) {
    tags.push_back(t);
  }
  result.insert("Tags", tags);
  result.insert("Address", address_);
  result.insert("Port", port_);

  return result;
}

bool Service::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* id = data.get_ptr("Id");
  if (id == nullptr || !id->isString()) {
    return false;
  }
  setId(id->asString());
  auto* service = data.get_ptr("Service");
  if (service == nullptr || !service->isString()) {
    return false;
  }
  setService(service->asString());
  auto* tags = data.get_ptr("Tags");
  if (tags == nullptr || !tags->isArray()) {
    return false;
  }

  std::vector<std::string> vec_tags;
  for (size_t i = 0; i < tags->size(); i++) {
    if (!tags->at(i).isString()) {
      return false;
    }
    vec_tags.push_back(tags->at(i).asString());
  }
  setTags(vec_tags);
  auto* address = data.get_ptr("Address");
  if (address == nullptr || !address->isString()) {
    return false;
  }
  setAddress(address->asString());
  auto* port = data.get_ptr("Port");
  if (port == nullptr || !port->isInt()) {
    return false;
  }
  setPort(port->asInt());

  return true;
}

void Member::describe(std::ostream& os) const {
  os << "Member{"
     << "Name='" << name_ << "'"
     << ", " << "Address='" << address_ << "'"
     << ", " << "Port=" << port_
     << ", " << "Tags={";
  for (auto& t : tags_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "Status=" << status_
     << ", " << "ProtocolMin=" << protocol_min_
     << ", " << "ProtocolMax=" << protocol_max_
     << ", " << "ProtocolCur=" << protocol_cur_
     << ", " << "DelegateMin=" << delegate_min_
     << ", " << "DelegateMax=" << delegate_max_
     << ", " << "DelegateCur=" << delegate_cur_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Member& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Member::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Name", name_);
  result.insert("Addr", address_);
  result.insert("Port", port_);
  folly::dynamic tags = folly::dynamic::object;
  for (auto& t : tags_) {
    tags.insert(t.first, t.second);
  }
  result.insert("Tags", tags);
  result.insert("Status", status_);
  result.insert("ProtocolMin", protocol_min_);
  result.insert("ProtocolMax", protocol_max_);
  result.insert("ProtocolCur", protocol_cur_);
  result.insert("DelegateMin", delegate_min_);
  result.insert("DelegateMax", delegate_max_);
  result.insert("DelegateCur", delegate_cur_);

  return result;
}

bool Member::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* address = data.get_ptr("Addr");
  if (address == nullptr || !address->isString()) {
    return false;
  }
  setAddress(address->asString());
  auto* port = data.get_ptr("Port");
  if (port == nullptr || !port->isInt()) {
    return false;
  }
  setPort(port->asInt());
  auto* tags = data.get_ptr("Tags");
  if (tags == nullptr || !tags->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_tags;
  for (auto iter = tags->keys().begin(); iter != tags->keys().end(); iter++) {
    if (!tags->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_tags.insert({iter->asString(), tags->at(*iter).asString()});
  }
  setTags(map_tags);
  auto* status = data.get_ptr("Status");
  if (status == nullptr || !status->isInt()) {
    return false;
  }
  setStatus(status->asInt());
  auto* protocol_min = data.get_ptr("ProtocolMin");
  if (protocol_min == nullptr || !protocol_min->isInt()) {
    return false;
  }
  setProtocolMin(protocol_min->asInt());
  auto* protocol_max = data.get_ptr("ProtocolMax");
  if (protocol_max == nullptr || !protocol_max->isInt()) {
    return false;
  }
  setProtocolMax(protocol_max->asInt());
  auto* protocol_cur = data.get_ptr("ProtocolCur");
  if (protocol_cur == nullptr || !protocol_cur->isInt()) {
    return false;
  }
  setProtocolCur(protocol_cur->asInt());
  auto* delegate_min = data.get_ptr("DelegateMin");
  if (delegate_min == nullptr || !delegate_min->isInt()) {
    return false;
  }
  setDelegateMin(delegate_min->asInt());
  auto* delegate_max = data.get_ptr("DelegateMax");
  if (delegate_max == nullptr || !delegate_max->isInt()) {
    return false;
  }
  setDelegateMax(delegate_max->asInt());
  auto* delegate_cur = data.get_ptr("DelegateCur");
  if (delegate_cur == nullptr || !delegate_cur->isInt()) {
    return false;
  }
  setDelegateCur(delegate_cur->asInt());

  return true;
}

void SelfConfig::describe(std::ostream& os) const {
  os << "SelfConfig{"
     << "Datacenter='" << datacenter_ << "'"
     << ", " << "NodeName='" << node_name_ << "'"
     << ", " << "Revision='" << revision_ << "'"
     << ", " << "Server=" << server_
     << ", " << "Version='" << version_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const SelfConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic SelfConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Datacenter", datacenter_);
  result.insert("NodeName", node_name_);
  result.insert("Revision", revision_);
  result.insert("Server", server_);
  result.insert("Version", version_);

  return result;
}

bool SelfConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* datacenter = data.get_ptr("Datacenter");
  if (datacenter == nullptr || !datacenter->isString()) {
    return false;
  }
  setDatacenter(datacenter->asString());
  auto* node_name = data.get_ptr("NodeName");
  if (node_name == nullptr || !node_name->isString()) {
    return false;
  }
  setNodeName(node_name->asString());
  auto* revision = data.get_ptr("Revision");
  if (revision == nullptr || !revision->isString()) {
    return false;
  }
  setRevision(revision->asString());
  auto* server = data.get_ptr("Server");
  if (server == nullptr || !server->isBool()) {
    return false;
  }
  setServer(server->asBool());
  auto* version = data.get_ptr("Version");
  if (version == nullptr || !version->isString()) {
    return false;
  }
  setVersion(version->asString());

  return true;
}

void SelfDebugConfig::describe(std::ostream& os) const {
  os << "SelfDebugConfig{"
     << "Bootstrap=" << bootstrap_
     << ", " << "DataDir='" << data_dir_ << "'"
     << ", " << "DnsRecursor='" << dns_recursor_ << "'"
     << ", " << "DnsDomain='" << dns_domain_ << "'"
     << ", " << "LogLevel=" << log_level_
     << ", " << "NodeId='" << node_id_ << "'"
     << ", " << "ClientAddresses=[";
  for (auto& t : client_addresses_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "BindAddress='" << bind_address_ << "'"
     << ", " << "LeaveOnTerm=" << leave_on_term_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const SelfDebugConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic SelfDebugConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Bootstrap", bootstrap_);
  result.insert("DataDir", data_dir_);
  result.insert("DnsRecursor", dns_recursor_);
  result.insert("DnsDomain", dns_domain_);
  folly::dynamic log_level = serializeSelfLogLevel(log_level_);
  result.insert("LogLevel", log_level);
  result.insert("NodeId", node_id_);
  folly::dynamic client_addresses = folly::dynamic::array;
  for (auto& t : client_addresses_) {
    client_addresses.push_back(t);
  }
  result.insert("ClientAddresses", client_addresses);
  result.insert("BindAddress", bind_address_);
  result.insert("LeaveOnTerm", leave_on_term_);

  return result;
}

bool SelfDebugConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* bootstrap = data.get_ptr("Bootstrap");
  if (bootstrap == nullptr || !bootstrap->isBool()) {
    return false;
  }
  setBootstrap(bootstrap->asBool());
  auto* data_dir = data.get_ptr("DataDir");
  if (data_dir == nullptr || !data_dir->isString()) {
    return false;
  }
  setDataDir(data_dir->asString());
  auto* dns_recursor = data.get_ptr("DnsRecursor");
  if (dns_recursor == nullptr || !dns_recursor->isString()) {
    return false;
  }
  setDnsRecursor(dns_recursor->asString());
  auto* dns_domain = data.get_ptr("DnsDomain");
  if (dns_domain == nullptr || !dns_domain->isString()) {
    return false;
  }
  setDnsDomain(dns_domain->asString());
  auto* log_level = data.get_ptr("LogLevel");
  if (log_level == nullptr) {
    return false;
  }

  SelfLogLevel itemlog_level;
  if (!deserializeSelfLogLevel(*log_level, &itemlog_level)) {
    return false;
  }
  setLogLevel(itemlog_level);
  auto* node_id = data.get_ptr("NodeId");
  if (node_id == nullptr || !node_id->isString()) {
    return false;
  }
  setNodeId(node_id->asString());
  auto* client_addresses = data.get_ptr("ClientAddresses");
  if (client_addresses == nullptr || !client_addresses->isArray()) {
    return false;
  }

  std::vector<std::string> vec_client_addresses;
  for (size_t i = 0; i < client_addresses->size(); i++) {
    if (!client_addresses->at(i).isString()) {
      return false;
    }
    vec_client_addresses.push_back(client_addresses->at(i).asString());
  }
  setClientAddresses(vec_client_addresses);
  auto* bind_address = data.get_ptr("BindAddress");
  if (bind_address == nullptr || !bind_address->isString()) {
    return false;
  }
  setBindAddress(bind_address->asString());
  auto* leave_on_term = data.get_ptr("LeaveOnTerm");
  if (leave_on_term == nullptr || !leave_on_term->isBool()) {
    return false;
  }
  setLeaveOnTerm(leave_on_term->asBool());

  return true;
}

void Self::describe(std::ostream& os) const {
  os << "Self{"
     << "Config=" << config_
     << ", " << "DebugConfig=" << debug_config_
     << ", " << "Member=" << member_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Self& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Self::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic config = folly::dynamic::object;
  config = config_.serialize();
  result.insert("Config", config);
  folly::dynamic debug_config = folly::dynamic::object;
  debug_config = debug_config_.serialize();
  result.insert("DebugConfig", debug_config);
  folly::dynamic member = folly::dynamic::object;
  member = member_.serialize();
  result.insert("Member", member);

  return result;
}

bool Self::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* config = data.get_ptr("Config");
  if (config == nullptr) {
    return false;
  }

  SelfConfig itemconfig;
  if (!itemconfig.deserialize(*config)) {
    return false;
  }
  setConfig(itemconfig);
  auto* debug_config = data.get_ptr("DebugConfig");
  if (debug_config == nullptr) {
    return false;
  }

  SelfDebugConfig itemdebug_config;
  if (!itemdebug_config.deserialize(*debug_config)) {
    return false;
  }
  setDebugConfig(itemdebug_config);
  auto* member = data.get_ptr("Member");
  if (member == nullptr) {
    return false;
  }

  Member itemmember;
  if (!itemmember.deserialize(*member)) {
    return false;
  }
  setMember(itemmember);

  return true;
}

void NewCheck::describe(std::ostream& os) const {
  os << "NewCheck{"
     << "Id='" << id_ << "'"
     << ", " << "Name='" << name_ << "'"
     << ", " << "ServiceId='" << service_id_ << "'"
     << ", " << "Notes='" << notes_ << "'"
     << ", " << "Script='" << script_ << "'"
     << ", " << "Http='" << http_ << "'"
     << ", " << "Tcp='" << tcp_ << "'"
     << ", " << "DockerContainerId='" << docker_container_id_ << "'"
     << ", " << "Shell='" << shell_ << "'"
     << ", " << "Interval='" << interval_ << "'"
     << ", " << "Timeout='" << timeout_ << "'"
     << ", " << "Ttl='" << ttl_ << "'"
     << ", " << "DeregisterCriticalServiceAfter='" << deregister_critical_service_after_ << "'"
     << ", " << "TlsSkipVerify='" << tls_skip_verify_ << "'"
     << ", " << "Status='" << status_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NewCheck& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NewCheck::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Id", id_);
  result.insert("Name", name_);
  result.insert("ServiceId", service_id_);
  result.insert("Notes", notes_);
  result.insert("Script", script_);
  result.insert("Http", http_);
  result.insert("Tcp", tcp_);
  result.insert("DockerContainerId", docker_container_id_);
  result.insert("Shell", shell_);
  result.insert("Interval", interval_);
  result.insert("Timeout", timeout_);
  result.insert("Ttl", ttl_);
  result.insert("DeregisterCriticalServiceAfter", deregister_critical_service_after_);
  result.insert("TlsSkipVerify", tls_skip_verify_);
  result.insert("Status", status_);

  return result;
}

bool NewCheck::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* id = data.get_ptr("Id");
  if (id == nullptr || !id->isString()) {
    return false;
  }
  setId(id->asString());
  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* service_id = data.get_ptr("ServiceId");
  if (service_id == nullptr || !service_id->isString()) {
    return false;
  }
  setServiceId(service_id->asString());
  auto* notes = data.get_ptr("Notes");
  if (notes == nullptr || !notes->isString()) {
    return false;
  }
  setNotes(notes->asString());
  auto* script = data.get_ptr("Script");
  if (script == nullptr || !script->isString()) {
    return false;
  }
  setScript(script->asString());
  auto* http = data.get_ptr("Http");
  if (http == nullptr || !http->isString()) {
    return false;
  }
  setHttp(http->asString());
  auto* tcp = data.get_ptr("Tcp");
  if (tcp == nullptr || !tcp->isString()) {
    return false;
  }
  setTcp(tcp->asString());
  auto* docker_container_id = data.get_ptr("DockerContainerId");
  if (docker_container_id == nullptr || !docker_container_id->isString()) {
    return false;
  }
  setDockerContainerId(docker_container_id->asString());
  auto* shell = data.get_ptr("Shell");
  if (shell == nullptr || !shell->isString()) {
    return false;
  }
  setShell(shell->asString());
  auto* interval = data.get_ptr("Interval");
  if (interval == nullptr || !interval->isString()) {
    return false;
  }
  setInterval(interval->asString());
  auto* timeout = data.get_ptr("Timeout");
  if (timeout == nullptr || !timeout->isString()) {
    return false;
  }
  setTimeout(timeout->asString());
  auto* ttl = data.get_ptr("Ttl");
  if (ttl == nullptr || !ttl->isString()) {
    return false;
  }
  setTtl(ttl->asString());
  auto* deregister_critical_service_after = data.get_ptr("DeregisterCriticalServiceAfter");
  if (deregister_critical_service_after == nullptr || !deregister_critical_service_after->isString()) {
    return false;
  }
  setDeregisterCriticalServiceAfter(deregister_critical_service_after->asString());
  auto* tls_skip_verify = data.get_ptr("TlsSkipVerify");
  if (tls_skip_verify == nullptr || !tls_skip_verify->isString()) {
    return false;
  }
  setTlsSkipVerify(tls_skip_verify->asString());
  auto* status = data.get_ptr("Status");
  if (status == nullptr || !status->isString()) {
    return false;
  }
  setStatus(status->asString());

  return true;
}

void NewServiceCheck::describe(std::ostream& os) const {
  os << "NewServiceCheck{"
     << "Script='" << script_ << "'"
     << ", " << "Interval='" << interval_ << "'"
     << ", " << "Ttl='" << ttl_ << "'"
     << ", " << "Http='" << http_ << "'"
     << ", " << "Tcp='" << tcp_ << "'"
     << ", " << "Timeout='" << timeout_ << "'"
     << ", " << "DeregisterCriticalServiceAfter='" << deregister_critical_service_after_ << "'"
     << ", " << "TlsSkipVerify='" << tls_skip_verify_ << "'"
     << ", " << "Status='" << status_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NewServiceCheck& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NewServiceCheck::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Script", script_);
  result.insert("Interval", interval_);
  result.insert("Ttl", ttl_);
  result.insert("Http", http_);
  result.insert("Tcp", tcp_);
  result.insert("Timeout", timeout_);
  result.insert("DeregisterCriticalServiceAfter", deregister_critical_service_after_);
  result.insert("TlsSkipVerify", tls_skip_verify_);
  result.insert("Status", status_);

  return result;
}

bool NewServiceCheck::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* script = data.get_ptr("Script");
  if (script == nullptr || !script->isString()) {
    return false;
  }
  setScript(script->asString());
  auto* interval = data.get_ptr("Interval");
  if (interval == nullptr || !interval->isString()) {
    return false;
  }
  setInterval(interval->asString());
  auto* ttl = data.get_ptr("Ttl");
  if (ttl == nullptr || !ttl->isString()) {
    return false;
  }
  setTtl(ttl->asString());
  auto* http = data.get_ptr("Http");
  if (http == nullptr || !http->isString()) {
    return false;
  }
  setHttp(http->asString());
  auto* tcp = data.get_ptr("Tcp");
  if (tcp == nullptr || !tcp->isString()) {
    return false;
  }
  setTcp(tcp->asString());
  auto* timeout = data.get_ptr("Timeout");
  if (timeout == nullptr || !timeout->isString()) {
    return false;
  }
  setTimeout(timeout->asString());
  auto* deregister_critical_service_after = data.get_ptr("DeregisterCriticalServiceAfter");
  if (deregister_critical_service_after == nullptr || !deregister_critical_service_after->isString()) {
    return false;
  }
  setDeregisterCriticalServiceAfter(deregister_critical_service_after->asString());
  auto* tls_skip_verify = data.get_ptr("TlsSkipVerify");
  if (tls_skip_verify == nullptr || !tls_skip_verify->isString()) {
    return false;
  }
  setTlsSkipVerify(tls_skip_verify->asString());
  auto* status = data.get_ptr("Status");
  if (status == nullptr || !status->isString()) {
    return false;
  }
  setStatus(status->asString());

  return true;
}

void NewService::describe(std::ostream& os) const {
  os << "NewService{"
     << "Id='" << id_ << "'"
     << ", " << "Name='" << name_ << "'"
     << ", " << "Tags=[";
  for (auto& t : tags_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "Address='" << address_ << "'"
     << ", " << "Port=" << port_
     << ", " << "EnableTagOverride=" << enable_tag_override_
     << ", " << "Check=" << check_
     << ", " << "Checks=[";
  for (auto& t : checks_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NewService& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NewService::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Id", id_);
  result.insert("Name", name_);
  folly::dynamic tags = folly::dynamic::array;
  for (auto& t : tags_) {
    tags.push_back(t);
  }
  result.insert("Tags", tags);
  result.insert("Address", address_);
  result.insert("Port", port_);
  result.insert("EnableTagOverride", enable_tag_override_);
  folly::dynamic check = folly::dynamic::object;
  check = check_.serialize();
  result.insert("Check", check);
  folly::dynamic checks = folly::dynamic::array;
  for (auto& t : checks_) {
    folly::dynamic vec_checks = folly::dynamic::object;
    vec_checks = t.serialize();
    checks.push_back(vec_checks);
  }
  result.insert("Checks", checks);

  return result;
}

bool NewService::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* id = data.get_ptr("Id");
  if (id == nullptr || !id->isString()) {
    return false;
  }
  setId(id->asString());
  auto* name = data.get_ptr("Name");
  if (name == nullptr || !name->isString()) {
    return false;
  }
  setName(name->asString());
  auto* tags = data.get_ptr("Tags");
  if (tags == nullptr || !tags->isArray()) {
    return false;
  }

  std::vector<std::string> vec_tags;
  for (size_t i = 0; i < tags->size(); i++) {
    if (!tags->at(i).isString()) {
      return false;
    }
    vec_tags.push_back(tags->at(i).asString());
  }
  setTags(vec_tags);
  auto* address = data.get_ptr("Address");
  if (address == nullptr || !address->isString()) {
    return false;
  }
  setAddress(address->asString());
  auto* port = data.get_ptr("Port");
  if (port == nullptr || !port->isInt()) {
    return false;
  }
  setPort(port->asInt());
  auto* enable_tag_override = data.get_ptr("EnableTagOverride");
  if (enable_tag_override == nullptr || !enable_tag_override->isBool()) {
    return false;
  }
  setEnableTagOverride(enable_tag_override->asBool());
  auto* check = data.get_ptr("Check");
  if (check == nullptr) {
    return false;
  }

  NewServiceCheck itemcheck;
  if (!itemcheck.deserialize(*check)) {
    return false;
  }
  setCheck(itemcheck);
  auto* checks = data.get_ptr("Checks");
  if (checks == nullptr || !checks->isArray()) {
    return false;
  }

  std::vector<NewServiceCheck> vec_checks;
  for (size_t i = 0; i < checks->size(); i++) {
    if (!checks->at(i).isObject()) {
      return false;
    }

    NewServiceCheck obj_checks;
    if (!obj_checks.deserialize(checks->at(i))) {
      return false;
    }
    vec_checks.push_back(obj_checks);
  }
  setChecks(vec_checks);

  return true;
}


}  // namespace consul
}  // namespace service_framework
