//
//  CacheLine.h
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef CacheLine_h
#define CacheLine_h
#include<iostream>
#include "utils.hpp"

class CacheLine {
public:
    bool valid;
    bool dirty;
    bool lock;
    bool is_translation;
    uint64_t tag;
    CoherenceState coh_state;
    
    CacheLine() : valid(false), dirty(false), lock(false), is_translation(false), tag(0), coh_state(INVALID) {}
    
    CoherenceAction setNextCoherenceState(kind txn_kind)
    {
        CoherenceAction coh_action = NONE;
        CoherenceState next_coh_state;
        switch(coh_state)
        {
            case MODIFIED:
                if(txn_kind == BROADCASTED_DATA_READ)
                {
                    next_coh_state = OWNER;
                    coh_action = MEMORY_WRITEBACK;
                }
                else if(txn_kind == BROADCASTED_DATA_WRITE)
                {
                    next_coh_state = INVALID;
                    coh_action = MEMORY_WRITEBACK;
                }
                else
                {
                    next_coh_state = coh_state;
                    coh_action = NONE;
                }
                break;
            case OWNER:
                if(txn_kind == BROADCASTED_DATA_WRITE)
                {
                    //Owner responsible for doing writeback of dirty data
                    next_coh_state = INVALID;
                    coh_action = MEMORY_WRITEBACK;
                }
                else if (txn_kind == DATA_WRITE)
                {
                    next_coh_state = MODIFIED;
                    coh_action = BROADCAST_WRITE;
                }
                else
                {
                    //Ideally, for txn_kind == BROADCASTED_DATA_READ, we will have to provide data with cache to cache transfer here
                    //But simulator does not support cache to cache transfers.
                    //So, skip
                    next_coh_state = coh_state;
                    coh_action = NONE;
                }
                break;
            case EXCLUSIVE:
                if(txn_kind == BROADCASTED_DATA_READ)
                {
                    next_coh_state = SHARED;
                    coh_action = NONE;
                }
                else if(txn_kind == BROADCASTED_DATA_WRITE)
                {
                    next_coh_state = INVALID;
                    coh_action = NONE;
                }
                else if(txn_kind == DATA_WRITE)
                {
                    next_coh_state = MODIFIED;
                    coh_action = BROADCAST_WRITE;
                }
                else
                {
                    next_coh_state = coh_state;
                    coh_action = NONE;
                }
                break;
            case SHARED:
                if (txn_kind == BROADCASTED_DATA_WRITE)
                {
                    next_coh_state = INVALID;
                    coh_action = NONE;
                }
                else if (txn_kind == DATA_WRITE)
                {
                    next_coh_state = MODIFIED;
                    coh_action = BROADCAST_WRITE;
                }
                else
                {
                    next_coh_state = coh_state;
                    coh_action = NONE;
                }
                break;
            case INVALID:
                if(txn_kind == BROADCASTED_DATA_READ || txn_kind == BROADCASTED_DATA_WRITE)
                {
                    next_coh_state = coh_state;
                    coh_action = NONE;
                }
                else if(txn_kind == DATA_READ)
                {
                    next_coh_state = EXCLUSIVE;
                    coh_action = BROADCAST_READ;
                }
                else if (txn_kind == DATA_WRITE)
                {
                    next_coh_state = MODIFIED;
                    coh_action = BROADCAST_WRITE;
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
    
    friend std::ostream& operator << (std::ostream &out, const CacheLine &l)
    {
        out << "|" << l.valid << "|" << l.dirty << "|" << l.lock << "|" << l.is_translation << "|" << std::hex << l.tag << "| -- ";
        return out;
    }
};


#endif /* CacheLine_h */
