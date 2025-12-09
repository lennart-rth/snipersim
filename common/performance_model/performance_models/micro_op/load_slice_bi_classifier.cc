#include "load_slice_bi_classifier.h"

#include "log.h"
#include "dynamic_micro_op.h"


UInt64 LoadSliceBiClassifier::ip_to_idx(const IntPtr ip) {
  return ip % NUM_ENTRIES;
}

UInt32 LoadSliceBiClassifier::ip_to_tagoff(const IntPtr ip) {
  return ip / NUM_ENTRIES;
}

LoadSliceBiClassifier::Way::Way()
  : m_tag_offset(NUM_ENTRIES, 0)
  , m_plru(NUM_ENTRIES, 0)
{}

LoadSliceBiClassifier::LoadSliceBiClassifier()
  : m_ways(NUM_WAYS)
  , m_lru_use_count(0)
  , producers(Sim()->getDecoder()->last_reg(), INVALID_ADDRESS)
{}

void LoadSliceBiClassifier::update(const IntPtr ip)
{
  UInt32 lru_way = 0;

  const UInt32 tag_offset = ip_to_tagoff(ip), index = ip_to_idx(ip);
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
  const UInt64 instruction_queue_type = microOp.getInstructionQueueType();
  LOG_ASSERT_ERROR(instruction_queue_type < instruction_queue_type::QUEUE_COUNT, "MicroOp has invalid instruction queue type");

  if(microOp.getMicroOp()->isStore()){
    for(unsigned int i = 0; i < microOp.getMicroOp()->getAddressRegistersLength(); ++i)
    {
      dl::Decoder::decoder_reg reg = microOp.getMicroOp()->getAddressRegister(i);
      LOG_ASSERT_ERROR(reg < Sim()->getDecoder()->last_reg(), "Address register src[%u] = %u is invalid", i, reg);
      uint64_t addressProducer = peekProducer(microOp.getMicroOp()->getAddressRegister(i));
      if(addressProducer != INVALID_ADDRESS)
      {
        update(addressProducer);
      }
    }
    return;
  }

  if(instruction_queue_type == instruction_queue_type::MAIN_QUEUE)
    return;

  if(!microOp.getMicroOp()->isLoad())
    update(microOp.getInstructionPointer().phys);

  for (uint32_t i = 0; i < microOp.getMicroOp()->getSourceRegistersLength(); i++)
  {
    const dl::Decoder::decoder_reg sourceRegister = microOp.getMicroOp()->getSourceRegister(i);
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
    const dl::Decoder::decoder_reg destinationRegister = microOp.getMicroOp()->getDestinationRegister(i);
    LOG_ASSERT_ERROR(destinationRegister < Sim()->getDecoder()->last_reg(), "Destination register dst[%u] = %u is invalid", i, destinationRegister);
    producers[destinationRegister] = microOp.getInstructionPointer().phys;
  }
}

UInt64 LoadSliceBiClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier())
    return instruction_queue_type::BIPASS_QUEUE;
  
  const IntPtr ip = microOp.getInstructionPointer().phys;
  const UInt32 index = ip_to_idx(ip), tag_offset = ip_to_tagoff(ip);
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
  return instruction_queue_type::QUEUE_COUNT;
}

void LoadSliceBiClassifier::issued(const DynamicMicroOp &microOp) {
  // No action needed on issue for this classifier
}