#pragma once

#include "communicationManager.hpp"

namespace HiCR::backend::cloudr
{

__INLINE__ void HiCR::backend::cloudr::CommunicationManager::exchangeGlobalMemorySlotsImpl(HiCR::GlobalMemorySlot::tag_t tag, const std::vector<globalKeyMemorySlotPair_t> &memorySlots)
{
    if (_cloudrInstanceManager->getCurrentInstance()->isRootInstance()) _cloudrInstanceManager->requestExchangeGlobalMemorySlots(tag);

    mpi::CommunicationManager::exchangeGlobalMemorySlotsImpl(tag, memorySlots);
}

__INLINE__ void HiCR::backend::cloudr::CommunicationManager::fenceImpl(HiCR::GlobalMemorySlot::tag_t tag)
{
    if (_cloudrInstanceManager->getCurrentInstance()->isRootInstance()) _cloudrInstanceManager->requestFence(tag);

    mpi::CommunicationManager::fenceImpl(tag);
}

}