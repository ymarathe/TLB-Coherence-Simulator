//
//  main.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "Cache.hpp"
#include "CacheSys.hpp"
#include "ROB.hpp"
#include "Core.hpp"
#include "TraceProcessor.hpp"
#include <memory>
#include "utils.hpp"

#define NUM_TRACES_PER_CORE 81000000L
#define NUM_TOTAL_TRACES NUM_TRACES_PER_CORE * NUM_CORES 
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
        l2_tlb.push_back(std::make_shared<Cache>(Cache(32, 16, 2 * 1024 * 1024, 14, TRANSLATION_ONLY, true)));

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

    //Make cores aware of each other
    for(int i = 0; i < NUM_CORES; i++)
    {
        for(int j = 0; j < NUM_CORES; j++)
        {
            if(i != j)
            {
                cores[i]->add_core(cores[j]);
            }
        }
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

    uint64_t num_traces_added = 0;

    std::cout << "Initial fill\n";
    for(int i = 0; i < NUM_INITIAL_FILL; i++)
    {
        Request *r = tp.generateRequest();

        //std::cout << "Request = " << std::hex << (*r) << std::dec;

        if(r->m_core_id >=0 && r->m_core_id < NUM_CORES)
        {
                cores[r->m_core_id]->add_trace(r);
                num_traces_added += int(r->m_is_memory_acc);
        } 
    }
    std::cout << "Initial fill done\n";

   bool done = false;
   bool timeout = false;
   while(!done && !timeout)
   {
	done = true;
   	for(int i = 0; i < NUM_CORES; i++)
   	{
        if(!cores[i]->is_done())
	        cores[i]->tick();
       
       if((num_traces_added < NUM_TOTAL_TRACES) && (cores[i]->must_add_trace()))
       {
           Request *r = tp.generateRequest();

           //std::cout << "Request = " << std::hex << (*r) << std::dec;

           if(r->m_core_id >= 0 && r->m_core_id < NUM_CORES)
           {
               cores[r->m_core_id]->add_trace(r);
           }
           
           if(num_traces_added % 1000000 == 0)
           {
               std::cout << "[NUM_TRACES_ADDED] Count = " << num_traces_added << "\n";
           }

           num_traces_added +=  int(r->m_is_memory_acc);
       }

	   done = done & cores[i]->is_done(); 
	   if(cores[i]->m_clk >= NUM_TRACES_PER_CORE * 5)
	   {
		   //std::cout << "Core " << i << " timed out " << std::endl;
		   //std::cout << "Blocking request = " ; cores[i]->m_rob->peek_commit_ptr();
           for(int j = 0; j < NUM_CORES; j++)
           {
               if(cores[j]->traceVec.size())
               {
                   std::cout << "Core " << j << " has unserviced requests = " << cores[j]->traceVec.size() << "\n";
               }

               if(!cores[j]->is_done())
               {
                   std::cout << "Core " << j << " NOT done\n";
		           std::cout << "Blocking request = " ; cores[j]->m_rob->peek_commit_ptr();
                   std::cout << "Core clk = " << cores[j]->m_clk << "\n";
               }

               if(cores[j]->stall)
               {
                   std::cout << "Core " << j << " STALLED\n";
               }

               if(!cores[j]->m_rob->can_issue())
               {
                   std::cout << "Core " << j << " can't issue\n";
               }
           }
		   timeout = true;
	   }
 	}
   }


    std::ofstream outFile;
#ifdef BASELINE
    outFile.open("dedup_baseline_oc1.out");
#else
    outFile.open("dedup_cotag_oc1.out");
#endif

    for(int i = 0; i < NUM_CORES;i++)
    {
        //cores[i]->m_rob->printContents();
        //
        outFile << "Cycles = " << cores[i]->m_clk << "\n";
        outFile << "Instructions = " << (tp.last_ts[i] - tp.warmup_period) << "\n";
        outFile << "IPC = " << (double) (tp.last_ts[i] - tp.warmup_period)/(cores[i]->m_clk) << "\n";
        outFile << "Stall cycles = " << cores[i]->num_stall_cycles << "\n";
        outFile << "Num shootdowns = " << cores[i]->num_shootdown << "\n";

        outFile << "--------Core " << i << " -------------" << "\n";
        outFile << "[L1 D$] data hits = " << l1_data_caches[i]->num_data_hits << "\n";
        outFile << "[L1 D$] translation hits = " << l1_data_caches[i]->num_tr_hits << "\n";
        outFile << "[L1 D$] data misses = " << l1_data_caches[i]->num_data_misses << "\n";
        outFile << "[L1 D$] translation misses = " << l1_data_caches[i]->num_tr_misses << "\n";
        outFile << "[L1 D$] MSHR data hits = " << l1_data_caches[i]->num_mshr_data_hits << "\n";
        outFile << "[L1 D$] MSHR translation hits = " << l1_data_caches[i]->num_mshr_tr_hits << "\n";
        outFile << "[L1 D$] data accesses = " << l1_data_caches[i]->num_data_accesses << "\n";
        outFile << "[L1 D$] translation accesses = " << l1_data_caches[i]->num_tr_accesses << "\n";
        outFile << "[L1 D$] MPKI = " << (double) (l1_data_caches[i]->num_data_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << "\n";

        outFile << "[L2 D$] data hits = " << l2_data_caches[i]->num_data_hits << "\n";
        outFile << "[L2 D$] translation hits = " << l2_data_caches[i]->num_tr_hits << "\n";
        outFile << "[L2 D$] data misses = " << l2_data_caches[i]->num_data_misses << "\n";
        outFile << "[L2 D$] translation misses = " << l2_data_caches[i]->num_tr_misses << "\n";
        outFile << "[L2 D$] MSHR data hits = " << l2_data_caches[i]->num_mshr_data_hits << "\n";
        outFile << "[L2 D$] MSHR translation hits = " << l2_data_caches[i]->num_mshr_tr_hits << "\n";
        outFile << "[L2 D$] data accesses = " << l2_data_caches[i]->num_data_accesses << "\n";
        outFile << "[L2 D$] translation accesses = " << l2_data_caches[i]->num_tr_accesses << "\n";
        outFile << "[L2 D$] MPKI = " << (double) ((l2_data_caches[i]->num_data_misses  + l2_data_caches[i]->num_tr_misses)* 1000.0)/(tp.last_ts[i] - tp.warmup_period) << "\n";

        outFile << "[L1 SMALL TLB] data hits = " << l1_tlb[2 * i]->num_data_hits << "\n";
        outFile << "[L1 SMALL TLB] translation hits = " << l1_tlb[2 * i]->num_tr_hits << "\n";
        outFile << "[L1 SMALL TLB] data misses = " << l1_tlb[2 * i]->num_data_misses << "\n";
        outFile << "[L1 SMALL TLB] translation misses = " << l1_tlb[2 * i]->num_tr_misses << "\n";
        outFile << "[L1 SMALL TLB] MSHR data hits = " << l1_tlb[2 * i]->num_mshr_data_hits << "\n";
        outFile << "[L1 SMALL TLB] MSHR translation hits = " << l1_tlb[2 * i]->num_mshr_tr_hits << "\n";
        outFile << "[L1 SMALL TLB] data accesses = " << l1_tlb[2 * i]->num_data_accesses << "\n";
        outFile << "[L1 SMALL TLB] translation accesses = " << l1_tlb[2 * i]->num_tr_accesses << "\n";
        outFile << "[L1 SMALL TLB] MPKI = " << (double) (l1_tlb[2 * i]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << "\n";

        outFile << "[L1 LARGE TLB] data hits = " << l1_tlb[2 * i + 1]->num_data_hits << "\n";
        outFile << "[L1 LARGE TLB] translation hits = " << l1_tlb[2 * i + 1]->num_tr_hits << "\n";
        outFile << "[L1 LARGE TLB] data misses = " << l1_tlb[2 * i + 1]->num_data_misses << "\n";
        outFile << "[L1 LARGE TLB] translation misses = " << l1_tlb[2 * i + 1]->num_tr_misses << "\n";
        outFile << "[L1 LARGE TLB] MSHR data hits = " << l1_tlb[2 * i + 1]->num_mshr_data_hits << "\n";
        outFile << "[L1 LARGE TLB] MSHR translation hits = " << l1_tlb[2 * i + 1]->num_mshr_tr_hits << "\n";
        outFile << "[L1 LARGE TLB] data accesses = " << l1_tlb[2 * i + 1]->num_data_accesses << "\n";
        outFile << "[L1 LARGE TLB] translation accesses = " << l1_tlb[2 * i + 1]->num_tr_accesses << "\n";
        outFile << "[L1 LARGE TLB] MPKI = " << (double) (l1_tlb[2 * i + 1]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << "\n";

        outFile << "[L2 SMALL TLB] data hits = " << l2_tlb[2 * i]->num_data_hits << "\n";
        outFile << "[L2 SMALL TLB] translation hits = " << l2_tlb[2 * i]->num_tr_hits << "\n";
        outFile << "[L2 SMALL TLB] data misses = " << l2_tlb[2 * i]->num_data_misses << "\n";
        outFile << "[L2 SMALL TLB] translation misses = " << l2_tlb[2 * i]->num_tr_misses << "\n";
        outFile << "[L2 SMALL TLB] MSHR data hits = " << l2_tlb[2 * i]->num_mshr_data_hits << "\n";
        outFile << "[L2 SMALL TLB] MSHR translation hits = " << l2_tlb[2 * i]->num_mshr_tr_hits << "\n";
        outFile << "[L2 SMALL TLB] data accesses = " << l2_tlb[2 * i]->num_data_accesses << "\n";
        outFile << "[L2 SMALL TLB] translation accesses = " << l2_tlb[2 * i]->num_tr_accesses << "\n";
        outFile << "[L2 SMALL TLB] MPKI = " << (double) (l2_tlb[2 * i]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << "\n";

        outFile << "[L2 LARGE TLB] data hits = " << l2_tlb[2 * i + 1]->num_data_hits << "\n";
        outFile << "[L2 LARGE TLB] translation hits = " << l2_tlb[2 * i + 1]->num_tr_hits << "\n";
        outFile << "[L2 LARGE TLB] data misses = " << l2_tlb[2 * i + 1]->num_data_misses << "\n";
        outFile << "[L2 LARGE TLB] translation misses = " << l2_tlb[2 * i + 1]->num_tr_misses << "\n";
        outFile << "[L2 LARGE TLB] MSHR data hits = " << l2_tlb[2 * i + 1]->num_mshr_data_hits << "\n";
        outFile << "[L2 LARGE TLB] MSHR translation hits = " << l2_tlb[2 * i + 1]->num_mshr_tr_hits << "\n";
        outFile << "[L2 LARGE TLB] data accesses = " << l2_tlb[2 * i + 1]->num_data_accesses << "\n";
        outFile << "[L2 LARGE TLB] translation accesses = " << l2_tlb[2 * i + 1]->num_tr_accesses << "\n";
        outFile << "[L2 LARGE TLB] MPKI = " << (double) (l2_tlb[2 * i + 1]->num_tr_misses * 1000.0)/(tp.last_ts[i] - tp.warmup_period) << "\n";
    }

    outFile << "[L3] data hits = " << llc->num_data_hits << "\n";
    outFile << "[L3] translation hits = " << llc->num_tr_hits << "\n";
    outFile << "[L3] data misses = " << llc->num_data_misses << "\n";
    outFile << "[L3] translation misses = " << llc->num_tr_misses << "\n";
    outFile << "[L3] MSHR data hits = " << llc->num_mshr_data_hits << "\n";
    outFile << "[L3] MSHR translation hits = " << llc->num_mshr_tr_hits << "\n";
    outFile << "[L3] data accesses = " << llc->num_data_accesses << "\n";
    outFile << "[L3] translation accesses = " << llc->num_tr_accesses << "\n";
    outFile << "[L3] MPKI = " << (double) ((llc->num_data_misses  + llc->num_tr_misses)* 1000.0)/(tp.last_ts[0] - tp.warmup_period) << "\n";

    outFile << "[L3 SMALL TLB] data hits = " << l3_tlb_small->num_data_hits << "\n";
    outFile << "[L3 SMALL TLB] translation hits = " << l3_tlb_small->num_tr_hits << "\n";
    outFile << "[L3 SMALL TLB] data misses = " << l3_tlb_small->num_data_misses << "\n";
    outFile << "[L3 SMALL TLB] translation misses = " << l3_tlb_small->num_tr_misses << "\n";
    outFile << "[L3 SMALL TLB] MSHR data hits = " << l3_tlb_small->num_mshr_data_hits << "\n";
    outFile << "[L3 SMALL TLB] MSHR translation hits = " << l3_tlb_small->num_mshr_tr_hits << "\n";
    outFile << "[L3 SMALL TLB] data accesses = " << l3_tlb_small->num_data_accesses << "\n";
    outFile << "[L3 SMALL TLB] translation accesses = " << l3_tlb_small->num_tr_accesses << "\n";
    outFile << "[L3 SMALL TLB] MPKI = " << (double) (l3_tlb_small->num_tr_misses * 1000.0)/(tp.last_ts[0] - tp.warmup_period) << "\n";

    outFile << "[L3 LARGE TLB] data hits = " << l3_tlb_large->num_data_hits << "\n";
    outFile << "[L3 LARGE TLB] translation hits = " << l3_tlb_large->num_tr_hits << "\n";
    outFile << "[L3 LARGE TLB] data misses = " << l3_tlb_large->num_data_misses << "\n";
    outFile << "[L3 LARGE TLB] translation misses = " << l3_tlb_large->num_tr_misses << "\n";
    outFile << "[L3 LARGE TLB] MSHR data hits = " << l3_tlb_large->num_mshr_data_hits << "\n";
    outFile << "[L3 LARGE TLB] MSHR translation hits = " << l3_tlb_large->num_mshr_tr_hits << "\n";
    outFile << "[L3 LARGE TLB] data accesses = " << l3_tlb_large->num_data_accesses << "\n";
    outFile << "[L3 LARGE TLB] translation accesses = " << l3_tlb_large->num_tr_accesses << "\n";
    outFile << "[L3 LARGE TLB] MPKI = " << (double) (l3_tlb_large->num_tr_misses * 1000.0)/(tp.last_ts[0] - tp.warmup_period) << "\n";

    outFile << "----------------------------------------------------------------------\n";
    outFile.close();
    
    /*for(auto it = tp.presence_map_small_page.begin(); it != tp.presence_map_small_page.end(); it++)
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
    }*/
}
