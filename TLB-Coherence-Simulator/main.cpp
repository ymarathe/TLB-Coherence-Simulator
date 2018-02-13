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

#define NUM_CORES 8
#define NUM_INSTRUCTIONS 1000000 

int main(int argc, char * argv[])
{
    int num_args = 1;
    int num_real_args = num_args + 1;
    
    TraceProcessor tp(NUM_CORES);
    if (argc < num_real_args)
    {
        std::cout << "Program takes " << num_args << " arguments" << std::endl;
        std::cout << "Path name of input config file" << std::endl;
        exit(0);
    }
    
    char* input_cfg=argv[1];
    
    tp.parseAndSetupInputs(input_cfg);
    
    tp.verifyOpenTraceFiles();
    
    std::shared_ptr<Cache> llc = std::make_shared<Cache>(Cache(8192, 16, 64, 25,  DATA_AND_TRANSLATION));
    
    bool ll_interface_complete = false;
    
    std::shared_ptr<Cache> l3_tlb_small = std::make_shared<Cache>(Cache(16384, 4, 4096, 1, TRANSLATION_ONLY));
    std::shared_ptr<Cache> l3_tlb_large = std::make_shared<Cache>(Cache(4096, 4, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true));
    
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
        l1_data_caches.push_back(std::make_shared<Cache>(Cache(64, 8, 64, 3, DATA_ONLY)));
        l2_data_caches.push_back(std::make_shared<Cache>(Cache(1024, 4, 64, 10, DATA_AND_TRANSLATION)));
        
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
        l2_tlb.push_back(std::make_shared<Cache>(Cache(64, 16, 4096, 1, TRANSLATION_ONLY)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(64, 16, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true)));
        
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
                data_hier[i]->add_cachesys(data_hier[j]);
            }
        }
    }
    
    //Make TLB hierarchies aware of each other
    for(int i = 0; i < NUM_CORES; i++)
    {
        for(int j = 0; j < NUM_CORES; j++)
        {
            if(i != j)
            {
                tlb_hier[i]->add_cachesys(tlb_hier[j]);
            }
        }
    }
    for(int i = 0; i < NUM_INSTRUCTIONS; i++)
    {
        Request *r = tp.generateRequest();
	cores[r->m_core_id]->add_trace(r);
    }

    for(int j = 0; j < NUM_INSTRUCTIONS * 2; j++)
    {
	for(int i = 0; i < NUM_CORES; i++)
	{
	    cores[i]->tick();
	}
    } 

    for(int i = 0; i < NUM_CORES;i++)
    {
	    cores[i]->m_rob->printContents();
    }

}
