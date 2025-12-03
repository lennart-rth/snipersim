#ifndef LOAD_SLICE_PERFORMANCE_MODEL_H
#define LOAD_SLICE_PERFORMANCE_MODEL_H

#include "micro_op_performance_model.h"
#include "load_slice_timer.h"

class LoadSlicePerformanceModel : public MicroOpPerformanceModel {
    public:
        LoadSlicePerformanceModel(Core *core);
        ~LoadSlicePerformanceModel();
    protected:
        boost::tuple<uint64_t,uint64_t> simulate(const std::vector<DynamicMicroOp*>& uops);
    private:
        LoadSliceTimer timer;
};

#endif
