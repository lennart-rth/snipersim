#ifndef INSTRUCTION_QUEUE_CLASSIFIER_H
#define INSTRUCTION_QUEUE_CLASSIFIER_H

#include "dynamic_micro_op.h"
#include "register_dependencies.h"

class InstructionQueueClassifier {
public:
  virtual UInt64 predict(const DynamicMicroOp& microOp) const = 0;
  virtual void clear() = 0;
  virtual void update(DynamicMicroOp& microOp, const RegisterDependencies&, const uint64_t lowestValidSequenceNumber) = 0;
  virtual UInt64 getNumQueues() const = 0;
  virtual void issued(const DynamicMicroOp& microOp) = 0;
  virtual ~InstructionQueueClassifier() = default;
  virtual const char * getQueueName(const UInt64 queueIdx) const = 0;
};

#endif // INSTRUCTION_QUEUE_CLASSIFIER_H