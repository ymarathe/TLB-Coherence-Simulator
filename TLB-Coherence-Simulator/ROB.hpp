//
//  ROB.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 1/4/18.
//  Copyright © 2018 Yashwant Marathe. All rights reserved.
//

#ifndef ROB_hpp
#define ROB_hpp

#include<iostream>
#include<vector>
#include "utils.hpp"

class ROB {
public:
    class ROBEntry {
    public:
        bool is_memory_access;
        kind txn_kind;
        uint64_t addr;
        bool done;
        uint64_t clk;
        
        ROBEntry() : is_memory_access(false), txn_kind(INVALID_TXN_KIND), addr(0), done(false), clk(0) {}
    };
    
    std::vector<ROBEntry> m_window;
    unsigned int m_issue_width = 4;
    unsigned int m_retire_width = 4;
    unsigned int m_issue_ptr = 0;
    unsigned int m_commit_ptr = 0;
    unsigned int m_num_waiting_instr = 0;
    unsigned int m_window_size = 128;
    
    ROB(unsigned int window_size = 128, unsigned int issue_width = 4, unsigned int retire_width = 4) : m_window_size(window_size),
        m_issue_width(issue_width),
        m_retire_width(retire_width)
    {
        for(int i = 0; i < m_window_size; i++)
        {
            m_window.push_back(ROBEntry());
        }
        
        m_issue_ptr = 0;
        m_commit_ptr = 0;
        m_num_waiting_instr = 0;
    }
    
    bool issue(bool is_memory_access, uint64_t addr, kind txn_kind, uint64_t clk);
    unsigned int retire(uint64_t clk);
    void mem_mark_done(uint64_t addr, kind txn_kind);
};

#endif /* ROB_hpp */
