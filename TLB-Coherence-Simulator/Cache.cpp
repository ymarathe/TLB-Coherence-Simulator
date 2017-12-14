//
//  Cache.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "Cache.hpp"
#include "CacheSys.hpp"
#include <assert.h>
#include <vector>
#include <iomanip>

uint64_t Cache::get_line_offset(const uint64_t addr)
{
    return (addr & ((1 << m_num_line_offset_bits) - 1));
}

uint64_t Cache::get_index(const uint64_t addr)
{
    return (addr >> m_num_line_offset_bits) & (((1 << m_num_index_bits) - 1));
}

uint64_t Cache::get_tag(const uint64_t addr)
{
    return ((addr >> m_num_line_offset_bits) >> m_num_index_bits);
}

bool Cache::is_hit(const std::vector<CacheLine> &set, const uint64_t tag, unsigned int &hit_pos)
{
    auto it = std::find_if(set.begin(), set.end(), [tag, this](const CacheLine &l)
    {
        return ((l.tag == tag) && (l.valid));
    });
    hit_pos = static_cast<unsigned int>(it - set.begin());
    //return (it != set.end() && !it->lock);
    return (it != set.end());
}

void Cache::invalidate(const uint64_t addr)
{
    unsigned int hit_pos;
    uint64_t tag = get_tag(addr);
    uint64_t index = get_index(addr);
    std::vector<CacheLine>& set = m_tagStore[index];
    
    //If we find line in the cache, invalidate the line
    
    //std::cout << "Saw back invalidate for addr: " << std::hex << addr << " in cache : " << m_cache_level << std::endl;
    std::cout << "Back invalidate[Cache: " << m_cache_level << "]: 0x" << std::hex << std::noshowbase << std::setw(16) << std::setfill('0') << addr <<  std::endl ;
    
    if(is_hit(set, tag, hit_pos))
    {
        set[hit_pos].valid = false;
    }
    
    //Go all the way up to highest cache
    try
    {
        auto higher_cache = m_higher_cache.lock();
        if(higher_cache != nullptr)
        {
            higher_cache->invalidate(addr);
        }
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << "Cache " << m_cache_level << ":" << e.what() << std::endl;
    }
}

void Cache::evict(uint64_t set_num, const CacheLine &line)
{
    //Send back invalidate
    uint64_t evict_addr = ((line.tag << m_num_line_offset_bits) << m_num_index_bits) | (set_num << m_num_line_offset_bits);
    try
    {
        auto higher_cache = m_higher_cache.lock();
        if(higher_cache != nullptr)
        {
            higher_cache->invalidate(evict_addr);
        }
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << "Cache " << m_cache_level << "doesn't have a valid higher cache" << std::endl;
    }
    
    //Send writeback if dirty
    //If not, due to inclusiveness, lower caches still have data
    //So do runtime check to ensure hit (and inclusiveness)
    std::shared_ptr<Cache> lower_cache;
    
    try
    {
        lower_cache = m_lower_cache.lock();
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << "Cache " << m_cache_level << "doesn't have a valid lower cache" << std::endl;
    }
    
    if(line.dirty)
    {
        //std::cout << m_cache_level << std::endl;
        if(lower_cache != nullptr)
        {
            assert(lower_cache->lookupAndFillCache(evict_addr, line.is_translation ? TRANSLATION_WRITEBACK : DATA_WRITEBACK));
        }
    }
    else
    {
        if(lower_cache != nullptr)
        {
            unsigned int hit_pos;
            uint64_t index = lower_cache->get_index(evict_addr);
            uint64_t tag = lower_cache->get_index(evict_addr);
            std::vector<CacheLine> set = lower_cache->m_tagStore[index];
            assert(lower_cache->is_hit(set, tag, hit_pos));
        }
    }
}

