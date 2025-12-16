#ifndef LOAD_SLICE_BI_ADAPTIVE_CLASSIFIER_H
#define LOAD_SLICE_BI_ADAPTIVE_CLASSIFIER_H
#include "load_slice_bi_classifier.h"

class LoadSliceBiAdaptiveClassifier : public LoadSliceBiClassifier {

  UInt64 l1_hits_;
  const UInt64 l1_hit_threshold_, adapt_interval_;

public:

  LoadSliceBiAdaptiveClassifier(
    const UInt64 ways, const UInt64 entries, const UInt64 l1_hit_threshold, const UInt64 adapt_interval
  );
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;
};

#endif // LOAD_SLICE_BI_ADAPTIVE_CLASSIFIER_H