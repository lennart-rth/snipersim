#include "load_slice_bi_adaptive_classifier.h"

#include <iostream>

LoadSliceBiAdaptiveClassifier::LoadSliceBiAdaptiveClassifier(
  const UInt64 ways, const UInt64 entries, const UInt64 l1_hit_threshold, const UInt64 adapt_interval
)
  : LoadSliceBiClassifier(ways, entries)
  , l1_hits_(0)
  , l1_hit_threshold_(l1_hit_threshold)
  , adapt_interval_(adapt_interval)
{
  LOG_ASSERT_ERROR(adapt_interval, "Adaptation interval must be greater than zero.");
  LOG_ASSERT_ERROR(l1_hit_threshold <= adapt_interval, "L1 miss threshold must be less than adaptation interval.");
}

UInt64 LoadSliceBiAdaptiveClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier())
    return instruction_queue_type::BIPASS_QUEUE;

  if(l1_hits_ > l1_hit_threshold_)
    return instruction_queue_type::MAIN_QUEUE;
  
  return LoadSliceBiClassifier::predict(microOp);
}

void LoadSliceBiAdaptiveClassifier::issued(const DynamicMicroOp &microOp){
  LoadSliceBiClassifier::issued(microOp);
  if(!microOp.getMicroOp()->isLoad())
    return;

  if(microOp.getDCacheHitWhere() == HitWhere::L1_OWN){
    l1_hits_ = std::min(l1_hits_ + 1, adapt_interval_);
  }else if (l1_hits_)
    l1_hits_--;
}

void LoadSliceBiAdaptiveClassifier::clear()
{
  LoadSliceBiClassifier::clear();
  l1_hits_ = 0;
}