bool Cache::lookupAndFillCache(uint64_t addr, enum kind txn_kind)
{
    unsigned int hit_pos;
    uint64_t tag = get_tag(addr);
    uint64_t index = get_index(addr);
    std::vector<CacheLine>& set = m_tagStore[index];
    
    if(is_hit(set, tag, hit_pos))
    {
        CacheLine &line = set[hit_pos];
        
        //Is line dirty now?
        line.dirty = line.dirty || (txn_kind == DATA_WRITE) || (txn_kind == TRANSLATION_WRITE) || (txn_kind == DATA_WRITEBACK) || (txn_kind == TRANSLATION_WRITEBACK);
        
        assert(!((txn_kind == TRANSLATION_WRITE) | (txn_kind == TRANSLATION_WRITEBACK) | (txn_kind == TRANSLATION_READ)) ^(line.is_translation));
        
        //TODO: Do we update during writeback as well?
        //Update replacement state
        if(txn_kind != TRANSLATION_WRITEBACK && txn_kind != DATA_WRITEBACK)
        {
            m_repl->updateReplState(index, hit_pos);
        }
        
        Request r = Request(addr, txn_kind, m_callback);
        
        m_cache_sys->m_wait_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->latency_cycles[m_cache_level], r));
        
        //std::cout << "Hit in " << index << ", " << hit_pos << ", in cache level " << m_cache_level << std::endl;
        return true;
    }
    
    auto it_not_valid = std::find_if(set.begin(), set.end(), [](const CacheLine &l) { return !l.valid; });
    bool needs_eviction = (it_not_valid == set.end());
    unsigned int insert_pos = needs_eviction ?  m_repl->getVictim(index) : (unsigned int)(it_not_valid - set.begin());
  
    CacheLine &line = set[insert_pos];
    
    //Evict the victim line and update replacement state
    if(needs_eviction)
    {
        evict(index, line);
        m_repl->updateReplState(index, insert_pos);
    }
    
    line.valid = true;
    line.lock = true;
    line.tag = tag;
    line.is_translation = (txn_kind == TRANSLATION_READ) || (txn_kind == TRANSLATION_WRITE) || (txn_kind == TRANSLATION_WRITEBACK);
    line.dirty = (txn_kind == TRANSLATION_WRITE) || (txn_kind == DATA_WRITE) || (txn_kind == TRANSLATION_WRITEBACK) || (txn_kind == DATA_WRITEBACK);
    
    if(!m_cache_sys->is_last_level(m_cache_level) && ((txn_kind != DATA_WRITEBACK) || (txn_kind != TRANSLATION_WRITEBACK)))
    {
        try
        {
            auto lower_cache = m_lower_cache.lock();
            lower_cache->lookupAndFillCache(addr, txn_kind);
        }
        catch(std::bad_weak_ptr &e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    else
    {
        /*
         std::cout << "I am in cache: " << m_cache_level << std::endl;
         Request r = Request(addr, txn_kind, m_cache_sys->m_caches[m_cache_sys->m_caches.size() - 1]->m_callback);
         std::cout << "clk: " << m_cache_sys->m_clk << std::endl;
         m_cache_sys->m_wait_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->latency_cycles[MEMORY_HIT_ID], r));
        */
    }
    
    return false;
}

void Cache::add_lower_cache(const std::weak_ptr<Cache>& c)
{
    m_lower_cache = c;
}

void Cache::add_higher_cache(const std::weak_ptr<Cache>& c)
{
    m_higher_cache = c;
}

void Cache::set_level(unsigned int level)
{
    m_cache_level = level;
}

unsigned int Cache::get_level()
{
    return m_cache_level;
}

void Cache::printContents()
{
    for(auto &set: m_tagStore)
    {
        for(auto &line : set)
        {
            std::cout << line;
        }
        std::cout << std::endl;
    }
}

void Cache::set_cache_sys(CacheSys *cache_sys)
{
    m_cache_sys = cache_sys;
}

void Cache::release_lock(Request &r)
{
    /*
    unsigned int tag = get_tag(r.m_addr);
    unsigned int index = get_index(r.m_addr);
    
    std::cout << "clk: " << m_cache_sys->m_clk << std::endl;
    std::cout << "Cache level: " << m_cache_level << std::endl;
    
    std::vector<CacheLine> &set = m_tagStore[index];
    
    std::copy(set.begin(), set.end(), std::ostream_iterator<CacheLine>(std::cout, " "));
    std::cout << std::endl;
    
    auto it = std::find_if(set.begin(), set.end(), [tag, this](const CacheLine &l) { return l.tag == tag;});
    std::cout << "hit_pos: " << (unsigned int)(it - set.begin()) << std::endl;
    
    it->lock = false;
    
    try
    {
        auto higher_cache = m_higher_cache.lock();
        if(higher_cache != nullptr)
        {
            higher_cache->release_lock(r);
        }
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << e.what() << std::endl;
    }
    */

}
