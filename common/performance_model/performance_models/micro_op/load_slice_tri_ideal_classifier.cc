#include "load_slice_tri_ideal_classifier.h"

#include "log.h"
#include "dynamic_micro_op.h"

void LoadSliceTriIdealClassifier::update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber)
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
        agis.insert(addressProducer);
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
        agis.insert(addressProducer);
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

  for (uint32_t i = 0; i < microOp.getMicroOp()->getSourceRegistersLength(); i++)
  {
    dl::Decoder::decoder_reg sourceRegister = microOp.getMicroOp()->getSourceRegister(i);
    LOG_ASSERT_ERROR(sourceRegister < Sim()->getDecoder()->last_reg(), "Source register src[%u] = %u is invalid", i, sourceRegister);
    const UInt64 producerIP = peekProducer(sourceRegister);
    if (producerIP != INVALID_ADDRESS)
    {
      agis.insert(producerIP);
    }
  }
}

UInt64 LoadSliceTriIdealClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier())
    return instruction_queue_type::LOAD_QUEUE;
    
  if (microOp.getMicroOp()->isStore())
    return instruction_queue_type::MAIN_QUEUE;

  if (agis.count(microOp.getInstructionPointer().phys))
    return instruction_queue_type::AGI_QUEUE;

  return instruction_queue_type::MAIN_QUEUE;
}

void LoadSliceTriIdealClassifier::clear()
{
  std::fill(producers.begin(), producers.end(), INVALID_ADDRESS);
  pending_deps.clear();
}

UInt64 LoadSliceTriIdealClassifier::peekProducer(const dl::Decoder::decoder_reg reg) const {
  if (reg == dl::Decoder::DL_REG_INVALID)
    return INVALID_ADDRESS;

  return producers[reg];
}


UInt64 LoadSliceTriIdealClassifier::getNumQueues() const {
  return instruction_queue_type::QUEUE_COUNT;
}

void LoadSliceTriIdealClassifier::issued(const DynamicMicroOp &microOp) {
  // No action needed on issue for this classifier
}