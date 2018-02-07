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

int main(int argc, const char * argv[])
{
#define NUM_CORES 2
    
    std::shared_ptr<Cache> llc = std::make_shared<Cache>(Cache(4, 8, 64, 25,  DATA_AND_TRANSLATION));
    
    bool ll_interface_complete = false;
    
    /*
    std::shared_ptr<Cache> l3_tlb_small = std::make_shared<Cache>(Cache(16384, 4, 4096, 1, TRANSLATION_ONLY));
    std::shared_ptr<Cache> l3_tlb_large = std::make_shared<Cache>(Cache(4096, 4, 2 * 1024 * 1024, 1, TRANSLATION_ONLY));
    */
    std::shared_ptr<Cache> l3_tlb_small = std::make_shared<Cache>(Cache(8, 8, 4096, 1, TRANSLATION_ONLY));
    std::shared_ptr<Cache> l3_tlb_large = std::make_shared<Cache>(Cache(8, 8, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true));
    
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
        l1_data_caches.push_back(std::make_shared<Cache>(Cache(1, 2, 64, 3, DATA_ONLY)));
        l2_data_caches.push_back(std::make_shared<Cache>(Cache(2, 4, 64, 10, DATA_AND_TRANSLATION)));
        
        data_hier[i]->add_cache_to_hier(l1_data_caches[i]);
        data_hier[i]->add_cache_to_hier(l2_data_caches[i]);
        data_hier[i]->add_cache_to_hier(llc);
        
        tlb_hier.push_back(std::make_shared<CacheSys>(CacheSys(true)));
        l1_tlb.push_back(std::make_shared<Cache>(Cache(2, 2, 4096, 1, TRANSLATION_ONLY)));
        l1_tlb.push_back(std::make_shared<Cache>(Cache(1, 2, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(4, 4, 4096, 1, TRANSLATION_ONLY)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(4, 4, 2 * 1024 * 1024, 1, TRANSLATION_ONLY, true)));
        
        /*l1_tlb.push_back(std::make_shared<Cache>(Cache(16, 4, 4096, 1, TRANSLATION_ONLY)));
        l1_tlb.push_back(std::make_shared<Cache>(Cache(8, 4, 2 * 1024 * 1024, 1, TRANSLATION_ONLY)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(64, 16, 4096, 1, TRANSLATION_ONLY)));
        l2_tlb.push_back(std::make_shared<Cache>(Cache(64, 16, 2 * 1024 * 1024, 1, TRANSLATION_ONLY)));*/
        
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
    
    Request req(0xFFFFFFFFFFC0, DATA_READ, 0, true, 0);
    cores[0]->add_trace(req);
    cores[0]->tick();
    std::cout << "---Core 0 tick complete--- " << std::endl;
    cores[1]->tick();
    std::cout << "---Core 1 tick complete--- " << std::endl;
    
    req.m_addr = 0x7FFFFFFFFFC0;
    req.update_request_type(DATA_WRITE);
    
    cores[0]->add_trace(req);
    cores[0]->tick();
    cores[1]->tick();
    
    cores[0]->tick();
    cores[1]->tick();
    
    for(int i = 0; i < 1000; i++)
    {
        cores[0]->tick();
        cores[1]->tick();
    }
    
    /*cores[0]->add_trace(req);
    cores[0]->tick();
    std::cout << "---Core 0 tick complete--- " << std::endl;
    cores[1]->tick();
    std::cout << "---Core 1 tick complete--- " << std::endl;
    
    for(int i = 0; i < 1000; i++)
    {
        cores[0]->tick();
        cores[1]->tick();
    }
    
    cores[0]->add_trace(req);
    cores[0]->tick();
    cores[1]->tick();
    
    cores[0]->tick();
    cores[1]->tick();
    
    cores[0]->tick();
    cores[1]->tick();
    
    cores[0]->tick();
    cores[1]->tick();*/
    
    std::cout << "---------Core 0 ROB----------" << std::endl;
    cores[0]->m_rob->printContents();
    std::cout << cores[0]->m_rob->m_num_waiting_instr << std::endl;
    std::cout << "-----------------------------" << std::endl;
    
    std::cout << "---------Core 1 ROB----------" << std::endl;
    cores[1]->m_rob->printContents();
    std::cout << cores[1]->m_rob->m_num_waiting_instr << std::endl;
    std::cout << "-----------------------------" << std::endl;
    
    std::cout << "---------------Core 1 cache---------------" << std::endl;
    data_hier[0]->printContents();
    std::cout << "--------------Core 1 cache end------------" << std::endl;
    
    std::cout << "---------------Core 2 cache---------------" << std::endl;
    data_hier[1]->printContents();
    std::cout << "--------------Core 2 cache end------------" << std::endl;
    
    std::cout << "--------------Core 1 TLB------------------" << std::endl;
    tlb_hier[0]->printContents();
    std::cout << "-------------Core 1 TLB end-------------" << std::endl;
    
    std::cout << "--------------Core 2 TLB------------------" << std::endl;
    tlb_hier[1]->printContents();
    std::cout << "-------------Core 2 TLB end-------------" << std::endl;
    
}
