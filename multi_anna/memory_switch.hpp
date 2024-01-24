#pragma once

#include <config.hpp>
#include <iostream>
#include <queue>

const uint64_t SWITCH_BATCH_SIZE = 128 / 64;

class MemorySwitch {
    using memory_query_t = std::pair<uint64_t, uint64_t>; // <target id, addr>

public:
    MemorySwitch(uint64_t num)
    {
        req_in.reserve(num);
        req_out.reserve(num);
        res_in.reserve(num);
        res_out.reserve(num);
        for (uint64_t i = 0; i < num; i++) {
            req_in.push_back({});
            req_out.push_back({});
            res_in.push_back({});
            res_out.push_back({});
        }
    }
    void ClockTick()
    {
        for (uint64_t i = 0; i < req_in.size(); i++) {
            for (int j = 0; j < SWITCH_BATCH_SIZE and not req_in[i].empty(); j++) {
                auto now = req_in[i].front();
                req_in[i].pop();
                req_out[now.first].push(std::make_pair(i, now.second));
            }
        }
        for (uint64_t i = 0; i < res_in.size(); i++) {
            for (int j = 0; j < SWITCH_BATCH_SIZE and not res_in[i].empty(); j++) {
                auto now = res_in[i].front();
                res_in[i].pop();
                res_out[now.first].push(std::make_pair(i, now.second));
            }
        }
    }

    void sendReq(uint64_t core_id, uint64_t mem_id, uint64_t addr)
    {
        req_in[core_id].push(std::make_pair(mem_id, addr));
    }
    uint64_t getReq(uint64_t mem_id)
    {
        if (req_out[mem_id].empty()) {
            return -1;
        }
        uint64_t ans = req_out[mem_id].front().second;
        req_out[mem_id].pop();
        return ans;
    }

    void sendRes(uint64_t mem_id, uint64_t core_id, uint64_t addr)
    {
        res_in[mem_id].push(std::make_pair(core_id, addr));
    }

    std::pair<uint64_t, uint64_t> getRes(uint64_t core_id)
    {
        if (res_out[core_id].empty()) {
            return std::make_pair(-1, -1);
        }
        auto ans = res_out[core_id].front();
        res_out[core_id].pop();
        return ans;
    }

private:
    std::vector<std::queue<memory_query_t>> req_in;
    std::vector<std::queue<memory_query_t>> req_out;
    std::vector<std::queue<memory_query_t>> res_in;
    std::vector<std::queue<memory_query_t>> res_out;
};
