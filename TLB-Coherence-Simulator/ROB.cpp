//
//  ROB.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 1/4/18.
//  Copyright © 2018 Yashwant Marathe. All rights reserved.
//

#include "ROB.hpp"

bool ROB::issue(bool is_memory_access, uint64_t addr, kind txn_kind)
{
    //Could not issue
    if(m_num_waiting_instr >= m_window_size)
        return false;
    
    //Can issue
    m_window[m_issue_ptr].done = false;
    m_window[m_issue_ptr].addr = addr;
    m_window[m_issue_ptr].txn_kind = txn_kind;
    m_window[m_issue_ptr].is_memory_access = is_memory_access;
    
    m_issue_ptr = (m_issue_ptr + 1) % m_window_size;
    m_num_waiting_instr++;
    
    return true;
}

unsigned int ROB::retire()
{
    unsigned int num_retired = 0;
    
    while((m_window[m_commit_ptr].done) && (num_retired < m_retire_width))
    {
        //Advance commit ptr
        m_commit_ptr = (m_commit_ptr + 1) % m_window_size;
        m_num_waiting_instr--;
        num_retired++;
    }
    
    return num_retired;
}

void ROB::mark_done(uint64_t addr, kind txn_kind)
{
    for(std::vector<ROBEntry>::iterator it = m_window.begin(); it != m_window.end(); it++)
    {
        if((it->addr == addr) && (it->txn_kind == txn_kind))
        {
            it->done = true;
            break;
        }
    }
}
