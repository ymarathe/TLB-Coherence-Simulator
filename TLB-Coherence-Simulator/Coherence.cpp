//
//  Coherence.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/29/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "Coherence.hpp"

CoherenceState CoherenceProtocol::getCoherenceState()
{
    return coh_state;
}

void CoherenceProtocol::forceCoherenceState(CoherenceState cs)
{
    coh_state = cs;
}

void CoherenceProtocol::set_level(unsigned int cache_level)
{
    m_cache_level = cache_level;
}

unsigned int CoherenceProtocol::get_level()
{
    return m_cache_level;
}


CoherenceAction MOESIProtocol::setNextCoherenceState(kind txn_kind, CoherenceState propagate_coh_state)
{
    //TODO: Need to handle writebacks here as well
    CoherenceAction coh_action = NONE;
    CoherenceState next_coh_state;
    switch(coh_state)
    {
        case MODIFIED:
            if(txn_kind == DIRECTORY_DATA_READ)
            {
                next_coh_state = OWNER;
                coh_action =  MEMORY_DATA_WRITEBACK;
            }
            else if (txn_kind == DIRECTORY_TRANSLATION_READ)
            {
                next_coh_state = OWNER;
                coh_action = MEMORY_TRANSLATION_WRITEBACK;
            }
            else if(txn_kind == DIRECTORY_DATA_WRITE)
            {
                next_coh_state = INVALID;
                coh_action =  MEMORY_DATA_WRITEBACK;
            }
            else if(txn_kind == DIRECTORY_TRANSLATION_WRITE)
            {
                next_coh_state = INVALID;
                coh_action = MEMORY_TRANSLATION_WRITEBACK;
            }
            else
            {
                next_coh_state = coh_state;
                coh_action = NONE;
            }
            break;
        case OWNER:
            if(txn_kind == DIRECTORY_DATA_WRITE)
            {
                //Owner responsible for doing writeback of dirty data
                next_coh_state = INVALID;
                coh_action =  MEMORY_DATA_WRITEBACK;
            }
            else if(txn_kind == DIRECTORY_TRANSLATION_WRITE)
            {
                next_coh_state = INVALID;
                coh_action = MEMORY_TRANSLATION_WRITEBACK;
            }
            else if (txn_kind == DATA_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = BROADCAST_DATA_WRITE;
            }
            else if (txn_kind == DATA_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else if (txn_kind == DATA_WRITEBACK || txn_kind == TRANSLATION_WRITEBACK)
            {
                next_coh_state = propagate_coh_state;
                coh_action = NONE;
            }
            else if(txn_kind == TRANSLATION_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = BROADCAST_TRANSLATION_WRITE;
            }
            else if(txn_kind == TRANSLATION_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else
            {
                //Ideally, for txn_kind == DIRECTORY_DATA_READ, we will have to provide data with cache to cache transfer here
                //We do not support cache to cache transfers.
                //So, skip
                next_coh_state = coh_state;
                coh_action = NONE;
            }
            break;
        case EXCLUSIVE:
            if(txn_kind == DIRECTORY_DATA_READ)
            {
                next_coh_state = SHARED;
                coh_action = NONE;
            }
            else if (txn_kind == DIRECTORY_TRANSLATION_READ)
            {
                next_coh_state = SHARED;
                coh_action = NONE;
            }
            else if(txn_kind == DIRECTORY_DATA_WRITE)
            {
                next_coh_state = INVALID;
                coh_action = NONE;
            }
            else if(txn_kind == DIRECTORY_TRANSLATION_WRITE)
            {
                next_coh_state = INVALID;
                coh_action = NONE;
            }
            else if(txn_kind == DATA_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = NONE;
            }
            else if (txn_kind == DATA_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else if(txn_kind == DATA_WRITEBACK || txn_kind == TRANSLATION_WRITEBACK)
            {
                next_coh_state = propagate_coh_state;
                coh_action = NONE;
            }
            else if(txn_kind == TRANSLATION_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = NONE;
            }
            else if(txn_kind == TRANSLATION_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else
            {
                next_coh_state = coh_state;
                coh_action = NONE;
            }
            break;
        case SHARED:
            if (txn_kind == DIRECTORY_DATA_WRITE)
            {
                next_coh_state = INVALID;
                coh_action = NONE;
            }
            else if (txn_kind == DIRECTORY_TRANSLATION_WRITE)
            {
                next_coh_state = INVALID;
                coh_action = NONE;
            }
            else if (txn_kind == DATA_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = BROADCAST_DATA_WRITE;
            }
            else if (txn_kind == DATA_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else if (txn_kind == DATA_WRITEBACK || txn_kind == TRANSLATION_WRITEBACK)
            {
                next_coh_state = propagate_coh_state;
                coh_action = NONE;
            }
            else if (txn_kind == TRANSLATION_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = BROADCAST_TRANSLATION_WRITE;
            }
            else if(txn_kind == TRANSLATION_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else
            {
                next_coh_state = coh_state;
                coh_action = NONE;
            }
            break;
        case INVALID:
            if(txn_kind == DATA_READ)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = BROADCAST_DATA_READ;
            }
            else if (txn_kind == TRANSLATION_READ)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = BROADCAST_TRANSLATION_READ;
            }
            else if (txn_kind == DATA_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = BROADCAST_DATA_WRITE;
            }
            else if (txn_kind == DATA_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else if (txn_kind == DATA_WRITEBACK || txn_kind == TRANSLATION_WRITEBACK)
            {
                next_coh_state = propagate_coh_state;
                coh_action = NONE;
            }
            else if(txn_kind == TRANSLATION_WRITE && m_cache_level == 1)
            {
                next_coh_state = MODIFIED;
                coh_action = BROADCAST_TRANSLATION_WRITE;
            }
            else if(txn_kind == TRANSLATION_WRITE)
            {
                next_coh_state = EXCLUSIVE;
                coh_action = NONE;
            }
            else
            {
                next_coh_state = coh_state;
                coh_action = NONE;
            }
            break;
        default:
            break;
    }
    
    coh_state = next_coh_state;
    
    return coh_action;
}
