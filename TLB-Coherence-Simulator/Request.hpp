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
    int m_core_id;
    std::function<void(std::unique_ptr<Request>&)>& m_callback;
    
    Request(uint64_t addr, kind type, std::function<void(std::unique_ptr<Request>&)>& callback, int core_id) :
    m_addr(addr),
    m_type(type),
    m_core_id(core_id),
    m_callback(callback) {}
    
    Request(uint64_t addr, kind type, std::function<void(std::unique_ptr<Request>&)>& callback) :
    Request(addr, type, callback, 0) {}
    
    friend std::ostream& operator << (std::ostream &out, const Request &r)
    {
        out << "Addr: " << r.m_addr << ", kind: " << r.m_type << ", core : " << r.m_core_id << std::endl;
        return out;
    }
};

#endif /* Request_hpp */
