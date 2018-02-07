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

kind txnKindForCohAction(CoherenceAction coh_action)
{
    switch(coh_action)
    {
        case BROADCAST_DATA_READ:
            return DIRECTORY_DATA_READ;
        case BROADCAST_DATA_WRITE:
            return DIRECTORY_DATA_WRITE;
        case BROADCAST_TRANSLATION_READ:
            return DIRECTORY_TRANSLATION_READ;
        case BROADCAST_TRANSLATION_WRITE:
            return DIRECTORY_TRANSLATION_WRITE;
        case MEMORY_DATA_WRITEBACK:
            return DATA_WRITEBACK;
        case MEMORY_TRANSLATION_WRITEBACK:
            return TRANSLATION_WRITEBACK;
        default:
            return INVALID_TXN_KIND;
    }
}

std::string trim(const std::string& str)
{
    std::string out;
    int i = 0;
    int l = (int) str.length();
    while ((i < l) && ((str[i] == ' ') || (str[i] == '\t')))
        i++;
    
    out = str.substr(i);
    
    l = (int) out.length();
    i = (l - 1);
    while (i >= 0)
    {
        if ((out[i] == ' ') || (out[i] == '\t')) i--;
        else break;
    }
    out = out.substr(0,i+1);
    return out;
}
