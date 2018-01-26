//
//  Core.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/14/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "Core.hpp"
#include "Cache.hpp"

void Core::interfaceHier()
{
    unsigned long num_tlbs = m_tlb_hier->m_caches.size();
    unsigned long num_caches = m_cache_hier->m_caches.size();
    
    std::shared_ptr<Cache> penultimate_tlb_large = m_tlb_hier->m_caches[num_tlbs - 3];
    std::shared_ptr<Cache> penultimate_tlb_small = m_tlb_hier->m_caches[num_tlbs - 4];
   
    std::shared_ptr<Cache> last_tlb_large = m_tlb_hier->m_caches[num_tlbs - 1];
    std::shared_ptr<Cache> last_tlb_small = m_tlb_hier->m_caches[num_tlbs - 2];
    
    std::shared_ptr<Cache> penultimate_cache = m_cache_hier->m_caches[num_caches - 2];
    
    std::shared_ptr<Cache> last_cache = m_cache_hier->m_caches[num_caches - 1];
    
    penultimate_tlb_large->add_lower_cache(penultimate_cache);
    penultimate_tlb_small->add_lower_cache(penultimate_cache);
    
    last_tlb_large->add_higher_cache(last_cache);
    last_tlb_small->add_higher_cache(last_cache);
    
    penultimate_cache->add_higher_cache(penultimate_tlb_large);
    penultimate_cache->add_higher_cache(penultimate_tlb_small);
    
    //If there are extra levels in between, interface them statically
    for(int i = 1; i < num_tlbs - 4; i++)
    {
        std::shared_ptr<Cache> current_tlb = m_tlb_hier->m_caches[i];
        std::shared_ptr<Cache> lower_tlb = m_tlb_hier->m_caches[i + 2];
        current_tlb->add_lower_cache(lower_tlb);
        lower_tlb->add_higher_cache(current_tlb);
    }
}

uint64_t Core::getL3TLBAddr(uint64_t va, uint64_t pid, bool is_large)
{
    // Convert virtual address to a TLB lookup address.
    // Use the is_large attribute to go to either the small L3 TLB or the large L3 TLB.
    
    uint64_t set_index;
    uint64_t l3_tlb_base_address;
    unsigned long num_tlbs = m_tlb_hier->m_caches.size();
    unsigned long last_level_small_tlb_idx = num_tlbs - 2;
    unsigned long last_level_large_tlb_idx = num_tlbs - 1;
    
    if (is_large)
    {  // VL-TLB large
        set_index            = m_tlb_hier->m_caches[last_level_large_tlb_idx]->get_index(va);
        l3_tlb_base_address  = m_l3_small_tlb_base + m_l3_small_tlb_size;
    }
    else
    {  // VL-TLB small
        set_index           = m_tlb_hier->m_caches[last_level_small_tlb_idx]->get_index(va);
        l3_tlb_base_address = m_l3_small_tlb_base;
    }
    
    uint64_t l3tlbaddr = l3_tlb_base_address + (set_index * 16 * 4);
    
    va2L3TLBAddr.insert(std::make_pair(l3tlbaddr, AddrMapKey(va, pid, is_large)));
    
    return l3tlbaddr; // each set holds 4 entries of 16B each.
}

uint64_t Core::retrieveActualAddr(uint64_t l3tlbaddr, uint64_t pid, bool is_large)
{
    auto iter = va2L3TLBAddr.find(l3tlbaddr);
    assert(iter != va2L3TLBAddr.end());
    uint64_t returnval = -1;
    
    if(pid == iter->second.m_pid && pid == iter->second.m_is_large)
    {
        returnval = iter->second.m_addr;
        va2L3TLBAddr.erase(iter);
    }
    
    return returnval;
}

std::shared_ptr<Cache> Core::get_lower_cache(uint64_t addr, bool is_translation, unsigned int level, CacheType cache_type)
{
    unsigned long num_tlbs = m_tlb_hier->m_caches.size();
    bool is_last_level_tlb = m_tlb_hier->is_last_level(level) && (cache_type == TRANSLATION_ONLY);
    bool is_penultimate_tlb = m_tlb_hier->is_penultimate_level(level) && (cache_type == TRANSLATION_ONLY);
    
    //If line is not translation entry and we are in lowest level cache, return nullptr
    //If we are in last level TLB, return nullptr
    if((!is_translation && m_cache_hier->is_last_level(level)) || is_last_level_tlb)
    {
        return nullptr;
    }
    
    //Not penultimate, return lower TLB (small/large)
    if(!is_penultimate_tlb && is_translation)
    {
        return (addr & 0x200000) ? m_tlb_hier->m_caches[2 * level - 1] : m_tlb_hier->m_caches[2 * level];
    }
    //Penultimate TLB. Return penultimate cache
    else if(is_penultimate_tlb && is_translation)
    {
        return m_cache_hier->m_caches[m_cache_hier->m_caches.size() - 2];
    }
    //Last level cache and translation line. Return appropriate last level TLB (small/large)
    else if(m_cache_hier->is_last_level(level) && is_translation)
    {
        return (addr & 0x200000) ? m_tlb_hier->m_caches[num_tlbs - 2] : m_tlb_hier->m_caches[num_tlbs - 1];
    }
    
    //Should never reach here!
    assert(true);
    return nullptr;
}

void Core::tick()
{
    m_tlb_hier->tick();
    m_cache_hier->tick();
    
    m_num_retired += m_rob.retire(m_clk);
    
    for(int i = 0; i < m_rob.m_issue_width; i++)
    {
        //Pull stuff out the trace and place it in ROB
    }
    
    m_clk++;
}
