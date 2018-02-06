//
//  Core.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/14/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "Core.hpp"
#include "Cache.hpp"

bool Core::interfaceHier(bool ll_interface_complete)
{
    unsigned long num_tlbs = m_tlb_hier->m_caches.size();
    unsigned long num_caches = m_cache_hier->m_caches.size();
    
    assert(num_tlbs >= MIN_NUM_TLBS);
    assert(num_caches >= MIN_NUM_CACHES);
    
    std::shared_ptr<Cache> penultimate_tlb_large = m_tlb_hier->m_caches[num_tlbs - 3];
    std::shared_ptr<Cache> penultimate_tlb_small = m_tlb_hier->m_caches[num_tlbs - 4];
   
    std::shared_ptr<Cache> last_tlb_large = m_tlb_hier->m_caches[num_tlbs - 1];
    std::shared_ptr<Cache> last_tlb_small = m_tlb_hier->m_caches[num_tlbs - 2];
    
    std::shared_ptr<Cache> penultimate_cache = m_cache_hier->m_caches[num_caches - 2];
    
    std::shared_ptr<Cache> last_cache = m_cache_hier->m_caches[num_caches - 1];
    
    penultimate_tlb_large->add_lower_cache(penultimate_cache);
    penultimate_tlb_small->add_lower_cache(penultimate_cache);
    
    if(!ll_interface_complete)
    {
        last_tlb_large->add_higher_cache(last_cache);
        last_tlb_small->add_higher_cache(last_cache);
        ll_interface_complete = true;
    }
    
    penultimate_cache->add_higher_cache(penultimate_tlb_small);
    penultimate_cache->add_higher_cache(penultimate_tlb_large);
    
    assert(penultimate_tlb_large->get_is_large_page_tlb());
    assert(!penultimate_tlb_small->get_is_large_page_tlb());

    for(int i = 2; i < num_tlbs - 2; i++)
    {
        std::shared_ptr<Cache> current_tlb = m_tlb_hier->m_caches[i];
        current_tlb->add_higher_cache(m_tlb_hier->m_caches[i - (i % 2) - 1]);
        current_tlb->add_higher_cache(m_tlb_hier->m_caches[i - (i % 2) - 2]);
    }
    
    return ll_interface_complete;
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

uint64_t Core::retrieveAddr(uint64_t l3tlbaddr, uint64_t pid, bool is_large, bool is_higher_cache_small_tlb, bool *propagate_access)
{
    auto iter = va2L3TLBAddr.find(l3tlbaddr);
    uint64_t returnval = -1;
    
    if(iter != va2L3TLBAddr.end() && pid == iter->second.m_pid && is_large == iter->second.m_is_large)
    {
        returnval = iter->second.m_addr;
        //propagate_to_small_tlb = 0, go with large
        //propagate_to_small_tlb = 1, go with small
        bool propagate_to_small_tlb = ((returnval & 0x200000) != 0);
        *propagate_access = (propagate_to_small_tlb == is_higher_cache_small_tlb);
        if(propagate_access)
        {
            va2L3TLBAddr.erase(iter);
        }
        return returnval;
    }
    
    *propagate_access = false;
    
    return returnval;
}

std::shared_ptr<Cache> Core::get_lower_cache(uint64_t addr, bool is_translation, bool is_large, unsigned int level, CacheType cache_type)
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
    //Last level cache and translation line. Return appropriate last level TLB (small/large)
    if(m_cache_hier->is_last_level(level) && is_translation)
    {
        return (is_large) ? m_tlb_hier->m_caches[num_tlbs - 1] : m_tlb_hier->m_caches[num_tlbs - 2];
    }
    
    //Not penultimate, return lower TLB (small/large)
    if(!is_penultimate_tlb && is_translation)
    {
        return (addr & 0x200000) ? m_tlb_hier->m_caches[level - (level % 2) + 2] : m_tlb_hier->m_caches[level - (level % 2) + 3];
    }
    
    //Penultimate TLB. Return penultimate cache
    if(is_penultimate_tlb && is_translation)
    {
        return m_cache_hier->m_caches[m_cache_hier->m_caches.size() - 2];
    }
    
    //Should never reach here!
    assert(false);
    return nullptr;
}

void Core::tick()
{
    m_tlb_hier->tick();
    m_cache_hier->tick();
    
    /*std::cout << "In data hier = " << std::dec << m_cache_hier->m_clk << std::endl;
    std::cout << "In TLB hier = " << std::dec << m_tlb_hier->m_clk << std::endl;*/
    
    m_num_retired += m_rob->retire(m_clk);
    
    for(std::list<Request>::iterator it = m_rob->data_hier_issueQ.begin(); it != m_rob->data_hier_issueQ.end();)
    {
        Request &req = *it;
        
        if(req.m_is_read && req.m_is_translation)
        {
            req.m_type = TRANSLATION_READ;
        }
        else if(req.m_is_read && !req.m_is_translation)
        {
            req.m_type = DATA_READ;
        }
        else if(!req.m_is_read && req.m_is_translation)
        {
            req.m_type = TRANSLATION_WRITE;
        }
        else if(!req.m_is_read && !req.m_is_translation)
        {
            req.m_type = DATA_WRITE;
        }

        RequestStatus data_req_status = m_cache_hier->lookupAndFillCache(req);
        if(data_req_status != REQUEST_RETRY)
        {
            std::cout << "Request issued to data hierarchy = " << std::hex << req.m_addr << std::endl;
            it = m_rob->data_hier_issueQ.erase(it);
        }
        else
        {
            it++;
        }
    }
    
    /*std::cout << "Tracevec empty? = " << traceVec.empty() << std::endl;
    std::cout << "ROB can issue? = " << m_rob->can_issue() << std::endl;
    std::cout << "Waiting instr = " << m_rob->m_num_waiting_instr << std::endl;*/
    
    for(int i = 0; i < m_rob->m_issue_width && !traceVec.empty() && m_rob->can_issue(); i++)
    {
        Request &req = traceVec.front();
        std::cout << "is_read = " << req.m_is_read << ", is_translation = " << req.m_is_translation << std::endl;
        kind act_req_kind = TRANSLATION_READ;
        std::cout << "is_read = " << req.m_is_read << ", is_translation = " << req.m_is_translation << std::endl;
        
        if(req.m_is_memory_acc)
        {
            std::swap(act_req_kind, req.m_type);
            RequestStatus tlb_req_status = m_tlb_hier->lookupAndFillCache(req);
            std::swap(act_req_kind, req.m_type);
            if(tlb_req_status != REQUEST_RETRY)
            {
                std::cout << "Request issued TLB hierarchy for address = " << std::hex << req.m_addr << std::endl;
                m_rob->issue(req.m_is_memory_acc, req, m_clk);
                std::cout << m_rob->m_window[0].req->m_addr << std::endl;
                traceVec.pop_front();
            }
        }
        else
        {
            m_rob->issue(req.m_is_memory_acc, req, m_clk);
            traceVec.pop_front();
        }
    }
    
    m_clk++;
}

void Core::set_core_id(unsigned int core_id)
{
    m_core_id = core_id;
    
    m_tlb_hier->set_core_id(core_id);
    m_cache_hier->set_core_id(core_id);
}

void Core::add_trace(Request &req)
{
    traceVec.push_back(req);
}
