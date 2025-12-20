#include "load_slice_only_load_classifier.h"

#include "log.h"
#include "dynamic_micro_op.h"

std::array<const char *, LoadSliceOnlyLoadClassifier::QUEUE_COUNT> const LoadSliceOnlyLoadClassifier::queueNames = {
  "LOAD_QUEUE", "MAIN_QUEUE"
};

void LoadSliceOnlyLoadClassifier::update(DynamicMicroOp &microOp, const RegisterDependencies& reg_dep, const uint64_t lowestValidSequenceNumber)
{
  const UInt64 instruction_queue_type = microOp.getInstructionQueueType();
  LOG_ASSERT_ERROR(instruction_queue_type < instruction_queue_type::QUEUE_COUNT, "MicroOp has invalid instruction queue type");

  if(microOp.getMicroOp()->isStore()){
    pending_deps.clear();
    for(unsigned int i = 0; i < microOp.getMicroOp()->getAddressRegistersLength(); ++i)
    {
      if(
        const UInt64 addressProducer = reg_dep.peekProducer(microOp.getMicroOp()->getAddressRegister(i), lowestValidSequenceNumber);
        addressProducer != INVALID_SEQNR
      )
        pending_deps.push_back(addressProducer);
    }

    return;
  }

  if(instruction_queue_type == instruction_queue_type::LOAD_QUEUE){
    for(const UInt64 dep : pending_deps)
      if(dep > lowestValidSequenceNumber)
        microOp.addDependency(dep);
    
    pending_deps.clear();
  }
  return;
}

UInt64 LoadSliceOnlyLoadClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier())
    return instruction_queue_type::LOAD_QUEUE;
  return instruction_queue_type::MAIN_QUEUE;
}

void LoadSliceOnlyLoadClassifier::clear()
{
  pending_deps.clear();
}

UInt64 LoadSliceOnlyLoadClassifier::getNumQueues() const {
  return instruction_queue_type::QUEUE_COUNT;
}

void LoadSliceOnlyLoadClassifier::issued(const DynamicMicroOp &microOp) {
  // No action needed on issue for this classifier
}

const char * LoadSliceOnlyLoadClassifier::getQueueName(const UInt64 queueIdx) const {
  LOG_ASSERT_ERROR(queueIdx < QUEUE_COUNT, "Invalid queue index: %lu", queueIdx);
  return queueNames[queueIdx];
}