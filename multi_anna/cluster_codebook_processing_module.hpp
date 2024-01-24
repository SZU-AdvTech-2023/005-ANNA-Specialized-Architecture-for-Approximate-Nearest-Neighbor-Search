#pragma once

#include <config.hpp>
#include <memory_reader.hpp>

class ClusterCodebookProcessingModule {
public:
    ClusterCodebookProcessingModule(MultiMemoryInterface* const memory_interface, uint64_t core_id)
        : memory_reader(memory_interface, core_id)
        , computing_remind(0)
    {
    }
    void ClockTick()
    {
        memory_reader.ClockTick();
        if (computing_remind <= COMPUTING_CYCLE_NUM and computing_remind > 0) {
            computing_remind--;
            return;
        }
        memory_reader.FinishAndReset();
        if (memory_reader.IsReady()) {
            if (computing_remind == COMPUTING_INVALID_NUM) {
                computing_remind = COMPUTING_CYCLE_NUM;
                test_value.cpm_computing_cycle_cnt += COMPUTING_CYCLE_NUM;
            } else if (!wait_addr.empty()) {
                for (uint64_t i = 0; i < N_cu; i++) {
                    if (wait_addr.empty()) {
                        break;
                    }
                    memory_reader.SubmitReadRequest(wait_addr.front(), VECTOR_SIZE);
                    wait_addr.pop();
                }
                computing_remind = COMPUTING_INVALID_NUM;
            }
        }
    }

    bool IsReadySubmit()
    {
        return wait_addr.empty();
    }
    bool IsFinished()
    {
        return wait_addr.empty() and computing_remind == 0;
    }
    void SubmitClusterRequest(std::vector<uint64_t> centroid_list)
    {
        for (auto addr : centroid_list) {
            wait_addr.push(cluster_centroid[addr]);
        }
    }

private:
    MemoryReader memory_reader;
    uint64_t computing_remind;
    std::queue<uint64_t> wait_addr;
    static const uint64_t COMPUTING_CYCLE_NUM = (N_scm * K_STAR * D + N_cu - 1) / N_cu;
    static const uint64_t COMPUTING_INVALID_NUM = -1;
};
