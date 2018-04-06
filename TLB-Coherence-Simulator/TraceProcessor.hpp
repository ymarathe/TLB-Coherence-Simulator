//
//  TraceProcessor.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 2/7/18.
//  Copyright © 2018 Yashwant Marathe. All rights reserved.
//

#ifndef TraceProcessor_hpp
#define TraceProcessor_hpp

#include <iostream>
#include "utils.hpp"
#include <fstream>
#include "Request.hpp"
#include <cstring>
#include <unordered_map>
#include <set>
#include <assert.h>

class RequestDesc {
    public:
        uint64_t m_addr;
        uint64_t m_tid;
        bool m_is_large;

        RequestDesc(uint64_t addr, uint64_t tid, bool is_large) : m_addr(addr), m_tid(tid), m_is_large(is_large) { }

        bool operator == (const RequestDesc &other) const
        {
            return ((other.m_addr == m_addr) && (other.m_is_large == m_is_large) && (other.m_tid == m_tid));
        }

        friend std::ostream& operator << (std::ostream& out, const RequestDesc &rdesc)
        {
            out << std::hex << rdesc.m_addr << std::dec << "|" << rdesc.m_tid << "|" << rdesc.m_is_large << "|";
            return out;
        }
};

class RequestDescHasher {
    public:
	std::size_t operator () (const RequestDesc &r) const
	{
		using std::size_t;
		using std::hash;

		size_t res = 42;
		res = res * 31 + hash<uint64_t>() (r.m_addr);
	    res = res * 31 + hash<uint64_t>() (r.m_tid);
		res = res * 31 + hash<bool>() (r.m_is_large);
		return res;
	}
};

class TraceProcessor {

private:
    trace_tlb_entry_t *buf1;
    trace_tlb_tid_entry_t *buf2;
    trace_shootdown_entry_t *buf3;
    char fmt[1024];
    char trace[NUM_CORES][1024];
    char shootdown[1024];
    FILE *trace_fp[NUM_CORES];
    FILE *shootdown_fp;
    bool *used_up, *empty_file;
    bool used_up_shootdown, empty_file_shootdown;
    uint64_t *entry_count;
    unsigned int num_cores;
    int global_index;
    uint64_t *curr_ts;
    
public:
    //Variables
    uint64_t *last_ts;
    uint64_t global_ts;
#ifdef LONG_WARMUP
    uint64_t warmup_period = 10000000000;
#else
    uint64_t warmup_period = 100000;
#endif
    bool is_multicore;
    uint64_t total_instructions_in_real_run[NUM_CORES];
    uint64_t ideal_cycles_in_real_run[NUM_CORES];
    uint64_t num_tlb_misses_in_real_run[NUM_CORES];
    double   avg_pw_cycles_in_real_run[NUM_CORES];
    uint64_t l2d_lat, l3d_lat, vl_lat, dram_lat;
    uint64_t   l3_small_tlb_size = 1024*1024;
    uint64_t   l3_large_tlb_size = 256*1024;
    uint64_t context_switch_count;
    uint64_t tid_offset = 0;
    
    std::unordered_map<RequestDesc, std::set<uint64_t>, RequestDescHasher> presence_map_small_page;
    std::unordered_map<RequestDesc, std::set<uint64_t>, RequestDescHasher> presence_map_large_page;

    //Constructor
    TraceProcessor(unsigned int num_cores = 8) : num_cores(num_cores)
    {
        buf1 = new trace_tlb_entry_t[num_cores];
        buf2 = new trace_tlb_tid_entry_t[num_cores];
        buf3 = new trace_shootdown_entry_t;
        
        used_up = new bool [num_cores];
        empty_file = new bool [num_cores];
        entry_count = new uint64_t [num_cores];
        
        last_ts = new uint64_t [num_cores];
        curr_ts = new uint64_t [num_cores];

        for(int i = 0; i < num_cores; i++)
        {
        	last_ts[i] = warmup_period;
        }

        global_ts = warmup_period;

        context_switch_count = (5000000 - 3000000) * (rand()/(double) RAND_MAX);
        std::cout << "Context switch count = " << context_switch_count << "\n";

        empty_file_shootdown = false;
    }

    ~TraceProcessor()
    {
        delete [] buf1;
        delete [] buf2;
        delete buf3;

        delete [] used_up;
        delete [] empty_file;

        delete [] entry_count;
        
        delete [] last_ts;
        delete [] curr_ts;

    }
    
    //Methods
    void processPair(std::string name, std::string val);
    
    void parseAndSetupInputs(char *input_cfg);
    
    void verifyOpenTraceFiles();
    
    int getNextEntry();

    void getShootdownEntry();
    
    Request* generateRequest();

    uint64_t switch_threads();

    void add_to_presence_map(Request &r);

    void remove_from_presence_map(uint64_t addr, uint64_t tid, bool is_large, unsigned int core_id);
    
};

#endif /* TraceProcessor_hpp */
