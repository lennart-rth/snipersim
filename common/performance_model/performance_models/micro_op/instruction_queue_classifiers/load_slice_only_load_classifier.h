#ifndef __LOAD_SLICE_ONLY_LOAD_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_ONLY_LOAD_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"

class LoadSliceOnlyLoadClassifier : public InstructionQueueClassifier
{ 
  enum instruction_queue_type {
    LOAD_QUEUE = 0,
    MAIN_QUEUE,
    QUEUE_COUNT
  };

  std::vector<UInt64> pending_deps;

public:
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
};

#endif // __LOAD_SLICE_ONLY_LOAD_CLASSIFIER_TABLE_H