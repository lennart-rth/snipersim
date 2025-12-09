#include "load_slice_bi_ideal_classifier.h"

#include "log.h"
#include "dynamic_micro_op.h"

LoadSliceBiIdealClassifier::LoadSliceBiIdealClassifier()
  : producers(Sim()->getDecoder()->last_reg(), INVALID_ADDRESS)
{}

void LoadSliceBiIdealClassifier::update(const DynamicMicroOp &microOp)
{
  const UInt64 instruction_queue_type = microOp.getInstructionQueueType();
  LOG_ASSERT_ERROR(instruction_queue_type < instruction_queue_type::QUEUE_COUNT, "MicroOp has invalid instruction queue type");

  if(microOp.getMicroOp()->isStore() || microOp.getMicroOp()->isLoad()){
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
      agis.insert(producerIP);
  }
}

UInt64 LoadSliceBiIdealClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier() || agis.count(microOp.getInstructionPointer().phys))
    return instruction_queue_type::BIPASS_QUEUE;
  
  return instruction_queue_type::MAIN_QUEUE;
}

void LoadSliceBiIdealClassifier::clear()
{
  std::fill(producers.begin(), producers.end(), INVALID_ADDRESS);
}

UInt64 LoadSliceBiIdealClassifier::peekProducer(const dl::Decoder::decoder_reg reg) const {
  if (reg == dl::Decoder::DL_REG_INVALID)
    return INVALID_ADDRESS;

  return producers[reg];
}

UInt64 LoadSliceBiIdealClassifier::getNumQueues() const {
  return instruction_queue_type::QUEUE_COUNT;
}

void LoadSliceBiIdealClassifier::issued(const DynamicMicroOp &microOp) {
  // No action needed on issue for this classifier
}