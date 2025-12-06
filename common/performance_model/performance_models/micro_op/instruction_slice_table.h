#ifndef __INSTRUCTION_SLICE_TABLE_H
#define __INSTRUCTION_SLICE_TABLE_H

#include "register_dependency_table.h"

#define NUM_WAYS 2
#define NUM_ENTRIES 128

class InstructionSliceTable
{
  // index = ip[6:0] (7 bits) for 128 entries
  #define IP_TO_INDEX(_ip) ((_ip) % NUM_ENTRIES)
  // tag = ip[31:7] (25 bits) for 128 entries
  #define IP_TO_TAG(_ip) ((_ip) / NUM_ENTRIES)
  
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