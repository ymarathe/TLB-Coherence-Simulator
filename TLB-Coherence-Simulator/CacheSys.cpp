//
//  CacheSys.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/7/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "CacheSys.hpp"
#include "Cache.hpp"

void CacheSys::add_cache_to_hier(std::shared_ptr<Cache>& cache)
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
    
}

void CacheSys::makeCachesSentient()
{
    for(int i = 0; i < m_caches.size(); i++)
    {
        if(i > 0)
        {
            m_caches[i]->add_higher_cache(m_caches[i - 1]);
        }
        else
        {
            m_caches[i]->add_higher_cache(std::weak_ptr<Cache>());
        }
        
        m_caches[i]->set_level(i + 1);
        
        if(i + 1 < m_caches.size())
        {
            m_caches[i]->add_lower_cache(m_caches[i + 1]);
        }
        
        m_caches[i]->set_cache_sys(this);
    }
    
    m_caches[m_caches.size() - 1]->add_lower_cache(std::weak_ptr<Cache>());
}

void CacheSys::tick()
{
    //First, tick
    m_clk++;
    
    //Then retire elements from wait list
    for(std::map<uint64_t, Request>::iterator it = m_wait_list.begin();
        it != m_wait_list.end();
        )
    {
        if(m_clk >= it->first)
        {
            it->second.m_callback(it->second);
            for(std::map<uint64_t, Request>::iterator secondit = m_wait_list.begin(); secondit != m_wait_list.end(); secondit++)
            {
                std::cout << secondit->first << ", " << std::hex << secondit->second.m_addr << std::endl;
            }
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
