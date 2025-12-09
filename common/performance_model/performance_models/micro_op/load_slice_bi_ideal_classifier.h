#ifndef __LOAD_SLICE_BI_IDEAL_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_BI_IDEAL_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"

#include <set>
#include <bitset>

class LoadSliceBiIdealClassifier : public InstructionQueueClassifier
{
  std::set<UInt64> agis;

  enum instruction_queue_type {
    BIPASS_QUEUE = 0,
    MAIN_QUEUE,
    QUEUE_COUNT
  };
  
  std::vector<UInt64> producers, pending_deps;
  std::bitset<instruction_queue_type::QUEUE_COUNT> pending_deps_not_cleared;
  UInt64 peekProducer(const dl::Decoder::decoder_reg reg) const;

public:
  LoadSliceBiIdealClassifier();
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
};

#endif // __LOAD_SLICE_BI_IDEAL_CLASSIFIER_TABLE_H