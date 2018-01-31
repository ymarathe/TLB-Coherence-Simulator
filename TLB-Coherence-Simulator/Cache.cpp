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
#include "Core.hpp"

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

bool Cache::is_found(const std::vector<CacheLine>& set, const uint64_t tag, bool is_translation, unsigned int &hit_pos)
{
    auto it = std::find_if(set.begin(), set.end(), [tag, is_translation, this](const CacheLine &l)
                           {
                               return ((l.tag == tag) && (l.valid) && (l.is_translation == is_translation));
                           });
    
    hit_pos = static_cast<unsigned int>(it - set.begin());
    return (it != set.end());
}

bool Cache::is_hit(const std::vector<CacheLine> &set, const uint64_t tag, bool is_translation, unsigned int &hit_pos)
{
    return is_found(set, tag, is_translation, hit_pos) & !set[hit_pos].lock;
}

void Cache::invalidate(const uint64_t addr, bool is_translation)
{
    unsigned int hit_pos;
    uint64_t tag = get_tag(addr);
    uint64_t index = get_index(addr);
    std::vector<CacheLine>& set = m_tagStore[index];
    
    //If we find line in the cache, invalidate the line
    
    if(is_found(set, tag, is_translation, hit_pos))
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
                higher_cache->invalidate(addr, is_translation);
            }
        }
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << "Cache " << m_cache_level << ":" << e.what() << std::endl;
    }
}

