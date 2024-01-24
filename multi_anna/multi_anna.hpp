#include <cluster_codebook_processing_module.hpp>
#include <encoded_vector_fetch_module.hpp>
#include <multi_memory_interface.hpp>
#include <query.hpp>
#include <similarity_computation_module.hpp>
#include <sys/stat.h>
#include <utils.hpp>

class ANNACore {
public:
    ANNACore(MultiMemoryInterface* memory, uint64_t core_id)
        : memory(memory)
        , cpm(memory, core_id)
        , efm(memory, core_id)
        , scm()
        , core_id(core_id)
        , clock(0)
    {
    }
    void ClockTick()
    {
        clock++;
        cpm.ClockTick();
        efm.ClockTick();
        scm.ClockTick();

        // save
        if (scm_processing_index != -1 and scm.IsReady()) {
            for (uint64_t i = saving_queries_l; i < saving_queries_r; i++) {
                test_value.queries_end_clock[query_set_list[scm_processing_index][i]] = clock;
            }

            if (scm_processing_index == cluster_list.size() - 1 and saving_queries_r == query_set_list[scm_processing_index].size()) {
                Finished = 1;
            }
            scm_processing_index = -1;
        }

        // scm process
        if (efm_processing_index != -1 /* and saving_queries_l == -1 */ and scm_processing_index == -1 and efm.GetFetchedRequestNum() == efm_fetching_request_num and scm.IsReady()) {
            scm_processing_index = efm_processing_index;
            scm.Submit(cluster_addr[scm_processing_index].vector_num * query_set_list[scm_processing_index].size());
            test_value.efm_encoded_vector_buffer_read += cluster_addr[scm_processing_index].bytes * query_set_list[scm_processing_index].size();
            saving_queries_l = process_queries_l;
            saving_queries_r = process_queries_r;
            efm_processing_index = -1;
            // process_queries_l = -1;
            if (process_queries_r == query_set_list.size()) {
                efm_processing_index = -1;
            }
            if (debug)
                std::cout << "core: " << core_id << " scm query id: " << scm_processing_index << std::endl;
        }

        // pre fetch
        if (cpm_processing_index != -1 /* and process_queries_l == -1 */ and efm_processing_index == -1 and cpm.IsFinished() and efm.IsReadySubmit()) {
            efm_processing_index = cpm_processing_index;
            if (efm_processing_index != pre_efm_processing_index) {
                efm_fetching_request_num = efm.SubmitRequest({ cluster_list[efm_processing_index] });
                if (debug)
                    std::cout << "core: " << core_id << " fetching id: " << efm_processing_index << std::endl;
            }
            pre_efm_processing_index = efm_processing_index;
            process_queries_l = next_queries_l;
            process_queries_r = next_queries_r;
            cpm_processing_index = -1;
            // next_queries_l = -1;
            if (debug)
                std::cout << "core: " << core_id << " efm query id: " << efm_processing_index << std::endl;
        }


        if (cpm_processing_index == -1 and cpm.IsReadySubmit() and slice < cluster_list.size()) {
            cpm_processing_index = slice;

            if (slice_changed) {
                next_queries_l = 0;
                slice_queries = 0;
                slice_changed = 0;
            } else {
                slice_queries += batch_size;
                next_queries_l = slice_queries;
            }
            next_queries_r = std::min(next_queries_l + batch_size, query_set_list[cpm_processing_index].size());
            for (uint64_t i = next_queries_l; i < next_queries_r; i++)
                if (test_value.queries_start_clock[query_set_list[cpm_processing_index][i]] == -1)
                    test_value.queries_start_clock[query_set_list[cpm_processing_index][i]] = clock;
            if (next_queries_r == query_set_list[cpm_processing_index].size()) {
                slice_changed = 1;
                slice++;
            }
            if (debug)
                std::cout << "core: " << core_id << " cpm query id: " << cpm_processing_index << " l: " << next_queries_l << std::endl;

            cpm.SubmitClusterRequest({ cluster_list[cpm_processing_index] });
        }
    }

    void Setup(std::vector<uint64_t>&& cluster_list,
        std::vector<std::vector<uint64_t>>&& query_set_list,
        int batch_size)
    {
        Finished = 0;
        this->batch_size = batch_size;
        this->cluster_list = std::move(cluster_list);
        this->query_set_list = std::move(query_set_list);

        slice = 0;
        slice_changed = 0;

        // init process state
        cpm_processing_index = -1;
        efm_processing_index = -1;
        scm_processing_index = -1;

        fetched_cluster = -1;
        next_fetch_cluster = -1;

        next_queries_l = -1;
        next_queries_r = -1;
        process_queries_l = -1;
        process_queries_r = -1;
        saving_queries_l = -1;
        saving_queries_r = -1;

        slice_changed = 1;
        pre_efm_processing_index = -1;
    }

