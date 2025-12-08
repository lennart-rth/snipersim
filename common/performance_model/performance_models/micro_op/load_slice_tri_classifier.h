#ifndef __LOAD_SLICE_TRI_CLASSIFIER_TABLE_H
#define __LOAD_SLICE_TRI_CLASSIFIER_TABLE_H

#include "instruction_queue_classifier.h"

class LoadSliceTriClassifier : public InstructionQueueClassifier
{
  static constexpr std::size_t NUM_WAYS = 4;
  static constexpr std::size_t NUM_ENTRIES = 512;

  #define IP_TO_INDEX(_ip) ((_ip >> 4) & 0x1ff)

  #define TAG_OFFSET_MASK 0x3fe00f
  #define IP_TO_TAGOFF(_ip) (_ip & TAG_OFFSET_MASK)
  // offset = ip[3:0] (4 bits)
  // index = ip[12:4] (9 bits), 512 entries
  // tag = ip[21:13] (9 bits)
  class Way
  {
  public:
    Way()
      : m_tag_offset(NUM_ENTRIES, 0)
      , m_plru(NUM_ENTRIES, 0)
    {}
    std::vector<UInt32> m_tag_offset; // tag and offset data
    std::vector<UInt64> m_plru; // Should be pseudo-LRU, using LRU instead
  };
  std::vector<Way> m_ways;
  UInt64 m_lru_use_count;

  enum instruction_queue_type {
    LOAD_QUEUE = 0,
    AGI_QUEUE,
    MAIN_QUEUE,
    QUEUE_COUNT
  };
  
  std::vector<UInt64> producers;

  void update(const IntPtr ip);
  UInt64 peekProducer(const dl::Decoder::decoder_reg reg) const;

public:
  LoadSliceTriClassifier()
    : m_ways(NUM_WAYS)
    , m_lru_use_count(0)
    , producers(Sim()->getDecoder()->last_reg(), INVALID_ADDRESS)
  {}
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void update(const DynamicMicroOp &microOp) override;
  UInt64 getNumQueues() const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
};

#endif // __LOAD_SLICE_TRI_CLASSIFIER_TABLE_H