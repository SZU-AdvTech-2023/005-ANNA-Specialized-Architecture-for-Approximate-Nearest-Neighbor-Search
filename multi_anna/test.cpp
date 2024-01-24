#include <multi_anna.hpp>

std::vector<std::vector<uint64_t>> queries;

std::string database_name;

// random config for test
void prepare_random_config()
{
    // Setup below
    // std::vector<std::tuple<uint64_t, uint64_t>> metadata_addr;
    // std::vector<uint64_t> cluster_addr[CLUSTER_NUM];
    // uint64_t cluster_centroid[CLUSTER_NUM];
    for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
        cluster_centroid[i] = cluster_centroid_addr_start + i * VECTOR_SIZE;
    }

    uint64_t start_addr = cluster_addr_start;
    for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
        uint64_t len = rand() % MAX_CLUSTER_LEN;
        cluster_addr.emplace_back(start_addr, (len * ENCODED_VECTOR_SIZE_PER_BITS + 7) / 8, len);
        start_addr += (len * ENCODED_VECTOR_SIZE_PER_BITS + 7) / 8;
    }

    for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
        metadata_addr.push_back(std::make_pair(metadata_addr_start + i * METADATA_LEN, METADATA_LEN));
    }
}

void prepare_random_queries(uint64_t queries_num, uint64_t cluster_num)
{
    for (uint64_t i = 0; i < queries_num; i++) {
        std::vector<uint64_t> clusters;
        for (uint64_t j = 0; j < cluster_num; j++) {
            clusters.push_back(rand() % CLUSTER_NUM);
        }
        queries.push_back(std::move(clusters));
    }
}

void prepare_config()
{
    // Setup below
    // std::vector<std::tuple<uint64_t, uint64_t>> metadata_addr;
    // std::vector<uint64_t> cluster_addr[CLUSTER_NUM];
    // uint64_t cluster_centroid[CLUSTER_NUM];

    std::string cluster_size_path = database_path + "wdnmd.csv";

    for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
        cluster_centroid[i] = cluster_centroid_addr_start + i * VECTOR_SIZE;
    }
    std::vector<int> cluster_size;
    LoadList(cluster_size_path, cluster_size);
    uint64_t now_addr = cluster_addr_start;
    for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
        cluster_addr.emplace_back(now_addr, (cluster_size[i] * ENCODED_VECTOR_SIZE_PER_BITS + 7) / 8, cluster_size[i]);
        now_addr += (cluster_size[i] * ENCODED_VECTOR_SIZE_PER_BITS + 7) / 8;
    }

    for (uint64_t i = 0; i < CLUSTER_NUM; i++) {
        metadata_addr.push_back(std::make_pair(metadata_addr_start + i * METADATA_LEN, METADATA_LEN));
    }
}

void prepare_queries(uint64_t query_num)
{
    std::string queries_path = database_path + database_name + "-cluster-256.csv";
    std::vector<std::vector<int>> csv;
    LoadCSV(queries_path, queries, query_num, W);
}

