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

uint64_t Core::getL3TLBAddr(uint64_t va, kind type, uint64_t tid, bool is_large, bool insert)
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
    
    if(insert)
    {
        if(va2L3TLBAddr.find(l3tlbaddr) != va2L3TLBAddr.end())
        {
            va2L3TLBAddr[l3tlbaddr].insert(AddrMapKey(va, type, tid, is_large));
        }
        else
        {
            va2L3TLBAddr[l3tlbaddr] = {};
            va2L3TLBAddr[l3tlbaddr].insert(AddrMapKey(va, type, tid, is_large));
        }
    }
    
    return l3tlbaddr; // each set holds 4 entries of 16B each.
}

std::vector<uint64_t> Core::retrieveAddr(uint64_t l3tlbaddr, kind type, uint64_t tid, bool is_large, bool is_higher_cache_small_tlb)
{
    auto iter = va2L3TLBAddr.find(l3tlbaddr);
    bool propagate_access = true;
    std::vector<uint64_t> addresses = {};
    
    if(iter != va2L3TLBAddr.end())
    {
    
        for(std::set<AddrMapKey>::iterator amk_iter = iter->second.begin(); amk_iter != iter->second.end();)
        {
            AddrMapKey val = *amk_iter;
            if(is_large == val.m_is_large && tid == val.m_tid && val.m_type == type)
            {
                //propagate_to_small_tlb = 0, go with large
                //propagate_to_small_tlb = 1, go with small
                //bool propagate_to_small_tlb = ((val.m_addr & 0x200000) != 0);
                bool propagate_to_small_tlb = !val.m_is_large;
                propagate_access = (propagate_to_small_tlb == is_higher_cache_small_tlb);
                if(propagate_access)
                {
                    //va2L3TLBAddr.erase(iter);
                    addresses.push_back(val.m_addr);
                    amk_iter = iter->second.erase(amk_iter);
                }
                else
                {
                    amk_iter++;
                }
            }
            else
            {
                amk_iter++;
            }
        }
        
        //If all the entries are to be propagated, remove entry from va2L3TLBAddr
        if(iter->second.size() == 0)
        {
            va2L3TLBAddr.erase(iter);
        }
    }
    
    return addresses;
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
        //return (addr & 0x200000) ? m_tlb_hier->m_caches[level - (level % 2) + 2] : m_tlb_hier->m_caches[level - (level % 2) + 3];
        return (is_large) ? m_tlb_hier->m_caches[level - (level % 2) + 3] : m_tlb_hier->m_caches[level - (level % 2) + 2];
    }
    
    //Penultimate TLB. Return penultimate cache
    if(is_penultimate_tlb && is_translation)
    {
        return m_cache_hier->m_caches[m_cache_hier->m_caches.size() - 2];
    }
    
    std::cout << "Level = " << level << ", is_translation = " << is_translation << "\n";
    //Should never reach here!
    assert(false);
    return nullptr;
}

