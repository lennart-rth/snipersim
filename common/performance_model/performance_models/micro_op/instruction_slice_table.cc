#include "instruction_slice_table.h"
#include "dynamic_micro_op.h"

void InstructionSliceTable::update(const IntPtr ip)
{
  UInt32 lru_way = 0;

  const UInt32 tag_offset = IP_TO_TAGOFF(ip), index = IP_TO_INDEX(ip);
  for (unsigned int w = 0 ; w < NUM_WAYS ; ++w )
  {
    if (m_ways[w].m_tag_offset[index] == tag_offset)
    {
      m_ways[w].m_plru[index] = m_lru_use_count++;
      return;
    }

    if (m_ways[w].m_plru[index] < m_ways[lru_way].m_plru[index])
    {
      lru_way = w;
    }
  }

  m_ways[lru_way].m_tag_offset[index] = tag_offset;
  m_ways[lru_way].m_plru[index] = m_lru_use_count++;
}

void InstructionSliceTable::update(const DynamicMicroOp &microOp, const RegisterDependencyTable &regDepTable)
{
  if(!(microOp.getMicroOp()->isStore() || predict(microOp)))
    return;

  update(microOp.getIP());

  for (uint32_t i = 0; i < microOp.getMicroOp()->getSourceRegistersLength(); i++)
  {
    dl::Decoder::decoder_reg sourceRegister = microOp.getMicroOp()->getSourceRegister(i);
    LOG_ASSERT_ERROR(sourceRegister < Sim()->getDecoder()->last_reg(), "Source register src[%u] = %u is invalid", i, sourceRegister);
    const UInt64 producerIP = regDepTable.peekProducer(sourceRegister);
    if (producerIP != INVALID_ADDRESS)
    {
      update(producerIP);
    }
  }
}

bool InstructionSliceTable::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad())
    return true;
  
  if (microOp.getMicroOp()->isStore())
    return false;
  
  const IntPtr ip = microOp.getIP();
  const UInt32 index = IP_TO_INDEX(ip), tag_offset = IP_TO_TAGOFF(ip);
  for (unsigned int i = 0 ; i < NUM_WAYS ; ++i )
  {
    if (m_ways[i].m_tag_offset[index] == tag_offset)
    {
      return true;
    }
  }
  return false;
}