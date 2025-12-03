#include "load_slice_performance_model.h"
#include "config.hpp"

LoadSlicePerformanceModel::LoadSlicePerformanceModel(Core *core)
: MicroOpPerformanceModel(core, false)
, timer(core,
    Sim()->getCfg()->getIntArray("perf_model/core/interval_timer/dispatch_width", core->getId()),
    Sim()->getCfg()->getIntArray("perf_model/core/interval_timer/window_size", core->getId()))
{

}

LoadSlicePerformanceModel::~LoadSlicePerformanceModel()
{

}

boost::tuple<uint64_t,uint64_t> LoadSlicePerformanceModel::simulate(const std::vector<DynamicMicroOp*>& uops)
{
    return timer.simulate(uops);
}

void LoadSlicePerformanceModel::notifyElapsedTimeUpdate()
{

}
