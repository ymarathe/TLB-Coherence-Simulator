//
//  CacheSys.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/7/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef CacheSys_hpp
#define CacheSys_hpp

#include <iostream>
#include <vector>
#include <assert.h>
#include <map>
#include "Request.hpp"

class Cache;

enum {
    L1_HIT_ID,
    L2_HIT_ID,
    L3_HIT_ID,
    NUM_MAX_CACHES,
    MEMORY_ACCESS_ID,
    CACHE_TO_CACHE_ID,
    NUM_POPULATED_LATENCIES
};

class CacheSys {
public:
    
    std::vector<std::shared_ptr<Cache>> m_caches;
    
    //This is where requests wait until they are served
    std::map<uint64_t, std::unique_ptr<Request>> m_wait_list;
    
    uint64_t m_clk;
    
    uint64_t m_cache_latency_cycles[NUM_MAX_CACHES];
    uint64_t m_total_latency_cycles[NUM_POPULATED_LATENCIES];
    
    uint64_t m_memory_latency;
    uint64_t m_cache_to_cache_latency;
    
    CacheSys(uint64_t memory_latency = 200, uint64_t cache_to_cache_latency = 50) :
    m_clk(0), m_memory_latency(memory_latency), m_cache_to_cache_latency(cache_to_cache_latency)
    {
         m_total_latency_cycles[MEMORY_ACCESS_ID] = m_memory_latency;
    }

    CacheSys(std::vector<std::shared_ptr<Cache>> &caches, uint64_t memory_latency = 200, uint64_t cache_to_cache_latency = 50):
        m_caches(caches),
        m_clk(0),
        m_memory_latency(memory_latency),
        m_cache_to_cache_latency(cache_to_cache_latency)
    {
        m_total_latency_cycles[MEMORY_ACCESS_ID] = m_memory_latency;
        makeCachesSentient();
    }
    
    void makeCachesSentient();
    
    void add_cache_to_hier(std::shared_ptr<Cache>& c);
    
    void tick();
    
    bool is_last_level(unsigned int cache_level);
    
    std::shared_ptr<Cache>& operator [](int idx)
    {
        return m_caches.at(idx);
    }
    
    void printContents();
};

#endif /* CacheSys_hpp */
