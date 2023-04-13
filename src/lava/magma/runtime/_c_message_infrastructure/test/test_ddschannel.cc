// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
// See: https://spdx.org/licenses/

#include <core/channel_factory.h>
#include <core/multiprocessing.h>
#include <channel/dds/dds_channel.h>
#include <core/message_infrastructure_logging.h>
#include <gtest/gtest.h>

namespace message_infrastructure {

const size_t DATA_LENGTH = 10000;
const uint32_t loop_number = 10;
const size_t DEPTH = 32;

void dds_stop_fn() {
  // exit(0);
}

void dds_target_fn_a1_bound(int loop,
                        AbstractChannelPtr mp_to_a1,
                        AbstractChannelPtr a1_to_mp,
                        AbstractChannelPtr a1_to_a2,
                        AbstractChannelPtr a2_to_a1) {
  auto from_mp = mp_to_a1->GetRecvPort();
  from_mp->Start();
  auto to_mp   = a1_to_mp->GetSendPort();
  to_mp->Start();
  auto to_a2   = a1_to_a2->GetSendPort();
  to_a2->Start();
  auto from_a2 = a2_to_a1->GetRecvPort();
  from_a2->Start();
  while (loop--) {
    MetaDataPtr data = from_mp->Recv();
    (*reinterpret_cast<int64_t*>(data->mdata))++;
    to_a2->Send(data);
    free(data->mdata);
    data = from_a2->Recv();
    (*reinterpret_cast<int64_t*>(data->mdata))++;
    to_mp->Send(data);
    free(data->mdata);
  }
  from_mp->Join();
  from_a2->Join();
  to_a2->Join();
  to_mp->Join();
}

void dds_target_fn_a2_bound(int loop,
                        AbstractChannelPtr a1_to_a2,
                        AbstractChannelPtr a2_to_a1) {
  auto from_a1 = a1_to_a2->GetRecvPort();
  from_a1->Start();
  auto to_a1 = a2_to_a1->GetSendPort();
  to_a1->Start();
  while (loop--) {
    MetaDataPtr data = from_a1->Recv();
    (*reinterpret_cast<int64_t*>(data->mdata))++;
    to_a1->Send(data);
    free(data->mdata);
  }
  from_a1->Join();
  to_a1->Join();
}

void dds_protocol(std::string topic_name,
                  DDSTransportType transfer_type,
                  DDSBackendType dds_backend) {
  MultiProcessing mp;
  int loop = loop_number;
  AbstractChannelPtr mp_to_a1 = GetChannelFactory()
    .GetDDSChannel(topic_name + "mp_to_a1",
                   topic_name + "mp_to_a1111",
                   DEPTH,
                   0,
                   transfer_type,
                   dds_backend);
  AbstractChannelPtr a1_to_mp = GetChannelFactory()
    .GetDDSChannel(topic_name + "a1_to_mp",
                   topic_name + "a1_to_mp111",
                   DEPTH,
                   0,
                   transfer_type,
                   dds_backend);
  AbstractChannelPtr a1_to_a2 = GetChannelFactory()
    .GetDDSChannel(topic_name + "a1_to_a2",
                   topic_name + "a1_to_a2111",
                   DEPTH,
                   0,
                   transfer_type,
                   dds_backend);
  AbstractChannelPtr a2_to_a1 = GetChannelFactory()
    .GetDDSChannel(topic_name + "a2_to_a1",
                   topic_name + "a2_to_a1111",
                   DEPTH,
                   0,
                   transfer_type,
                   dds_backend);

  auto target_fn_a1 = std::bind(&dds_target_fn_a1_bound, loop,
                                mp_to_a1, a1_to_mp, a1_to_a2,
                                a2_to_a1);
  auto target_fn_a2 = std::bind(&dds_target_fn_a2_bound, loop, a1_to_a2,
                                a2_to_a1);

  ProcessType actor1 = mp.BuildActor(target_fn_a1);
  ProcessType actor2 = mp.BuildActor(target_fn_a2);

  auto to_a1   = mp_to_a1->GetSendPort();
  to_a1->Start();
  auto from_a1 = a1_to_mp->GetRecvPort();
  from_a1->Start();
  MetaDataPtr metadata = std::make_shared<MetaData>();
  int64_t dims[] = {10000, 0, 0, 0, 0};
  int64_t nd = 1;
  int64_t* array_ = reinterpret_cast<int64_t*>
                    (malloc(sizeof(int64_t) * dims[0]));
  memset(array_, 0, sizeof(int64_t) * dims[0]);
  std::fill(array_, array_ + 10, 1);
  GetMetadata(metadata, array_, nd,
              static_cast<int64_t>(METADATA_TYPES::LONG), dims);
  MetaDataPtr mptr;
  LAVA_DUMP(LOG_UTTEST, "main process loop: %d\n", loop);
  const clock_t start_time = std::clock();
  while (loop--) {
    to_a1->Send(metadata);
    free(metadata->mdata);
    mptr = from_a1->Recv();
    metadata = mptr;
  }
  const clock_t end_time = std::clock();
  from_a1->Join();
  to_a1->Join();
  mp.Stop();
  mp.Cleanup(true);

  std::printf("dds cpp loop timedelta: %f\n",
           ((end_time - start_time)/static_cast<double>(CLOCKS_PER_SEC)));
  LAVA_DUMP(LOG_UTTEST, "exit\n");
}

#if defined(FASTDDS_ENABLE)
TEST(TestDDSDelivery, FastDDSSHMLoop) {
  GTEST_SKIP();
  dds_protocol("fast_shm_",
               DDSTransportType::DDSSHM,
               DDSBackendType::FASTDDSBackend);
}
TEST(TestDDSDelivery, FastDDSUDPv4Loop) {
  dds_protocol("fast_UDPv4",
               DDSTransportType::DDSUDPv4,
               DDSBackendType::FASTDDSBackend);
}
#endif

#if defined(CycloneDDS_ENABLE)
TEST(TestDDSDelivery, CycloneDDSUDPv4Loop) {
  dds_protocol("cyclone_UDPv4",
               DDSTransportType::DDSUDPv4,
               DDSBackendType::CycloneDDSBackend);
}
#endif

TEST(TestDDSSingleProcess, DDS1Process) {
  GTEST_SKIP();
  LAVA_DUMP(LOG_UTTEST, "TestDDSSingleProcess starts.\n");
  AbstractChannelPtr dds_channel = GetChannelFactory()
    .GetDDSChannel("test_DDSChannel_from",
    "test_DDSChannel_to",
    5,
    0,
    DDSTransportType::DDSSHM,
    DDSBackendType::FASTDDSBackend);

  auto send_port = dds_channel->GetSendPort();
  send_port->Start();
  auto recv_port = dds_channel->GetRecvPort();
  recv_port->Start();

  MetaDataPtr metadata = std::make_shared<MetaData>();
  metadata->nd = 1;
  metadata->type = 7;
  metadata->elsize = 8;
  metadata->total_size = 1;
  metadata->dims[0] = 1;
  metadata->strides[0] = 1;
  metadata->mdata =
    (reinterpret_cast<char*>
    (malloc(sizeof(int64_t)+sizeof(MetaData)))+sizeof(MetaData));
  *reinterpret_cast<int64_t*>(metadata->mdata) = 1;

  MetaDataPtr mptr;
  int loop = loop_number;
  int i = 0;
  while (loop--) {
    if (!(loop % 1000))
      LAVA_DUMP(LOG_UTTEST, "At iteration : %d * 1000\n", i++);
    send_port->Send(metadata);
    free(metadata->mdata);
    mptr = recv_port->Recv();
    EXPECT_EQ(*reinterpret_cast<int64_t*>(mptr->mdata),
              *reinterpret_cast<int64_t*>(metadata->mdata));
    (*reinterpret_cast<int64_t*>(mptr->mdata))++;
    metadata = mptr;
  }
  recv_port->Join();
  send_port->Join();
}
}  // namespace message_infrastructure
