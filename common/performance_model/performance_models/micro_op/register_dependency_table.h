#ifndef __REGISTER_DEPENDENCY_TABLE_H
#define __REGISTER_DEPENDENCY_TABLE_H

#include <vector>

#include <decoder.h>
#include <fixed_types.h>

class DynamicMicroOp;

class RegisterDependencyTable {
private:
  std::vector<UInt64> producers;
public:
  RegisterDependencyTable();
  void clear();
  void setDependency(const DynamicMicroOp& microOp);
  UInt64 peekProducer(const dl::Decoder::decoder_reg reg) const;
};

#endif /* __REGISTER_DEPENDENCY_TABLE_H */