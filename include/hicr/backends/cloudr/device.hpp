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
 * @file device.hpp
 * @brief This file implements the Device class for the hwloc backend
 * @author S. M. Martin
 * @date 18/12/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/device.hpp>
#include "computeResource.hpp"
#include "memorySpace.hpp"

namespace HiCR::backend::cloudr
{

/**
 * This class represents a mutable device, as emulated by CloudR
 */
class Device final : public HiCR::Device
{
  public:


  /**
   * Constructor for the device class of the HWLoC backend
   *
   * @param[in] computeResources The compute resources (cores or hyperthreads) detected in this device (CPU)
   * @param[in] memorySpaces The memory spaces (e.g., NUMA domains) detected in this device (CPU)
   */
  Device(const std::string& type, const computeResourceList_t &computeResources, const memorySpaceList_t &memorySpaces)
    : HiCR::Device(computeResources, memorySpaces),
      _type(type)
      {};

  /**
   * Empty constructor for serialization / deserialization
   */
  Device()
    : HiCR::Device(){};

  /**
   * Deserializing constructor
   *
   * The instance created will contain all information, if successful in deserializing it, corresponding to the passed NUMA domain
   * This instance should NOT be used for anything else than reporting/printing the contained resources
   *
   * @param[in] input A JSON-encoded serialized NUMA domain information
   */
  Device(const nlohmann::json &input)
    : HiCR::Device()
  {
    deserialize(input);
  }

  /**
   * Default destructor
   */
  ~Device() override = default;

  protected:

  [[nodiscard]] __INLINE__ std::string getType() const override { return _type; }

  private:

  /// Mutable type
  std::string _type;

  __INLINE__ void serializeImpl(nlohmann::json &output) const override
  {
    // Storing numa domain identifier
    output["Type"] = _type;
  }

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override
  {
    _type = hicr::json::getString(input, "Type");
    
    // Iterating over the compute resource list
    for (const auto &computeResource : input[_HICR_DEVICE_COMPUTE_RESOURCES_KEY_])
    {
      // Getting device type
      const auto type = computeResource["Type"].get<std::string>();

      // Deserializing new device
      auto computeResourceObj = std::make_shared<cloudr::ComputeResource>(computeResource);

      // Inserting device into the list
      this->addComputeResource(computeResourceObj);
    }

    // Iterating over the memory space list
    for (const auto &memorySpace : input[_HICR_DEVICE_MEMORY_SPACES_KEY_])
    {
      // Getting device type
      const auto type = memorySpace["Type"].get<std::string>();

      // Deserializing new device
      auto memorySpaceObj = std::make_shared<cloudr::MemorySpace>(memorySpace);

      // Inserting device into the list
      this->addMemorySpace(memorySpaceObj);
    }
  }
};

} // namespace HiCR::backend::cloudr
