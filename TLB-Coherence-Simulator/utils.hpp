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
#define MIN_NUM_CACHES 2
#define NUM_CORES 8
#define MIN_NUM_TLBS 4
#define stringify(name) #name

unsigned int log2(unsigned int num);

typedef enum {
    NO_REQUEST,
    REQUEST_HIT,
    REQUEST_MISS,
    REQUEST_RETRY,
    MSHR_HIT,
    MSHR_HIT_AND_LOCKED,
} RequestStatus;

typedef enum {
    MODIFIED,
    OWNER,
    EXCLUSIVE,
    SHARED,
    INVALID,
} CoherenceState;

typedef enum {
    NONE,
    BROADCAST_DATA_READ,
    BROADCAST_DATA_WRITE,
    BROADCAST_TRANSLATION_READ,
    BROADCAST_TRANSLATION_WRITE,
    MEMORY_DATA_WRITEBACK,
    MEMORY_TRANSLATION_WRITEBACK,
    STATE_CORRECTION,
} CoherenceAction;

typedef enum {
    INVALID_TXN_KIND,
    DATA_READ,
    DATA_WRITE,
    TRANSLATION_READ,
    TRANSLATION_WRITE,
    DATA_WRITEBACK,
    TRANSLATION_WRITEBACK,
    DIRECTORY_DATA_WRITE,
    DIRECTORY_DATA_READ,
    DIRECTORY_TRANSLATION_WRITE,
    DIRECTORY_TRANSLATION_READ,
} kind;

typedef enum {
    DATA_ONLY,
    TRANSLATION_ONLY,
    DATA_AND_TRANSLATION,
} CacheType;

typedef struct {
    bool     large;
    uint64_t ts;
    uint64_t va;
    int      write;
} trace_tlb_entry_t;

typedef struct {
    bool     large;
    uint64_t ts;
    uint64_t va;
    int      write;
    uint64_t tid;
} trace_tlb_tid_entry_t;

kind txnKindForCohAction(CoherenceAction coh_action);

std::string trim(const std::string& str);

#endif /* utils_hpp */
