#include "load_slice_bi_classifier.h"
#include "dynamic_micro_op.h"

void LoadSliceBiClassifier::update(const IntPtr ip)
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

void LoadSliceBiClassifier::update(const DynamicMicroOp &microOp)
{
  if(!(microOp.getMicroOp()->isStore() || microOp.getMicroOp()->isLoad() || predict(microOp)))
    return;

  update(microOp.getInstructionPointer().phys);

  for (uint32_t i = 0; i < microOp.getMicroOp()->getSourceRegistersLength(); i++)
  {
    dl::Decoder::decoder_reg sourceRegister = microOp.getMicroOp()->getSourceRegister(i);
    LOG_ASSERT_ERROR(sourceRegister < Sim()->getDecoder()->last_reg(), "Source register src[%u] = %u is invalid", i, sourceRegister);
    const UInt64 producerIP = peekProducer(sourceRegister);
    if (producerIP != INVALID_ADDRESS)
    {
      update(producerIP);
    }
  }

  // Update the producers
  for(uint32_t i = 0; i < microOp.getMicroOp()->getDestinationRegistersLength(); i++)
  {
    const uint32_t destinationRegister = microOp.getMicroOp()->getDestinationRegister(i);
    LOG_ASSERT_ERROR(destinationRegister < Sim()->getDecoder()->last_reg(), "Destination register dst[%u] = %u is invalid", i, destinationRegister);
    producers[destinationRegister] = microOp.getInstructionPointer().phys;
  }
}

UInt64 LoadSliceBiClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad())
    return instruction_queue_type::BIPASS_QUEUE;
    return instruction_queue_type::MAIN_QUEUE;
  if (microOp.getMicroOp()->isStore())
    return instruction_queue_type::MAIN_QUEUE;
  
  const IntPtr ip = microOp.getInstructionPointer().phys;
  const UInt32 index = IP_TO_INDEX(ip), tag_offset = IP_TO_TAGOFF(ip);
  for (unsigned int i = 0 ; i < NUM_WAYS ; ++i )
  {
    if (m_ways[i].m_tag_offset[index] == tag_offset)
    {
      return instruction_queue_type::BIPASS_QUEUE;
    }
  }
  return instruction_queue_type::MAIN_QUEUE;
}

void LoadSliceBiClassifier::clear()
{
  std::fill(producers.begin(), producers.end(), INVALID_ADDRESS);
}

UInt64 LoadSliceBiClassifier::peekProducer(const dl::Decoder::decoder_reg reg) const {
  if (reg == dl::Decoder::DL_REG_INVALID)
    return INVALID_ADDRESS;

  return producers[reg];
}


UInt64 LoadSliceBiClassifier::getNumQueues() const {
  return 2ull;
}