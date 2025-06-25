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
 * @file memorySpace.hpp
 * @brief This file implements the memory space class for the CloudR backend
 * @author S. M. Martin
 * @date 6/19/2025
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/memorySpace.hpp>

namespace HiCR::backend::cloudr
{

/**
 * This class represents a memory space, as visible by the hwloc backend. That is, the entire RAM that the running CPU has access to.
 */
class MemorySpace final : public HiCR::MemorySpace
{
  public:

  /**
   * Deserializing constructor
   *
   * @param[in] input A JSON-encoded serialized host RAM information
   */
  MemorySpace(const nlohmann::json &input)
    : HiCR::MemorySpace()
  {
    deserialize(input);
  }

  /**
   * Default destructor
   */
  ~MemorySpace() override = default;

  [[nodiscard]] __INLINE__ std::string getType() const override { return _type; }

  private:

  /// Mutable type
  std::string _type;

  __INLINE__ void serializeImpl(nlohmann::json &output) const override {}

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override { _type = input["Type"].get<std::string>(); }
};

} // namespace HiCR::backend::cloudr
