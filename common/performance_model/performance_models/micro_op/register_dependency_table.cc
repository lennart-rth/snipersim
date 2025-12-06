#include "register_dependency_table.h"
#include "dynamic_micro_op.h"

RegisterDependencyTable::RegisterDependencyTable():
  producers(Sim()->getDecoder()->last_reg())
{
  clear();
}

void RegisterDependencyTable::clear()
{
  std::fill(producers.begin(), producers.end(), INVALID_ADDRESS);
}

void RegisterDependencyTable::setDependency(const DynamicMicroOp& microOp){
  // Update the producers
  for(uint32_t i = 0; i < microOp.getMicroOp()->getDestinationRegistersLength(); i++)
  {
    const uint32_t destinationRegister = microOp.getMicroOp()->getDestinationRegister(i);
    LOG_ASSERT_ERROR(destinationRegister < Sim()->getDecoder()->last_reg(), "Destination register dst[%u] = %u is invalid", i, destinationRegister);
    producers[destinationRegister] = microOp.getIP();
  }
}

UInt64 RegisterDependencyTable::peekProducer(const dl::Decoder::decoder_reg reg) const {
  if (reg == dl::Decoder::DL_REG_INVALID)
    return INVALID_ADDRESS;

  return producers[reg];
}