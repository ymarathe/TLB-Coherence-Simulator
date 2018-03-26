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

class CoherenceProtocol;

class CacheLine {
public:
    bool valid;
    bool dirty;
    bool lock;
    bool is_translation;
    bool is_large;
    uint64_t tag;
    uint64_t tid;
    uint64_t cotag;
    CoherenceProtocol *m_coherence_prot;
    
    CacheLine() : valid(false),
                  dirty(false),
                  lock(false),
                  is_translation(false),
                  is_large(false),
                  tid(0),
                  cotag(0),
                  tag(0) { }
    
    CoherenceProtocol* get_coherence_prot()
    {
        return m_coherence_prot;
    }

    void set_coherence_prot(CoherenceProtocol *coherence_prot)
    {
        m_coherence_prot = coherence_prot;
    }
    
    friend std::ostream& operator << (std::ostream &out, const CacheLine &l)
    {
        if(l.is_translation)
        {
            out << "|" << l.valid << "|" << l.dirty << "|" << l.lock << "|" << l.is_translation << "|" << std::hex << l.tag << "|" << l.cotag << "| --" ;
        }
        else
        {
            out << "|" << l.valid << "|" << l.dirty << "|" << l.lock << "|" << l.is_translation << "|" << std::hex << l.tag << "| -- ";
        }
        return out;
    }
};


#endif /* CacheLine_h */