    bool IsFinished()
    {
        return Finished;
        return (scm_processing_index == cluster_list.size() - 1 and saving_queries_r == query_set_list[scm_processing_index].size());
    }

private:
    MultiMemoryInterface* memory;

    ClusterCodebookProcessingModule cpm;
    EncodedVectorFetchModule efm;
    SimilarityComputationModule scm;

    uint64_t core_id;

    uint64_t cpm_processing_index;
    uint64_t efm_processing_index;
    uint64_t scm_processing_index;
    uint64_t efm_fetching_request_num;

    std::vector<Query> queries;

    uint64_t slice;
    uint64_t slice_changed;
    uint64_t batch_size;

    // each fetch cluster index and its queries' index
    std::vector<uint64_t> cluster_list;
    std::vector<std::vector<uint64_t>> query_set_list;

    // multi batch processing state
    uint64_t fetched_cluster;
    uint64_t next_fetch_cluster;

    uint64_t next_queries_l;
    uint64_t next_queries_r;
    uint64_t process_queries_l;
    uint64_t process_queries_r;
    uint64_t saving_queries_l;
    uint64_t saving_queries_r;
    uint64_t slice_queries;
    uint64_t pre_efm_processing_index;

    bool Finished;

    std::set<uint64_t> complete_work;

    uint64_t clock;
};

class MultiANNA {
public:
    MultiANNA(int core_num)
        : memory("configs/HBM2_8Gb_x128.ini", output_path.c_str(), core_num)
        , clock(0)
        , core_num(core_num)
    {
        anna_core_list.reserve(core_num);
        for (int i = 0; i < core_num; i++) {
            anna_core_list.emplace_back(&memory, i);
        }
    }

    void ClockTick()
    {
        clock++;
        memory.ClockTick();
        // #pragma omp parallel for
        for (uint64_t i = 0; i < anna_core_list.size(); i++) {
            anna_core_list[i].ClockTick();
        }
    }
    void Run(uint64_t batch_size)
    {

        uint64_t total_work_num = queries.size();
        uint64_t r = 0;
        for (uint64_t l = 0; r < total_work_num; l = r) {
            r = std::min(l + batch_size, total_work_num);
            std::vector<std::vector<uint64_t>> now;
            for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
                now.push_back({});
            }
            for (uint64_t i = l; i < r; i++) {
                for (auto c : queries[i]) {
                    now[c].push_back(i);
                }
            }
            for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
                if (not now[i].empty()) {
                    cluster_list.push_back(i);
                    query_set_list.push_back(std::move(now[i]));
                }
            }
        }

        // setup cluster_list and query_set_list for each core
        std::vector<std::vector<uint64_t>> core_cluster_list;
        std::vector<std::vector<std::vector<uint64_t>>> core_query_set_list;
        this->batch_size = batch_size;

        for (uint64_t i = 0; i < core_num; i++) {
            core_cluster_list.push_back({});
            core_query_set_list.push_back({});
        }

        for (uint64_t i = 0; i < cluster_list.size(); i++) {
            core_cluster_list[i % core_num].push_back(cluster_list[i]);
            core_query_set_list[i % core_num].push_back(query_set_list[i]);
        }

        for (uint64_t i = 0; i < anna_core_list.size(); i++) {
            anna_core_list[i].Setup(std::move(core_cluster_list[i]), std::move(core_query_set_list[i]), batch_size);
        }

        while (true) {
            ClockTick();
            bool Finished = 1;
            for (uint64_t i = 0; i < anna_core_list.size(); i++) {
                Finished &= anna_core_list[i].IsFinished();
            }
            if (Finished) {
                break;
            }
        }
        printf("Done!\n");
        memory.PrintStats();
    }
    void Setup(std::vector<std::vector<uint64_t>>&& queries)
    {
        this->queries = std::move(queries);
    }

    uint64_t getClockNum() { return clock; }

private:
    MultiMemoryInterface memory;
    std::vector<ANNACore> anna_core_list;

    // each fetch cluster index and its queries' index
    std::vector<uint64_t> cluster_list;
    std::vector<std::vector<uint64_t>> query_set_list;

    uint64_t batch_size;
    std::vector<std::vector<uint64_t>> queries;

    uint64_t core_num;

    uint64_t clock;
};
