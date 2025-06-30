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
 * @author S. M. Martin
 * @date 27/06/2025
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/localMemorySlot.hpp>
#include <hicr/core/communicationManager.hpp>
#include "instanceManager.hpp"

namespace HiCR::backend::cloudr
{

class CommunicationManager final : public HiCR::CommunicationManager
{
  public:

  CommunicationManager(HiCR::backend::cloudr::InstanceManager* instanceManager)
    : HiCR::CommunicationManager(),
      _instanceManager(instanceManager)
  {
  }

  ~CommunicationManager() override = default;

  private:

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::LocalMemorySlot> &destination,
                             const size_t                                  dst_offset,
                             const std::shared_ptr<HiCR::LocalMemorySlot> &source,
                             const size_t                                  src_offset,
                             const size_t                                  size) override
  {
    _instanceManager->getCommunicationManager()->memcpy(destination, dst_offset, source, src_offset, size);
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::LocalMemorySlot>  &destinationSlot,
                             size_t                                         dst_offset,
                             const std::shared_ptr<HiCR::GlobalMemorySlot> &sourceSlotPtr,
                             size_t                                         sourceOffset,
                             size_t                                         size) override
  {
    _instanceManager->getCommunicationManager()->memcpy(destinationSlot, dst_offset, sourceSlotPtr, sourceOffset, size);
  }

  __INLINE__ void memcpyImpl(const std::shared_ptr<HiCR::GlobalMemorySlot> &destinationSlotPtr,
                             size_t                                         dst_offset,
                             const std::shared_ptr<HiCR::LocalMemorySlot>  &sourceSlot,
                             size_t                                         sourceOffset,
                             size_t                                         size) override
  {
    _instanceManager->getCommunicationManager()->memcpy(destinationSlotPtr, dst_offset, sourceSlot, sourceOffset, size);
  }

  __INLINE__ void queryMemorySlotUpdatesImpl(std::shared_ptr<HiCR::LocalMemorySlot> memorySlot) override
  {
    _instanceManager->getCommunicationManager()->queryMemorySlotUpdates(memorySlot);
  }

  __INLINE__ void deregisterGlobalMemorySlotImpl(const std::shared_ptr<HiCR::GlobalMemorySlot> &memorySlot) override
  {
    _instanceManager->getCommunicationManager()->deregisterGlobalMemorySlot(memorySlot);
  }

  __INLINE__ void fenceImpl(HiCR::GlobalMemorySlot::tag_t tag) override
  {
    _instanceManager->getCommunicationManager()->fence(tag);
  }

  __INLINE__ void exchangeGlobalMemorySlotsImpl(HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override
  {
    printf("Exchanging global memory slots\n");
    const auto RPCEngine = _instanceManager->getRPCEngine();

    _instanceManager->getCommunicationManager()->exchangeGlobalMemorySlots(tag, memorySlots);
  }

  __INLINE__ void destroyGlobalMemorySlotImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlotPtr) override
  {
    _instanceManager->getCommunicationManager()->destroyGlobalMemorySlot(memorySlotPtr);
  }

  __INLINE__ bool acquireGlobalLockImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override
  {
    return _instanceManager->getCommunicationManager()->acquireGlobalLock(memorySlot);
  }

  __INLINE__ void releaseGlobalLockImpl(std::shared_ptr<HiCR::GlobalMemorySlot> memorySlot) override
  {
    _instanceManager->getCommunicationManager()->releaseGlobalLock(memorySlot);
  }

  std::shared_ptr<HiCR::GlobalMemorySlot> getGlobalMemorySlotImpl(HiCR::GlobalMemorySlot::tag_t tag, HiCR::GlobalMemorySlot::globalKey_t globalKey) override { return nullptr; }

  HiCR::backend::cloudr::InstanceManager* const _instanceManager;
};

} // namespace HiCR::backend::cloudr
