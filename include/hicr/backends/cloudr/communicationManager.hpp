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
class CommunicationManager final : public HiCR::backend::mpi::CommunicationManager
{
  public:

  /**
   * Constructor for the cloudr backend.
   *
   * \param[in] comm The CloudR subcommunicator to use in the communication operations in this backend.
   * If not specified, it will use CloudR_COMM_WORLD
   */
  CommunicationManager(HiCR::backend::cloudr::InstanceManager* const cloudrInstanceManager) : HiCR::backend::mpi::CommunicationManager(MPI_COMM_WORLD),
  _cloudrInstanceManager(cloudrInstanceManager)
  {
  }

  ~CommunicationManager() override = default;

  void exchangeGlobalMemorySlotsImpl(HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots) override;
  void fenceImpl(HiCR::GlobalMemorySlot::tag_t tag) override;

  private:

  HiCR::backend::cloudr::InstanceManager* const _cloudrInstanceManager;  
};

} // namespace HiCR::backend::cloudr
