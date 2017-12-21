//
//  utils.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef utils_hpp
#define utils_hpp

#include <iostream>

//Defines
#define ADDR_SIZE 48

unsigned int log2(unsigned int num);

typedef enum {
    REQUEST_HIT,
    REQUEST_MISS,
    REQUEST_RETRY,
    MSHR_HIT,
    MSHR_HIT_AND_LOCKED,
} RequestStatus;

#endif /* utils_hpp */
