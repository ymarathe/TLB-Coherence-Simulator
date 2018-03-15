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
#include <unordered_map>
#include <memory>
#include "utils.hpp"
#include "ReplPolicy.hpp"
#include "CacheLine.h"
#include "Request.hpp"
#include "Coherence.hpp"

class CacheSys;
class Core;

class Cache
{
private:
    
    class MSHREntry {
    public:
        kind m_txn_kind;
        CacheLine *m_line;
        bool m_is_core_agnostic;
        
        MSHREntry(kind txn_kind, CacheLine *line) : m_txn_kind(txn_kind), m_line(line), m_is_core_agnostic(false) {}
    };

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
    
    std::shared_ptr<Core> m_core;
    
    std::unordered_map<Request, MSHREntry*, RequestHasher> m_mshr_entries;
    
    unsigned int m_cache_level;
    unsigned int m_latency_cycles;
    
    std::function<void(std::shared_ptr<Request>)> m_callback;
    
    bool m_is_coherence_enabled;
    
    bool m_inclusive;
    
    CacheType m_cache_type;
    
    bool m_is_large_page_tlb;
    
    unsigned int m_core_id;
    
    bool m_is_callback_initialized;
    
public:
    uint64_t num_data_hits = 0;
    uint64_t num_tr_hits = 0;
    uint64_t num_data_misses = 0;
    uint64_t num_tr_misses = 0;
    uint64_t num_mshr_data_hits = 0;
    uint64_t num_mshr_tr_hits = 0;
    uint64_t num_data_accesses = 0;
    uint64_t num_tr_accesses = 0;

    Cache(int num_sets, int associativity, int line_size, unsigned int latency_cycles, CacheType cache_type = DATA_ONLY, bool is_large_page_tlb = false, enum ReplPolicyEnum pol = LRU_POLICY, enum CoherenceProtocolEnum prot = MOESI_COHERENCE, bool inclusive = false):
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
                set.emplace_back();
                
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
        
        m_inclusive = inclusive;
        
        m_cache_type = cache_type;
        
        m_is_large_page_tlb = is_large_page_tlb;
        
        m_is_callback_initialized = false;
    }

    void initialize_callback();
    uint64_t get_index(const uint64_t addr);
    uint64_t get_tag(const uint64_t addr);
    uint64_t get_line_offset(const uint64_t addr);
    bool is_found(const std::vector<CacheLine>& set, const uint64_t tag, bool is_translation, uint64_t tid, unsigned int &hit_pos);
    bool is_hit(const std::vector<CacheLine> &set, const uint64_t tag, bool is_translation, uint64_t tid, unsigned int &hit_pos);
    void invalidate(const uint64_t addr, uint64_t tid, bool is_translation);
    void evict(uint64_t set_num, const CacheLine &line);
    RequestStatus lookupAndFillCache(Request &r, unsigned int curr_latency = 0, CoherenceState propagate_coh_state = INVALID);
    void add_lower_cache(const std::weak_ptr<Cache>& c);
    void add_higher_cache(const std::weak_ptr<Cache>& c);
    void set_level(unsigned int level);
    unsigned int get_level();
    void release_lock(std::shared_ptr<Request> r);
    void printContents();
    void set_cache_sys(CacheSys *cache_sys);
    unsigned int get_latency_cycles();
    bool handle_coherence_action(CoherenceAction coh_action, Request& r, unsigned int curr_latency, bool same_cache_sys);
    void set_cache_type(CacheType cache_type);
    CacheType get_cache_type();
    void set_core(std::shared_ptr<Core>& coreptr);
    std::shared_ptr<Cache> find_lower_cache_in_core(uint64_t addr, bool is_translation, bool is_large = false);
    void propagate_release_lock(std::shared_ptr<Request> r);
    bool get_is_large_page_tlb();
    void set_core_id(unsigned int core_id);
    unsigned int get_core_id();
    bool is_found_by_cotag(uint64_t pom_tlb_addr, unsigned int &index, unsigned int &hit_pos);
};
#endif /* Cache_hpp */
