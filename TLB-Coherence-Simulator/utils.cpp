//
//  utils.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "utils.hpp"

unsigned int log2(unsigned int num)
{
    int val = 0;
    while((num >> 1))
    {
        val++;
        num = num >> 1;
    }
    return val;
}
