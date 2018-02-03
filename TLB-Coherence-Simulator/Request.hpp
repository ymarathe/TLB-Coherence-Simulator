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
    uint64_t m_addr;
    kind m_type;
    unsigned int m_core_id;
    unsigned int m_cache_level;
    uint64_t m_tid;
    bool m_is_large;
    std::function<void(std::unique_ptr<Request>&)>& m_callback;
    
    Request(uint64_t addr, kind type, std::function<void(std::unique_ptr<Request>&)>& callback, uint64_t tid, bool is_large, unsigned int core_id, unsigned int cache_level) :
    m_addr(addr),
    m_type(type),
    m_core_id(core_id),
    m_cache_level(cache_level),
    m_callback(callback),
    m_tid(tid),
    m_is_large(is_large)
    {}
    
    Request(uint64_t addr, kind type, std::function<void(std::unique_ptr<Request>&)>& callback) :
    Request(addr, type, callback, 0, 0, 0, 0) {}
    
    Request(uint64_t addr, kind type, std::function<void(std::unique_ptr<Request>&)>& callback, unsigned int core_id) :
    Request(addr, type, callback, 0, 0, core_id, 0) {}
    
    Request(uint64_t addr, kind type, std::function<void(std::unique_ptr<Request>&)>& callback, uint64_t tid, bool is_large, unsigned int core_id) :
    Request(addr, type, callback, tid, is_large, core_id, 0) {}
    
    bool is_translation_request();
    
    friend std::ostream& operator << (std::ostream &out, const Request &r)
    {
        out << "Addr: " << r.m_addr << ", kind: " << r.m_type << ", core : " << r.m_core_id << std::endl;
        return out;
    }
};

#endif /* Request_hpp */
