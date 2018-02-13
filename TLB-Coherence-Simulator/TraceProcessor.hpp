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

class TraceProcessor {

private:
    trace_tlb_entry_t *buf1;
    trace_tlb_tid_entry_t *buf2;
    char fmt[1024];
    char trace[32][1024];
    FILE *trace_fp[32];
    bool *used_up, *empty_file;
    uint64_t *entry_count;
    unsigned int num_cores;
    int global_index;
    uint64_t *last_ts;
    uint64_t *curr_ts;
    uint64_t warmup_period = 10000000000;
    
public:
    
    //Variables
    uint64_t total_instructions_in_real_run[32];
    uint64_t ideal_cycles_in_real_run[32];
    uint64_t num_tlb_misses_in_real_run[32];
    double   avg_pw_cycles_in_real_run[32];
    uint64_t l2d_lat, l3d_lat, vl_lat, dram_lat;
    uint64_t   l3_small_tlb_size = 1024*1024;
    uint64_t   l3_large_tlb_size = 256*1024;
    
    //Constructor
    TraceProcessor(unsigned int num_cores = 8) : num_cores(num_cores)
    {
        buf1 = new trace_tlb_entry_t[num_cores];
        buf2 = new trace_tlb_tid_entry_t[num_cores];
        
        used_up = new bool [num_cores];
        empty_file = new bool [num_cores];
        entry_count = new uint64_t [num_cores];
        
        last_ts = new uint64_t [num_cores];
        curr_ts = new uint64_t [num_cores];

	for(int i = 0; i < num_cores; i++)
	{
		last_ts[i] = warmup_period;
	}
    }
    
    //Methods
    void processPair(std::string name, std::string val);
    
    void parseAndSetupInputs(char *input_cfg);
    
    void verifyOpenTraceFiles();
    
    int getNextEntry();
    
    Request* generateRequest();
    
};

#endif /* TraceProcessor_hpp */
