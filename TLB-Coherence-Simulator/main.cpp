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
#define NUM_INITIAL_FILL 100 

int main(int argc, char * argv[])
{
    int num_args = 1;
    int num_real_args = num_args + 1;
    uint64_t num_total_traces;
    
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

    if(tp.is_multicore)
    {
        num_total_traces = NUM_TRACES_PER_CORE * NUM_CORES;
    }
    else
    {
        num_total_traces = NUM_TRACES_PER_CORE;
    }
    
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

        if((r != nullptr) && r->m_core_id >=0 && r->m_core_id < NUM_CORES)
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
       
       if((num_traces_added < num_total_traces) && (cores[i]->must_add_trace()))
       {
           Request *r = tp.generateRequest();

           //std::cout << "Request = " << std::hex << (*r) << std::dec;

           if((r != nullptr) && r->m_core_id >= 0 && r->m_core_id < NUM_CORES)
           {
               cores[r->m_core_id]->add_trace(r);
           }
           
           if(num_traces_added % 1000000 == 0)
           {
               std::cout << "[NUM_TRACES_ADDED] Count = " << num_traces_added << "\n";
           }

           if(r != nullptr)
           {
               num_traces_added +=  int(r->m_is_memory_acc);
           }
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
#ifdef IDEAL
    outFile.open("dedup_baseline_oc1_test_ideal.out");
#else
    outFile.open("dedup_baseline_oc1_test.out");
#endif
#else
    outFile.open("dedup_cotag_oc1_test.out");
#endif
    uint64_t total_num_cycles = 0;
    uint64_t total_stall_cycles = 0;
    uint64_t total_shootdowns = 0;
    double l1d_agg_mpki;
    double l2d_agg_mpki;
    double l1ts_agg_mpki;
    double l1tl_agg_mpki;
    double l2ts_agg_mpki;
    double l2tl_agg_mpki;

    for(int i = 0; i < NUM_CORES;i++)
    {
        //cores[i]->m_rob->printContents();
        //
        outFile << "--------Core " << i << " -------------" << "\n";
        if(tp.is_multicore)
        {
            outFile << "Cycles = " << cores[i]->m_clk << "\n";
            outFile << "Instructions = " << (tp.last_ts[i] - tp.warmup_period) << "\n";
            if(cores[i]->m_clk > 0)
            {
                outFile << "IPC = " << (double) (tp.last_ts[i] - tp.warmup_period)/(cores[i]->m_clk) << "\n";
            }
            outFile << "Stall cycles = " << cores[i]->num_stall_cycles << "\n";
            outFile << "Num shootdowns = " << cores[i]->num_shootdown << "\n";
        }
        else
        {
            outFile << "Cycles = " << cores[i]->m_clk << "\n";
            total_num_cycles += cores[i]->m_clk;
            total_stall_cycles += cores[i]->num_stall_cycles;
            total_shootdowns += cores[i]->num_shootdown;
        }

        outFile << "[L1 D$] data hits = " << l1_data_caches[i]->num_data_hits << "\n";
        outFile << "[L1 D$] translation hits = " << l1_data_caches[i]->num_tr_hits << "\n";
        outFile << "[L1 D$] data misses = " << l1_data_caches[i]->num_data_misses << "\n";
        outFile << "[L1 D$] translation misses = " << l1_data_caches[i]->num_tr_misses << "\n";
        outFile << "[L1 D$] MSHR data hits = " << l1_data_caches[i]->num_mshr_data_hits << "\n";
        outFile << "[L1 D$] MSHR translation hits = " << l1_data_caches[i]->num_mshr_tr_hits << "\n";
        outFile << "[L1 D$] data accesses = " << l1_data_caches[i]->num_data_accesses << "\n";
        outFile << "[L1 D$] translation accesses = " << l1_data_caches[i]->num_tr_accesses << "\n";
        if(tp.last_ts[i] > tp.warmup_period)
        {
            double l1d_mpki = (double) (l1_data_caches[i]->num_data_misses * 1000.0)/((tp.is_multicore ? tp.last_ts[i]: tp.global_ts) - tp.warmup_period);
            outFile << "[L1 D$] MPKI = " << l1d_mpki << "\n";
            if(!tp.is_multicore) l1d_agg_mpki += l1d_mpki;
        }

        outFile << "[L2 D$] data hits = " << l2_data_caches[i]->num_data_hits << "\n";
        outFile << "[L2 D$] translation hits = " << l2_data_caches[i]->num_tr_hits << "\n";
        outFile << "[L2 D$] data misses = " << l2_data_caches[i]->num_data_misses << "\n";
        outFile << "[L2 D$] translation misses = " << l2_data_caches[i]->num_tr_misses << "\n";
        outFile << "[L2 D$] MSHR data hits = " << l2_data_caches[i]->num_mshr_data_hits << "\n";
        outFile << "[L2 D$] MSHR translation hits = " << l2_data_caches[i]->num_mshr_tr_hits << "\n";
        outFile << "[L2 D$] data accesses = " << l2_data_caches[i]->num_data_accesses << "\n";
        outFile << "[L2 D$] translation accesses = " << l2_data_caches[i]->num_tr_accesses << "\n";
        if(tp.last_ts[i] > tp.warmup_period)
        {
            double l2d_mpki = (double) ((l2_data_caches[i]->num_data_misses  + l2_data_caches[i]->num_tr_misses)* 1000.0)/((tp.is_multicore ? tp.last_ts[i]: tp.global_ts) - tp.warmup_period);
            outFile << "[L2 D$] MPKI = " << l2d_mpki << "\n";
            if(!tp.is_multicore) l2d_agg_mpki += l2d_mpki;
        }

        outFile << "[L1 SMALL TLB] data hits = " << l1_tlb[2 * i]->num_data_hits << "\n";
        outFile << "[L1 SMALL TLB] translation hits = " << l1_tlb[2 * i]->num_tr_hits << "\n";
        outFile << "[L1 SMALL TLB] data misses = " << l1_tlb[2 * i]->num_data_misses << "\n";
        outFile << "[L1 SMALL TLB] translation misses = " << l1_tlb[2 * i]->num_tr_misses << "\n";
        outFile << "[L1 SMALL TLB] MSHR data hits = " << l1_tlb[2 * i]->num_mshr_data_hits << "\n";
        outFile << "[L1 SMALL TLB] MSHR translation hits = " << l1_tlb[2 * i]->num_mshr_tr_hits << "\n";
        outFile << "[L1 SMALL TLB] data accesses = " << l1_tlb[2 * i]->num_data_accesses << "\n";
        outFile << "[L1 SMALL TLB] translation accesses = " << l1_tlb[2 * i]->num_tr_accesses << "\n";
        if(tp.last_ts[i] > tp.warmup_period)
        {
            double l1ts_mpki = (double) ((l1_tlb[2 * i]->num_data_misses  + l1_tlb[2 * i]->num_tr_misses)* 1000.0)/((tp.is_multicore ? tp.last_ts[i]: tp.global_ts) - tp.warmup_period);
            outFile << "[L1 SMALL TLB] MPKI = " << l1ts_mpki << "\n";
            if(!tp.is_multicore) l1ts_agg_mpki += l1ts_mpki;
        }

        outFile << "[L1 LARGE TLB] data hits = " << l1_tlb[2 * i + 1]->num_data_hits << "\n";
        outFile << "[L1 LARGE TLB] translation hits = " << l1_tlb[2 * i + 1]->num_tr_hits << "\n";
        outFile << "[L1 LARGE TLB] data misses = " << l1_tlb[2 * i + 1]->num_data_misses << "\n";
        outFile << "[L1 LARGE TLB] translation misses = " << l1_tlb[2 * i + 1]->num_tr_misses << "\n";
        outFile << "[L1 LARGE TLB] MSHR data hits = " << l1_tlb[2 * i + 1]->num_mshr_data_hits << "\n";
        outFile << "[L1 LARGE TLB] MSHR translation hits = " << l1_tlb[2 * i + 1]->num_mshr_tr_hits << "\n";
        outFile << "[L1 LARGE TLB] data accesses = " << l1_tlb[2 * i + 1]->num_data_accesses << "\n";
        outFile << "[L1 LARGE TLB] translation accesses = " << l1_tlb[2 * i + 1]->num_tr_accesses << "\n";
        if(tp.last_ts[i] > tp.warmup_period)
        {
            double l1tl_mpki = (double) ((l1_tlb[2 * i + 1]->num_data_misses  + l1_tlb[2 * i + 1]->num_tr_misses)* 1000.0)/((tp.is_multicore ? tp.last_ts[i]: tp.global_ts) - tp.warmup_period);
            outFile << "[L1 LARGE TLB] MPKI = " << l1tl_mpki << "\n";
            if(!tp.is_multicore) l1tl_agg_mpki += l1tl_mpki;
        }

        outFile << "[L2 SMALL TLB] data hits = " << l2_tlb[2 * i]->num_data_hits << "\n";
        outFile << "[L2 SMALL TLB] translation hits = " << l2_tlb[2 * i]->num_tr_hits << "\n";
        outFile << "[L2 SMALL TLB] data misses = " << l2_tlb[2 * i]->num_data_misses << "\n";
        outFile << "[L2 SMALL TLB] translation misses = " << l2_tlb[2 * i]->num_tr_misses << "\n";
        outFile << "[L2 SMALL TLB] MSHR data hits = " << l2_tlb[2 * i]->num_mshr_data_hits << "\n";
        outFile << "[L2 SMALL TLB] MSHR translation hits = " << l2_tlb[2 * i]->num_mshr_tr_hits << "\n";
        outFile << "[L2 SMALL TLB] data accesses = " << l2_tlb[2 * i]->num_data_accesses << "\n";
        outFile << "[L2 SMALL TLB] translation accesses = " << l2_tlb[2 * i]->num_tr_accesses << "\n";
        if(tp.last_ts[i] > tp.warmup_period)
        {
            double l2ts_mpki = (double) ((l2_tlb[2 * i]->num_data_misses  + l2_tlb[2 * i]->num_tr_misses)* 1000.0)/((tp.is_multicore ? tp.last_ts[i]: tp.global_ts) - tp.warmup_period);
            outFile << "[L2 SMALL TLB] MPKI = " << l2ts_mpki << "\n";
            if(!tp.is_multicore) l2ts_agg_mpki += l2ts_mpki;
        }

        outFile << "[L2 LARGE TLB] data hits = " << l2_tlb[2 * i + 1]->num_data_hits << "\n";
        outFile << "[L2 LARGE TLB] translation hits = " << l2_tlb[2 * i + 1]->num_tr_hits << "\n";
        outFile << "[L2 LARGE TLB] data misses = " << l2_tlb[2 * i + 1]->num_data_misses << "\n";
        outFile << "[L2 LARGE TLB] translation misses = " << l2_tlb[2 * i + 1]->num_tr_misses << "\n";
        outFile << "[L2 LARGE TLB] MSHR data hits = " << l2_tlb[2 * i + 1]->num_mshr_data_hits << "\n";
        outFile << "[L2 LARGE TLB] MSHR translation hits = " << l2_tlb[2 * i + 1]->num_mshr_tr_hits << "\n";
        outFile << "[L2 LARGE TLB] data accesses = " << l2_tlb[2 * i + 1]->num_data_accesses << "\n";
        outFile << "[L2 LARGE TLB] translation accesses = " << l2_tlb[2 * i + 1]->num_tr_accesses << "\n";
        if(tp.last_ts[i] > tp.warmup_period)
        {
            double l2tl_mpki = (double) ((l2_tlb[2 * i + 1]->num_data_misses  + l2_tlb[2 * i + 1]->num_tr_misses)* 1000.0)/((tp.is_multicore ? tp.last_ts[i]: tp.global_ts) - tp.warmup_period);
            outFile << "[L2 LARGE TLB] MPKI = " << l2tl_mpki << "\n";
            if(!tp.is_multicore) l2tl_agg_mpki += l2tl_mpki;
        }
    }

    outFile << "----------------------------------------------------------------------\n";

    if(!tp.is_multicore)
    {
        outFile << "Cycles = " << total_num_cycles << "\n";
        outFile << "Instructions = " << (tp.global_ts - tp.warmup_period) << "\n";
        if(total_num_cycles > 0)
        {
            outFile << "IPC = " << (double) (tp.global_ts - tp.warmup_period)/(total_num_cycles) << "\n";
        }
        outFile << "Stall cycles = " << total_stall_cycles << "\n";
        outFile << "Num shootdowns = " << total_shootdowns << "\n";
        outFile << "[L1 D$] Aggregate MPKI = " << l1d_agg_mpki << "\n";
        outFile << "[L2 D$] Aggregate MPKI = " << l2d_agg_mpki << "\n";
        outFile << "[L1 SMALL TLB] Aggregate MPKI = " << l1ts_agg_mpki << "\n";
        outFile << "[L1 LARGE TLB] Aggregate MPKI = " << l1tl_agg_mpki << "\n";
        outFile << "[L2 SMALL TLB] Aggregate MPKI = " << l2ts_agg_mpki << "\n";
        outFile << "[L2 LARGE TLB] Aggregate MPKI = " << l2tl_agg_mpki << "\n";
    }

    outFile << "----------------------------------------------------------------------\n";

    outFile << "[L3] data hits = " << llc->num_data_hits << "\n";
    outFile << "[L3] translation hits = " << llc->num_tr_hits << "\n";
    outFile << "[L3] data misses = " << llc->num_data_misses << "\n";
    outFile << "[L3] translation misses = " << llc->num_tr_misses << "\n";
    outFile << "[L3] MSHR data hits = " << llc->num_mshr_data_hits << "\n";
    outFile << "[L3] MSHR translation hits = " << llc->num_mshr_tr_hits << "\n";
    outFile << "[L3] data accesses = " << llc->num_data_accesses << "\n";
    outFile << "[L3] translation accesses = " << llc->num_tr_accesses << "\n";
    if(tp.last_ts[0] > tp.warmup_period)
    {
        outFile << "[L3] MPKI = " << (double) ((llc->num_data_misses  + llc->num_tr_misses)* 1000.0)/((tp.is_multicore ? tp.last_ts[0]: tp.global_ts) - tp.warmup_period) << "\n";
    }

    outFile << "[L3 SMALL TLB] data hits = " << l3_tlb_small->num_data_hits << "\n";
    outFile << "[L3 SMALL TLB] translation hits = " << l3_tlb_small->num_tr_hits << "\n";
    outFile << "[L3 SMALL TLB] data misses = " << l3_tlb_small->num_data_misses << "\n";
    outFile << "[L3 SMALL TLB] translation misses = " << l3_tlb_small->num_tr_misses << "\n";
    outFile << "[L3 SMALL TLB] MSHR data hits = " << l3_tlb_small->num_mshr_data_hits << "\n";
    outFile << "[L3 SMALL TLB] MSHR translation hits = " << l3_tlb_small->num_mshr_tr_hits << "\n";
    outFile << "[L3 SMALL TLB] data accesses = " << l3_tlb_small->num_data_accesses << "\n";
    outFile << "[L3 SMALL TLB] translation accesses = " << l3_tlb_small->num_tr_accesses << "\n";
    if(tp.last_ts[0] > tp.warmup_period)
    {
        outFile << "[L3 SMALL TLB] MPKI = " << (double) (l3_tlb_small->num_tr_misses * 1000.0)/((tp.is_multicore ? tp.last_ts[0]: tp.global_ts) - tp.warmup_period) << "\n";
    }

    outFile << "[L3 LARGE TLB] data hits = " << l3_tlb_large->num_data_hits << "\n";
    outFile << "[L3 LARGE TLB] translation hits = " << l3_tlb_large->num_tr_hits << "\n";
    outFile << "[L3 LARGE TLB] data misses = " << l3_tlb_large->num_data_misses << "\n";
    outFile << "[L3 LARGE TLB] translation misses = " << l3_tlb_large->num_tr_misses << "\n";
    outFile << "[L3 LARGE TLB] MSHR data hits = " << l3_tlb_large->num_mshr_data_hits << "\n";
    outFile << "[L3 LARGE TLB] MSHR translation hits = " << l3_tlb_large->num_mshr_tr_hits << "\n";
    outFile << "[L3 LARGE TLB] data accesses = " << l3_tlb_large->num_data_accesses << "\n";
    outFile << "[L3 LARGE TLB] translation accesses = " << l3_tlb_large->num_tr_accesses << "\n";
    if(tp.last_ts[0] > tp.warmup_period)
    {
        outFile << "[L3 LARGE TLB] MPKI = " << (double) (l3_tlb_large->num_tr_misses * 1000.0)/((tp.is_multicore ? tp.last_ts[0]: tp.global_ts) - tp.warmup_period) << "\n";
    }

    outFile << "----------------------------------------------------------------------\n";
    outFile.close();

}
