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
    Cache l1c(1, 2, 64, 3);
    Cache l2c(2, 4, 64, 10);
    Cache l3c(4, 8, 64, 25);
    
    std::shared_ptr<Cache> l1c_ptr = std::make_shared<Cache>(l1c);
    std::shared_ptr<Cache> l2c_ptr = std::make_shared<Cache>(l2c);
    std::shared_ptr<Cache> l3c_ptr = std::make_shared<Cache>(l3c);
    
    CacheSys cache_hier;
    cache_hier.add_cache_to_hier(l1c_ptr);
    cache_hier.add_cache_to_hier(l2c_ptr);
    cache_hier.add_cache_to_hier(l3c_ptr);

    cache_hier[0]->lookupAndFillCache(0xFFFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x7FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x3FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x1FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x0FFFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x07FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x03FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x01FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    cache_hier[0]->lookupAndFillCache(0x00FFFFFFFFC0, TRANSLATION_WRITE);
    cache_hier.tick();
    
    for(int i = 0; i < 246; i++)
    {
        cache_hier.tick();
    }
    
    cache_hier.printContents();
}