int main(int argc, char* argv[])
{
    database_name = "deep1B";
    if (argc > 1) {
        database_name = argv[1];
    }
    output_path = "./output_" + database_name + "/";
    mkdir(output_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    for (int i = 0; i < 8; i++) {
        mkdir((output_path + std::to_string(i)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    uint64_t query_num = QUERY_NUM;
    srand(0x114514);
    prepare_config();
    prepare_queries(query_num);
    for (uint64_t i = 0; i < queries.size(); i++) {
        test_value.queries_size.push_back(0);
        for (auto c : queries[i]) {
            *test_value.queries_size.rbegin() += cluster_addr[c].vector_num;
            test_value.cluster_vector_cnt += cluster_addr[c].vector_num;
        }
    }
    test_value.cpm_topk_calc_cnt = cluster_addr.size() * query_num;
    test_value.cpm_cu_calc_cnt = (cluster_addr.size() + W + W * K_STAR) * query_num * D;

    test_value.scm_topk_calc_cnt = test_value.cluster_vector_cnt;
    test_value.scm_reduction_tree_calc_cnt = test_value.cluster_vector_cnt * M;

    test_value.cpm_query_buffer_read = (cluster_addr.size() + W + W * K_STAR) * query_num * VECTOR_SIZE;
    test_value.cpm_query_buffer_write = W * query_num * VECTOR_SIZE;

    test_value.cpm_codebook_buffer_read = VECTOR_ELEMENT_SIZE * K_STAR * D * W * query_num;
    test_value.cpm_codebook_buffer_write = VECTOR_ELEMENT_SIZE * K_STAR * D;

    test_value.scm_lookup_table_buffer_read = test_value.cluster_vector_cnt * VECTOR_ELEMENT_SIZE;
    test_value.scm_lookup_table_buffer_write = M * K_STAR * W * query_num * VECTOR_ELEMENT_SIZE;

    test_value.cpm_memory_reader_read = cluster_addr.size() * VECTOR_SIZE * query_num;

    for (uint64_t i = 0; i < query_num; i++) {
        test_value.queries_start_clock.push_back(-1);
        test_value.queries_end_clock.push_back(-1);
    }

    MultiANNA anna(8); // need to be matched with address mapping
    anna.Setup(std::move(queries));
    queries.clear();
    std::cout << database_name << std::endl;
    anna.Run(BATCH_SIZE);

    std::cout << "using switch: " << use_memory_switch << std::endl;
    std::cout << "N_u: " << N_u << std::endl;
    std::cout << "N_cu: " << N_cu << std::endl;
    std::cout << "N_scm: " << N_scm << std::endl;
    std::cout << "nprobe: " << W << std::endl;
    std::cout << "query num: " << query_num << std::endl;
    std::cout << "batch size: " << BATCH_SIZE << std::endl;
    std::cout << "clocks cnt: " << anna.getClockNum() << std::endl;
    std::cout << "cluster vector cnt: " << test_value.cluster_vector_cnt << std::endl;
    std::cout << "memory req cnt: " << test_value.memory_req_cnt << std::endl;
    std::cout << "cpm computing cycle cnt: " << test_value.cpm_computing_cycle_cnt << std::endl;
    std::cout << "scm computing cycle cnt: " << test_value.scm_computing_cycle_cnt << std::endl;
    std::cout << "efm blocked cycle cnt: " << test_value.efm_blocked_cycle_cnt << std::endl;
    std::cout << "average_bandwidth: " << test_value.memory_req_cnt * 64.0 / anna.getClockNum() / 1024 * 1000 / 1024 * 1000 / 1024 * 1000 << std::endl;

    double average_latency = 0;
    for (uint64_t i = 0; i < query_num; i++) {
        average_latency += test_value.queries_end_clock[i] - test_value.queries_start_clock[i];
        std::cout << "query: " << i << ",st: " << test_value.queries_start_clock[i]
                  << ",ed: " << test_value.queries_end_clock[i] << ",latency: " << test_value.queries_end_clock[i] - test_value.queries_start_clock[i] << ",size: " << test_value.queries_size[i] << std::endl;
    }
    average_latency /= query_num;

    std::ofstream log_file;
    log_file.open(result_path + database_name + "_log.txt", std::ios::out);

    log_file << "using switch: " << use_memory_switch << std::endl;
    log_file << "N_u: " << N_u << std::endl;
    log_file << "N_cu: " << N_cu << std::endl;
    log_file << "N_scm: " << N_scm << std::endl;
    log_file << "nprobe: " << W << std::endl;
    log_file << "query num: " << query_num << std::endl;
    log_file << "batch size: " << BATCH_SIZE << std::endl;
    log_file << "clocks cnt: " << anna.getClockNum() << std::endl;
    log_file << "cluster vector cnt: " << test_value.cluster_vector_cnt << std::endl;
    log_file << "memory req cnt: " << test_value.memory_req_cnt << std::endl;
    log_file << "cpm computing cycle cnt: " << test_value.cpm_computing_cycle_cnt << std::endl;
    log_file << "scm computing cycle cnt: " << test_value.scm_computing_cycle_cnt << std::endl;
    log_file << "efm blocked cycle cnt: " << test_value.efm_blocked_cycle_cnt << std::endl;
    log_file << "average_bandwidth: " << test_value.memory_req_cnt * 64.0 / anna.getClockNum() / 1024 * 1000 / 1024 * 1000 / 1024 * 1000 << std::endl;

    for (uint64_t i = 0; i < query_num; i++) {
        log_file << "query: " << i << ",st: " << test_value.queries_start_clock[i]
                 << ",ed: " << test_value.queries_end_clock[i] << ",latency: " << test_value.queries_end_clock[i] - test_value.queries_start_clock[i] << ",size: " << test_value.queries_size[i] << std::endl;
    }

    std::ofstream result_file;
    result_file.open(result_path + database_name + "_result.csv", std::ios::out);
    result_file << "total_time(ms),average_latency(ms),cpm_topk_calc_cnt,cpm_cu_calc_cnt,scm_topk_calc_cnt,"
                << "scm_reduction_tree_calc_cnt,cpm_query_buffer_read,cpm_query_buffer_write,"
                << "cpm_codebook_buffer_read,cpm_codebook_buffer_write,"
                << "efm_encoded_vector_buffer_read,efm_encoded_vector_buffer_write,scm_lookup_table_buffer_read,scm_lookup_table_buffer_write,"
                << "cpm_memory_reader_read(Bytes),efm_memory_reader_read(Bytes),"
                << std::endl;
    result_file << anna.getClockNum() / 1e6 << ","
                << average_latency / 1e6 << ","
                << test_value.cpm_topk_calc_cnt << ","
                << test_value.cpm_cu_calc_cnt << ","
                << test_value.scm_topk_calc_cnt << ","
                << test_value.scm_reduction_tree_calc_cnt << ","
                << test_value.cpm_query_buffer_read << ","
                << test_value.cpm_query_buffer_write << ","
                << test_value.cpm_codebook_buffer_read << ","
                << test_value.cpm_codebook_buffer_write << ","
                << test_value.efm_encoded_vector_buffer_read << ","
                << test_value.efm_encoded_vector_buffer_write << ","
                << test_value.scm_lookup_table_buffer_read << ","
                << test_value.scm_lookup_table_buffer_write << ","
                << test_value.cpm_memory_reader_read << ","
                << test_value.efm_memory_reader_read << ","
                << std::endl;
    return 0;
}
