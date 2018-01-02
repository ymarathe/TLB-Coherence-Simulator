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
#include "utils.hpp"

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

bool Cache::is_found(const std::vector<CacheLine>& set, const uint64_t tag, unsigned int &hit_pos)
{
    auto it = std::find_if(set.begin(), set.end(), [tag, this](const CacheLine &l)
                           {
                               return ((l.tag == tag) && (l.valid));
                           });
    
    hit_pos = static_cast<unsigned int>(it - set.begin());
    return (it != set.end());
}

bool Cache::is_hit(const std::vector<CacheLine> &set, const uint64_t tag, unsigned int &hit_pos)
{
    return is_found(set, tag, hit_pos) & !set[hit_pos].lock;
}

void Cache::invalidate(const uint64_t addr)
{
    unsigned int hit_pos;
    uint64_t tag = get_tag(addr);
    uint64_t index = get_index(addr);
    std::vector<CacheLine>& set = m_tagStore[index];
    
    //If we find line in the cache, invalidate the line
    //std::cout << "Back invalidate[Cache: " << m_cache_level << "]: 0x" << std::hex << std::noshowbase << std::setw(16) << std::setfill('0') << addr <<  std::endl ;
    
    if(is_found(set, tag, hit_pos))
    {
        set[hit_pos].valid = false;
    }
    
    //Go all the way up to highest cache
    //There might be more than one higher cache (example L3)
    try
    {
        for(int i = 0; i < m_higher_caches.size(); i++)
        {
            auto higher_cache = m_higher_caches[i].lock();
            if(higher_cache != nullptr)
            {
                higher_cache->invalidate(addr);
            }
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
    
    //Simulating non-inclusive caches for now.
    if(0)
    {
        try
        {
            for(int i = 0; i < m_higher_caches.size(); i++)
            {
                auto higher_cache = m_higher_caches[i].lock();
                if(higher_cache != nullptr)
                {
                    higher_cache->invalidate(evict_addr);
                }
            }
        }
        catch(std::bad_weak_ptr &e)
        {
            std::cout << "Cache " << m_cache_level << "doesn't have a valid higher cache" << std::endl;
        }
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
        if(lower_cache != nullptr)
        {
            RequestStatus val = lower_cache->lookupAndFillCache(evict_addr, line.is_translation ? TRANSLATION_WRITEBACK : DATA_WRITEBACK);
            //On eviction, force coherence state to be invalid.
            line.m_coherence_prot->forceCoherenceState(INVALID);
            assert((val == REQUEST_HIT) || (val == MSHR_HIT_AND_LOCKED));
        }
        else
        {
            //Writeback to memory
            
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
            assert(lower_cache->is_found(set, tag, hit_pos));
        }
    }
}

RequestStatus Cache::lookupAndFillCache(uint64_t addr, kind txn_kind)
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
        
        //Update replacement state
        if(txn_kind != TRANSLATION_WRITEBACK && txn_kind != DATA_WRITEBACK)
        {
            m_repl->updateReplState(index, hit_pos);
        }
        
        //TODO:: Have some kind of callback here to retire memory instruction from ROB
        m_callback = std::bind(&Cache::release_lock, this, std::placeholders::_1);
        std::unique_ptr<Request> r = std::make_unique<Request>(Request(addr, txn_kind, m_callback));
        m_cache_sys->m_hit_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->m_total_latency_cycles[m_cache_level - 1], std::move(r)));
        
        //Coherence handling
        CoherenceAction coh_action = line.m_coherence_prot->setNextCoherenceState(txn_kind);
        
        handle_coherence_action(coh_action, addr, true);
        
        return REQUEST_HIT;
    }
    
    auto it_not_valid = std::find_if(set.begin(), set.end(), [](const CacheLine &l) { return !l.valid; });
    bool needs_eviction = (it_not_valid == set.end());
  
    unsigned int insert_pos = m_repl->getVictim(index);
    
    CacheLine &line = set[insert_pos];
    
    //Evict the victim line if not locked and update replacement state
    if(needs_eviction)
    {
        evict(index, line);
    }
    
    if(txn_kind != TRANSLATION_WRITEBACK && txn_kind != DATA_WRITEBACK)
    {
        m_repl->updateReplState(index, insert_pos);
    }
    
    auto mshr_iter = m_mshr_entries.find(addr);
    if(mshr_iter != m_mshr_entries.end())
    {
        //MSHR hit
        if((txn_kind == TRANSLATION_WRITE) || (txn_kind == DATA_WRITE) || (txn_kind == TRANSLATION_WRITEBACK) || (txn_kind == DATA_WRITEBACK))
        {
            mshr_iter->second->dirty = true;
        }
        
        if(txn_kind == TRANSLATION_WRITEBACK || txn_kind == DATA_WRITEBACK)
        {
            assert(mshr_iter->second->lock);
            return MSHR_HIT_AND_LOCKED;
        }
        else
        {
            return MSHR_HIT;
        }
    }
    else if(m_mshr_entries.size() < 16)
    {
        //MSHR miss, add entry
        line.valid = true;
        line.lock = true;
        line.tag = tag;
        line.is_translation = (txn_kind == TRANSLATION_READ) || (txn_kind == TRANSLATION_WRITE) || (txn_kind == TRANSLATION_WRITEBACK);
        line.dirty = (txn_kind == TRANSLATION_WRITE) || (txn_kind == DATA_WRITE) || (txn_kind == TRANSLATION_WRITEBACK) || (txn_kind == DATA_WRITEBACK);
        m_mshr_entries.insert(std::make_pair(addr, &line));
    }
    else
    {
        //MSHR full
        return REQUEST_RETRY;
    }
    
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
        //TODO:: YMARATHE. Move std::bind elsewhere, performance hit.
        m_callback = std::bind(&Cache::release_lock, this, std::placeholders::_1);
        std::unique_ptr<Request> r = std::make_unique<Request>(Request(addr, txn_kind, m_callback));
        m_cache_sys->m_wait_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->m_total_latency_cycles[MEMORY_ACCESS_ID], std::move(r)));
    }
    
    CoherenceAction coh_action = line.m_coherence_prot->setNextCoherenceState(txn_kind);
    
    handle_coherence_action(coh_action, addr, true);
    
    return REQUEST_MISS;
}

