//
//  Cache.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef Cache_hpp
#define Cache_hpp

#include <iostream>
#include <vector>
#include <map>
#include "utils.hpp"
#include "ReplPolicy.hpp"
#include "CacheLine.h"
#include "Request.hpp"
#include "Coherence.hpp"

class CacheSys;

class Cache
{
private:
    unsigned int m_num_sets;
    unsigned int m_associativity;
    unsigned int m_line_size;
    unsigned int m_num_line_offset_bits;
    unsigned int m_num_index_bits;
    unsigned int m_num_tag_bits;
    
    std::vector<std::weak_ptr<Cache>> m_higher_caches;
    std::weak_ptr<Cache> m_lower_cache;
    
    std::vector<std::vector<CacheLine>> m_tagStore;
    ReplPolicy *m_repl;
    
    CacheSys *m_cache_sys;
    
    std::map<uint64_t, CacheLine*> m_mshr_entries;
    
    unsigned int m_cache_level;
    unsigned int m_latency_cycles;
    
    std::function<void(std::unique_ptr<Request>&)> m_callback;
    
    bool m_is_coherence_enabled;
    
public:
    Cache(int num_sets, int associativity, int line_size, unsigned int latency_cycles, enum ReplPolicyEnum pol = LRU_POLICY, enum CoherenceProtocolEnum prot = MOESI_COHERENCE):
    m_num_sets(num_sets), m_associativity(associativity), m_line_size(line_size), m_latency_cycles(latency_cycles)
    {
        
        m_num_line_offset_bits = log2(m_line_size);
        m_num_index_bits = log2(m_num_sets);
        m_num_tag_bits = ADDR_SIZE - m_num_line_offset_bits - m_num_index_bits;
        
        m_tagStore.reserve(m_num_sets);
        for(int i = 0; i < m_num_sets; i++)
        {
            std::vector<CacheLine> set;
            for(int j = 0; j < m_associativity; j++)
            {
                set.emplace_back(CacheLine());
                
                //Want to associate a coherence protocol object with each cache line
                CoherenceProtocol *coherence_prot = new MOESIProtocol();
                set[j].set_coherence_prot(coherence_prot);
                
            }
            m_tagStore.push_back(set);
        }
        
        switch(pol) {
            case LRU_POLICY:
            default:
                m_repl = new LRURepl(m_num_sets, m_associativity);
                break;
        }
        
        m_is_coherence_enabled = (prot != NO_COHERENCE);

    }
    
    ~Cache()
    {
        std::cout << "Deleting cache::" << m_cache_level << std::endl;
    }
    
    uint64_t get_index(const uint64_t addr);
    uint64_t get_tag(const uint64_t addr);
    uint64_t get_line_offset(const uint64_t addr);
    bool is_found(const std::vector<CacheLine>& set, const uint64_t tag, unsigned int &hit_pos);
    bool is_hit(const std::vector<CacheLine> &set, const uint64_t tag, unsigned int &hit_pos);
    void invalidate(const uint64_t addr);
    void evict(uint64_t set_num, const CacheLine &line);
    RequestStatus lookupAndFillCache(const uint64_t addr, kind txn_kind);
    void add_lower_cache(const std::weak_ptr<Cache>& c);
    void add_higher_cache(const std::weak_ptr<Cache>& c);
    void set_level(unsigned int level);
    unsigned int get_level();
    void release_lock(std::unique_ptr<Request> &r);
    void printContents();
    void set_cache_sys(CacheSys *cache_sys);
    unsigned int get_latency_cycles();
    void handle_coherence_action(CoherenceAction coh_action, uint64_t addr = 0x0, bool same_cache_sys = true);
};
#endif /* Cache_hpp */
