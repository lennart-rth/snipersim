#ifndef __LOAD_SLICE_ONLY_LOAD_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_ONLY_LOAD_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"
#include <array>

class LoadSliceOnlyLoadClassifier : public InstructionQueueClassifier
{
  std::vector<UInt64> pending_deps;

protected:
  enum instruction_queue_type {
    LOAD_QUEUE = 0,
    MAIN_QUEUE,
    QUEUE_COUNT
  };

  static std::array<const char *, QUEUE_COUNT> const queueNames;

public:
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
  const char * getQueueName(const UInt64 queueIdx) const override;
};

#endif // __LOAD_SLICE_ONLY_LOAD_CLASSIFIER_TABLE_H