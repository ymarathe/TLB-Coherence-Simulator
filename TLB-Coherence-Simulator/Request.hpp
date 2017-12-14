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

enum kind {
    DATA_READ = 0,
    DATA_WRITE = 1,
    TRANSLATION_READ = 2,
    TRANSLATION_WRITE = 3,
    DATA_WRITEBACK = 4,
    TRANSLATION_WRITEBACK = 5,
};

class Request {
public:
    uint64_t m_addr;
    kind m_type;
    int m_core_id;
    std::function<void(Request &)> m_callback;
    
    Request(uint64_t addr, kind type, std::function<void(Request&)> callback, int core_id) :
    m_addr(addr),
    m_type(type),
    m_core_id(core_id),
    m_callback(callback) {}
    
    Request(uint64_t addr, kind type, std::function<void(Request&)> callback) :
    Request(addr, type, callback, 0) {}
    
    Request(uint64_t addr, kind type) :
    Request(addr, type, [](Request& req) {}, 0) {}
};

#endif /* Request_hpp */
