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
    return ((m_type == TRANSLATION_WRITE) || (m_type == TRANSLATION_WRITEBACK) || (m_type == TRANSLATION_READ));
}
