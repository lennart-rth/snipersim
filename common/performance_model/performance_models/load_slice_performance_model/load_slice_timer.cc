#include "load_slice_timer.h"

#include "config.hpp"
#include "instruction.h"

void ScoreBoardEntry::init(DynamicMicroOp *_uop, uint64_t sequenceNumber) {
    uop = _uop;
    uop->setSequenceNumber(sequenceNumber);
    dispatched = SubsecondTime::MaxTime();
    issued = SubsecondTime::MaxTime();
    isReady = false;
    readyToIssue = SubsecondTime::Zero();
    readyToForward = SubsecondTime::MaxTime();
    readyToCommit = SubsecondTime::MaxTime();
}

LoadSliceTimer::LoadSliceTimer(
    Core *core,
    int dispatchWidth,
    int windowSize
)
: core(core)
, now(core->getDvfsDomain()) 
, dispatchWidth(dispatchWidth)
, issueWidth(dispatchWidth)
, commitWidth(dispatchWidth)
, windowSize(windowSize)
, scoreBoard(256)
, mainQueue(256)
, bypassQueue(256)
, mispredictionPenalty(8)
, bypassLoads(Sim()->getCfg()->getBool("perf_model/core/load_slice_timer/bypass_loads"))
, bypassStores(Sim()->getCfg()->getBool("perf_model/core/load_slice_timer/bypass_stores"))
, bypassGenerators(Sim()->getCfg()->getBool("perf_model/core/load_slice_timer/bypass_generators"))
, registerProducerMap(Sim()->getDecoder()->last_reg())
, memoryProducerMap()
, registerDependencyTable()
, instructionSliceTable()
{
    nextIssue = SubsecondTime::MaxTime();
    nextCommit = SubsecondTime::MaxTime();
    stalledUntil = SubsecondTime::Zero();
    stalledByCacheMiss = false;
    stalledByBranchMiss = false;
    nextSequenceNumber = 1;
    scoreBoardCount = 0;
}

LoadSliceTimer::~LoadSliceTimer() {

}

ScoreBoardEntry *LoadSliceTimer::findEntryBySequenceNumber(uint64_t sequenceNumber) {
    return &scoreBoard[sequenceNumber - scoreBoard[0].uop->getSequenceNumber()];
}

boost::tuple<uint64_t,uint64_t> LoadSliceTimer::simulate(const std::vector<DynamicMicroOp*>& uops) {
    for (DynamicMicroOp *uop : uops) {
        // insert into scoreboard
        ScoreBoardEntry *entry = &scoreBoard.next();
        entry->init(uop, nextSequenceNumber++);
        // register dependencies
        headSequenceNumber = scoreBoard.size() > 0 ? scoreBoard.front().uop->getSequenceNumber() : 0;
        for (int i = 0; i < uop->getMicroOp()->getSourceRegistersLength(); i++) {
            auto reg = uop->getMicroOp()->getSourceRegister(i);
            if (registerProducerMap[reg] != 0) {
                uint64_t producerSequenceNumber = registerProducerMap[reg];
                // producer still present in the scoreboard
                if (producerSequenceNumber >= headSequenceNumber) {
                    ScoreBoardEntry *producerEntry = findEntryBySequenceNumber(producerSequenceNumber);
                    // producer already issued -> execution time is known
                    if (producerEntry->isIssued()) {
                        entry->readyToIssue = std::max(entry->readyToIssue, producerEntry->readyToForward);
                    }
                    // producer yet to be issued -> will be handled later
                    else {
                        uop->addDependency(producerSequenceNumber);
                    }
                }
            }
        }
        for (int i = 0; i < uop->getMicroOp()->getDestinationRegistersLength(); i++) {
            auto reg = uop->getMicroOp()->getDestinationRegister(i);
            registerProducerMap[reg] = uop->getSequenceNumber();
        }
        // memory dependencies
        if (uop->getMicroOp()->isLoad()) {
            uint64_t address = uop->getLoadAccess().address;
            auto search = memoryProducerMap.find(address);
            if (search != memoryProducerMap.end()) {
                uint64_t producerSequenceNumber = search->second;
                // producer still present in the scoreboard
                if (producerSequenceNumber >= headSequenceNumber) {
                    ScoreBoardEntry *producerEntry = findEntryBySequenceNumber(producerSequenceNumber);
                    // producer already issued -> execution time is known
                    if (producerEntry->isIssued()) {
                        entry->readyToIssue = std::max(entry->readyToIssue, producerEntry->readyToForward);
                    }
                    // producer yet to be issued -> will be handled later
                    else {
                        uop->addDependency(producerSequenceNumber);
                    }
                }
            }
        }
        if (uop->getMicroOp()->isStore()) {
            uint64_t address = uop->getStoreAccess().address;
            memoryProducerMap[address] = uop->getSequenceNumber();
        }
        // load slice detection
        if (instructionSliceTable.predict(*uop)) {
            uop->setAddressGenerating();
        }
        instructionSliceTable.update(*uop, registerDependencyTable);
        registerDependencyTable.setDependency(*uop);
    }

    uint64_t totalInstructions = 0;
    uint64_t totalCycles = 0;
    
    while (true) {
        // if frontend is not stalled
        if (stalledUntil <= now) {
            // if there are not enough instructions to dispatch
            if (scoreBoard.size() < scoreBoardCount + dispatchWidth) {
                // wait for more instructions
                return boost::tuple<uint64_t,uint64_t>(totalInstructions, totalCycles);
            }
        }
        dispatch();
        issue();
        totalInstructions += commit();
        totalCycles += advance();
    }
}

