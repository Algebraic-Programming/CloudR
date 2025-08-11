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

/**
 * @file instance.hpp
 * @brief Provides a definition for the instance class for the CloudR backend
 * @author S. M. Martin
 * @date 18/06/2025
 */
#pragma once

#include <hicr/core/instance.hpp>
#include <hicr/core/topology.hpp>

namespace HiCR::backend::cloudr
{

/**
 * This class represents an abstract definition for a HICR instance as represented by the MPI backend:
 */
class Instance final : public HiCR::Instance
{
  public:

  /**
   * Constructor for a Instance class for the MPI backend
   * \param[in] instanceId The base instance identifier corresponding to this HiCR instance
   */
  Instance(const instanceId_t instanceId, HiCR::Instance *const baseInstance, const bool isRoot)
    : HiCR::Instance(instanceId),
      _baseInstance(baseInstance),
      _isRoot(isRoot)
  {}

  /**
   * Default destructor
   */
  ~Instance() override = default;

  [[nodiscard]] __INLINE__ bool isRootInstance() const override { return _isRoot; };

  __INLINE__ void setTopology(const nlohmann::json &topologyJs)
  {
    _topologyJs = topologyJs;

    const auto &devicesJs = hicr::json::getArray<nlohmann::json>(topologyJs, "Devices");
    _topology             = HiCR::Topology();
    for (const auto &deviceJs : devicesJs) _topology.addDevice(std::make_shared<HiCR::Device>(deviceJs));
  }

  __INLINE__ nlohmann::json &getTopologyJs() { return _topologyJs; }

  /**
  * Checks whether this instance satisfied a certain instance type.
  * That is, whether it contains the requested devices in the instance type provided
  *
  * The devices are checked in order. That is the first instance device that satisfies a requested device
  * will be removed from the list when checking the next requested device.
  * 
  * @param[in] instanceType The instance type requested to check for
  * 
  * @return true, if this instance satisfies the instance type; false, otherwise.
  */
  [[nodiscard]] __INLINE__ bool isCompatible(const HiCR::Topology requestedTopology) { return _topology.isSubset(requestedTopology); }

  __INLINE__ HiCR::Instance *getBaseInstance() const { return _baseInstance; }

  private:

  /// Already serialized topology
  nlohmann::json _topologyJs;

  /// Emulated topology for this instance
  HiCR::Topology _topology;

  /// Underlying instance implementing this instance
  HiCR::Instance *const _baseInstance;

  /// A flag that determines whether this instance is root
  const bool _isRoot;

  /// A flag that indicates this instance is currently deployed
  bool _isDeployed = false;
};

} // namespace HiCR::backend::cloudr
