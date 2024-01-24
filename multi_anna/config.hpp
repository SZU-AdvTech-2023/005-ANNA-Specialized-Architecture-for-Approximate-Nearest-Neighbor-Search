#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

const std::string database_path = "./resource/union/query/";
const std::string result_path = "./result/";
extern std::string output_path;

const uint64_t W = 128; // also known as nprobe
const uint64_t MEMORY_SIZE = 8 * (1ull << 30);
const uint64_t CLUSTER_NUM = 16384;
const uint64_t METADATA_LEN = 64;

const uint64_t QUERY_NUM = 1024;


extern std::vector<std::pair<uint64_t, uint64_t>> metadata_addr; // vector of (addr, len)

class Cluster {
public:
    uint64_t start_addr;
    uint64_t bytes;
    uint64_t vector_num;
    Cluster(uint64_t start_addr, uint64_t bytes, uint64_t vector_num)
        : start_addr(start_addr)
        , bytes(bytes)
        , vector_num(vector_num)

    {
    }
};
extern std::vector<Cluster> cluster_addr;

extern uint64_t cluster_centroid[CLUSTER_NUM];

const uint64_t use_memory_switch = 1;
const uint64_t debug = 1;
const uint64_t debug_switch = 0;
const uint64_t debug_switch_req = 0;
const uint64_t debug_switch_res = 0;

const uint64_t metadata_addr_start = 0x10000;
const uint64_t cluster_centroid_addr_start = 0x20000;
const uint64_t cluster_addr_start = 0x30000;

const uint64_t MAX_CLUSTER_LEN = 20000;

const uint64_t VECTOR_ELEMENT_SIZE = 2; // such as float16
const uint64_t N_cu = 128;
const uint64_t N_scm = 36;
const uint64_t N_u = 32; // match to M
const uint64_t D = 384; // vector dimension
const uint64_t M = 32;  // sub vector num in a vector
const uint64_t SCM_CYCLES_PER_QUERY = (M - 1) / N_u + 1;
const uint LOG_K_STAR = 8;
const uint64_t K_STAR = 1ull << LOG_K_STAR;

const uint64_t VECTOR_SIZE = D * VECTOR_ELEMENT_SIZE;
const uint64_t ENCODED_VECTOR_SIZE_PER_BITS = LOG_K_STAR * M;

const uint64_t BATCH_SIZE = 16;

class TestValue {
public:
    uint64_t memory_req_cnt = 0;
    uint64_t cpm_computing_cycle_cnt = 0;
    uint64_t scm_computing_cycle_cnt = 0;
    uint64_t efm_blocked_cycle_cnt = 0;
    uint64_t cluster_vector_cnt = 0;

    // calc usnig count, 统计该运算器件接收了多少个数据
    uint64_t cpm_topk_calc_cnt = 0;
    uint64_t cpm_cu_calc_cnt = 0;

    uint64_t scm_topk_calc_cnt = 0;
    uint64_t scm_reduction_tree_calc_cnt = 0;

    // ram read / write count (per bytes)
    uint64_t cpm_query_buffer_read = 0;
    uint64_t cpm_query_buffer_write = 0;

    uint64_t cpm_codebook_buffer_read = 0;
    uint64_t cpm_codebook_buffer_write = 0;

    uint64_t efm_encoded_vector_buffer_read = 0;
    uint64_t efm_encoded_vector_buffer_write = 0;

    uint64_t scm_lookup_table_buffer_read = 0;
    uint64_t scm_lookup_table_buffer_write = 0;

    uint64_t cpm_memory_reader_read = 0;
    uint64_t efm_memory_reader_read = 0;

    std::vector<uint64_t> queries_start_clock;
    std::vector<uint64_t> queries_end_clock;

    std::vector<uint64_t> queries_size;
};

extern TestValue test_value;
