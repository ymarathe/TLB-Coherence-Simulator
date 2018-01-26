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
class Core;

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
    
    //This is where requests wait until they are served a memory access
    std::map<uint64_t, std::unique_ptr<Request>> m_wait_list;
    
    //This is where requests wait until they are served by a hit
    std::map<uint64_t, std::unique_ptr<Request>> m_hit_list;
    
    //This is where coherence actions wait until they are served
    std::map<uint64_t, CoherenceAction> m_coh_act_list;
    
    uint64_t m_clk;
    
    uint64_t m_cache_latency_cycles[NUM_MAX_CACHES];
    uint64_t m_total_latency_cycles[NUM_POPULATED_LATENCIES];
    
    uint64_t m_memory_latency;
    uint64_t m_cache_to_cache_latency;
    
    std::vector<std::shared_ptr<CacheSys>> m_other_cache_sys;
    
    int m_core_id;
    
    Core *m_core;
    
    CacheSys(uint64_t memory_latency = 200, uint64_t cache_to_cache_latency = 50) :
    m_clk(0), m_memory_latency(memory_latency), m_cache_to_cache_latency(cache_to_cache_latency)
    {
         m_total_latency_cycles[MEMORY_ACCESS_ID] = m_memory_latency;
    }
    
    void add_cache_to_hier(std::shared_ptr<Cache> c);
    
    void add_cachesys(std::shared_ptr<CacheSys> cs);
    
    void set_core(Core* coreptr);
    
    void tick();
    
    bool is_last_level(unsigned int cache_level);
    
    void printContents();
    
    void set_core_id(int core_id);
    
    RequestStatus lookupAndFillCache(const uint64_t addr, kind txn_kind);
    
};

#endif /* CacheSys_hpp */
