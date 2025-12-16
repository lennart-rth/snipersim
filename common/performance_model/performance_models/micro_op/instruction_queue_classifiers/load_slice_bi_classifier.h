#ifndef __LOAD_SLICE_BI_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_BI_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"

#include <vector>
#include <array>

class LoadSliceBiClassifier : public InstructionQueueClassifier
{
  UInt64 ip_to_idx(const IntPtr ip) const;
  UInt32 ip_to_tagoff(const IntPtr ip) const;
  
  struct Way
  {
    Way(const UInt64 entries);
    std::vector<UInt32> m_tag_offset; // tag and offset data
    std::vector<UInt64> m_plru; // Should be pseudo-LRU, using LRU instead
  };
  std::vector<Way> m_ways;
  UInt64 m_lru_use_count;
  const UInt64 m_entries;

  std::vector<UInt64> producers, pending_deps;

  void update(const IntPtr ip);
  UInt64 peekProducer(const dl::Decoder::decoder_reg reg) const;
  void applyPendingDeps(DynamicMicroOp &microOp, const uint64_t lowestValidSequenceNumber);

protected:
  enum instruction_queue_type {
    BIPASS_QUEUE = 0,
    MAIN_QUEUE,
    QUEUE_COUNT
  };

  std::array<const char *, QUEUE_COUNT> const queueNames;

public:
  LoadSliceBiClassifier(const UInt64 ways, const UInt64 entries);
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(DynamicMicroOp &microOp, const RegisterDependencies&, const uint64_t lowestValidSequenceNumber) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
  const char * getQueueName(const UInt64 queueIdx) const override;
};

#endif // __LOAD_SLICE_BI_CLASSIFIER_TABLE_H