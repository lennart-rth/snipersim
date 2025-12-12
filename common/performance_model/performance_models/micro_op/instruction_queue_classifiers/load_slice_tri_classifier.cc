#include "load_slice_tri_classifier.h"

#include "log.h"
#include "dynamic_micro_op.h"

UInt64 LoadSliceTriClassifier::ip_to_idx(const IntPtr ip) const {
  return ip % m_entries;
}

UInt32 LoadSliceTriClassifier::ip_to_tagoff(const IntPtr ip) const {
  return ip / m_entries;
}

LoadSliceTriClassifier::Way::Way(const UInt64 entries)
  : m_tag_offset(entries, 0)
  , m_plru(entries, 0)
{}

LoadSliceTriClassifier::LoadSliceTriClassifier(const UInt64 ways, const UInt64 entries)
  : m_ways(ways, entries)
  , m_lru_use_count(0)
  , producers(Sim()->getDecoder()->last_reg(), INVALID_ADDRESS)
  , m_entries(entries)
{
  LOG_ASSERT_ERROR(entries, "Number of entries must be greater than zero.");
  LOG_ASSERT_ERROR(ways, "Number of ways must be greater than zero.");
}

void LoadSliceTriClassifier::update(const IntPtr ip)
{
  UInt32 lru_way = 0;

  const UInt32 tag_offset = ip_to_tagoff(ip), index = ip_to_idx(ip);
  for (unsigned int w = 0 ; w < m_ways.size() ; ++w )
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

void LoadSliceTriClassifier::update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber)
{
  const UInt64 instruction_queue_type = microOp.getInstructionQueueType();
  LOG_ASSERT_ERROR(instruction_queue_type < instruction_queue_type::QUEUE_COUNT, "MicroOp has invalid instruction queue type");

  if(microOp.getMicroOp()->isStore()){
    pending_deps.clear();
    for(unsigned int i = 0; i < microOp.getMicroOp()->getAddressRegistersLength(); ++i)
    {
      dl::Decoder::decoder_reg reg = microOp.getMicroOp()->getAddressRegister(i);
      LOG_ASSERT_ERROR(reg < Sim()->getDecoder()->last_reg(), "address register src[%u] = %u is invalid", i, reg);
      if(
        const UInt64 addressProducer = peekProducer(microOp.getMicroOp()->getAddressRegister(i));
        addressProducer != INVALID_ADDRESS
      )
        update(addressProducer);
      if(
        const UInt64 addressProducer = reg_dep.peekProducer(microOp.getMicroOp()->getAddressRegister(i), lowestValidSequenceNumber);
        addressProducer != INVALID_SEQNR
      )
        pending_deps.push_back(addressProducer);
    }

    return;
  }

  if(microOp.getMicroOp()->isLoad()){

    for(const UInt64 dep : pending_deps)
      if(dep > lowestValidSequenceNumber)
        microOp.addDependency(dep);
    
    pending_deps.clear();

    for(unsigned int i = 0; i < microOp.getMicroOp()->getAddressRegistersLength(); ++i)
    {
      dl::Decoder::decoder_reg reg = microOp.getMicroOp()->getAddressRegister(i);
      LOG_ASSERT_ERROR(reg < Sim()->getDecoder()->last_reg(), "address register src[%u] = %u is invalid", i, reg);
      const UInt64 addressProducer = peekProducer(microOp.getMicroOp()->getAddressRegister(i));
      if(addressProducer != INVALID_ADDRESS)
        update(addressProducer);
    }

    return;
  }

  // Update the producers
  for(uint32_t i = 0; i < microOp.getMicroOp()->getDestinationRegistersLength(); i++)
  {
    const uint32_t destinationRegister = microOp.getMicroOp()->getDestinationRegister(i);
    LOG_ASSERT_ERROR(destinationRegister < Sim()->getDecoder()->last_reg(), "Destination register dst[%u] = %u is invalid", i, destinationRegister);
    producers[destinationRegister] = microOp.getInstructionPointer().phys;
  }

  if(instruction_queue_type == instruction_queue_type::MAIN_QUEUE || microOp.getMicroOp()->isMemBarrier())
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
}

UInt64 LoadSliceTriClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier())
    return instruction_queue_type::LOAD_QUEUE;
  
  const IntPtr ip = microOp.getInstructionPointer().phys;
  const UInt32 tag_offset = ip_to_tagoff(ip);
  const UInt64 index = ip_to_idx(ip);
  for (unsigned int i = 0 ; i < m_ways.size() ; ++i )
  {
    if (m_ways[i].m_tag_offset[index] == tag_offset)
    {
      return instruction_queue_type::AGI_QUEUE;
    }
  }
  return instruction_queue_type::MAIN_QUEUE;
}

void LoadSliceTriClassifier::clear()
{
  std::fill(producers.begin(), producers.end(), INVALID_ADDRESS);
  pending_deps.clear();
}

UInt64 LoadSliceTriClassifier::peekProducer(const dl::Decoder::decoder_reg reg) const {
  if (reg == dl::Decoder::DL_REG_INVALID)
    return INVALID_ADDRESS;

  return producers[reg];
}


UInt64 LoadSliceTriClassifier::getNumQueues() const {
  return instruction_queue_type::QUEUE_COUNT;
}

void LoadSliceTriClassifier::issued(const DynamicMicroOp &microOp) {
  // No action needed on issue for this classifier
}