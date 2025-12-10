#ifndef LOAD_SLICE_TIMER_H
#define LOAD_SLICE_TIMER_H

#include <stdint.h>

#include "core.h"
#include "circular_queue.h"
#include "dynamic_micro_op.h"

#include "boost/tuple/tuple.hpp"

#include "register_dependency_table.h"
#include "instruction_slice_table.h"

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
};

class LoadSliceTimer {
    private:
        Core *core;
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
        CircularQueue<ScoreBoardEntry*> agiQueue;

        const int dispatchWidth;
        const int issueWidth;
        const int commitWidth;
        const uint64_t windowSize;
        const uint64_t mispredictionPenalty;

        const bool bypassLoads;
        const bool bypassStores;
        const bool bypassGenerators;

        uint64_t countBypassLoads;
        uint64_t countBypassStores;
        uint64_t countBypassGenerators;

        uint64_t stallLoadDep;
        uint64_t stallLoadTime;
        uint64_t stallStoreDep;
        uint64_t stallStoreTime;
        uint64_t stallAgiDep;
        uint64_t stallAgiTime;

        uint64_t nextSequenceNumber;
        uint64_t headSequenceNumber;
        uint64_t scoreBoardCount;

        std::vector<uint64_t> registerProducerMap;
        std::unordered_map<uint64_t,uint64_t> memoryProducerMap;

        // load slice detection
        RegisterDependencyTable registerDependencyTable;
        InstructionSliceTable instructionSliceTable;
    
    public:
        LoadSliceTimer(
            Core *core,
            int dispatchWidth,
            int windowSize
        );
        ~LoadSliceTimer();
        ScoreBoardEntry *findEntryBySequenceNumber(uint64_t sequenceNumber);
        boost::tuple<uint64_t, uint64_t> simulate(const std::vector<DynamicMicroOp *> &insts);
        bool shouldBypass(ScoreBoardEntry *entry);
        void dispatch();
        void issue();
        void issueInstruction(ScoreBoardEntry *entry);
        int commit();
        int advance();
        void print();
        void enable();
        void disable();
};

#endif
