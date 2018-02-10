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
#include <iterator>
#include <algorithm>
#include "CacheLine.h"

//Placeholder class for replacement state
//Currently tracks LRU stack position
//In future, may track more
enum ReplPolicyEnum {
    LRU_POLICY = 0,
};

class ReplState {
public:
    int m_lru_stack_position;
    
    friend std::ostream& operator << (std::ostream &out, const ReplState &r)
    {
        out << r.m_lru_stack_position << ", ";
        return out;
    }
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
    
    virtual unsigned int getVictim(std::vector<CacheLine>& set, uint64_t set_num) = 0;
    virtual void updateReplState(uint64_t set_num, int way) = 0;
    virtual void printReplStateArr(uint64_t set_num) = 0;
    virtual ~ReplPolicy() {}
    
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
    virtual unsigned int getVictim(std::vector<CacheLine>& set, uint64_t set_num) override final;
    virtual void updateReplState(uint64_t set_num, int way) override final;
    
    virtual void printReplStateArr(uint64_t set_num) override final
    {
        std::copy(replStateArr[set_num].begin(), replStateArr[set_num].end(), std::ostream_iterator<ReplState>(std::cout, " "));
        std::cout << std::endl;
    }
};

#endif /* ReplPolicy_hpp */
