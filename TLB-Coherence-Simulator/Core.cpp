//
//  Core.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/14/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "Core.hpp"
#include "Cache.hpp"

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
    
    return l3_tlb_base_address + (set_index * 16 * 4); // each set holds 4 entries of 16B each.
}

std::shared_ptr<Cache> Core::get_lower_cache(uint64_t addr, bool is_translation, unsigned int level, CacheType cache_type)
{
    unsigned long num_tlbs = m_tlb_hier->m_caches.size();
    bool is_last_level_tlb = (level == num_tlbs) || (level == (num_tlbs - 1));
    bool is_penultimate_tlb = (level == (num_tlbs - 2)) || (level == (num_tlbs - 3));
    
    if((!is_translation && m_cache_hier->is_last_level(level)) || is_last_level_tlb)
    {
        return nullptr;
    }
    
    //L1 TLB, return appropriate L2 TLB (small/large)
    if(level == 0 && cache_type == TRANSLATION_ONLY && is_translation)
    {
        return (addr & 0x200000) ? m_tlb_hier->m_caches[level + 1] : m_tlb_hier->m_caches[level + 2];
    }
    
    //Penultimate TLB. Return penultimate cache
    if(is_penultimate_tlb && (cache_type == TRANSLATION_ONLY) && is_translation)
    {
        return m_cache_hier->m_caches[m_cache_hier->m_caches.size() - 2];
    }
    
    //Last level cache and translation line. Return appropriate last level TLB (small/large)
    if(m_cache_hier->is_last_level(level) && is_translation)
    {
        return (addr & 0x200000) ? m_tlb_hier->m_caches[num_tlbs - 2] : m_tlb_hier->m_caches[num_tlbs - 1];
    }
    
    //Should never reach here!
    assert(true);
    return nullptr;
}

