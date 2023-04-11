// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
// See: https://spdx.org/licenses/

#include <channel/dds/dds_channel.h>
#include <channel/dds/dds.h>
#include <core/utils.h>
#include <core/message_infrastructure_logging.h>

namespace message_infrastructure {

DDSChannel::DDSChannel(const std::string &topic_name,
                       const DDSTransportType &dds_transfer_type,
                       const DDSBackendType &dds_backend,
                       const size_t &size) {
  LAVA_DEBUG(LOG_DDS, "Creating DDSChannel...\n");
  dds_ = GetDDSManagerSingleton().AllocDDS(topic_name,
                                  dds_transfer_type,
                                  dds_backend,
                                  size);
  send_port_ = std::make_shared<DDSSendPort>(dds_);
  recv_port_ = std::make_shared<DDSRecvPort>(dds_);
}

AbstractSendPortPtr DDSChannel::GetSendPort() {
  return send_port_;
}

AbstractRecvPortPtr DDSChannel::GetRecvPort() {
  return recv_port_;
}

std::shared_ptr<DDSChannel> GetDefaultDDSChannel(const std::string &topic_name,
                                                 const size_t &size) {
  DDSBackendType BackendType = DDSBackendType::FASTDDSBackend;
  #if defined(CycloneDDS_ENABLE)
    DDSBackendType BackendType = DDSBackendType::CycloneDDSBackend;
  #endif
  return std::make_shared<DDSChannel>(topic_name,
                                      DDSTransportType::DDSTCPv6 ,
                                      BackendType,
                                      size);
}

}  // namespace message_infrastructure
