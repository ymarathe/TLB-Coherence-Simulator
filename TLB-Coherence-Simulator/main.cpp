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

int main(int argc, const char * argv[])
{
#define NUM_CORES 2
    
    std::shared_ptr<Cache> l1c_ptr_c1 = std::make_shared<Cache>(Cache(1, 2, 64, 3));
    std::shared_ptr<Cache> l2c_ptr_c1 = std::make_shared<Cache>(Cache(2, 4, 64, 10));
    std::shared_ptr<Cache> l3c_ptr = std::make_shared<Cache>(Cache(4, 8, 64, 25));
    
    std::shared_ptr<Cache> l1c_ptr_c2 = std::make_shared<Cache>(Cache(1, 2, 64, 3));
    std::shared_ptr<Cache> l2c_ptr_c2 = std::make_shared<Cache>(Cache(2, 4, 64, 10));
    
    std::vector<std::shared_ptr<CacheSys>> cs;
    
    for(int i = 0; i < NUM_CORES; i++)
    {
        cs.push_back(std::make_shared<CacheSys>(CacheSys()));
        cs[i]->set_core_id(i);
    }
    
    //Build cache hierarchies
    cs[0]->add_cache_to_hier(l1c_ptr_c1);
    cs[0]->add_cache_to_hier(l2c_ptr_c1);
    cs[0]->add_cache_to_hier(l3c_ptr);
    
    cs[1]->add_cache_to_hier(l1c_ptr_c2);
    cs[1]->add_cache_to_hier(l2c_ptr_c2);
    cs[1]->add_cache_to_hier(l3c_ptr);
    
    //Make cache hierarchies aware of each other
    for(int i = 0; i < NUM_CORES; i++)
    {
        for(int j = 0; j < NUM_CORES; j++)
        {
            if(i != j)
            {
                cs[i]->add_cachesys(cs[j]);
            }
        }
    }
    
    l1c_ptr_c1->lookupAndFillCache(0xFFFFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x7FFFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x3FFFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x1FFFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    RequestStatus r = l1c_ptr_c1->lookupAndFillCache(0x0FFFFFFFFFC0, TRANSLATION_WRITE);
    std::cout << "Request status: " << r << std::endl;
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x07FFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x03FFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x01FFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x00FFFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x007FFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x007FFFFFFFC0, TRANSLATION_WRITE);
    cs[0]->tick();
    cs[1]->tick();
    
    
    for(int i = 0; i < 1000; i++)
    {
        cs[0]->tick();
        cs[1]->tick();
    }
    
    r = l1c_ptr_c1->lookupAndFillCache(0x007FFFFFFFC0, TRANSLATION_READ);
    std::cout << "Request status::" << r << std::endl;
    cs[0]->tick();
    cs[1]->tick();
    
    std::cout << "-------Core 1 caches begin -------" << std::endl;
    cs[0]->printContents();
    std::cout << "-------Core 1 caches end -------" << std::endl;
    std::cout << "-------Core 2 caches begin -------" << std::endl;
    cs[1]->printContents();
    std::cout << "-------Core 2 caches end ---------" << std::endl;
    
    r = l1c_ptr_c1->lookupAndFillCache(0x007FFFFFFFC0, TRANSLATION_WRITE);
    std::cout << "Request status::" << r << std::endl;
    cs[0]->tick();
    cs[1]->tick();
    
}
