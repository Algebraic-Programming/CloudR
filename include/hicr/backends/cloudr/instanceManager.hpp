
#pragma once

#include <hicr/core/exceptions.hpp>
#include <hicr/core/definitions.hpp>
#include <hicr/core/instanceManager.hpp>
#include <deployr/deployr.hpp>

namespace HiCR::backend::cloudr
{

class InstanceManager final : public HiCR::InstanceManager
{
  public:

  InstanceManager() : HiCR::InstanceManager()
  {
  }

  ~InstanceManager() {}

  __INLINE__ void initialize(int *pargc, char ***pargv)
  {
  }

  __INLINE__ void finalize() override {  }

  __INLINE__ void abort(int errorCode) override {  }

  [[nodiscard]] __INLINE__ HiCR::Instance::instanceId_t getRootInstanceId() const override { return 0; }


}; // class CloudR

} // namespace cloudr