void Cache::evict(uint64_t set_num, const CacheLine &line, bool is_large)
{
    //Send back invalidate
    
    uint64_t evict_addr = ((line.tag << m_num_line_offset_bits) << m_num_index_bits) | (set_num << m_num_line_offset_bits);
    
    if(m_inclusive)
    {
        try
        {
            //No harm in blindly invalidating, since is_found checks is_translation.
            for(int i = 0; i < m_higher_caches.size(); i++)
            {
                auto higher_cache = m_higher_caches[i].lock();
                if(higher_cache != nullptr)
                {
                    higher_cache->invalidate(evict_addr, line.is_translation);
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
    std::shared_ptr<Cache> lower_cache = find_lower_cache_in_core(evict_addr, line.is_translation, is_large);
    
    if(line.dirty)
    {
        if(lower_cache != nullptr)
        {
            RequestStatus val = lower_cache->lookupAndFillCache(evict_addr, line.is_translation ? TRANSLATION_WRITEBACK : DATA_WRITEBACK);
            line.m_coherence_prot->forceCoherenceState(INVALID);
            
            if(m_inclusive)
            {
                assert((val == REQUEST_HIT) || (val == MSHR_HIT_AND_LOCKED));
            }
        }
        else
        {
            //Writeback to memory
            
        }
    }
    else
    {
        if(lower_cache != nullptr && m_inclusive)
        {
            unsigned int hit_pos;
            uint64_t index = lower_cache->get_index(evict_addr);
            uint64_t tag = lower_cache->get_index(evict_addr);
            std::vector<CacheLine> set = lower_cache->m_tagStore[index];
            assert(lower_cache->is_found(set, tag, line.is_translation, hit_pos));
        }
    }
}

RequestStatus Cache::lookupAndFillCache(uint64_t addr, kind txn_kind, uint64_t tid, bool is_large)
{
    unsigned int hit_pos;
    uint64_t tag = get_tag(addr);
    uint64_t index = get_index(addr);
    std::vector<CacheLine>& set = m_tagStore[index];
    
    bool is_translation = (txn_kind == TRANSLATION_WRITE) | (txn_kind == TRANSLATION_WRITEBACK) | (txn_kind == TRANSLATION_READ);
    
    if(is_hit(set, tag, is_translation, hit_pos))
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
        
        m_callback = std::bind(&Cache::release_lock, this, std::placeholders::_1);
        std::unique_ptr<Request> r = std::make_unique<Request>(Request(addr, txn_kind, m_callback, tid, is_large));
        m_cache_sys->m_hit_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->m_total_latency_cycles[m_cache_level - 1], std::move(r)));
        
        //Coherence handling
        CoherenceAction coh_action = line.m_coherence_prot->setNextCoherenceState(txn_kind);
        
        handle_coherence_action(coh_action, addr, true);
        
        return REQUEST_HIT;
    }
    
    auto it_not_valid = std::find_if(set.begin(), set.end(), [](const CacheLine &l) { return !l.valid; });
    bool needs_eviction = (it_not_valid == set.end()) && (!is_found(set, tag, is_translation, hit_pos));
  
    unsigned int insert_pos = m_repl->getVictim(set, index);
    
    CacheLine &line = set[insert_pos];
    
    uint64_t cur_addr = ((line.tag << m_num_line_offset_bits) << m_num_index_bits) | (index << m_num_line_offset_bits);
    
    //Evict the victim line if not locked and update replacement state
    if(needs_eviction)
    {
        evict(index, line, is_large);
    }
    
    if(txn_kind != TRANSLATION_WRITEBACK && txn_kind != DATA_WRITEBACK)
    {
        m_repl->updateReplState(index, insert_pos);
    }
    
    auto mshr_iter = m_mshr_entries.find(addr);
    
    //Blocking for TLB access.
    unsigned int mshr_size = (m_cache_type != TRANSLATION_ONLY) ? 16 : 1;
    if(mshr_iter != m_mshr_entries.end())
    {
        //MSHR hit
        if(((txn_kind == TRANSLATION_WRITE) || (txn_kind == DATA_WRITE) || (txn_kind == TRANSLATION_WRITEBACK) || (txn_kind == DATA_WRITEBACK)) && \
                (get_tag(addr) == mshr_iter->second->m_line->tag) && \
                (is_translation == mshr_iter->second->m_line->is_translation))
        {
            mshr_iter->second->m_line->dirty = true;
        }
        
        if(get_tag(addr) == mshr_iter->second->m_line->tag && \
            (is_translation == mshr_iter->second->m_line->is_translation))
        {
            CoherenceAction coh_action = mshr_iter->second->m_line->m_coherence_prot->setNextCoherenceState(txn_kind);
            
            //If we need to do writeback, we need to do it for addr already in cache
            //If we need to broadcast, we need to do it for addr in lookupAndFillCache call
        
            uint64_t coh_addr = (coh_action == MEMORY_DATA_WRITEBACK || coh_action == MEMORY_TRANSLATION_WRITEBACK) ? cur_addr : addr;
            
            handle_coherence_action(coh_action, coh_addr, true);
        }
        
        if(txn_kind == TRANSLATION_WRITEBACK || txn_kind == DATA_WRITEBACK)
        {
            assert(mshr_iter->second->m_line->lock);
            return MSHR_HIT_AND_LOCKED;
        }
        else
        {
            return MSHR_HIT;
        }
    }
    else if(m_mshr_entries.size() < mshr_size)
    {
        //MSHR miss, add entry
        line.valid = true;
        line.lock = true;
        line.tag = tag;
        line.is_translation = (txn_kind == TRANSLATION_READ) || (txn_kind == TRANSLATION_WRITE) || (txn_kind == TRANSLATION_WRITEBACK);
        line.dirty = (txn_kind == TRANSLATION_WRITE) || (txn_kind == DATA_WRITE) || (txn_kind == TRANSLATION_WRITEBACK) || (txn_kind == DATA_WRITEBACK);
        MSHREntry *mshr_entry = new MSHREntry(txn_kind, &line);
        m_mshr_entries.insert(std::make_pair(addr, mshr_entry));
    }
    else
    {
        //MSHR full
        return REQUEST_RETRY;
    }
    
    //We are in upper levels of TLB/cache and we aren't doing writeback.
    if(!m_cache_sys->is_last_level(m_cache_level) && ((txn_kind != DATA_WRITEBACK) || (txn_kind != TRANSLATION_WRITEBACK)))
    {
        std::shared_ptr<Cache> lower_cache = find_lower_cache_in_core(addr, is_translation, is_large);
        if(lower_cache != nullptr)
        {
            CacheType lower_cache_type = lower_cache->get_cache_type();
            bool is_tr_to_dat_boundary = (m_cache_type == TRANSLATION_ONLY) && (lower_cache_type == DATA_AND_TRANSLATION);
            //TODO: YMARATHE: Add correct thread ID and is_large flag here
            uint64_t access_addr = (is_tr_to_dat_boundary) ? m_core->getL3TLBAddr(addr, 0, 0): addr;
            lower_cache->lookupAndFillCache(access_addr, txn_kind);
        }
    }
    //We are in last level of cache hier and handling a data entry
    //We are in last level of TLB hier and handling a TLB entry
    else if((m_cache_sys->is_last_level(m_cache_level) && !is_translation && !m_cache_sys->get_is_translation_hier()) || \
            (m_cache_sys->is_last_level(m_cache_level) && is_translation && (m_cache_sys->get_is_translation_hier())))
    {
        //TODO:: YMARATHE. Move std::bind elsewhere, performance hit.
        m_callback = std::bind(&Cache::release_lock, this, std::placeholders::_1);
        std::unique_ptr<Request> r = std::make_unique<Request>(Request(addr, txn_kind, m_callback, tid, is_large));
        m_cache_sys->m_wait_list.insert(std::make_pair(m_cache_sys->m_clk + m_cache_sys->m_total_latency_cycles[MEMORY_ACCESS_ID], std::move(r)));
    }
    //We are in last level of cache hier and translation entry.
    else
    {
        std::shared_ptr<Cache> lower_cache = find_lower_cache_in_core(addr, is_translation, is_large);
        if(lower_cache != nullptr)
        {
            lower_cache->lookupAndFillCache(addr, txn_kind);
        }
    }
 
    
    CoherenceAction coh_action = line.m_coherence_prot->setNextCoherenceState(txn_kind);

    //If we need to do writeback, we need to do it for addr already in cache
    //If we need to broadcast, we need to do it for addr in lookupAndFillCache call
    
    uint64_t coh_addr = (coh_action == MEMORY_DATA_WRITEBACK || coh_action == MEMORY_TRANSLATION_WRITEBACK) ? cur_addr : addr;
    
    handle_coherence_action(coh_action, coh_addr, true);
    
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
            std::cout << *(line.m_coherence_prot) << line;
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
        
        if(get_tag(r->m_addr) == it->second->m_line->tag)
        {
            it->second->m_line->lock = false;
        }
        
        delete(it->second);
        
        m_mshr_entries.erase(it);
        
        //Ensure erasure in the MSHR
        assert(m_mshr_entries.find(r->m_addr) == m_mshr_entries.end());
    }
    
    //We are in L1
    if(m_cache_level == 1 && m_cache_type == DATA_ONLY)
    {
        m_core->m_rob.mem_mark_done(r->m_addr, r->m_type);
    }
    
    higher_caches_release_lock(r);
}

