//
//  main.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include <iostream>
#include "Cache.hpp"
#include "CacheSys.hpp"
#include "ROB.hpp"
#include "Core.hpp"
#include "TraceProcessor.hpp"
#include <memory>
#include "utils.hpp"

#define NUM_INSTRUCTIONS 5000000 
#define NUM_INITIAL_FILL 100 

int main(int argc, char * argv[])
{
    int num_args = 1;
    int num_real_args = num_args + 1;
    
    TraceProcessor tp(8);
    if (argc < num_real_args)
    {
        std::cout << "Program takes " << num_args << " arguments" << std::endl;
        std::cout << "Path name of input config file" << std::endl;
        exit(0);
    }
    
    char* input_cfg=argv[1];
    
    tp.parseAndSetupInputs(input_cfg);
    
    tp.verifyOpenTraceFiles();
    
    std::shared_ptr<Cache> llc = std::make_shared<Cache>(Cache(8192, 16, 64, 38,  DATA_AND_TRANSLATION));
    
    bool ll_interface_complete = false;
    
    std::shared_ptr<Cache> l3_tlb_small = std::make_shared<Cache>(Cache(16384, 4, 4096, 75, TRANSLATION_ONLY));
    std::shared_ptr<Cache> l3_tlb_large = std::make_shared<Cache>(Cache(4096, 4, 2 * 1024 * 1024, 75, TRANSLATION_ONLY, true));
   
    /* 
    std::shared_ptr<Cache> l3_tlb_small = std::make_shared<Cache>(Cache(8, 8, 4096, 1, TRANSLATION_ONLY));
    std::shared_ptr<Cache> l3_tlb_large = std::make_shared<Cache>(Cache(8, 8, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true));
    */

    std::vector<std::shared_ptr<CacheSys>> data_hier;
    std::vector<std::shared_ptr<CacheSys>> tlb_hier;
    std::vector<std::shared_ptr<Cache>> l1_data_caches;
    std::vector<std::shared_ptr<Cache>> l2_data_caches;
    
    std::vector<std::shared_ptr<Cache>> l1_tlb;
    std::vector<std::shared_ptr<Cache>> l2_tlb;
    
    std::vector<std::shared_ptr<ROB>> rob_arr;
    
    std::vector<std::shared_ptr<Core>> cores;
    
    for(int i = 0; i < NUM_CORES; i++)
    {
        data_hier.push_back(std::make_shared<CacheSys>(CacheSys(false)));
        l1_data_caches.push_back(std::make_shared<Cache>(Cache(64, 8, 64, 4, DATA_ONLY)));
        l2_data_caches.push_back(std::make_shared<Cache>(Cache(1024, 4, 64, 12, DATA_AND_TRANSLATION)));
        
        data_hier[i]->add_cache_to_hier(l1_data_caches[i]);
        data_hier[i]->add_cache_to_hier(l2_data_caches[i]);
        data_hier[i]->add_cache_to_hier(llc);
        
        tlb_hier.push_back(std::make_shared<CacheSys>(CacheSys(true)));
        
        /*l1_tlb.push_back(std::make_shared<Cache>(Cache(2, 2, 4096, 1, TRANSLATION_ONLY)));
        l1_tlb.push_back(std::make_shared<Cache>(Cache(1, 2, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(4, 4, 4096, 1, TRANSLATION_ONLY)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(4, 4, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true)));*/
        
        l1_tlb.push_back(std::make_shared<Cache>(Cache(16, 4, 4096, 1, TRANSLATION_ONLY)));
        l1_tlb.push_back(std::make_shared<Cache>(Cache(8, 4, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(64, 16, 4096, 14, TRANSLATION_ONLY)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(64, 16, 2 * 1024 * 1024, 14, TRANSLATION_ONLY, true)));

        l1_tlb[2 * i]->add_traceprocessor(&tp);
        l1_tlb[2 * i + 1]->add_traceprocessor(&tp);
        l2_tlb[2 * i]->add_traceprocessor(&tp);
        l2_tlb[2 * i + 1]->add_traceprocessor(&tp);
       
        tlb_hier[i]->add_cache_to_hier(l1_tlb[2 * i]);
        tlb_hier[i]->add_cache_to_hier(l1_tlb[2 * i + 1]);
        tlb_hier[i]->add_cache_to_hier(l2_tlb[2 * i]);
        tlb_hier[i]->add_cache_to_hier(l2_tlb[2 * i + 1]);
        tlb_hier[i]->add_cache_to_hier(l3_tlb_small);
        tlb_hier[i]->add_cache_to_hier(l3_tlb_large);
        
        rob_arr.push_back(std::make_shared<ROB>(ROB()));
        
        cores.push_back(std::make_shared<Core>(Core(data_hier[i], tlb_hier[i], rob_arr[i])));
        
        data_hier[i]->set_core(cores[i]);
        tlb_hier[i]->set_core(cores[i]);
        
        cores[i]->set_core_id(i);
        
        ll_interface_complete = cores[i]->interfaceHier(ll_interface_complete);
    }
    
    //Make cache hierarchies aware of each other
    for(int i = 0; i < NUM_CORES; i++)
    {
        for(int j = 0; j < NUM_CORES; j++)
        {
            if(i != j)
            {
                //Data caches see other data caches
                data_hier[i]->add_cachesys(data_hier[j]);

                //TLB see other data caches
                tlb_hier[i]->add_cachesys(data_hier[j]);
            }
        }
    }

    for(int i = 0; i < NUM_CORES; i++)
    {
        for(int j = 0; j < NUM_CORES; j++)
        {
            if(i != j)
            {
                //Data caches see other TLBs 
                //Done this way because indexing is dependent on having all data caches first, and then all the TLBs
                data_hier[i]->add_cachesys(tlb_hier[j]);
            }
        }
    }
    
    std::cout << "Initial fill\n";
    for(int i = 0; i < NUM_INITIAL_FILL; i++)
    {
        Request *r = tp.generateRequest();

        if(r->m_core_id >=0 && r->m_core_id < NUM_CORES)
        {
                cores[r->m_core_id]->add_trace(r);
        } 
    }
    std::cout << "Initial fill done\n";

   bool done = false;
   bool timeout = false;
   uint64_t num_traces_added = NUM_INITIAL_FILL;
   while(!done && !timeout)
   {
	done = true;
   	for(int i = 0; i < NUM_CORES; i++)
   	{
	   cores[i]->tick();
       
       if((num_traces_added < NUM_INSTRUCTIONS) && (cores[i]->must_add_trace()))
       {
           Request *r = tp.generateRequest();

           if(r->m_core_id >= 0 && r->m_core_id < NUM_CORES)
           {
               cores[r->m_core_id]->add_trace(r);
           }
           
           if(num_traces_added % 1000000 == 0)
           {
               std::cout << "Number of traces added = " << num_traces_added << "\n";
           }

           num_traces_added++;
       }

	   done = done & cores[i]->is_done(); 
	   if(cores[i]->m_clk >= NUM_INSTRUCTIONS * 10)
	   {
		   std::cout << "Core " << i << " timed out " << std::endl;
		   std::cout << "Blocking request = " ; cores[i]->m_rob->peek_commit_ptr();
		   timeout = true;
	   }
 	}
   }

    for(int i = 0; i < NUM_CORES;i++)
    {
        //cores[i]->m_rob->printContents();
        //
        std::cout << "Cycles = " << cores[i]->m_clk << "\n";
        std::cout << "Instructions = " << (tp.last_ts[i] - tp.warmup_period) << "\n"; 
        std::cout << "IPC = " << (double) (tp.last_ts[i] - tp.warmup_period)/(cores[i]->m_clk) << "\n";

        std::cout << "--------Core " << i << " -------------" << std::endl; 
        std::cout << "[L1 D$] Number of data hits = " << l1_data_caches[i]->num_data_hits << std::endl;
        std::cout << "[L1 D$] Number of translation hits = " << l1_data_caches[i]->num_tr_hits << std::endl;
        std::cout << "[L1 D$] Number of data misses = " << l1_data_caches[i]->num_data_misses << std::endl;
        std::cout << "[L1 D$] Number of translation misses = " << l1_data_caches[i]->num_tr_misses << std::endl;
        std::cout << "[L1 D$] Number of MSHR data hits = " << l1_data_caches[i]->num_mshr_data_hits << std::endl;
        std::cout << "[L1 D$] Number of MSHR translation hits = " << l1_data_caches[i]->num_mshr_tr_hits << std::endl;
        std::cout << "[L1 D$] Number of data accesses = " << l1_data_caches[i]->num_data_accesses << std::endl;
        std::cout << "[L1 D$] Number of translation accesses = " << l1_data_caches[i]->num_tr_accesses << std::endl;
        std::cout << "[L1 D$] MPKI = " << (double) (l1_data_caches[i]->num_data_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << std::endl; 
        
        std::cout << "[L2 D$] Number of data hits = " << l2_data_caches[i]->num_data_hits << std::endl;
        std::cout << "[L2 D$] Number of translation hits = " << l2_data_caches[i]->num_tr_hits << std::endl;
        std::cout << "[L2 D$] Number of data misses = " << l2_data_caches[i]->num_data_misses << std::endl;
        std::cout << "[L2 D$] Number of translation misses = " << l2_data_caches[i]->num_tr_misses << std::endl;
        std::cout << "[L2 D$] Number of MSHR data hits = " << l2_data_caches[i]->num_mshr_data_hits << std::endl;
        std::cout << "[L2 D$] Number of MSHR translation hits = " << l2_data_caches[i]->num_mshr_tr_hits << std::endl;
        std::cout << "[L2 D$] Number of data accesses = " << l2_data_caches[i]->num_data_accesses << std::endl;
        std::cout << "[L2 D$] Number of translation accesses = " << l2_data_caches[i]->num_tr_accesses << std::endl;
        std::cout << "[L2 D$] MPKI = " << (double) ((l2_data_caches[i]->num_data_misses  + l2_data_caches[i]->num_tr_misses)* 1000.0)/(tp.last_ts[i] - tp.warmup_period) << std::endl; 

        std::cout << "[L1 SMALL TLB] Number of data hits = " << l1_tlb[2 * i]->num_data_hits << std::endl;
        std::cout << "[L1 SMALL TLB] Number of translation hits = " << l1_tlb[2 * i]->num_tr_hits << std::endl;
        std::cout << "[L1 SMALL TLB] Number of data misses = " << l1_tlb[2 * i]->num_data_misses << std::endl;
        std::cout << "[L1 SMALL TLB] Number of translation misses = " << l1_tlb[2 * i]->num_tr_misses << std::endl;
        std::cout << "[L1 SMALL TLB] Number of MSHR data hits = " << l1_tlb[2 * i]->num_mshr_data_hits << std::endl;
        std::cout << "[L1 SMALL TLB] Number of MSHR translation hits = " << l1_tlb[2 * i]->num_mshr_tr_hits << std::endl;
        std::cout << "[L1 SMALL TLB] Number of data accesses = " << l1_tlb[2 * i]->num_data_accesses << std::endl;
        std::cout << "[L1 SMALL TLB] Number of translation accesses = " << l1_tlb[2 * i]->num_tr_accesses << std::endl;
        std::cout << "[L1 SMALL TLB] MPKI = " << (double) (l1_tlb[2 * i]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << std::endl; 

        std::cout << "[L1 LARGE TLB] Number of data hits = " << l1_tlb[2 * i + 1]->num_data_hits << std::endl;
        std::cout << "[L1 LARGE TLB] Number of translation hits = " << l1_tlb[2 * i + 1]->num_tr_hits << std::endl;
        std::cout << "[L1 LARGE TLB] Number of data misses = " << l1_tlb[2 * i + 1]->num_data_misses << std::endl;
        std::cout << "[L1 LARGE TLB] Number of translation misses = " << l1_tlb[2 * i + 1]->num_tr_misses << std::endl;
        std::cout << "[L1 LARGE TLB] Number of MSHR data hits = " << l1_tlb[2 * i + 1]->num_mshr_data_hits << std::endl;
        std::cout << "[L1 LARGE TLB] Number of MSHR translation hits = " << l1_tlb[2 * i + 1]->num_mshr_tr_hits << std::endl;
        std::cout << "[L1 LARGE TLB] Number of data accesses = " << l1_tlb[2 * i + 1]->num_data_accesses << std::endl;
        std::cout << "[L1 LARGE TLB] Number of translation accesses = " << l1_tlb[2 * i + 1]->num_tr_accesses << std::endl;
        std::cout << "[L1 LARGE TLB] MPKI = " << (double) (l1_tlb[2 * i + 1]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << std::endl; 

        std::cout << "[L2 SMALL TLB] Number of data hits = " << l2_tlb[2 * i]->num_data_hits << std::endl;
        std::cout << "[L2 SMALL TLB] Number of translation hits = " << l2_tlb[2 * i]->num_tr_hits << std::endl;
        std::cout << "[L2 SMALL TLB] Number of data misses = " << l2_tlb[2 * i]->num_data_misses << std::endl;
        std::cout << "[L2 SMALL TLB] Number of translation misses = " << l2_tlb[2 * i]->num_tr_misses << std::endl;
        std::cout << "[L2 SMALL TLB] Number of MSHR data hits = " << l2_tlb[2 * i]->num_mshr_data_hits << std::endl;
        std::cout << "[L2 SMALL TLB] Number of MSHR translation hits = " << l2_tlb[2 * i]->num_mshr_tr_hits << std::endl;
        std::cout << "[L2 SMALL TLB] Number of data accesses = " << l2_tlb[2 * i]->num_data_accesses << std::endl;
        std::cout << "[L2 SMALL TLB] Number of translation accesses = " << l2_tlb[2 * i]->num_tr_accesses << std::endl;
        std::cout << "[L2 SMALL TLB] MPKI = " << (double) (l2_tlb[2 * i]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << std::endl; 

        std::cout << "[L2 LARGE TLB] Number of data hits = " << l2_tlb[2 * i + 1]->num_data_hits << std::endl;
        std::cout << "[L2 LARGE TLB] Number of translation hits = " << l2_tlb[2 * i + 1]->num_tr_hits << std::endl;
        std::cout << "[L2 LARGE TLB] Number of data misses = " << l2_tlb[2 * i + 1]->num_data_misses << std::endl;
        std::cout << "[L2 LARGE TLB] Number of translation misses = " << l2_tlb[2 * i + 1]->num_tr_misses << std::endl;
        std::cout << "[L2 LARGE TLB] Number of MSHR data hits = " << l2_tlb[2 * i + 1]->num_mshr_data_hits << std::endl;
        std::cout << "[L2 LARGE TLB] Number of MSHR translation hits = " << l2_tlb[2 * i + 1]->num_mshr_tr_hits << std::endl;
        std::cout << "[L2 LARGE TLB] Number of data accesses = " << l2_tlb[2 * i + 1]->num_data_accesses << std::endl;
        std::cout << "[L2 LARGE TLB] Number of translation accesses = " << l2_tlb[2 * i + 1]->num_tr_accesses << std::endl;
        std::cout << "[L2 LARGE TLB] MPKI = " << (double) (l2_tlb[2 * i + 1]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << std::endl; 
    }
    
    std::cout << "[L3] Number of data hits = " << llc->num_data_hits << std::endl;
    std::cout << "[L3] Number of translation hits = " << llc->num_tr_hits << std::endl;
    std::cout << "[L3] Number of data misses = " << llc->num_data_misses << std::endl;
    std::cout << "[L3] Number of translation misses = " << llc->num_tr_misses << std::endl;
    std::cout << "[L3] Number of MSHR data hits = " << llc->num_mshr_data_hits << std::endl;
    std::cout << "[L3] Number of MSHR translation hits = " << llc->num_mshr_tr_hits << std::endl;
    std::cout << "[L3] Number of data accesses = " << llc->num_data_accesses << std::endl;
    std::cout << "[L3] Number of translation accesses = " << llc->num_tr_accesses << std::endl;
    std::cout << "[L3] MPKI = " << (double) ((llc->num_data_misses  + llc->num_tr_misses)* 1000.0)/(tp.last_ts[0] - tp.warmup_period) << std::endl; 

    std::cout << "[L3 SMALL TLB] Number of data hits = " << l3_tlb_small->num_data_hits << std::endl;
    std::cout << "[L3 SMALL TLB] Number of translation hits = " << l3_tlb_small->num_tr_hits << std::endl;
    std::cout << "[L3 SMALL TLB] Number of data misses = " << l3_tlb_small->num_data_misses << std::endl;
    std::cout << "[L3 SMALL TLB] Number of translation misses = " << l3_tlb_small->num_tr_misses << std::endl;
    std::cout << "[L3 SMALL TLB] Number of MSHR data hits = " << l3_tlb_small->num_mshr_data_hits << std::endl;
    std::cout << "[L3 SMALL TLB] Number of MSHR translation hits = " << l3_tlb_small->num_mshr_tr_hits << std::endl;
    std::cout << "[L3 SMALL TLB] Number of data accesses = " << l3_tlb_small->num_data_accesses << std::endl;
    std::cout << "[L3 SMALL TLB] Number of translation accesses = " << l3_tlb_small->num_tr_accesses << std::endl;
    std::cout << "[L3 SMALL TLB] MPKI = " << (double) (l3_tlb_small->num_tr_misses * 1000.0)/(tp.last_ts[0] - tp.warmup_period) << std::endl; 
    
    std::cout << "[L3 LARGE TLB] Number of data hits = " << l3_tlb_large->num_data_hits << std::endl;
    std::cout << "[L3 LARGE TLB] Number of translation hits = " << l3_tlb_large->num_tr_hits << std::endl;
    std::cout << "[L3 LARGE TLB] Number of data misses = " << l3_tlb_large->num_data_misses << std::endl;
    std::cout << "[L3 LARGE TLB] Number of translation misses = " << l3_tlb_large->num_tr_misses << std::endl;
    std::cout << "[L3 LARGE TLB] Number of MSHR data hits = " << l3_tlb_large->num_mshr_data_hits << std::endl;
    std::cout << "[L3 LARGE TLB] Number of MSHR translation hits = " << l3_tlb_large->num_mshr_tr_hits << std::endl;
    std::cout << "[L3 LARGE TLB] Number of data accesses = " << l3_tlb_large->num_data_accesses << std::endl;
    std::cout << "[L3 LARGE TLB] Number of translation accesses = " << l3_tlb_large->num_tr_accesses << std::endl;
    std::cout << "[L3 LARGE TLB] MPKI = " << (double) (l3_tlb_large->num_tr_misses * 1000.0)/(tp.last_ts[0] - tp.warmup_period) << std::endl; 

    std::cout << "----------------------------------------------------------------------\n";

    for(auto it = tp.presence_map_small_page.begin(); it != tp.presence_map_small_page.end(); it++)
    {
        const RequestDesc &r = it->first;
        std::cout << r << std::dec << ": ";

        for(auto setit = it->second.begin(); setit != it->second.end(); setit++)
        {
            std::cout << *setit << ",";
        }

        std::cout << "\n";
    }

    std::cout << "----------------------------------------------------------------------\n";

    for(auto it = tp.presence_map_large_page.begin(); it != tp.presence_map_large_page.end(); it++)
    {
        const RequestDesc &r = it->first;
        std::cout << r << std::dec << ": ";

        for(auto setit = it->second.begin(); setit != it->second.end(); setit++)
        {
            std::cout << *setit << ",";
        }

        std::cout << "\n";
    }
}
