//
//  ReplPolicy.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "ReplPolicy.hpp"
#include <assert.h>

unsigned int LRURepl::getVictim(std::vector<CacheLine>& set, uint64_t set_num)
{
    assert(set_num < m_num_sets);
    auto it_not_valid = std::find_if(set.begin(), set.end(), [](const CacheLine &l) { return !l.valid;});
    
    if(it_not_valid != set.end())
    {
        return (unsigned int)(it_not_valid - set.begin());
    }
    
    std::vector<ReplState> setReplState = replStateArr[set_num];
    int victim = -1;
    
    for(int i = 0; i < setReplState.size(); i++)
    {
        if(setReplState[i].m_lru_stack_position == (m_associativity - 1))
        {
            victim = i;
            break;
        }
    }
    
    assert(victim != -1);
    return victim;
}

void LRURepl::updateReplState(uint64_t set_num, int way)
{
    assert(set_num < m_num_sets);
    assert(way < m_associativity);
    std::vector<ReplState>& setReplState = replStateArr[set_num];
    int pivot = setReplState[way].m_lru_stack_position;
    
    for(int i = 0; i < setReplState.size(); i++)
    {
        if(setReplState[i].m_lru_stack_position < pivot)
        {
            setReplState[i].m_lru_stack_position += 1;
            assert(setReplState[i].m_lru_stack_position <= m_associativity);
        }
    }
    
    setReplState[way].m_lru_stack_position = 0;
}
