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
#include "instanceManager.hpp"

namespace HiCR::backend::cloudr
{

class InstanceManager;

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
  CommunicationManager(HiCR::backend::cloudr::InstanceManager *const cloudrInstanceManager, HiCR::CommunicationManager *communicationManager)
    : HiCR::CommunicationManager(),
      _cloudrInstanceManager(cloudrInstanceManager),
      _baseCommunicationManager(communicationManager)
  {}

  ~CommunicationManager() override = default;

  std::shared_ptr<GlobalMemorySlot> getGlobalMemorySlotImpl(GlobalMemorySlot::tag_t tag, GlobalMemorySlot::globalKey_t globalKey) override;
  void                              exchangeGlobalMemorySlotsImpl(HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override;
  void                              queryMemorySlotUpdatesImpl(std::shared_ptr<LocalMemorySlot> memorySlot) override;
  void                              destroyGlobalMemorySlotImpl(std::shared_ptr<GlobalMemorySlot> memorySlot) override;
  void                              fenceImpl(HiCR::GlobalMemorySlot::tag_t tag) override;
  bool                              acquireGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot) override;
  void                              releaseGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot) override;

  void                              lock() override;
  void                              unlock() override;
  uint8_t                          *serializeGlobalMemorySlot(const std::shared_ptr<HiCR::GlobalMemorySlot> &globalSlot) const override;
  std::shared_ptr<GlobalMemorySlot> deserializeGlobalMemorySlot(uint8_t *buffer, GlobalMemorySlot::tag_t tag) override;
  std::shared_ptr<GlobalMemorySlot> promoteLocalMemorySlot(const std::shared_ptr<LocalMemorySlot> &localMemorySlot, GlobalMemorySlot::tag_t tag) override;
  void                              destroyPromotedGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot) override;
  virtual void                      flushReceived() override;
  virtual void                      flushSent() override;
  void                              deregisterGlobalMemorySlotImpl(const std::shared_ptr<GlobalMemorySlot> &memorySlot) override;
  void memcpyImpl(const std::shared_ptr<LocalMemorySlot> &destination, size_t dst_offset, const std::shared_ptr<LocalMemorySlot> &source, size_t src_offset, size_t size) override;
  void memcpyImpl(const std::shared_ptr<GlobalMemorySlot> &destination, size_t dst_offset, const std::shared_ptr<LocalMemorySlot> &source, size_t src_offset, size_t size) override;
  void memcpyImpl(const std::shared_ptr<LocalMemorySlot> &destination, size_t dst_offset, const std::shared_ptr<GlobalMemorySlot> &source, size_t src_offset, size_t size) override;
  void fenceImpl(const std::shared_ptr<LocalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd) override;
  void fenceImpl(const std::shared_ptr<GlobalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd) override;

  private:

  HiCR::backend::cloudr::InstanceManager *const _cloudrInstanceManager;

  /**
   * HiCR Communication manager that does the actual operations
  */
  HiCR::CommunicationManager *_baseCommunicationManager;

  /**
   * Keep track if there are pending exchanges operations
  */
  bool _isExchangePending = false;
};

} // namespace HiCR::backend::cloudr
