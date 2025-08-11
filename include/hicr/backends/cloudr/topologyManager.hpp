/*
 *   Copyright 2025 Huawei Technologies Co., Ltd.
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
 */

/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file topologyManager.hpp
 * @brief This file implements the TopologyManager class for the CloudR Backend
 * @author L. Terracciano
 * @date 11/08/2025
 */

#pragma once

#include <memory>
#include <hicr/core/device.hpp>
#include <hicr/core/topology.hpp>
#include <hicr/core/topologyManager.hpp>

namespace HiCR::backend::cloudr
{

/**
 * Implementation of the topology manager for the discovery and use of CloudR devices
 */
class TopologyManager final : public HiCR::TopologyManager
{
  public:

  /**
   * Default constructor
   */
  TopologyManager()
    : HiCR::TopologyManager()
  {}

  /**
   * Default destructor
   */
  ~TopologyManager() = default;

  __INLINE__ void setTopologyJs(const nlohmann::json &topologyJs) { _topologyJs = topologyJs; }

  __INLINE__ HiCR::Topology queryTopology() override
  {
    // Storage for the topology to return
    HiCR::Topology t;

    if (_topologyJs == "") return t;

    const auto &devicesJs = hicr::json::getArray<nlohmann::json>(_topologyJs, "Devices");
    for (const auto &deviceJs : devicesJs) t.addDevice(std::make_shared<HiCR::Device>(deviceJs));

    // Returning topology
    return t;
  }

  /**
   * Static implementation of the _deserializeTopology Function
   * @param[in] topology see: _deserializeTopology
   * @return see: _deserializeTopology
   */
  __INLINE__ static HiCR::Topology deserializeTopology(const nlohmann::json &topology)
  {
    // Verifying input's syntax
    HiCR::Topology::verify(topology);

    // New topology to create
    HiCR::Topology t;

    // Iterating over the device list entries in the serialized input
    for (const auto &device : topology["Devices"])
    {
      // Getting device type
      const auto type = device["Type"].get<std::string>();

      // If the device type is recognized, add it to the new topology
      t.addDevice(std::make_shared<HiCR::Device>(device));
    }

    // Returning new topology
    return t;
  }

  __INLINE__ HiCR::Topology _deserializeTopology(const nlohmann::json &topology) const override { return deserializeTopology(topology); }

  private:

  /// Already serialized topology
  nlohmann::json _topologyJs;
};

} // namespace HiCR::backend::cloudr
