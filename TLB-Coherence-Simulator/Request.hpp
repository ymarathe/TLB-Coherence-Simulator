//
//  Request.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/11/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef Request_hpp
#define Request_hpp

#include <iostream>
#include <functional>
#include "utils.hpp"

class Request {
public:
    friend class RequestComparator;
    uint64_t m_addr;
    kind m_type;
    unsigned int m_core_id;
    uint64_t m_tid;
    bool m_is_large;
    bool m_is_core_agnostic;
    const std::function<void(std::unique_ptr<Request>&)>& m_callback;
    
    Request(uint64_t addr, kind type, uint64_t tid, bool is_large, unsigned int core_id, const std::function<void(std::unique_ptr<Request>&)>& callback = nullptr) :
    m_addr(addr),
    m_type(type),
    m_core_id(core_id),
    m_callback(callback),
    m_tid(tid),
    m_is_large(is_large),
    m_is_core_agnostic(false)
    {}
    
    bool is_translation_request();
    
    friend std::ostream& operator << (std::ostream &out, const Request &r)
    {
        out << "Addr: " << r.m_addr << ", kind: " << r.m_type <<  ", tid: " << r.m_tid << ", is_large: " << r.m_is_large << ", core: " << r.m_core_id << ", is_agnostic:" << r.m_is_core_agnostic << std::endl;
        return out;
    }
    
};

class RequestComparator {
public:
    bool operator () (const Request &r1, const Request &r2) const
    {
        //If all else is equal, and one of the requests is core agnostic, return false.
        if(r1.m_addr < r2.m_addr)
            return true;
        if(r1.m_type < r2.m_type)
            return true;
        if(r1.m_tid < r2.m_tid)
            return true;
        if(r1.m_is_large < r2.m_is_large)
            return true;
        if(!r1.m_is_core_agnostic && !r2.m_is_core_agnostic && (r1.m_core_id < r2.m_core_id))
            return true;
        
        return false;
    }
};

#endif /* Request_hpp */
