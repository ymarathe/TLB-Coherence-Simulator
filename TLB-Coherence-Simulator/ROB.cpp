//
//  ROB.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 1/4/18.
//  Copyright © 2018 Yashwant Marathe. All rights reserved.
//

#include "ROB.hpp"
#include <assert.h>
#include <algorithm>

bool ROB::issue(bool is_memory_access, Request *r, uint64_t clk)
{
    //Could not issue
    if(m_num_waiting_instr >= m_window_size)
        return false;
    
    //Can issue
    m_window[m_issue_ptr].valid = true;
    m_window[m_issue_ptr].done = false;
    m_window[m_issue_ptr].req = r;
    m_window[m_issue_ptr].is_memory_access = is_memory_access;
    m_window[m_issue_ptr].clk = clk;
    
    if(is_memory_access)
    {
        kind act_txn_kind = r->m_type;
        if(act_txn_kind != TRANSLATION_WRITE)
        {
            r->update_request_type_from_core(TRANSLATION_READ);
        }
        request_queue.push_back(*r);
        auto req_ready_iter = is_request_ready.find(*r);
        if(req_ready_iter != is_request_ready.end())
        {
            req_ready_iter->second.num_occ_in_req_queue++;
        }
        else
        {
            ReqQueueMetaData reqqmd;
            is_request_ready.insert(std::pair<Request, ReqQueueMetaData>(*r, reqqmd));
            assert(is_request_ready[*r].num_occ_in_req_queue == 1);
            assert(!is_request_ready[*r].ready);
        }

        r->update_request_type_from_core(act_txn_kind);
    }

    m_issue_ptr = (m_issue_ptr + 1) % m_window_size;
    m_num_waiting_instr++;

    return true;
}

unsigned int ROB::retire(uint64_t clk)
{
    unsigned int num_retired = 0;
    
    if(m_num_waiting_instr == 0)
    {
        return 0;
    }
    
    while(m_window[m_commit_ptr].valid && ((m_window[m_commit_ptr].done) || ((m_window[m_commit_ptr].clk < clk) && (!m_window[m_commit_ptr].is_memory_access))) && (num_retired < m_retire_width))
    {
        //Advance commit ptr
        m_window[m_commit_ptr].valid = false;
	    delete m_window[m_commit_ptr].req;
        m_window[m_commit_ptr].req = nullptr;
        m_commit_ptr = (m_commit_ptr + 1) % m_window_size;
        m_num_waiting_instr--;
        num_retired++;
    }
    
    return num_retired;
}

void ROB::mem_mark_done(Request &r)
{
    for(std::vector<ROBEntry>::iterator it = m_window.begin(); it != m_window.end();)
    {
        if(it->valid && r == *(it->req))
        {
            it->done = true;
            it++;
        }
        else
        {
            it++;
        }
    }

    bool check_read = false;

    if (r.m_type == TRANSLATION_WRITE)
    {
        check_read = true;
        r.m_type = TRANSLATION_READ;
    }

    for(std::vector<ROBEntry>::iterator it = m_window.begin(); it != m_window.end();)
    {
        if(it->valid && r == *(it->req))
        {
            it->done = true;
            it++;
        }
        else
        {
            it++;
        }
    }

    if(check_read)
        r.m_type = TRANSLATION_WRITE;

}

void ROB::printContents()
{
    for(int i = 0; i < m_window_size; i++)
    {
        std::cout << m_window[i];
    }
    std::cout << "---------------------------------" << std::endl;
}

bool ROB::can_issue()
{
    return (m_num_waiting_instr < m_window_size);
}

void ROB::mem_mark_translation_done(Request &r)
{
    auto entry = is_request_ready.find(r);
    if(entry != is_request_ready.end())
    {
        entry->second.ready = true;
    }

    bool check_read = false;

    if (r.m_type == TRANSLATION_WRITE)
    {
        check_read = true;
        r.m_type = TRANSLATION_READ;
    }

    entry = is_request_ready.find(r);
    if(entry != is_request_ready.end())
    {
        entry->second.ready = true;
    }

    if(check_read)
        r.m_type = TRANSLATION_WRITE;
}

bool ROB::is_empty()
{
	return (m_num_waiting_instr == 0);
}

void ROB::peek_commit_ptr()
{
	std::cout << "[" << m_window[m_commit_ptr].req << "] Request at commit ptr = " << std::hex << *(m_window[m_commit_ptr].req) << std::dec;
}

void ROB::peek(unsigned int ptr)
{
	std::cout << "[" << m_window[ptr].req << "] Request at ptr = " << std::hex << *(m_window[ptr].req) << std::dec;
}
