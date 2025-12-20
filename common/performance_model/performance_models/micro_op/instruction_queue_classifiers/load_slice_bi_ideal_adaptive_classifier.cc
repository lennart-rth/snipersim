#include "load_slice_bi_ideal_adaptive_classifier.h"

LoadSliceBiIdealAdaptiveClassifier::LoadSliceBiIdealAdaptiveClassifier(
  const UInt64 l1_hit_threshold, const UInt64 adapt_interval
)
  : l1_hit_window_(adapt_interval) 
  , l1_hits_(0)
  , l1_hit_window_front_(0)
  , l1_hit_threshold_(l1_hit_threshold)
{
  LOG_ASSERT_ERROR(adapt_interval, "Adaptation interval must be greater than zero.");
  LOG_ASSERT_ERROR(l1_hit_threshold < adapt_interval, "L1 miss threshold must be less than adaptation interval.");
}

UInt64 LoadSliceBiIdealAdaptiveClassifier::predict(const DynamicMicroOp &microOp) const
{
  if (microOp.getMicroOp()->isLoad() || microOp.getMicroOp()->isMemBarrier())
    return instruction_queue_type::BIPASS_QUEUE;

  if(l1_hits_ > l1_hit_threshold_)
    return instruction_queue_type::MAIN_QUEUE;
  
  return LoadSliceBiIdealClassifier::predict(microOp);
}

void LoadSliceBiIdealAdaptiveClassifier::issued(const DynamicMicroOp &microOp){
  LoadSliceBiIdealClassifier::issued(microOp);
  if(!microOp.getMicroOp()->isLoad())
    return;

  if(l1_hit_window_[l1_hit_window_front_])
    l1_hits_--;

  if(microOp.getDCacheHitWhere() == HitWhere::L1_OWN){
    l1_hit_window_[l1_hit_window_front_] = true;
    l1_hits_++;
  }
  else
    l1_hit_window_[l1_hit_window_front_] = false;

  l1_hit_window_front_ = (l1_hit_window_front_ + 1) % l1_hit_window_.size();
}

void LoadSliceBiIdealAdaptiveClassifier::clear()
{
  LoadSliceBiIdealClassifier::clear();
  l1_hits_ = 0;
  std::fill(l1_hit_window_.begin(), l1_hit_window_.end(), false);
}