void Cache::add_lower_cache(const std::weak_ptr<Cache>& c)
{
    m_lower_cache = c;
}

void Cache::add_higher_cache(const std::weak_ptr<Cache>& c)
{
    m_higher_caches.push_back(c);
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

void Cache::release_lock(std::unique_ptr<Request>& r)
{
    auto it = m_mshr_entries.find(r->m_addr);
    
    if(it != m_mshr_entries.end())
    {
        //Handle corner case where a line is evicted when it is still in the 'lock' state
        //In this case, tag of the line would have changed, and hence we don't want to change the lock state.
        if(get_tag(r->m_addr) == it->second->tag)
        {
            it->second->lock = false;
        }
        
        m_mshr_entries.erase(it);
        
        //Ensure erasure in the MSHR
        assert(m_mshr_entries.find(r->m_addr) == m_mshr_entries.end());
    }
    
    try
    {
        for(int i = 0; i < m_higher_caches.size(); i++)
        {
            auto higher_cache = m_higher_caches[i].lock();
            if(higher_cache != nullptr)
            {
                higher_cache->release_lock(r);
            }
        }
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << e.what() << std::endl;
    }
}

unsigned int Cache::get_latency_cycles()
{
    return m_latency_cycles;
}

void Cache::handle_coherence_action(CoherenceAction coh_action, uint64_t addr, bool same_cache_sys)
{
    if(coh_action == MEMORY_TRANSLATION_WRITEBACK || coh_action == MEMORY_DATA_WRITEBACK)
    {
        //Send request to memory.
        m_callback = std::bind(&Cache::release_lock, this, std::placeholders::_1);
        kind coh_txn_kind = txnKindForCohAction(coh_action);
        std::unique_ptr<Request> r = std::make_unique<Request>(Request(addr, coh_txn_kind, m_callback));
        m_cache_sys->m_wait_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->m_total_latency_cycles[MEMORY_ACCESS_ID], std::move(r)));
    }
    else if(coh_action == BROADCAST_DATA_READ || coh_action == BROADCAST_DATA_WRITE || \
            coh_action == BROADCAST_TRANSLATION_READ || coh_action == BROADCAST_TRANSLATION_WRITE
            )
    {
        //Update caches in all other cache hierarchies if call is from the same hierarchy
        //Pass addr and coherence action enforced by this cache
        if(same_cache_sys && !m_cache_sys->is_last_level(m_cache_level))
        {
            for(int i = 0; i < m_cache_sys->m_other_cache_sys.size(); i++)
            {
                std::cout << "[" << m_cache_level << "] Coherence update from " << m_cache_sys->m_core_id << " to " << m_cache_sys->m_other_cache_sys[i]->m_core_id << " for addr " << std::hex << addr << std::endl;
                m_cache_sys->m_other_cache_sys[i]->m_coh_act_list.insert(std::make_pair(addr, coh_action));
            }
        }
        else if(!same_cache_sys)
        {
            //If call came from different cache hierarchy, handle it!
            //Invalidate if we see BROADCAST_*_WRITE
            unsigned int hit_pos;
            uint64_t tag = get_tag(addr);
            uint64_t index = get_index(addr);
            std::vector<CacheLine>& set = m_tagStore[index];
            if(is_found(set, tag, hit_pos))
            {
                std::cout << "Invalidating " << std::hex << "for addr " << addr << ", tag " << tag << " on core " << m_cache_sys->m_core_id << " on cache " << m_cache_level << std::endl;
                CacheLine &line = set[hit_pos];
                kind coh_txn_kind = txnKindForCohAction(coh_action);
                line.m_coherence_prot->setNextCoherenceState(coh_txn_kind);
                if(coh_txn_kind == DIRECTORY_DATA_WRITE || coh_txn_kind == DIRECTORY_TRANSLATION_WRITE)
                {
                    //std::cout << "Invalidating addr " << std::hex << addr << std::endl;
                    line.valid = false;
                }
            }
        }
    }
}

