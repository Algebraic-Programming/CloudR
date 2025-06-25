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
 * @file computeResource.hpp
 * @brief This file implements the compute resource class for CloudR
 * @author S. M. Martin
 * @date 6/19/2025
 */

#pragma once

#include <hicr/core/definitions.hpp>
#include <hicr/core/exceptions.hpp>
#include <hicr/core/computeResource.hpp>

namespace HiCR::backend::cloudr
{

/**
 * This class represents a mutable compute resource, visible by HWLoc.
 */
class ComputeResource final : public HiCR::ComputeResource
{
  public:

  /**
   * Constructor for the compute resource class of cloudr
   */
  ComputeResource(const nlohmann::json &input)
    : HiCR::ComputeResource()
  {
    deserialize(input);
  }

  ~ComputeResource() override = default;

  /**
   * Default constructor for serialization/deserialization purposes
   */
  ComputeResource() = default;

  __INLINE__ std::string getType() const override { return _type; }

  protected:

  __INLINE__ void serializeImpl(nlohmann::json &output) const override {}

  __INLINE__ void deserializeImpl(const nlohmann::json &input) override { _type = input["Type"].get<std::string>(); }

  private:

  /// Type can change based on the configuration
  std::string _type;
};

} // namespace HiCR::backend::cloudr
