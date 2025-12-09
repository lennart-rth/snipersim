#ifndef __LOAD_SLICE_BI_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_BI_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"

class LoadSliceBiClassifier : public InstructionQueueClassifier
{
  static constexpr std::size_t NUM_WAYS = 4;
  static constexpr std::size_t NUM_ENTRIES = 512;

  static UInt64 ip_to_idx(const IntPtr ip);
  static UInt32 ip_to_tagoff(const IntPtr ip);
  // offset = ip[3:0] (4 bits)
  // index = ip[12:4] (9 bits), 512 entries
  // tag = ip[21:13] (9 bits)
  struct Way
  {
    Way();
    std::vector<UInt32> m_tag_offset; // tag and offset data
    std::vector<UInt64> m_plru; // Should be pseudo-LRU, using LRU instead
  };
  std::vector<Way> m_ways;
  UInt64 m_lru_use_count;

  enum instruction_queue_type {
    BIPASS_QUEUE = 0,
    MAIN_QUEUE,
    QUEUE_COUNT
  };
  
  std::vector<UInt64> producers;

  void update(const IntPtr ip);
  UInt64 peekProducer(const dl::Decoder::decoder_reg reg) const;

public:
  LoadSliceBiClassifier();
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(const DynamicMicroOp &microOp) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
};

#endif // __LOAD_SLICE_BI_CLASSIFIER_TABLE_H