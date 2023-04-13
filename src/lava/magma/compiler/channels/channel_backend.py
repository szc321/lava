# INTEL CORPORATION CONFIDENTIAL AND PROPRIETARY
# 
# Copyright © 2020 Intel Corporation.
# 
# This software and the related documents are Intel copyrighted
# materials, and your use of them is governed by the express 
# license under which they were provided to you (License). Unless
# the License provides otherwise, you may not use, modify, copy, 
# publish, distribute, disclose or transmit  this software or the
# related documents without Intel's prior written permission.
# 
# This software and the related documents are provided as is, with
# no express or implied warranties, other than those that are 
# expressly stated in the License.
from enum import Enum

from lava.magma.runtime.message_infrastructure.MessageInfrastructurePywrapper \
    import ChannelType as ChannelBackend  # noqa: E402


class ChannelBackend(Enum):
    PYSHMEM = None
    SHMEM = ChannelBackend.SHMEMCHANNEL
    RPC = ChannelBackend.RPCCHANNEL
    DDS = ChannelBackend.DDSCHANNEL
    SOCKET = ChannelBackend.SOCKETCHANNEL
