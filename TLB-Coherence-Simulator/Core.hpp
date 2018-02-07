//
//  Core.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/14/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef Core_hpp
#define Core_hpp

#include <iostream>
#include "CacheSys.hpp"
#include "ROB.hpp"
#include "Request.hpp"
#include <deque>
#include <list>
#include <set>

class Core {
private:
    
    class AddrMapKey {
    public:
        uint64_t m_addr;
        bool m_is_large;
        
        AddrMapKey(uint64_t addr, bool is_large) : m_addr(addr), m_is_large(is_large) {}
        
        friend std::ostream& operator << (std::ostream& out, AddrMapKey &a)
        {
            out << "|" << a.m_addr << "|" << a.m_is_large << "|" << std::endl;
            return out;
        }
    };
    
    class AddrMapComparator {
    public:
        bool operator () (const AddrMapKey &a, const AddrMapKey &b) const
        {
            if(a.m_addr < b.m_addr)
                return true;
            if(a.m_is_large < b.m_is_large)
                return true;
            
            return false;
        }
    };
    
    std::shared_ptr<CacheSys> m_cache_hier;
    std::shared_ptr<CacheSys> m_tlb_hier;
    uint64_t m_l3_small_tlb_base = 0x0;
    uint64_t m_l3_small_tlb_size = 1024 * 1024;
    uint64_t m_clk;
    
    //Stats
    uint64_t m_num_issued  = 0;
    uint64_t m_num_retired = 0;
    
    unsigned int m_core_id;
    
    std::map<uint64_t, std::set<AddrMapKey, AddrMapComparator>> va2L3TLBAddr;
    
    std::deque<Request> traceVec;
    
public:
    std::shared_ptr<ROB> m_rob;
    Core(std::shared_ptr<CacheSys> cache_hier, std::shared_ptr<CacheSys> tlb_hier, std::shared_ptr<ROB> rob, uint64_t l3_small_tlb_base = 0x0, uint64_t l3_small_tlb_size = 1024 * 1024) :
        m_cache_hier(cache_hier),
        m_tlb_hier(tlb_hier),
        m_rob(rob),
        m_l3_small_tlb_base(l3_small_tlb_base),
        m_l3_small_tlb_size(l3_small_tlb_size)
        {
            assert(!cache_hier->get_is_translation_hier());
            assert(tlb_hier->get_is_translation_hier());
            m_clk = 0;
        }
    
    bool interfaceHier(bool ll_interface_complete);
    
    void set_core_id(unsigned int core_id);
    
    uint64_t getL3TLBAddr(uint64_t va, uint64_t pid, bool is_large);
    
    std::vector<uint64_t> retrieveAddr(uint64_t l3tlbaddr, uint64_t pid, bool is_large, bool is_higher_cache_small_tlb);
    
    std::shared_ptr<Cache> get_lower_cache(uint64_t addr, bool is_translation, bool is_large, unsigned int cache_level, CacheType cache_type);
    
    void tick();

    void add_trace(Request& req);
};

#endif /* Core_hpp */
