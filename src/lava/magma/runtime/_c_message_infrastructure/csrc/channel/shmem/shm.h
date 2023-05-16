// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
// See: https://spdx.org/licenses/

#ifndef CHANNEL_SHMEM_SHM_H_
#define CHANNEL_SHMEM_SHM_H_

#include <core/message_infrastructure_logging.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <memory>
#include <unordered_map>
#include <set>
#include <string>
#include <atomic>
#include <functional>
#include <cstdlib>
#include <ctime>

namespace message_infrastructure {

#define SHM_FLAG O_RDWR | O_CREAT
#define SHM_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH

using HandleFn = std::function<void(void *)>;

class SharedMemory {
 public:
  SharedMemory() {}
  SharedMemory(const size_t &mem_size, void* mmap, const int &key, const size_t &esize_);
  SharedMemory(const size_t &mem_size, void* mmap);
  ~SharedMemory();
  void Start();
  bool Load(HandleFn consume_fn);
  void BlockLoad(HandleFn consume_fn);
  void Read(HandleFn consume_fn);
  void Store(HandleFn store_fn);
  void Peek(HandleFn consume_fn);
  void Close();
  bool TryProbe();
  void InitSemaphore(sem_t* req, sem_t *ack);
  int GetDataElem(int offset);
  std::string GetReq();
  std::string GetAck();
  sem_t all_index_sem;

 private:
  size_t size_;
  size_t esize_;
  std::string req_name_ = "req";
  std::string ack_name_ = "ack";
  sem_t *req_;
  sem_t *ack_;
  void *data_ = nullptr;
};

class RwSharedMemory {
 public:
  RwSharedMemory(const size_t &mem_size, void* mmap, const int &key);
  ~RwSharedMemory();
  void InitSemaphore();
  void Start();
  void Handle(HandleFn handle_fn);
  void Close();

 private:
  size_t size_;
  std::string sem_name_ = "sem";
  sem_t *sem_;
  void *data_;
};

// SharedMemory object needs to be transfered to ShmemPort.
// RwSharedMemory object needs to be transfered to ShmemPort.
// Also need to be handled in SharedMemManager.
// Use std::shared_ptr.
using SharedMemoryPtr = std::shared_ptr<SharedMemory>;
using RwSharedMemoryPtr = std::shared_ptr<RwSharedMemory>;

class SharedMemManager {
 public:
  SharedMemManager(const SharedMemManager&) = delete;
  SharedMemManager(SharedMemManager&&) = delete;
  SharedMemManager& operator=(const SharedMemManager&) = delete;
  SharedMemManager& operator=(SharedMemManager&&) = delete;
  template <typename T>
  std::shared_ptr<T> AllocChannelSharedMemory(const size_t &mem_size, const size_t &size) {
    size_t real_mem_size = mem_size*size;
    int random = std::rand();
    std::string str = shm_str_ + std::to_string(random);
    int shmfd = shm_open(str.c_str(), SHM_FLAG, SHM_MODE);
    LAVA_DEBUG(LOG_SMMP, "Shm fd and name open: %s %d\n",
                str.c_str(), shmfd);
    if (shmfd == -1) {
      LAVA_LOG_FATAL("Create shared memory object failed.\n");
    }
    int err = ftruncate(shmfd, real_mem_size);
    if (err == -1) {
      LAVA_LOG_FATAL("Resize shared memory segment failed.\n");
    }
    shm_fd_strs_.insert({shmfd, str});
    void *mmap_address = mmap(nullptr, real_mem_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, shmfd, 0);
    if (mmap_address == reinterpret_cast<void*>(-1)) {
      LAVA_LOG_ERR("Get shmem address error, errno: %d\n", errno);
      LAVA_DUMP(1, "size: %ld, shmfd_: %d\n", real_mem_size, shmfd);
    }
    shm_mmap_.insert({mmap_address, real_mem_size});
    std::shared_ptr<T> shm =
      std::make_shared<T>(real_mem_size, mmap_address, random, size);
    std::string req_name = shm->GetReq();
    std::string ack_name = shm->GetAck();
    sem_t *req = sem_open(req_name.c_str(), O_CREAT, 0644, 0);
    sem_t *ack = sem_open(ack_name.c_str(), O_CREAT, 0644, size);
    shm->InitSemaphore(req, ack);
    sem_p_strs_.insert({req, req_name});
    sem_p_strs_.insert({ack, ack_name});
    return shm;
  }

  void DeleteAllSharedMemory();
  friend SharedMemManager &GetSharedMemManagerSingleton();

 private:
  SharedMemManager() {
    std::srand(std::time(nullptr));
    alloc_pid_ = getpid();
  }
  ~SharedMemManager() = default;
  std::unordered_map<int, std::string> shm_fd_strs_;
  std::unordered_map<sem_t*, std::string> sem_p_strs_;
  std::unordered_map<void*, int64_t> shm_mmap_;
  static SharedMemManager smm_;
  std::string shm_str_ = "shm";
  int alloc_pid_;
};

SharedMemManager& GetSharedMemManagerSingleton();

}  // namespace message_infrastructure

#endif  // CHANNEL_SHMEM_SHM_H_
