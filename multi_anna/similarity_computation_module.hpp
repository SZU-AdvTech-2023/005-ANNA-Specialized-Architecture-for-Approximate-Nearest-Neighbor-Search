#pragma once

#include <config.hpp>
#include <memory_reader.hpp>

class SimilarityComputationModule {
public:
    void ClockTick()
    {
        if (computing_remind) {
            computing_remind--;
            return;
        }
    }

    bool IsReady()
    {
        return computing_remind == 0;
    }

    void Submit(uint64_t vector_num)
    {
        computing_remind = COMPUTING_CYCLE_NUM * ((1 + (vector_num - 1) / N_scm));
        test_value.scm_computing_cycle_cnt += computing_remind;
        // computing_remind = COMPUTING_CYCLE_NUM;
    }

private:
    uint64_t computing_remind;
    static const uint64_t COMPUTING_CYCLE_NUM = SCM_CYCLES_PER_QUERY;
};