bool LoadSliceTimer::shouldBypass(ScoreBoardEntry *entry) {
    if (bypassLoads && entry->uop->getMicroOp()->isLoad()) {
        return true;
    }
    if (bypassStores && entry->uop->getMicroOp()->isStore()) {
        return true;
    }
    if (bypassGenerators && entry->uop->isAddressGenerating()) {
        return true;
    }
    return false;
}

void LoadSliceTimer::dispatch() {
    int dispatchCount = 0;
    nextDispatch = SubsecondTime::MaxTime();
    if (now < stalledUntil) {
        nextDispatch = stalledUntil;
        return;
    }
    while (dispatchCount < dispatchWidth && scoreBoardCount < windowSize) {
        ScoreBoardEntry *entry = &scoreBoard.at(scoreBoardCount);

        // instruction cache miss
        if (entry->uop->getICacheHitWhere() != HitWhere::L1I) {
            // miss penalty must be applied
            if (!stalledByCacheMiss) {
                stalledUntil = now + entry->uop->getICacheLatency();
                stalledByCacheMiss = true;
                break;
            }
            // miss penalty has been applied
            else {
                stalledByCacheMiss = false;
            }
        }

        // one of the queues is full
        if (bypassQueue.full() || mainQueue.full()) {
            break;
        }
        
        // dispatch!
        dispatchCount++;
        scoreBoardCount++;
        entry->dispatched = now;

        // can be issued in the next cycle at the earliest
        SubsecondTime readyToIssue = now + 1;
        // update time when instruction is ready to be issued
        entry->readyToIssue = std::max(entry->readyToIssue, readyToIssue);
        // mark as ready if there are no dependencies
        if (entry->uop->getDependenciesLength() == 0) {
            entry->isReady = true;
        }

        // push to the queue
        if (shouldBypass(entry)) {
            bypassQueue.push(entry);
        }
        else {
            mainQueue.push(entry);
        }

        // branch misprediction
        if (entry->uop->getMicroOp()->isBranch()) {
            if (entry->uop->isBranchMispredicted()) {
                stalledUntil = SubsecondTime::MaxTime();
                stalledByBranchMiss = true;
                break;
            }
        }
    }
}

void LoadSliceTimer::issue() {
    int issueCount = 0;
    nextIssue = SubsecondTime::MaxTime();
    // issue from bypass queue
    while (issueCount < issueWidth && !bypassQueue.empty()) {
        ScoreBoardEntry *entry = bypassQueue.front();

        // blocked by dependency
        if (!entry->isReady) {
            break;
        }
        if (entry->readyToIssue > now) {
            // update time when next instruction is ready to be issued
            nextIssue = std::min(nextIssue, entry->readyToIssue);
            break;
        }
        // blocked by barrier?
        // blocked by load queue?
        // blocked by store queue?

        // issue!
        issueCount++;
        issueInstruction(entry);

        // pop from the queue
        bypassQueue.pop();
    }
    // issue from main queue
    while (issueCount < issueWidth && !mainQueue.empty()) {
        ScoreBoardEntry *entry = mainQueue.front();

        // blocked by dependency
        if (!entry->isReady) {
            break;
        }
        if (entry->readyToIssue > now) {
            // update time when next instruction is ready to be issued
            nextIssue = std::min(nextIssue, entry->readyToIssue);
            break;
        }
        // blocked by barrier?
        // blocked by load queue?
        // blocked by store queue?

        // issue!
        issueCount++;
        issueInstruction(entry);

        // pop from the queue
        mainQueue.pop();
    }
}

