#ifndef LOAD_SLICE_BI_IDEAL_ADAPTIVE_CLASSIFIER_H
#define LOAD_SLICE_BI_IDEAL_ADAPTIVE_CLASSIFIER_H
#include "load_slice_bi_ideal_classifier.h"

#include <vector>

class LoadSliceBiIdealAdaptiveClassifier : public LoadSliceBiIdealClassifier {

  std::vector<bool> l1_hit_window_;
  UInt64 l1_hits_, l1_hit_window_front_;
  const UInt64 l1_hit_threshold_;

public:
  LoadSliceBiIdealAdaptiveClassifier(const UInt64 l1_hit_threshold, const UInt64 adapt_interval);
  UInt64 predict(const DynamicMicroOp &microOp) const override;
  void issued(const DynamicMicroOp &microOp) override;
  void clear() override;

};

#endif // LOAD_SLICE_BI_IDEAL_ADAPTIVE_CLASSIFIER_H