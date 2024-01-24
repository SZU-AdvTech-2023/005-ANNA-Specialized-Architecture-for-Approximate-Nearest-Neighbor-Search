#pragma once

#include <config.hpp>
#include <multi_memory_interface.hpp>

class MemoryReader {
public:
    MemoryReader(MultiMemoryInterface* const memory_interface, uint64_t core_id)
        : memory_interface(memory_interface)
        , id(memory_interface->RegisterId(core_id))
        , processing_num(0)
        , finished(1)
    {
    }
    void SubmitReadRequest(uint64_t addr, uint64_t len, uint64_t stride = 64)
    {
        for (uint64_t i = 0; i < len; i += stride) {
            request_queue.push(addr + i);
            processing_num++;
        }
        finished = 0;
    }
    void ClockTick()
    {
        uint64_t cnt = 0;
        while (not request_queue.empty()) {
            memory_interface->RequestRead(id, request_queue.front());
            request_queue.pop();
            cnt++;
            if (cnt > 16)
                break;
        }
        uint64_t res;
        res = memory_interface->GetAllReadyReadRequest(id);
        processing_num -= res;
    }

    bool IsReady()
    {
        return processing_num == 0;
    }

    bool FinishAndReset()
    {
        if (processing_num == 0 and not finished) {
            finished = 1;
            return true;
        }
        return false;
    }

private:
    MultiMemoryInterface* const memory_interface;
    const uint64_t id;
    std::queue<uint64_t> request_queue;
    uint64_t processing_num;
    bool finished;
};
