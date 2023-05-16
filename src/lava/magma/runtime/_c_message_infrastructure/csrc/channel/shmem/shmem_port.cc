// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
// See: https://spdx.org/licenses/

#include <channel/shmem/shmem_port.h>
#include <core/utils.h>
#include <core/message_infrastructure_logging.h>
#include <semaphore.h>
#include <unistd.h>
#include <thread>  // NOLINT
#include <mutex>  // NOLINT
#include <memory>
#include <string>
#include <cassert>
#include <cstring>
#include <cstdlib>

namespace message_infrastructure {

namespace {

void MetaDataPtrFromPointer(const MetaDataPtr &ptr, void *p, int nbytes) {
  std::memcpy(ptr.get(), p, sizeof(MetaData));
  int len = ptr->elsize * ptr->total_size;
  if (len > nbytes) {
    LAVA_LOG_ERR("Recv %d data but max support %d length\n", len, nbytes);
    len = nbytes;
  }
  LAVA_DEBUG(LOG_SMMP, "data len: %d, nbytes: %d\n", len, nbytes);
  ptr->mdata = std::calloc(len, 1);
  if (ptr->mdata == nullptr) {
    LAVA_LOG_ERR("alloc failed, errno: %d\n", errno);
  }
  LAVA_DEBUG(LOG_SMMP, "memory allocates: %p\n", ptr->mdata);
  std::memcpy(ptr->mdata,
    reinterpret_cast<char *>(p) + sizeof(MetaData), len);
  LAVA_DEBUG(LOG_SMMP, "Metadata created\n");
}

}  // namespace

template<>
void RecvQueue<MetaDataPtr>::FreeData(MetaDataPtr data) {
  free(data->mdata);
}

ShmemSendPort::ShmemSendPort(const std::string &name,
                SharedMemoryPtr shm,
                const size_t &size,
                const size_t &nbytes)
  : AbstractSendPort(name, size, nbytes), shm_(shm), done_(false), idx_(0)
{}

void ShmemSendPort::Start() {
  shm_->Start();
}

void ShmemSendPort::Send(DataPtr metadata) {
  auto mdata = reinterpret_cast<MetaData*>(metadata.get());
  int len = mdata->elsize * mdata->total_size;
  if (len > nbytes_ - sizeof(MetaData)) {
    LAVA_LOG_ERR("Send data too large\n");
  }
  shm_->Store([this, len, &metadata](void* data){
    char* cptr = reinterpret_cast<char*>(data) + idx_*nbytes_;
    std::memcpy(cptr, metadata.get(), sizeof(MetaData));
    cptr += sizeof(MetaData);
    std::memcpy(cptr,
                reinterpret_cast<MetaData*>(metadata.get())->mdata,
                len);
    idx_ = (idx_ + 1) % size_;
  });
}

bool ShmemSendPort::Probe() {
  return false;
}

void ShmemSendPort::Join() {
  done_ = true;
}

ShmemRecvPort::ShmemRecvPort(const std::string &name,
                SharedMemoryPtr shm,
                const size_t &size,
                const size_t &nbytes)
  : AbstractRecvPort(name, size, nbytes), shm_(shm), done_(false), idx_(0){
}

ShmemRecvPort::~ShmemRecvPort() {
}

void ShmemRecvPort::Start() {
}

bool ShmemRecvPort::Probe() {
  return shm_->TryProbe();
}

MetaDataPtr ShmemRecvPort::Recv() {
  while (!done_.load()) {
    bool ret = false;
    MetaDataPtr metadata_res = std::make_shared<MetaData>();
    ret = shm_->Load([this, &metadata_res](void* data){
            char* cptr = reinterpret_cast<char*>(data) + idx_*nbytes_;
            void* cptr2 = reinterpret_cast<void*>(cptr);
            MetaDataPtrFromPointer(metadata_res, cptr2,
                                  nbytes_ - sizeof(MetaData));
            this->idx_ = (this->idx_ + 1) % this->size_;
          });
    if (ret) {
      LAVA_DEBUG(LOG_LAYER, "ret ok\n");
      return metadata_res;
    }
    if (!ret) {
        // sleep
        helper::Sleep();
    }
  }
}

void ShmemRecvPort::Join() {
  if (!done_) {
    done_ = true;
    // if (recv_queue_thread_.joinable())
    //   recv_queue_thread_.join();
    // recv_queue_->Stop();
  }
}

MetaDataPtr ShmemRecvPort::Peek() {
  MetaDataPtr metadata_res = std::make_shared<MetaData>();
  shm_->Peek([this](void* data){
          void* cptr = data + idx_*nbytes_;
          MetaDataPtr metadata_res = std::make_shared<MetaData>();
          MetaDataPtrFromPointer(metadata_res, cptr,
                                nbytes_ - sizeof(MetaData));
        });
  return metadata_res;
}

ShmemBlockRecvPort::ShmemBlockRecvPort(const std::string &name,
  SharedMemoryPtr shm, const size_t &nbytes)
  : AbstractRecvPort(name, 1, nbytes), shm_(shm)
{}

MetaDataPtr ShmemBlockRecvPort::Recv() {
  MetaDataPtr metadata_res = std::make_shared<MetaData>();
  shm_->BlockLoad([&metadata_res, this](void* data){
    MetaDataPtrFromPointer(metadata_res, data,
                           nbytes_ - sizeof(MetaData));
  });
  return metadata_res;
}

MetaDataPtr ShmemBlockRecvPort::Peek() {
  MetaDataPtr metadata_res = std::make_shared<MetaData>();
  shm_->Read([&metadata_res, this](void* data){
    MetaDataPtrFromPointer(metadata_res, data,
                           nbytes_ - sizeof(MetaData));
  });
  return metadata_res;
}

bool ShmemBlockRecvPort::Probe() {
  return shm_->TryProbe();
}

}  // namespace message_infrastructure
