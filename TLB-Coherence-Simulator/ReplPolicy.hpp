//
//  ReplPolicy.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef ReplPolicy_hpp
#define ReplPolicy_hpp

#include <iostream>
#include <vector>
#include "CacheLine.h"

//Placeholder class for replacement state
//Currently tracks LRU stack position
//In future, may track more
enum Policy {
    LRU_POLICY = 0,
};

class ReplState {
public:
    int m_lru_stack_position;
};

//Interface class for replacement policy
class ReplPolicy {
protected:
    int m_num_sets;
    int m_associativity;
    std::vector<std::vector<ReplState>> replStateArr;
    
public:
    
    ReplPolicy(int num_sets, int associativity) :
        m_num_sets(num_sets), m_associativity(associativity)
    {
        replStateArr.reserve(m_num_sets);
        
        for(int i = 0; i < m_num_sets; i++)
        {
            std::vector<ReplState> setReplState;
            for(int j = 0; j < m_associativity; j++)
            {
                setReplState.push_back(ReplState());
            }
            replStateArr.push_back(setReplState);
        }
    }
    
    virtual unsigned int getVictim(uint64_t set_num) = 0;
    virtual void updateReplState(uint64_t set_num, int way) = 0;
};

//LRU replacement class
class LRURepl : public ReplPolicy {
public:
    LRURepl(int num_sets, int associativity) : ReplPolicy(num_sets, associativity) {
        for(auto &elem: replStateArr)
        {
            for(int i = 0; i < m_associativity; i++)
            {
                elem[i].m_lru_stack_position = m_associativity - i - 1;
            }
        }
    }
    virtual unsigned int getVictim(uint64_t set_num) override final;
    virtual void updateReplState(uint64_t set_num, int way) override final;
};

#endif /* ReplPolicy_hpp */
