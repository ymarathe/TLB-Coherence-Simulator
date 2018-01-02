//
//  CacheSys.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/7/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "CacheSys.hpp"
#include "Cache.hpp"

void CacheSys::add_cache_to_hier(std::shared_ptr<Cache> cache)
{
    //Add higher caches
    if(m_caches.size() > 0)
    {
        cache->add_higher_cache(m_caches[m_caches.size() - 1]);
    }
    else
    {
        cache->add_higher_cache(std::weak_ptr<Cache>());
    }
    
    //Determine level
    cache->set_level(int(m_caches.size() + 1));
   
    //Add lower cache for the previous last level cache
    if(m_caches.size() > 0)
    {
        m_caches[m_caches.size() - 1]->add_lower_cache(cache);
    }
    
    //Point current cache lower_cache to nullptr
    cache->add_lower_cache(std::weak_ptr<Cache>());
    
    //Set cache sys pointer
    cache->set_cache_sys(this);
    
    m_caches.push_back(cache);

    assert(m_caches.size() <= NUM_MAX_CACHES);
    
    unsigned int curr_cache_latency = cache->get_latency_cycles();
    m_cache_latency_cycles[m_caches.size() - 1] = curr_cache_latency;
    if(m_caches.size() > 1)
    {
        m_total_latency_cycles[m_caches.size() - 1] = m_total_latency_cycles[m_caches.size() - 2] + curr_cache_latency;
        m_total_latency_cycles[MEMORY_ACCESS_ID] += curr_cache_latency;
    }
    else
    {
        m_total_latency_cycles[m_caches.size() - 1] = m_cache_latency_cycles[m_caches.size() - 1];
        m_total_latency_cycles[MEMORY_ACCESS_ID] += curr_cache_latency;
    }
}

void CacheSys::add_cachesys(std::shared_ptr<CacheSys> cs)
{
    m_other_cache_sys.push_back(cs);
}

void CacheSys::tick()
{
    //First, handle coherence actions in the current clock cycle
    for(std::map<uint64_t, CoherenceAction>::iterator it = m_coh_act_list.begin();
        it != m_coh_act_list.end(); )
    {
        for(int i = 0; i < m_caches.size() - 1; i++)
        {
            m_caches[i]->handle_coherence_action(it->second, it->first, false);
        }
        
        it = m_coh_act_list.erase(it);
    }
    
    assert(m_coh_act_list.empty());
    
    //Then, tick
    m_clk++;
    
    //Retire elements from hit list
    for(std::map<uint64_t, std::unique_ptr<Request>>::iterator it = m_hit_list.begin();
        it != m_hit_list.end();
        )
    {
        if(m_clk >= it->first)
        {
            it = m_hit_list.erase(it);
        }
        else
        {
            it++;
        }
    }
    
    //Then retire elements from wait list
    for(std::map<uint64_t, std::unique_ptr<Request>>::iterator it = m_wait_list.begin();
        it != m_wait_list.end();
        )
    {
        if(m_clk >= it->first)
        {
            //std::cout << "Calling callback with " << it->second << " and for addr " << std::hex << it->second->m_addr << std::endl;
            it->second->m_callback(it->second);
            it = m_wait_list.erase(it);
        }
        else
        {
            it++;
        }
    }
}

bool CacheSys::is_last_level(unsigned int cache_level)
{
    return (cache_level == m_caches.size());
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
}
