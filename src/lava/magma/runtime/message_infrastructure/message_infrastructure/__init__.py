# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause
# See: https://spdx.org/licenses/

from MessageInfrastructurePywrapper import CppMultiProcessing
from MessageInfrastructurePywrapper import ProcessType
from MessageInfrastructurePywrapper import Actor
from MessageInfrastructurePywrapper import ActorStatus
from MessageInfrastructurePywrapper import ActorCmd
from MessageInfrastructurePywrapper import ChannelType as ChannelBackend
from MessageInfrastructurePywrapper import RecvPort
from MessageInfrastructurePywrapper import AbstractTransferPort
from MessageInfrastructurePywrapper import support_grpc_channel
from MessageInfrastructurePywrapper import support_dds_channel
from .ports import SendPort, Channel

ChannelQueueSize = 32
SupportGRPCChannel = support_grpc_channel()
SupportDDSChannel = support_dds_channel()

if SupportGRPCChannel:
    from .ports import GetRPCChannel
if SupportDDSChannel:
    from .ports import GetDDSChannel
    from MessageInfrastructurePywrapper import DDSTransportType
    from MessageInfrastructurePywrapper import DDSBackendType
