//
//  CacheSys.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/7/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "CacheSys.hpp"
#include "Core.hpp"
#include "Cache.hpp"

void CacheSys::add_cache_to_hier(std::shared_ptr<Cache> cache)
{
    
    if(m_is_translation_hier)
    {
        assert(cache->get_cache_type() == TRANSLATION_ONLY);
    }
    else
    {
        assert(cache->get_cache_type() != TRANSLATION_ONLY);
    }
    
    if(!m_is_translation_hier)
    {
        if(m_caches.size() > 0)
        {
            cache->add_higher_cache(m_caches[m_caches.size() - 1]);
        }

        cache->set_level(int(m_caches.size() + 1));
     
        //Add lower cache for the previous last level cache
        if(m_caches.size() > 0)
        {
            m_caches[m_caches.size() - 1]->add_lower_cache(cache);
        }
    }
    else
    {
        unsigned long cur_size = m_caches.size();
        
        cache->set_level((unsigned int)((cur_size + 2)/2));
    }
    
    //Point current cache lower_cache to nullptr
    cache->add_lower_cache(std::weak_ptr<Cache>());
    
    //Set cache sys pointer
    cache->set_cache_sys(this);
    cache->set_core(m_core);
    
    m_caches.push_back(cache);

    if(m_is_translation_hier)
    {
        assert(m_caches.size() <= NUM_MAX_CACHES * 2);
    }
    else
    {
        assert(m_caches.size() <= NUM_MAX_CACHES);
    }
}

void CacheSys::add_cachesys(std::shared_ptr<CacheSys> cs)
{
    m_other_cache_sys.push_back(cs);
}

void CacheSys::tick()
{
    //First, handle coherence actions in the current clock cycle
    bool needs_state_correction = false;
    bool state_corrected = false;
    for(std::map<std::shared_ptr<Request>, CoherenceAction>::iterator it = m_coh_act_list.begin();
        it != m_coh_act_list.end(); )
    {
        int limit = (int) (m_is_translation_hier ? m_caches.size() - 2 : m_caches.size() - 1);
        for(int i = 0; i < limit; i++)
        {
            needs_state_correction = m_caches[i]->handle_coherence_action(it->second, *it->first, 0, false);
            
            if(needs_state_correction && !state_corrected)
            {
                m_caches[i]->handle_coherence_action(STATE_CORRECTION, *it->first, 0, true);
                state_corrected = true;
            }
        }
        
        it = m_coh_act_list.erase(it);
    }
    
    assert(m_coh_act_list.empty());
    
    //Retire elements from hit list
    for(std::map<uint64_t, std::shared_ptr<Request>>::iterator it = m_hit_list.begin();
        it != m_hit_list.end();
        )
    {
        if(m_clk >= it->first)
        {
            it->second->m_callback(it->second);
            it = m_hit_list.erase(it);
        }
        else
        {
            it++;
        }
    }
    
    //Then retire elements from wait list
    for(std::map<uint64_t, std::shared_ptr<Request>>::iterator it = m_wait_list.begin();
        it != m_wait_list.end();
        )
    {
        if(m_clk >= it->first)
        {
            it->second->m_callback(it->second);
            it = m_wait_list.erase(it);
        }
        else
        {
            it++;
        }
    }
    m_clk++;
}

bool CacheSys::is_last_level(unsigned int cache_level)
{
    if(m_is_translation_hier)
    {
        return (cache_level == (m_caches.size()/2));
    }
    
    return (cache_level == m_caches.size());
}

bool CacheSys::is_penultimate_level(unsigned int cache_level)
{
    if(m_is_translation_hier)
    {
        return (cache_level == (m_caches.size()/2) - 1);
    }
    
    return (cache_level == m_caches.size()  - 1);
}

void CacheSys::printContents()
{
    for(int i = 0; i < m_caches.size(); i++)
    {
        m_caches[i]->printContents();
        std::cout << "------------------------" << std::endl;
    }
}

void CacheSys::set_core_id(int core_id)
{
    m_core_id = core_id;
    int limit = (m_is_translation_hier) ? (int)(m_caches.size() - 2) : (int)(m_caches.size() - 1);
    
    for(int i = 0; i < limit; i++)
    {
        m_caches[i]->set_core_id(core_id);
    }
    
    //Identifier for last level TLB/cache.
    if(m_is_translation_hier)
    {
        m_caches[m_caches.size() - 2]->set_core_id(-1);
    }
    
    m_caches[m_caches.size() - 1]->set_core_id(-1);
}

RequestStatus CacheSys::lookupAndFillCache(Request &r)
{
    if(!m_is_translation_hier)
    {
        return m_caches[0]->lookupAndFillCache(r, m_caches[0]->get_latency_cycles());
    }
    else
    {
        return (r.m_is_large) ? m_caches[1]->lookupAndFillCache(r, m_caches[1]->get_latency_cycles()) : m_caches[0]->lookupAndFillCache(r, m_caches[0]->get_latency_cycles());
    }
}

void CacheSys::set_core(std::shared_ptr<Core>& coreptr)
{
    m_core = coreptr;
    
    for(int i = 0; i < m_caches.size(); i++)
    {
        m_caches[i]->set_core(coreptr);
    }
}

bool CacheSys::get_is_translation_hier()
{
    return m_is_translation_hier;
}

unsigned int CacheSys::get_core_id()
{
    return m_core_id;
}

void CacheSys::clflush(uint64_t addr, uint64_t tid, bool is_translation)
{
    for(int i = 0; i < m_caches.size(); i++)
    {
        m_caches[i]->invalidate(addr, tid, is_translation);
    }
}

void CacheSys::tlb_invalidate(uint64_t addr, uint64_t tid, bool is_large)
{
    assert(m_is_translation_hier);

    int start = (is_large) ? 1 : 0;

    for(int i = start; i < m_caches.size(); i += 2)
    {
        m_caches[i]->invalidate(addr, tid, true);

        //If penultimate level, remove entry from presence map 
        if(is_penultimate_level(m_caches[i]->get_level()))
        {
            TraceProcessor *tp = m_caches[i]->get_traceprocessor();
            std::cout << "[INVALIDATION] Removing from presence map, Addr = " << std::hex << addr << std::dec << ", tid = " << tid << ", is_large = " << m_caches[i]->get_is_large_page_tlb() << ", on core = " << m_core_id << "\n"; 
            tp->remove_from_presence_map(addr, tid, m_caches[i]->get_is_large_page_tlb(), m_core_id);
        }
    }

    for(int i = start; i < m_caches.size(); i += 2)
    {
        Request req(addr, TRANSLATION_READ, tid, is_large, m_core_id); 
        //TODO: FIXME: Enable this assert
        //assert(!m_caches[i]->lookupCache(req));
    }
}

bool CacheSys::is_done()
{
    return ((m_wait_list.size() == 0) && (m_hit_list.size() == 0));
}
