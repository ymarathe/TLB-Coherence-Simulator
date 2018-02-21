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
#include <memory>
#include "utils.hpp"

class Request {
public:
    //friend class RequestComparator;
    uint64_t m_addr;
    kind m_type;
    unsigned int m_core_id;
    uint64_t m_tid;
    //Following two fields are to dispatch request to data hierarchy.
    //m_is_read | m_is_translation  | Consequence
    //  0       |       0           | Dispatch DATA_WRITE
    //  0       |       1           | Dispatch TRANSLATION_WRITE
    //  1       |       0           | Dispatch DATA_READ
    //  0       |       1           | Dispacth TRANSLATION_READ
    //This is necessary because we always do a TRANSLATION_READ before doing memory access.
    //And once the request bounces back from TLB hierarchy, we need to dispatch the right kind of request to data hierarchy.
    bool m_is_read;
    bool m_is_translation;
    bool m_is_large;
    bool m_is_core_agnostic;
    bool m_is_memory_acc;
    std::function<void(std::shared_ptr<Request>)> m_callback;
    
    
    Request(uint64_t addr, kind type, uint64_t tid, bool is_large, unsigned int core_id, std::function<void(std::shared_ptr<Request>)> callback = nullptr, bool is_memory_acc = true) :
    m_addr(addr),
    m_type(type),
    m_core_id(core_id),
    m_callback(callback),
    m_tid(tid),
    m_is_large(is_large),
    m_is_core_agnostic(false),
    m_is_memory_acc(is_memory_acc)
    {
        update_request_type(m_type);
    }
    
    Request() : Request(0, INVALID_TXN_KIND, 0, 0, 0) {}
    
    bool is_translation_request();
    
    void update_request_type(kind txn_kind);
    
    void update_request_type_from_core(kind txn_kind);
    
    void add_callback(std::function<void(std::shared_ptr<Request>)>& callback);
    
    friend std::ostream& operator << (std::ostream &out, const Request &r)
    {
        out << "Addr: " << r.m_addr << ", kind: " << r.m_type <<  ", tid: " << r.m_tid << ", is_large: " << r.m_is_large << ", core: " << r.m_core_id << ", is_agnostic:" << r.m_is_core_agnostic << std::endl;
        return out;
    }
    
    bool operator == (const Request &r) const
    {
        return ((r.m_addr == m_addr) && (r.m_tid == m_tid) && (r.m_type == m_type) && (r.m_is_large == m_is_large)) && ((!r.m_is_core_agnostic && !m_is_core_agnostic) ? (r.m_core_id == m_core_id) : true);
    }

    bool operator < (const Request &r) const
    {
        if(m_addr < r.m_addr)
	{
            return true;
	}

        if(m_type < r.m_type)
	{
            return true;
	}

        if(m_tid < r.m_tid)
	{
            return true;
	}

        if(m_is_large < r.m_is_large)
	{
            return true;
	}

        if(!m_is_core_agnostic && !r.m_is_core_agnostic && (m_core_id < r.m_core_id))
	{
            return true;
	}

        return false;
    }
    
};

struct RequestHasher {
	std::size_t operator () (const Request &r) const
	{
		using std::size_t;
		using std::hash;
	
		size_t res = 42;
		res = res * 31 + hash<uint64_t>() (r.m_addr);
		res = res * 31 + hash<unsigned int>() (r.m_type);
	       	res = res * 31 + hash<uint64_t>() (r.m_tid);
		res = res * 31 + hash<bool>() (r.m_is_large);	
		return res;
	}
	
};

/*class RequestComparator {
public:
    bool operator () (const Request &r1, const Request &r2) const
    {
        //If all else is equal, and one of the requests is core agnostic, return false.
        if(r1.m_addr < r2.m_addr)
	{
            return true;
	}

        if(r1.m_type < r2.m_type)
	{
            return true;
	}

        if(r1.m_tid < r2.m_tid)
	{
            return true;
	}

        if(r1.m_is_large < r2.m_is_large)
	{
            return true;
	}

        if(!r1.m_is_core_agnostic && !r2.m_is_core_agnostic && (r1.m_core_id < r2.m_core_id))
	{
            return true;
	}

        return false;
    }
};*/

#endif /* Request_hpp */
