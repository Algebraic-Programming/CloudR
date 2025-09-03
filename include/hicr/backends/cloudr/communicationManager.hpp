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
 * @file communicationManager.hpp
 * @brief This file implements the communication manager class for the CloudR backend
 * @author S. M. Martin & L. Terracciano
 * @date 19/12/2023
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/communicationManager.hpp>
#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/cloudr/instanceManager.hpp>
#include "instanceManager.hpp"

namespace HiCR::backend::cloudr
{

/**
 * Implementation of the CloudR backend
 */
class CommunicationManager final : public HiCR::CommunicationManager
{
  public:

  /**
   * Constructor for the cloudr backend.
   *
   * \param[in] cloudrInstanceManager The CloudR Instance Manager
   */
  CommunicationManager(HiCR::CommunicationManager *communicationManager, HiCR::backend::cloudr::InstanceManager *instanceManager)
    : HiCR::CommunicationManager(),
      _communicationManager(communicationManager),
      _instanceManager(instanceManager)
  {}

  ~CommunicationManager() override = default;

  std::shared_ptr<GlobalMemorySlot> getGlobalMemorySlotImpl(GlobalMemorySlot::tag_t tag, GlobalMemorySlot::globalKey_t globalKey)
  {
    // Forward call to base communication manager
    return _communicationManager->getGlobalMemorySlot(tag, globalKey);
  }
  __INLINE__ void exchangeGlobalMemorySlotsImpl(HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots)
  {
    // Request non active instances to participate in the exchange
    if (_instanceManager->getCurrentInstance()->isRootInstance()) _instanceManager->requestExchangeGlobalMemorySlots(tag);

    // Initiate the exchange
    _communicationManager->exchangeGlobalMemorySlots(tag, memorySlots);

    // Keep track of the initiated exchange
    _isExchangePending = true;
  }

  void queryMemorySlotUpdatesImpl(std::shared_ptr<LocalMemorySlot> memorySlot)
  {
    // Forward call to base communication manager
    _communicationManager->queryMemorySlotUpdates(memorySlot);
  }

  void destroyGlobalMemorySlotImpl(std::shared_ptr<GlobalMemorySlot> memorySlot)
  {
    // Forward call to base communication manager
    _communicationManager->destroyGlobalMemorySlot(memorySlot);
  }

  __INLINE__ void fenceImpl(HiCR::GlobalMemorySlot::tag_t tag)
  {
    // Request non active instances to participate in the fence
    if (_instanceManager->getCurrentInstance()->isRootInstance()) _instanceManager->requestFence(tag);

    // Execute the fence
    _communicationManager->fence(tag);

    // If any exchange operation was initiated
    if (_isExchangePending == true)
    {
      // Get the global memory slot tag-key map
      const auto &globalMap = _communicationManager->getGlobalMemorySlotTagKeyMap();

      // Update CloudR global memory slot tag-key map
      setGlobalMemorySlotTagKeyMap(globalMap);

      // All the changes of the exchange operations are reflected in CloudR
      _isExchangePending = false;
    }
  }

  bool acquireGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot)
  {
    // Forward call to base communication manager
    return _communicationManager->acquireGlobalLock(memorySlot);
  }

  void releaseGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot)
  {
    // Forward call to base communication manager
    return _communicationManager->releaseGlobalLock(memorySlot);
  }

  void lock()
  {
    // Forward call to base communication manager
    // _communicationManager->lock();
  }

  void unlock()
  {
    // Forward call to base communication manager
    // _communicationManager->unlock();
  }

  uint8_t *serializeGlobalMemorySlot(const std::shared_ptr<HiCR::GlobalMemorySlot> &globalSlot) const
  {
    // Forward call to base communication manager
    return _communicationManager->serializeGlobalMemorySlot(globalSlot);
  }

  std::shared_ptr<GlobalMemorySlot> deserializeGlobalMemorySlot(uint8_t *buffer, GlobalMemorySlot::tag_t tag)
  {
    // Forward call to base communication manager
    return _communicationManager->deserializeGlobalMemorySlot(buffer, tag);
  }

  std::shared_ptr<GlobalMemorySlot> promoteLocalMemorySlot(const std::shared_ptr<LocalMemorySlot> &localMemorySlot, GlobalMemorySlot::tag_t tag)
  {
    // Forward call to base communication manager
    return _communicationManager->promoteLocalMemorySlot(localMemorySlot, tag);
  }

  void destroyPromotedGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    // Forward call to base communication manager
    _communicationManager->destroyGlobalMemorySlot(memorySlot);
  }

  void flushReceived()
  {
    // Forward call to base communication manager
    _communicationManager->flushReceived();
  }

  void flushSent()
  {
    // Forward call to base communication manager
    _communicationManager->flushSent();
  }

  void deregisterGlobalMemorySlotImpl(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
  {
    // Forward call to base communication manager
    _communicationManager->deregisterGlobalMemorySlot(memorySlot);
  }

  void memcpyImpl(const std::shared_ptr<LocalMemorySlot> &destination, size_t dst_offset, const std::shared_ptr<LocalMemorySlot> &source, size_t src_offset, size_t size)
  {
    // Forward call to base communication manager
    _communicationManager->memcpy(destination, dst_offset, source, src_offset, size);
  }

  void memcpyImpl(const std::shared_ptr<GlobalMemorySlot> &destination, size_t dst_offset, const std::shared_ptr<LocalMemorySlot> &source, size_t src_offset, size_t size)
  {
    // Forward call to base communication manager
    _communicationManager->memcpy(destination, dst_offset, source, src_offset, size);
  }

  void memcpyImpl(const std::shared_ptr<LocalMemorySlot> &destination, size_t dst_offset, const std::shared_ptr<GlobalMemorySlot> &source, size_t src_offset, size_t size)
  {
    // Forward call to base communication manager
    _communicationManager->memcpy(destination, dst_offset, source, src_offset, size);
  }

  void fenceImpl(const std::shared_ptr<LocalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd)
  {
    // Forward call to base communication manager
    _communicationManager->fence(slot, expectedSent, expectedRcvd);
  }

  void fenceImpl(const std::shared_ptr<GlobalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd)
  {
    // Forward call to base communication manager
    _communicationManager->fence(slot, expectedSent, expectedRcvd);
  }

  private:

  /**
   * HiCR Communication manager that does the actual operations
  */
  HiCR::CommunicationManager *const _communicationManager;

  /**
   * HiCR Instance manager to check whether this is a root instance
  */
  HiCR::backend::cloudr::InstanceManager *const _instanceManager;

  /**
   * Keep track if there are pending exchanges operations
  */
  bool _isExchangePending = false;
};

} // namespace HiCR::backend::cloudr
