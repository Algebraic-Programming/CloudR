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
#include "device.hpp"

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
  Instance(const instanceId_t instanceId, HiCR::Instance* const baseInstance, const bool isRoot)
    : HiCR::Instance(instanceId),
       _baseInstance(baseInstance),
       _isRoot(isRoot)
  {}

  /**
   * Default destructor
   */
  ~Instance() override = default;

  [[nodiscard]] __INLINE__ bool isRootInstance() const override
  {
    return _isRoot;
  };

  __INLINE__ void setTopology(const nlohmann::json& topologyJs)
  {
    _topologyJs = topologyJs;

    const auto& devicesJs = hicr::json::getArray<nlohmann::json>(topologyJs, "Devices");
    _topology = HiCR::Topology();
    for (const auto& deviceJs : devicesJs) _topology.addDevice(std::make_shared<cloudr::Device>(deviceJs));
  }

    __INLINE__ nlohmann::json&  getTopologyJs()
  {
    return _topologyJs;
  }

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
  [[nodiscard]] __INLINE__ bool isCompatible(const HiCR::Topology requestedTopology)
  {
    // Making a copy of the instance topology.
    // Devices will be removed as we match them with the requested device
    auto instanceDevices = _topology.getDevices();

    ////////// Checking for requested devices
    const auto requestedDevices = requestedTopology.getDevices();

    for (const auto &requestedDevice : requestedDevices)
    {
      const auto requestedDeviceType             = requestedDevice->getType();
      const auto requestedDeviceMemorySpaces     = requestedDevice->getMemorySpaceList();
      const auto requestedDeviceComputeResources = requestedDevice->getComputeResourceList();

      // Iterating over all the instance devices to see if one of them satisfies this requested device
      bool foundCompatibleDevice = false;
      for (auto instanceDeviceItr = instanceDevices.begin(); instanceDeviceItr != instanceDevices.end() && foundCompatibleDevice == false; instanceDeviceItr++)
      {
        // Getting instance device object
        const auto &instanceDevice = instanceDeviceItr.operator*();

        // Checking type
        const auto &instanceDeviceType = instanceDevice->getType();

        // If this device is the same type as requested, proceed to check compute resources and memory spaces
        if (instanceDeviceType == requestedDeviceType)
        {
          // Flag to indicate this device is compatible with the request
          bool deviceIsCompatible = true;

          ///// Checking requested compute resources
          auto instanceComputeResources = instanceDevice->getComputeResourceList();

          // Getting compute resources in this instance device 
          for (const auto &requestedComputeResource : requestedDeviceComputeResources)
          {
            bool foundComputeResource = false;
            for (auto instanceComputeResourceItr = instanceComputeResources.begin(); instanceComputeResourceItr != instanceComputeResources.end() && foundComputeResource == false; instanceComputeResourceItr++)
            {
              // Getting instance device object
              const auto &instanceComputeResource = instanceComputeResourceItr.operator*();

              // If it's the same type as requested
              if (instanceComputeResource->getType() == requestedComputeResource->getType())
              {
                // Set compute resource as found
                foundComputeResource = true;

                // Erase this element from the list to not re-use it
                instanceComputeResources.erase(instanceComputeResourceItr);
              }
            }
            
            // If no compute resource was found, abandon search in this device
            if (foundComputeResource == false)
            {
              deviceIsCompatible = false;
              break;
            } 
          }
          
          // If no suitable device was found, advance with the next one
          if (deviceIsCompatible == false) continue;

          ///// Checking requested compute resources
          auto instanceMemorySpaces= instanceDevice->getMemorySpaceList();

          // Getting compute resources in this instance device 
          for (const auto &requestedDeviceMemorySpace : requestedDeviceMemorySpaces)
          {
            bool foundMemorySpace = false;
            for (auto instanceMemorySpaceItr = instanceMemorySpaces.begin(); instanceMemorySpaceItr != instanceMemorySpaces.end() && foundMemorySpace == false; instanceMemorySpaceItr++)
            {
              // Getting instance device object
              const auto &instanceDeviceMemorySpace = instanceMemorySpaceItr.operator*();

              // If it's the same type as requested
              if (instanceDeviceMemorySpace->getType() == requestedDeviceMemorySpace->getType())
              {
                // Check whether the size is at least as big as requested
                if (instanceDeviceMemorySpace->getSize() >= requestedDeviceMemorySpace->getSize())
                {
                  // Set compute resource as found
                  foundMemorySpace = true;

                  // Erase this element from the list to not re-use it
                  instanceMemorySpaces.erase(instanceMemorySpaceItr);
                }
              }
            }
            
            // If no compute resource was found, abandon search in this device
            if (foundMemorySpace == false)
            {
              deviceIsCompatible = false;
              break;
            } 
          }

          // If no suitable device was found, advance with the next one
          if (deviceIsCompatible == false) continue;

          // If we reached this point, we've found the device
          foundCompatibleDevice = true;

          // Deleting device to prevent it from being counted again
          instanceDevices.erase(instanceDeviceItr);

          // Stop checking
          break;
        }
      }

      // If no instance devices could satisfy the requested device, return false now
      if (foundCompatibleDevice == false) return false;
    }

    // All requirements have been met, returning true
    //printf("Requirements met\n");
    return true;
  }

  __INLINE__ HiCR::Instance* getBaseInstance() const { return _baseInstance; }

  private:

  /// Already serialized topology
  nlohmann::json _topologyJs;

  /// Emulated topology for this instance
  HiCR::Topology _topology;

  /// Underlying instance implementing this instance
  HiCR::Instance* const _baseInstance;

  /// A flag that determines whether this instance is root
  const bool _isRoot;

  /// A flag that indicates this instance is currently deployed
  bool _isDeployed = false;
};

} // namespace HiCR::backend::mpi