unsigned int Cache::get_latency_cycles()
{
    return m_latency_cycles;
}

void Cache::handle_coherence_action(CoherenceAction coh_action, uint64_t addr, bool same_cache_sys)
{
    if(coh_action == MEMORY_TRANSLATION_WRITEBACK || coh_action == MEMORY_DATA_WRITEBACK)
    {
        //Send request to memory
        //Writeback should be from the cache line in question?
        m_callback = std::bind(&Cache::release_lock, this, std::placeholders::_1);
        kind coh_txn_kind = txnKindForCohAction(coh_action);
        //TODO: YMARATHE: make request with TID here
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
            bool is_translation = (coh_action == BROADCAST_TRANSLATION_WRITE) || (coh_action == BROADCAST_TRANSLATION_READ);
            if(is_found(set, tag, is_translation, hit_pos))
            {
                CacheLine &line = set[hit_pos];
                kind coh_txn_kind = txnKindForCohAction(coh_action);
                line.m_coherence_prot->setNextCoherenceState(coh_txn_kind);
                if(coh_txn_kind == DIRECTORY_DATA_WRITE || coh_txn_kind == DIRECTORY_TRANSLATION_WRITE)
                {
                    line.valid = false;
                    assert(line.m_coherence_prot->getCoherenceState() == INVALID);
                }
            }
        }
    }
}

void Cache::set_cache_type(CacheType cache_type)
{
    m_cache_type = cache_type;
}

CacheType Cache::get_cache_type()
{
    return m_cache_type;
}

void Cache::set_core(Core *coreptr)
{
    m_core = coreptr;
}

std::shared_ptr<Cache> Cache::find_lower_cache_in_core(uint64_t addr, bool is_translation, bool is_large)
{
    std::shared_ptr<Cache> lower_cache;
    
    //Check if lower cache is statically determined
    try
    {
        lower_cache = m_lower_cache.lock();
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << "Cache " << m_cache_level << "doesn't have a valid lower cache" << std::endl;
    }
    
    //Determine lower_cache dynamically based on the type of transaction
    if(lower_cache == nullptr)
    {
        lower_cache = m_core->get_lower_cache(addr, is_translation, is_large, m_cache_level, m_cache_type);
    }
    
    return lower_cache;
}

void Cache::higher_caches_release_lock(std::unique_ptr<Request> &r)
{
    try
    {
        for(int i = 0; i < m_higher_caches.size(); i++)
        {
            auto higher_cache = m_higher_caches[i].lock();
            if(higher_cache != nullptr)
            {
                //Do this only for appropriate cache type
                //No harm in calling release_lock in both small and large.
                //Worst case, no MSHR hit.
                if((r->is_translation_request() && higher_cache->get_cache_type() != DATA_ONLY) || \
                   (!r->is_translation_request() && higher_cache->get_cache_type() != TRANSLATION_ONLY))
                {
                    
                    bool is_dat_to_tr_boundary = (m_cache_type == DATA_AND_TRANSLATION) && (higher_cache->get_cache_type() == TRANSLATION_ONLY);
                    r->m_addr = (is_dat_to_tr_boundary) ? m_core->retrieveActualAddr(r->m_addr, r->m_tid, r->m_is_large) : r->m_addr;
                    higher_cache->release_lock(r);
                }
            }
        }
    }
    catch(std::bad_weak_ptr &e)
    {
        std::cout << e.what() << std::endl;
    }
}
    
