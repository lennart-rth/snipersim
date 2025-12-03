#ifndef LOAD_SLICE_TIMER_H
#define LOAD_SLICE_TIMER_H

#include <stdint.h>

#include "core.h"
#include "circular_queue.h"
#include "dynamic_micro_op.h"

#include "boost/tuple/tuple.hpp"

class ScoreBoardEntry {
    public:
        DynamicMicroOp *uop;
        SubsecondTime dispatched;
        SubsecondTime issued;
        bool isReady;
        SubsecondTime readyToIssue;
        SubsecondTime readyToForward;
        SubsecondTime readyToCommit;

        void init(DynamicMicroOp *_uop, uint64_t sequence_number);
        bool isDispatched() { return dispatched != SubsecondTime::MaxTime(); }
        bool isIssued() { return issued != SubsecondTime::MaxTime(); }
        bool shouldBypass();
};

class LoadSliceTimer {
    private:
        ComponentTime now;
        SubsecondTime nextDispatch;
        SubsecondTime nextIssue;
        SubsecondTime nextCommit;

        SubsecondTime stalledUntil;
        bool stalledByCacheMiss;
        bool stalledByBranchMiss;

        CircularQueue<ScoreBoardEntry> scoreBoard;
        CircularQueue<ScoreBoardEntry*> mainQueue;
        CircularQueue<ScoreBoardEntry*> bypassQueue;

        const int dispatchWidth;
        const int issueWidth;
        const int commitWidth;
        const uint64_t windowSize;
        const uint64_t mispredictionPenalty;

        uint64_t nextSequenceNumber;
        uint64_t headSequenceNumber;
        uint64_t scoreBoardCount;
    
    public:
        LoadSliceTimer(
            Core *core,
            int dispatchWidth,
            int windowSize
        );
        ~LoadSliceTimer();
        ScoreBoardEntry *findEntryBySequenceNumber(uint64_t sequenceNumber);
        boost::tuple<uint64_t, uint64_t> simulate(const std::vector<DynamicMicroOp *> &insts);
        void dispatch();
        void issue();
        void issueInstruction(ScoreBoardEntry *entry);
        int commit();
        int advance();
};

#endif
