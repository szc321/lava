// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
// See: https://spdx.org/licenses/

#ifndef PORT_PROXY_H_
#define PORT_PROXY_H_

#include <core/abstract_port.h>
#include <core/utils.h>
#include <pybind11/pybind11.h>
#include <chrono>
#include <condition_variable>
#include <tuple>
#include <variant>
#include <utility>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

namespace message_infrastructure {

namespace py = pybind11;

class PortProxy {
 public:
  PortProxy() {}
  PortProxy(py::tuple shape, py::object d_type) :
            shape_(shape), d_type_(d_type) {}
  py::object DType();
  py::tuple Shape();
 private:
  py::object d_type_;
  py::tuple shape_;
};

class SendPortProxy : public PortProxy {
 public:
  SendPortProxy() {}
  SendPortProxy(ChannelType channel_type,
                AbstractSendPortPtr send_port,
                py::tuple shape = py::make_tuple(),
                py::object type = py::none()) :
                PortProxy(shape, type),
                channel_type_(channel_type),
                send_port_(send_port) {}
  ChannelType GetChannelType();
  void Start();
  bool Probe();
  void Send(py::object* object);
  void Join();
  std::string Name();
  size_t Size();

 private:
  DataPtr DataFromObject_(py::object* object);
  ChannelType channel_type_;
  AbstractSendPortPtr send_port_;
};


class RecvPortProxy : public PortProxy {
 public:
  RecvPortProxy() {}
  RecvPortProxy(ChannelType channel_type,
                AbstractRecvPortPtr recv_port,
                py::tuple shape = py::make_tuple(),
                py::object type = py::none()) :
                PortProxy(shape, type),
                channel_type_(channel_type),
                recv_port_(recv_port) {}

  ChannelType GetChannelType();
  void Start();
  bool Probe();
  py::object Recv();
  void Join();
  py::object Peek();
  std::string Name();
  size_t Size();
 void add_observer(std::function<void()> observer);

 private:
  py::object MDataToObject_(MetaDataPtr metadata);
  ChannelType channel_type_;
  AbstractRecvPortPtr recv_port_;
};

// Users should be allowed to copy port objects.
// Use std::shared_ptr.
using SendPortProxyPtr = std::shared_ptr<SendPortProxy>;
using RecvPortProxyPtr = std::shared_ptr<RecvPortProxy>;
using SendPortProxyList = std::vector<SendPortProxyPtr>;
using RecvPortProxyList = std::vector<RecvPortProxyPtr>;


class Selector {
 private:
  std::condition_variable cv;
  mutable std::mutex cv_mutex;
  // std::chrono::seconds all_time;
  // int count;

 public:
  void _changed() {
      std::unique_lock<std::mutex> lock(cv_mutex);
      cv.notify_all();
  }

  void _set_observer(py::args channel_actions, std::function<void()> observer) {
      for (auto& channel_action : channel_actions) {
          RecvPortProxy port = channel_action[0].cast<RecvPortProxy>();
          // std::function<py::object()> callback = channel_actions[1].cast<std::function<py::object()>>();
          port.add_observer(observer);
      }
  }

  // template<typename... Args>
  auto select(py::args channel_actions) {
    // std::vector<std::pair<RecvPortProxy, std::function<void()>>>\
    //         channel_actions = { std::make_pair(std::forward<Args>(args))... };
    std::function<void()> observer = std::bind(&Selector::_changed, this);
    std::unique_lock<std::mutex> lock(cv_mutex);
    _set_observer(channel_actions, observer);
      while (true) {
          // auto start_time = std::chrono::high_resolution_clock::now();
          for (auto& channel_action : channel_actions) {
              RecvPortProxy port = channel_action[0].cast<RecvPortProxy>();
              if (port.Probe()) {
                  _set_observer(channel_actions, nullptr);
                  py::function result = channel_actions[1].cast<py::function>();
                  // auto end_time = std::chrono::high_resolution_clock::now();
                  // count++;
                  // all_time += (end_time - start_time);
                  return result;
              }
          }
          // auto end_time = std::chrono::high_resolution_clock::now();
          // count++;
          // all_time += (end_time - start_time);
          cv.wait(lock);
      }
    }
};

}  // namespace message_infrastructure

#endif  // PORT_PROXY_H_
