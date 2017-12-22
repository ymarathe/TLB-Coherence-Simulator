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
    std::shared_ptr<Cache> l1c_ptr_c1 = std::make_shared<Cache>(Cache(1, 2, 64, 3));
    std::shared_ptr<Cache> l2c_ptr_c1 = std::make_shared<Cache>(Cache(2, 8, 64, 10));
    std::shared_ptr<Cache> l3c_ptr = std::make_shared<Cache>(Cache(4, 8, 64, 25));
    
    std::shared_ptr<Cache> l1c_ptr_c2 = std::make_shared<Cache>(Cache(1, 2, 64, 3));
    std::shared_ptr<Cache> l2c_ptr_c2 = std::make_shared<Cache>(Cache(2, 8, 64, 10));
    
    CacheSys cache_hier[2];
    
    cache_hier[0].add_cache_to_hier(l1c_ptr_c1);
    cache_hier[0].add_cache_to_hier(l2c_ptr_c1);
    cache_hier[0].add_cache_to_hier(l3c_ptr);
    
    cache_hier[1].add_cache_to_hier(l1c_ptr_c2);
    cache_hier[1].add_cache_to_hier(l2c_ptr_c2);
    cache_hier[1].add_cache_to_hier(l3c_ptr);
    
    l1c_ptr_c1->lookupAndFillCache(0xFFFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x7FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x3FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x1FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x0FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x07FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x03FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c2->lookupAndFillCache(0x01FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    std::cout << "-------B4: Core 1 caches begin -------" << std::endl;
    cache_hier[0].printContents();
    std::cout << "-------B4: Core 1 caches end -------" << std::endl;
    std::cout << "-------B4: Core 2 caches begin -------" << std::endl;
    cache_hier[1].printContents();
    std::cout << "-------B4: Core 2 caches end ---------" << std::endl;
    
    l1c_ptr_c1->lookupAndFillCache(0x00FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();
    
    l1c_ptr_c1->lookupAndFillCache(0x007FFFFFFFC0, TRANSLATION_WRITE);
    cache_hier[0].tick();
    cache_hier[1].tick();

    std::cout << "-------Core 1 caches begin -------" << std::endl;
    cache_hier[0].printContents();
    std::cout << "-------Core 1 caches end -------" << std::endl;
    std::cout << "-------Core 2 caches begin -------" << std::endl;
    cache_hier[1].printContents();
    std::cout << "-------Core 2 caches end ---------" << std::endl;
    
    for(int i = 0; i < 236; i++)
    {
        cache_hier[0].tick();
        cache_hier[1].tick();
    }

}
