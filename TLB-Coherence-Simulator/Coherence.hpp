//
//  Coherence.hpp
//  TLB-Coherence-Simulator
//
//  Created by Yashwant Marathe on 12/29/17.
//  Copyright © 2017 Yashwant Marathe. All rights reserved.
//

#ifndef Coherence_hpp
#define Coherence_hpp
#include "utils.hpp"

enum CoherenceProtocolEnum {
    NO_COHERENCE,
    MSI_COHERENCE, //Placeholder, Not implemented
    MESI_COHERENCE, //Placeholder, Not implemented
    MOESI_COHERENCE,
};

class CoherenceProtocol {
protected:
    CoherenceState coh_state = INVALID;
public:
    virtual CoherenceAction setNextCoherenceState(kind txn_kind) = 0;
    CoherenceState getCoherenceState();
    void forceCoherenceState(CoherenceState coh_state);
    
    friend std::ostream& operator << (std::ostream& out, CoherenceProtocol &c)
    {
        switch(c.coh_state)
        {
            case MODIFIED:
                out << "|M|";
                break;
            case OWNER:
                out << "|O|";
                break;
            case EXCLUSIVE:
                out << "|E|";
                break;
            case SHARED:
                out << "|S|";
                break;
            case INVALID:
                out << "|I|";
                break;
        }
        return out;
    }
};

class MOESIProtocol : public CoherenceProtocol {
public:
    virtual CoherenceAction setNextCoherenceState(kind txn_kind) final override;
};

#endif /* Coherence_hpp */
