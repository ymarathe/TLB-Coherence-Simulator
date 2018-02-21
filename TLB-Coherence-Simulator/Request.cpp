//
//  Request.cpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/11/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#include "Request.hpp"

bool Request::is_translation_request()
{
    return (m_type == TRANSLATION_WRITE) || (m_type == TRANSLATION_READ) || (m_type == TRANSLATION_WRITEBACK);
}

void Request::add_callback(std::function<void (std::shared_ptr<Request>)>& callback)
{
    m_callback = callback;
}

void Request::update_request_type(kind txn_kind)
{
    m_type = txn_kind;
    
    if(m_type == DATA_READ || m_type == TRANSLATION_READ)
    {
        m_is_read = true;
    }
    else if(m_type == DATA_WRITE || m_type == TRANSLATION_WRITE)
    {
        m_is_read = false;
    }
    
    if(m_type == DATA_READ || m_type == DATA_WRITE)
    {
        m_is_translation = false;
    }
    else if(m_type == TRANSLATION_READ || m_type == TRANSLATION_WRITE)
    {
        m_is_translation = true;
    }
}

void Request::update_request_type_from_core(kind txn_kind)
{
    m_type = txn_kind;
}
