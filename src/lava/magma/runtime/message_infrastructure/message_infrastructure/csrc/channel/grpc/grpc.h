// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
// See: https://spdx.org/licenses/

#ifndef CHANNEL_GRPC_GRPC_H_
#define CHANNEL_GRPC_GRPC_H_

#include <message_infrastructure/csrc/core/utils.h>
#include <message_infrastructure/csrc/core/message_infrastructure_logging.h>

#include <string>
#include <vector>
#include <set>
#include <memory>

namespace message_infrastructure {

class GrpcManager {
 public:
  ~GrpcManager();
  bool CheckURL(const std::string &url) {
    if (url_set_.count(url)) {
      return false;
    }
    url_set_.insert(url);
    return true;
  }
  std::string AllocURL() {
    std::string url = base_url_ + std::to_string(base_port_ + port_num_);
    port_num_++;
    while (!CheckURL(url)) {
      std::string url = base_url_ + std::to_string(base_port_ + port_num_);
      port_num_++;
    }
    return url;
  }
  void Release(const std::string &url) {
    url_set_.erase(url);
  }
  friend GrpcManager &GetGrpcManager();

 private:
  GrpcManager() {}
  std::string base_url_ = "127.11.2.78";
  int base_port_ = 8000;
  int port_num_ = 0;
  static GrpcManager grpcm_;
  std::set<std::string> url_set_;
};

GrpcManager& GetGrpcManager();

using GrpcManagerPtr = std::shared_ptr<GrpcManager>;

}  // namespace message_infrastructure

#endif  // CHANNEL_GRPC_GRPC_H_
