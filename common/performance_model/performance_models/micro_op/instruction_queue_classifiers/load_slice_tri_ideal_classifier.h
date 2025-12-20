#ifndef __LOAD_SLICE_TRI_IDEAL_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_TRI_IDEAL_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"

#include <set>
#include <array>

class LoadSliceTriIdealClassifier : public InstructionQueueClassifier
{
  std::vector<UInt64> producers, pending_deps;
  std::set<UInt64> agis;
  UInt64 peekProducer(const dl::Decoder::decoder_reg reg) const;

protected:
  enum instruction_queue_type {
    LOAD_QUEUE = 0,
    AGI_QUEUE,
    MAIN_QUEUE,
    QUEUE_COUNT
  };

  static std::array<const char *, QUEUE_COUNT> const queueNames;

public:
  LoadSliceTriIdealClassifier();
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
  const char * getQueueName(const UInt64 queueIdx) const override;
};

#endif // __LOAD_SLICE_TRI_IDEAL_CLASSIFIER_TABLE_H