void LoadSliceTimer::issueInstruction(ScoreBoardEntry *entry) {
    // perform memory access - taken from ROB performance model
    if ((entry->uop->getMicroOp()->isLoad() || entry->uop->getMicroOp()->isStore())
      && entry->uop->getDCacheHitWhere() == HitWhere::UNKNOWN) {
        MemoryResult res = core->accessMemory(
            Core::NONE,
            entry->uop->getMicroOp()->isLoad() ? Core::READ : Core::WRITE,
            entry->uop->getAddress().address,
            NULL,
            entry->uop->getMicroOp()->getMemoryAccessSize(),
            Core::MEM_MODELED_RETURN,
            entry->uop->getMicroOp()->getInstruction() ? entry->uop->getMicroOp()->getInstruction()->getAddress() : static_cast<uint64_t>(NULL),
            now.getElapsedTime()
        );
        uint64_t latency = SubsecondTime::divideRounded(res.latency, now.getPeriod());
        entry->uop->setExecLatency(entry->uop->getExecLatency() + latency);
        entry->uop->setDCacheHitWhere(res.hit_where);
   }

    entry->issued = now;
    // can be forwarded right after it finishes execution
    entry->readyToForward = now + entry->uop->getExecLatency();
    // can be committed one cycle after it finishes execution
    entry->readyToCommit = now + entry->uop->getExecLatency() + 1;

    if (entry->uop->getMicroOp()->isLoad()) {
        // push to load queue?
    }
    if (entry->uop->getMicroOp()->isStore()) {
        // push to store queue?
        // can be forwarded one cycle after it is issued
        entry->readyToForward = now + 1;
        // can be committed one cycle after it is issued
        entry->readyToCommit = now + 1;
    }

    ScoreBoardEntry *producer = entry;
    for (int i = 0; i < scoreBoard.size(); i++) {
        ScoreBoardEntry *consumer = &scoreBoard[i];
        bool isDependent = false;
        for (int j = 0; j < consumer->uop->getDependenciesLength(); j++) {
            if (consumer->uop->getDependency(j) == producer->uop->getSequenceNumber()) {
                isDependent = true;
                break;
            }
        }
        if (isDependent) {
            consumer->uop->removeDependency(producer->uop->getSequenceNumber());
            // update time when instruction is ready to be issued
            consumer->readyToIssue = std::max(consumer->readyToIssue, producer->readyToForward);
            // mark as ready if there are no more dependencies
            if (consumer->uop->getDependenciesLength() == 0) {
                consumer->isReady = true;
            }
        }
    }

    // branch misprediction
    if (entry->uop->getMicroOp()->isBranch()) {
        if (entry->uop->isBranchMispredicted()) {
            stalledUntil = now + mispredictionPenalty; 
        }
    }
}

int LoadSliceTimer::commit() {
    int commitCount = 0;
    int instructionCount = 0;
    nextCommit = SubsecondTime::MaxTime();
    while (commitCount < commitWidth && !scoreBoard.empty()) {
        ScoreBoardEntry *entry = &scoreBoard.front();

        // not ready to commit
        if (entry->readyToCommit > now) {
            // update time when next instruction is ready to be committed
            nextCommit = std::min(nextCommit, entry->readyToCommit);
            break;
        }

        // commit!
        commitCount++;
        if (entry->uop->isLast()) {
            instructionCount++;
        }

        // remove from scoreboard
        delete entry->uop;
        scoreBoard.pop();
        scoreBoardCount--;
    }
    return instructionCount;
}

int LoadSliceTimer::advance() {
    SubsecondTime nextEvent = std::min(nextDispatch, std::min(nextIssue, nextCommit));
    SubsecondTime skip = now.getPeriod();
    if (nextEvent != SubsecondTime::MaxTime() && nextEvent > now + 1) {
        skip = nextEvent - now;
    }
    now += skip;
    return SubsecondTime::divideRounded(skip, now.getPeriod());
}

void LoadSliceTimer::print() {
    printf("now=%d\n", SubsecondTime::divideRounded(now, now.getPeriod()));
    printf("mainQueue: size=%d\n", mainQueue.size());
    for (ScoreBoardEntry *entry : mainQueue) {
        printf("%d\t%d\t%d\t%s\n",
            entry->uop->getSequenceNumber(),
            entry->uop->getDependenciesLength(),
            entry->isReady ? SubsecondTime::divideRounded(entry->readyToIssue, now.getPeriod()) : -1,
            entry->uop->getMicroOp()->getInstruction() ? entry->uop->getMicroOp()->getInstruction()->getDisassembly().c_str() : "?"
        );
    }
}
