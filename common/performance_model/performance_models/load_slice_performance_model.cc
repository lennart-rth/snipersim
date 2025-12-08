#include "load_slice_performance_model.h"
#include "config.hpp"

LoadSlicePerformanceModel::LoadSlicePerformanceModel(Core *core)
: MicroOpPerformanceModel(core, false)
, timer(core,
       this,
       m_core_model,
       Sim()->getCfg()->getIntArray("perf_model/branch_predictor/mispredict_penalty", core->getId()),
       Sim()->getCfg()->getIntArray("perf_model/core/interval_timer/dispatch_width", core->getId()),
       Sim()->getCfg()->getIntArray("perf_model/core/interval_timer/window_size", core->getId())
)
{

}

LoadSlicePerformanceModel::~LoadSlicePerformanceModel()
{

}

boost::tuple<uint64_t,uint64_t> LoadSlicePerformanceModel::simulate(const std::vector<DynamicMicroOp*>& uops)
{
   uint64_t ins; SubsecondTime latency;
   boost::tie(ins, latency) = timer.simulate(uops);

   return boost::tuple<uint64_t,uint64_t>(ins, SubsecondTime::divideRounded(latency, m_elapsed_time.getPeriod()));
}


void LoadSlicePerformanceModel::notifyElapsedTimeUpdate()
{
   timer.synchronize(m_elapsed_time.getElapsedTime());
}
