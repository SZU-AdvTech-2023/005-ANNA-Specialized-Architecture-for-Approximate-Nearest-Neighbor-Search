#pragma once

#include <config.hpp>
#include <memory_reader.hpp>

class EncodedVectorFetchModule {

public:
    EncodedVectorFetchModule(MultiMemoryInterface* const memory_interface, uint64_t core_id)
        : metadata_reader(memory_interface, core_id)
        , vector_reader(memory_interface, core_id)
        , submited_request_num(0)
        , fetched_request_num(0)
        , fetching_request_num(0)
    {
    }
    void ClockTick()
    {
        metadata_reader.ClockTick();
        vector_reader.ClockTick();
        if (metadata_reader.FinishAndReset()) {
            fetching_request_num++;
        }
        if (not metadata_reader.IsReady() or not vector_reader.IsReady()) {
            test_value.efm_blocked_cycle_cnt++;
        }
        if (fetching_request_num == submited_request_num - 1 and metadata_reader.IsReady()) {
            for (uint64_t cluster : metadata_queue) {
                metadata_reader.SubmitReadRequest(metadata_addr[cluster].first, metadata_addr[cluster].second);
                test_value.efm_memory_reader_read += metadata_addr[cluster].second;
            }
            if (!cluster_queue.empty()) {
                for (uint64_t cluster : cluster_queue) {
                    vector_queue.push_back(cluster);
                }
                cluster_queue.clear();
            }
            for (uint64_t cluster : metadata_queue) {
                cluster_queue.push_back(cluster);
            }
            metadata_queue.clear();
        }
        if (vector_reader.FinishAndReset()) {
            fetched_request_num++;
        }
        if (fetched_request_num == fetching_request_num - 1 and vector_reader.IsReady()) {
            for (auto v : vector_queue) {
                vector_reader.SubmitReadRequest(cluster_addr[v].start_addr, cluster_addr[v].bytes);
                test_value.efm_memory_reader_read += cluster_addr[v].bytes;
                test_value.efm_encoded_vector_buffer_write += cluster_addr[v].bytes;
            }
            vector_queue.clear();
        }
    }

    uint64_t SubmitRequest(std::vector<uint64_t> clusters)
    {
        assert(metadata_queue.empty());
        for (uint64_t cluster : clusters) {
            metadata_queue.push_back(cluster);
        }
        submited_request_num++;
        return submited_request_num - 1;
    }

    bool IsReadySubmit()
    {
        return metadata_queue.empty();
    }

    uint64_t GetFetchedRequestNum() const
    {
        return fetched_request_num;
    }

private:
    MemoryReader metadata_reader;
    MemoryReader vector_reader;
    std::vector<uint64_t> metadata_queue; // to read for metadata
    std::vector<uint64_t> vector_queue;   // to read for vector data
    std::vector<uint64_t> cluster_queue;  // reading for metadata so that now cannot read for vector data

    uint64_t fetched_request_num;
    uint64_t submited_request_num;
    uint64_t fetching_request_num;
};
