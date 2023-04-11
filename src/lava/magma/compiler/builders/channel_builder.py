# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: LGPL 2.1 or later
# See: https://spdx.org/licenses/

import typing as ty
from dataclasses import dataclass

from lava.magma.compiler.builders.interfaces import \
    AbstractChannelBuilder, \
    AbstractProcessModel
from lava.magma.compiler.builders. \
    runtimeservice_builder import RuntimeServiceBuilder
from lava.magma.compiler.channels.channel_backend import ChannelBackend
from lava.magma.runtime.message_infrastructure import (
    Channel,
)
from lava.magma.compiler.utils import PortInitializer
from lava.magma.runtime.message_infrastructure \
    .message_infrastructure_interface import (MessageInfrastructureInterface)
from lava.magma.runtime.message_infrastructure.interfaces import ChannelType
if ty.TYPE_CHECKING:
    from lava.magma.core.process.process import AbstractProcess
    from lava.magma.runtime.runtime import Runtime


@dataclass
class ChannelBuilderMp(AbstractChannelBuilder):
    """A ChannelBuilder assuming Python multi-processing is used as messaging
    and multi processing backbone.
    """

    channel_type: ChannelType
    src_process: "AbstractProcess"
    dst_process: "AbstractProcess"
    src_port_initializer: PortInitializer
    dst_port_initializer: PortInitializer
    channel_backend: ChannelBackend

    def build(
            self, messaging_infrastructure: MessageInfrastructureInterface
    ) -> Channel:
        """Given the message passing framework builds a channel

        Parameters
        ----------
        messaging_infrastructure : MessageInfrastructureInterface

        Returns
        -------
        Channel
            Channel

        Raises
        ------
        Exception
            Can't build channel of type specified
        """
        return messaging_infrastructure.channel(self.channel_type,
                                                self.channel_backend,
                                                self.src_port_initializer.name,
                                                self.dst_port_initializer.name,
                                                self.src_port_initializer.shape,
                                                self.src_port_initializer.d_type,  # noqa: E501
                                                self.src_port_initializer.size)


@dataclass
class ServiceChannelBuilderMp(AbstractChannelBuilder):
    """A RuntimeServiceChannelBuilder assuming Python multi-processing is used
    as messaging and multi processing backbone.
    """

    channel_type: ChannelType
    src_process: ty.Union[RuntimeServiceBuilder,
                          ty.Type["AbstractProcessModel"]]
    dst_process: ty.Union[RuntimeServiceBuilder,
                          ty.Type["AbstractProcessModel"]]
    port_initializer: PortInitializer
    channel_backend: ChannelBackend

    def build(
            self, messaging_infrastructure: MessageInfrastructureInterface
    ) -> Channel:
        """Given the message passing framework builds a channel

        Parameters
        ----------
        messaging_infrastructure : MessageInfrastructureInterface

        Returns
        -------
        Channel
            PyPyChannel

        Raises
        ------
        Exception
            Can't build channel of type specified
        """
        channel_name: str = self.port_initializer.name
        return messaging_infrastructure.channel(self.channel_type,
                                                self.channel_backend,
                                                channel_name + "_src",
                                                channel_name + "_dst",
                                                self.port_initializer.shape,
                                                self.port_initializer.d_type,
                                                self.port_initializer.size,
                                                sync=True)


@dataclass
class RuntimeChannelBuilderMp(AbstractChannelBuilder):
    """A RuntimeChannelBuilder assuming Python multi-processing is
    used as messaging and multi processing backbone.
    """

    channel_type: ChannelType
    src_process: ty.Union[RuntimeServiceBuilder, ty.Type["Runtime"]]
    dst_process: ty.Union[RuntimeServiceBuilder, ty.Type["Runtime"]]
    port_initializer: PortInitializer
    channel_backend: ChannelBackend

    def build(
            self, messaging_infrastructure: MessageInfrastructureInterface
    ) -> Channel:
        """Given the message passing framework builds a channel

        Parameters
        ----------
        messaging_infrastructure : MessageInfrastructureInterface

        Returns
        -------
        Channel
            PyPyChannel

        Raises
        ------
        Exception
            Can't build channel of type specified
        """
        channel_name: str = self.port_initializer.name
        return messaging_infrastructure.channel(self.channel_type,
                                                self.channel_backend,
                                                channel_name + "_src",
                                                channel_name + "_dst",
                                                self.port_initializer.shape,
                                                self.port_initializer.d_type,
                                                self.port_initializer.size,
                                                sync=True)


@dataclass
class ChannelBuilderNx(AbstractChannelBuilder):
    """A ChannelBuilder for CNc and NcC Channels with NxBoard as the messaging
    infrastructure.
    """

    channel_type: ChannelType
    src_process: "AbstractProcess"
    dst_process: "AbstractProcess"
    src_port_initializer: PortInitializer
    dst_port_initializer: PortInitializer
    channel_backend: ChannelBackend = None

    def build(
            self, messaging_infrastructure: MessageInfrastructureInterface
    ) -> Channel:
        """Given the message passing framework builds a channel

        Parameters
        ----------
        messaging_infrastructure : MessageInfrastructureInterface

        Returns
        -------
        Channel
            Channel

        Raises
        ------
        Exception
            Can't build channel of type specified
        """
        pass
