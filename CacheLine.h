//
//  CacheLine.h
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/6/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef CacheLine_h
#define CacheLine_h
#include<iostream>

class CacheLine {
public:
    bool valid;
    bool dirty;
    bool lock;
    bool is_translation;
    uint64_t tag;
    
    CacheLine() : valid(false), dirty(false), lock(false), is_translation(false), tag(0) {}
    
    friend std::ostream& operator << (std::ostream &out, const CacheLine &l)
    {
        out << "|" << l.valid << "|" << l.dirty << "|" << l.lock << "|" << l.is_translation << "|" << std::hex << l.tag << "| -- ";
        return out;
    }
};


#endif /* CacheLine_h */
