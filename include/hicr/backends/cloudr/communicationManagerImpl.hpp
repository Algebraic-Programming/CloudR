#pragma once

#include "communicationManager.hpp"

namespace HiCR::backend::cloudr
{
std::shared_ptr<GlobalMemorySlot> HiCR::backend::cloudr::CommunicationManager::getGlobalMemorySlotImpl(GlobalMemorySlot::tag_t tag, GlobalMemorySlot::globalKey_t globalKey)
{
  // Forward call to base communication manager
  return _baseCommunicationManager->getGlobalMemorySlot(tag, globalKey);
}
__INLINE__ void HiCR::backend::cloudr::CommunicationManager::exchangeGlobalMemorySlotsImpl(HiCR::GlobalMemorySlot::tag_t                 tag,
                                                                                           const std::vector<globalKeyMemorySlotPair_t> &memorySlots)
{
  // Request non active instances to participate in the exchange
  if (_cloudrInstanceManager->getCurrentInstance()->isRootInstance()) _cloudrInstanceManager->requestExchangeGlobalMemorySlots(tag);

  // Initiate the exchange
  _cloudrInstanceManager->getCommunicationManager()->exchangeGlobalMemorySlots(tag, memorySlots);

  // Keep track of the initiated exchange
  _isExchangePending = true;
}

void HiCR::backend::cloudr::CommunicationManager::queryMemorySlotUpdatesImpl(std::shared_ptr<LocalMemorySlot> memorySlot)
{
  // Forward call to base communication manager
  _baseCommunicationManager->queryMemorySlotUpdates(memorySlot);
}

void HiCR::backend::cloudr::CommunicationManager::destroyGlobalMemorySlotImpl(std::shared_ptr<GlobalMemorySlot> memorySlot)
{
  // Forward call to base communication manager
  _baseCommunicationManager->destroyGlobalMemorySlot(memorySlot);
}

__INLINE__ void HiCR::backend::cloudr::CommunicationManager::fenceImpl(HiCR::GlobalMemorySlot::tag_t tag)
{
  // Request non active instances to participate in the fence
  if (_cloudrInstanceManager->getCurrentInstance()->isRootInstance()) _cloudrInstanceManager->requestFence(tag);

  // Execute the fence
  _baseCommunicationManager->fence(tag);

  // If any exchange operation was initiated
  if (_isExchangePending == true)
  {
    // Get the global memory slot tag-key map
    const auto &globalMap = _baseCommunicationManager->getGlobalMemorySlotTagKeyMap();

    // Update CloudR global memory slot tag-key map
    setGlobalMemorySlotTagKeyMap(globalMap);

    // All the changes of the exchange operations are reflected in CloudR
    _isExchangePending = false;
  }
}

bool HiCR::backend::cloudr::CommunicationManager::acquireGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot)
{
  // Forward call to base communication manager
  return _baseCommunicationManager->acquireGlobalLock(memorySlot);
}

void HiCR::backend::cloudr::CommunicationManager::releaseGlobalLockImpl(std::shared_ptr<GlobalMemorySlot> memorySlot)
{
  // Forward call to base communication manager
  return _baseCommunicationManager->releaseGlobalLock(memorySlot);
}

void HiCR::backend::cloudr::CommunicationManager::lock()
{
  // Forward call to base communication manager
  _baseCommunicationManager->lock();
}

void HiCR::backend::cloudr::CommunicationManager::unlock()
{
  // Forward call to base communication manager
  _baseCommunicationManager->unlock();
}

uint8_t *HiCR::backend::cloudr::CommunicationManager::serializeGlobalMemorySlot(const std::shared_ptr<HiCR::GlobalMemorySlot> &globalSlot) const
{
  // Forward call to base communication manager
  return _baseCommunicationManager->serializeGlobalMemorySlot(globalSlot);
}

std::shared_ptr<GlobalMemorySlot> HiCR::backend::cloudr::CommunicationManager::deserializeGlobalMemorySlot(uint8_t *buffer, GlobalMemorySlot::tag_t tag)
{
  // Forward call to base communication manager
  return _baseCommunicationManager->deserializeGlobalMemorySlot(buffer, tag);
}

std::shared_ptr<GlobalMemorySlot> HiCR::backend::cloudr::CommunicationManager::promoteLocalMemorySlot(const std::shared_ptr<LocalMemorySlot> &localMemorySlot,
                                                                                                      GlobalMemorySlot::tag_t                 tag)
{
  // Forward call to base communication manager
  return _baseCommunicationManager->promoteLocalMemorySlot(localMemorySlot, tag);
}

void HiCR::backend::cloudr::CommunicationManager::destroyPromotedGlobalMemorySlot(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
{
  // Forward call to base communication manager
  _baseCommunicationManager->destroyGlobalMemorySlot(memorySlot);
}

void HiCR::backend::cloudr::CommunicationManager::flushReceived()
{
  // Forward call to base communication manager
  _baseCommunicationManager->flushReceived();
}

void HiCR::backend::cloudr::CommunicationManager::flushSent()
{
  // Forward call to base communication manager
  _baseCommunicationManager->flushSent();
}

void HiCR::backend::cloudr::CommunicationManager::deregisterGlobalMemorySlotImpl(const std::shared_ptr<GlobalMemorySlot> &memorySlot)
{
  // Forward call to base communication manager
  _baseCommunicationManager->deregisterGlobalMemorySlot(memorySlot);
}

void HiCR::backend::cloudr::CommunicationManager::memcpyImpl(const std::shared_ptr<LocalMemorySlot> &destination,
                                                             size_t                                  dst_offset,
                                                             const std::shared_ptr<LocalMemorySlot> &source,
                                                             size_t                                  src_offset,
                                                             size_t                                  size)
{
  // Forward call to base communication manager
  _baseCommunicationManager->memcpy(destination, dst_offset, source, src_offset, size);
}

void HiCR::backend::cloudr::CommunicationManager::memcpyImpl(const std::shared_ptr<GlobalMemorySlot> &destination,
                                                             size_t                                   dst_offset,
                                                             const std::shared_ptr<LocalMemorySlot>  &source,
                                                             size_t                                   src_offset,
                                                             size_t                                   size)
{
  // Forward call to base communication manager
  _baseCommunicationManager->memcpy(destination, dst_offset, source, src_offset, size);
}

void HiCR::backend::cloudr::CommunicationManager::memcpyImpl(const std::shared_ptr<LocalMemorySlot>  &destination,
                                                             size_t                                   dst_offset,
                                                             const std::shared_ptr<GlobalMemorySlot> &source,
                                                             size_t                                   src_offset,
                                                             size_t                                   size)
{
  // Forward call to base communication manager
  _baseCommunicationManager->memcpy(destination, dst_offset, source, src_offset, size);
}

void HiCR::backend::cloudr::CommunicationManager::fenceImpl(const std::shared_ptr<LocalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd)
{
  // Forward call to base communication manager
  _baseCommunicationManager->fence(slot, expectedSent, expectedRcvd);
}

void HiCR::backend::cloudr::CommunicationManager::fenceImpl(const std::shared_ptr<GlobalMemorySlot> &slot, size_t expectedSent, size_t expectedRcvd)
{
  // Forward call to base communication manager
  _baseCommunicationManager->fence(slot, expectedSent, expectedRcvd);
}

} // namespace HiCR::backend::cloudr