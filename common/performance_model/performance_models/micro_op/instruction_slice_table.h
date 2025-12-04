#ifndef __INSTRUCTION_SLICE_TABLE_H
#define __INSTRUCTION_SLICE_TABLE_H

#include "register_dependency_table.h"

#define NUM_WAYS 4
#define NUM_ENTRIES 512

class InstructionSliceTable
{
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

  void update(const IntPtr ip);
public:
  InstructionSliceTable()
    : m_ways(NUM_WAYS)
    , m_lru_use_count(0)
  {}
  bool predict(const DynamicMicroOp &microOp) const;
  void update(const DynamicMicroOp &microOp, const RegisterDependencyTable &regDepTable);
};

#endif // __INSTRUCTION_SLICE_TABLE_H