void Core::tick()
{
    m_tlb_hier->tick();
    m_cache_hier->tick();
    
#ifdef BASELINE
    if(stall)
    {
        //Till translation coherence is serviced, stall issue
        if(num_stall_cycles_per_shootdown == tlb_shootdown_penalty)
        {
            //Invalidate from other cores
            for(int i = 0; i < m_other_cores.size(); i++)
            {
                m_other_cores[i]->tlb_invalidate(tlb_shootdown_addr, tlb_shootdown_tid, tlb_shootdown_is_large);
            }
            stall = false;
            std::cout << "Unstalling core " << m_core_id << " at cycle = " << m_clk << "\n";
            std::cout << "Number of stall cycles = " << num_stall_cycles << " on core " << m_core_id << "\n";
            std::cout << "Stall on core " << m_core_id << " = " << stall << "\n";
            std::cout << "Number of shootdowns on core = " << m_core_id << " = " << num_shootdown << "\n";
        }
        else if(!m_rob->is_empty())
        {
            num_stall_cycles++;
            num_stall_cycles_per_shootdown++;
        }
    }
#else
    if(tr_wr_in_progress && m_rob->m_window[tr_coh_issue_ptr].done)
    {
        //If translation coherence request is serviced, issue CLFLUSH
        bool is_translation = true;
        std::cout << "Translation write done = " << m_rob->m_window[tr_coh_issue_ptr].done << ", flushing caches\n";
        m_cache_hier->clflush(tlb_shootdown_addr, tlb_shootdown_tid, is_translation);
        num_stall_cycles += 100;
        std::cout << "Flushed caches\n";

        for(int i = 0; i < m_cache_hier->m_caches.size(); i++)
        {
            //Request *r = m_rob->m_window[tr_coh_issue_ptr].req;
            Request req(tlb_shootdown_addr, TRANSLATION_READ, tlb_shootdown_tid, tlb_shootdown_is_large, m_core_id);
            assert(!m_cache_hier->m_caches[i]->lookupCache(req));
        }
        tr_wr_in_progress = false;
        std::cout << "Number of stall cycles = " << num_stall_cycles << " on core " << m_core_id << "\n";
        std::cout << "Number of shootdowns on core = " << m_core_id << " = " << num_shootdown << "\n";
    }
#endif
    
    if(!stall)
    {
        m_num_retired += m_rob->retire(m_clk);
    }

    if(m_rob->request_queue.size() > 0)
    {
        Request req = m_rob->request_queue.front();
        auto rr_iter = m_rob->is_request_ready.find(req);
        assert(rr_iter != m_rob->is_request_ready.end());
        bool can_issue = rr_iter->second.ready;

        if(can_issue && !stall)
        {

            if(req.m_is_read && req.m_is_translation)
            {
                req.update_request_type_from_core(TRANSLATION_READ);
            }
            else if(req.m_is_read && !req.m_is_translation)
            {
                req.update_request_type_from_core(DATA_READ);
            }
            else if(!req.m_is_read && req.m_is_translation)
            {
                req.update_request_type_from_core(TRANSLATION_WRITE);
            }
            else if(!req.m_is_read && !req.m_is_translation)
            {
                req.update_request_type_from_core(DATA_WRITE);
            }

            if(req.m_type != TRANSLATION_WRITE)
            {
                RequestStatus data_req_status = m_cache_hier->lookupAndFillCache(req);

                if(data_req_status != REQUEST_RETRY)
                {
                    rr_iter->second.num_occ_in_req_queue--;
                    if(rr_iter->second.num_occ_in_req_queue == 0)
                    {
                        m_rob->is_request_ready.erase(rr_iter);
                    }
                    m_rob->request_queue.pop_front();
                }
            }
            else if(req.m_type == TRANSLATION_WRITE)
            {
#ifdef BASELINE
                rr_iter->second.num_occ_in_req_queue--;
                if(rr_iter->second.num_occ_in_req_queue == 0)
                {
                    m_rob->is_request_ready.erase(rr_iter);
                }
                m_rob->request_queue.pop_front();
                m_rob->mem_mark_done(req);
                stall = true;
                tlb_shootdown_addr = req.m_addr;
                tlb_shootdown_tid = req.m_tid;
                tlb_shootdown_is_large = req.m_is_large;
                #ifdef IDEAL
                tlb_shootdown_penalty = 0;
                #else
                #ifdef SHOOTDOWN_PENALTY
                tlb_shootdown_penalty = SHOOTDOWN_PENALTY;
                #else
                tlb_shootdown_penalty = 0;
                #endif
                #endif
                num_stall_cycles_per_shootdown = 0;
                num_shootdown++;
                std::cout << "Stalling core " << m_core_id << " at cycle = " << m_clk << " until translation coherence is complete\n";
#else
                std::cout << "Issuing translation coherence write to data hierarchy\n";
                RequestStatus data_req_status = m_cache_hier->lookupAndFillCache(req);
                if(data_req_status != REQUEST_RETRY)
                {
                    rr_iter->second.num_occ_in_req_queue--;
                    if(rr_iter->second.num_occ_in_req_queue == 0)
                    {
                        m_rob->is_request_ready.erase(rr_iter);
                    }
                    m_rob->request_queue.pop_front();
                    tr_wr_in_progress = true;
                    tlb_shootdown_addr = req.m_addr;
                    tlb_shootdown_tid = req.m_tid;
                    tlb_shootdown_is_large = req.m_is_large;
                    num_stall_cycles_per_shootdown = 0;
                    num_shootdown++;
                    std::cout << "Stalling core " << m_core_id << " at cycle = " << m_clk << " until translation coherence is complete\n";
                }
#endif
            }
        }
    }

    for(int i = 0; i < m_rob->m_issue_width && !traceVec.empty() && m_rob->can_issue() && !stall; i++)
    {
        Request *req = traceVec.front();
        kind act_req_kind = req->m_type;
        
        if(req->m_is_memory_acc && (req->m_type != TRANSLATION_WRITE))
        {
            req->update_request_type_from_core(TRANSLATION_READ);
            RequestStatus tlb_req_status = m_tlb_hier->lookupAndFillCache(*req);
            req->update_request_type_from_core(act_req_kind);
            if(tlb_req_status != REQUEST_RETRY)
            {
                m_rob->issue(req->m_is_memory_acc, req, m_clk);
                traceVec.pop_front();
            }
        }
        else if(req->m_is_memory_acc && (req->m_type == TRANSLATION_WRITE))
        {
            std::cout << "Encountered TLB shootdown request on Core " << m_core_id << "\n";
            std::cout << std::hex << (*req) << std::dec;

            //Local TLB flush
            tlb_invalidate(req->m_addr, req->m_tid, req->m_is_large);

            //We generate a store to the POM-TLB address here
            req->m_addr = getL3TLBAddr(req->m_addr, req->m_type, req->m_tid, req->m_is_large, false);

            //Note the issue_ptr in the ROB
            tr_coh_issue_ptr = m_rob->m_issue_ptr;

            //Issue in ROB and remove from issue queue
            m_rob->issue(req->m_is_memory_acc, req, m_clk);
            traceVec.pop_front();

            //Mark as translation done in is_request_ready queue
            //Ready for dispatch to data hierarchy
            m_rob->mem_mark_translation_done(*req);

            m_rob->peek(tr_coh_issue_ptr);
            std::cout << "Translation write done? " << m_rob->m_window[tr_coh_issue_ptr].done << "\n";
        }
        else
        {
            m_rob->issue(req->m_is_memory_acc, req, m_clk);
            traceVec.pop_front();
        }
    }

    if(!m_rob->is_empty() && !stall)
    {
        m_clk++;
    }
}

void Core::set_core_id(unsigned int core_id)
{
    m_core_id = core_id;
    
    m_tlb_hier->set_core_id(core_id);
    m_cache_hier->set_core_id(core_id);
}

void Core::add_trace(Request *req)
{
    traceVec.push_back(req);
}

bool Core::is_done()
{
	return ((m_clk > 0) && (m_rob->is_empty()) && (m_tlb_hier->is_done()) && (m_cache_hier->is_done()));
}

bool Core::must_add_trace()
{
    return (traceVec.size() < 1000000);
}

void Core::tlb_invalidate(uint64_t addr, uint64_t tid, bool is_large)
{
    m_tlb_hier->tlb_invalidate(addr, tid, is_large);
}

void Core::add_core(std::shared_ptr<Core> other_core)
{
    m_other_cores.push_back(other_core);
}
