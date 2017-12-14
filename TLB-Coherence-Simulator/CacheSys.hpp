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
    L1_HIT_ID = 1,
    L2_HIT_ID = 2,
    L3_HIT_ID = 3,
    MEMORY_HIT_ID = 4,
};

class CacheSys {
public:
    
    std::vector<std::shared_ptr<Cache>> m_caches;
    
    //This is where requests wait until they are served
    std::map<uint64_t, Request> m_wait_list;
    
    uint64_t m_clk;
    
    uint64_t latency_cycles[5] = {0, 4, 4 + 12, 4 + 12 + 31, 4 + 12 + 31 + 200};
    
    CacheSys() : m_clk(0) {}

    CacheSys(std::vector<std::shared_ptr<Cache>>& caches) :
        m_caches(caches),
        m_clk(0)
    {
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
};

#endif /* CacheSys_hpp */
