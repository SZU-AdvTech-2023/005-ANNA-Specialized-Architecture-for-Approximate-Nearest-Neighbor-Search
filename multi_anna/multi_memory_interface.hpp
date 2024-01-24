#pragma once

#include <config.hpp>
#include <memory_switch.hpp>
#include <memory_system.h>
#include <unordered_map>
#include <utils.hpp>

const uint MAPPER_OFFSET = 16;

class MultiMemoryInterface {
public:
    MultiMemoryInterface(const std::string& config_file, const std::string& output_dir, int st_num)
        : id_num(0)
        , mapper(MAPPER_OFFSET, MAPPER_OFFSET + 3)
        , st_num(st_num)
        , memory_switch(st_num)
    {
        memory_system_list.reserve(st_num);
        for (int i = 0; i < st_num; i++) {
            memory_system_list.emplace_back(config_file, output_dir + "/" + std::to_string(i),
                std::bind(&MultiMemoryInterface::ReadCallback, this, i, std::placeholders::_1),
                std::bind(&MultiMemoryInterface::WriteCallback, this, i, std::placeholders::_1));
            read_request_queue.emplace_back();
            read_ready_queues.emplace_back();
            id_map.emplace_back();
        }
    }
    void ClockTick()
    {
        if (use_memory_switch) {
            memory_switch.ClockTick();
            for (uint64_t st = 0; st < st_num; st++) {
                uint64_t addr;
                while ((addr = memory_switch.getReq(st)) != -1) {
                    read_request_queue[st].insert(addr);
                    if (debug_switch_req)
                        std::cout << "memory req out addr: " << addr << std::endl;
                }
            }
            for (uint64_t core_id = 0; core_id < st_num; core_id++) {
                while (true) {
                    auto res = memory_switch.getRes(core_id);
                    if (res.first == -1) {
                        break;
                    }
                    uint64_t st = res.first;
                    uint64_t addr = res.second;
                    auto it = id_map[st].find(addr);
                    read_ready_queues[st][it->second].insert(addr);
                    id_map[st].erase(it);
                    if (debug_switch_res)
                        std::cout << "memory res out addr: " << addr << std::endl;
                }
            }
        }
#pragma omp parallel for
        for (int i = 0; i < st_num; i++) {
            memory_system_list[i].ClockTick();
            while (not read_request_queue[i].empty()) {
                uint64_t addr = *read_request_queue[i].begin();

                if (memory_system_list[i].WillAcceptTransaction(addr, 0)) {
#pragma omp atomic
                    test_value.memory_req_cnt++;
                    memory_system_list[i].AddTransaction(addr, 0);
                    read_request_queue[i].erase(read_request_queue[i].begin());
                } else {
                    break;
                }
            }
        }
    }
    void RequestRead(uint64_t id, uint64_t addr)
    {
        auto real_addr = mapper.map(addr);
        uint64_t st = real_addr.first;
        addr = real_addr.second;
        if (debug_switch_req)
            std::cout << "memory req in addr: " << addr << ", id: " << id << std::endl;
        if (use_memory_switch) {
            if (id2core[id] == st) {
                read_request_queue[st].insert(addr);
            } else {
                memory_switch.sendReq(id2core[id], st, addr);
            }
        } else {
            read_request_queue[st].insert(addr);
        }

        id_map[st].insert({ addr, id });
    }
    uint64_t GetOneReadyReadRequest(uint64_t id)
    {
        for (uint64_t st = 0; st < st_num; st++) {
            if (not read_ready_queues[st][id].empty()) {
                uint64_t addr = *read_ready_queues[st][id].begin();
                read_ready_queues[st][id].erase(read_ready_queues[st][id].begin());
                return addr;
            }
        }
        return -1;
    }

    uint64_t GetAllReadyReadRequest(uint64_t id)
    {
        uint64_t res = 0;
        for (uint64_t st = 0; st < st_num; st++) {
            res += read_ready_queues[st][id].size();
            read_ready_queues[st][id].clear();
        }
        return res;
    }
    void ReadCallback(uint64_t st, uint64_t addr)
    {
        auto it = id_map[st].find(addr);
        
        if (it == id_map[st].end() or it->first != addr)
            assert(0);
        if (debug_switch_res)
            std::cout << "memory read res in addr: " << addr << std::endl;
        if (use_memory_switch) {
            if (id2core[it->second] == st) {
                read_ready_queues[st][it->second].insert(addr);
                id_map[st].erase(it);
            } else {
                memory_switch.sendRes(st, id2core[it->second], addr);
            }
        } else {
            read_ready_queues[st][it->second].insert(addr);
            id_map[st].erase(it);
        }
    }
    void WriteCallback(uint64_t st, uint64_t addr) { }
    void PrintStats()
    {
        for (int i = 0; i < st_num; i++) {
            memory_system_list[i].PrintStats();
        }
    }
    bool AllFinished()
    {
        return false;
    }
    uint64_t RegisterId(uint64_t core_id)
    {
        id2core[id_num] = core_id;
        id_num++;
        for (uint64_t st = 0; st < st_num; st++)
            read_ready_queues[st].push_back({});
        return id_num - 1;
    }

private:
    uint64_t st_num;
    std::vector<dramsim3::MemorySystem> memory_system_list;
    std::vector<std::multiset<uint64_t>> read_request_queue;
    std::vector<std::vector<std::multiset<uint64_t>>> read_ready_queues;
    std::vector<std::multimap<uint64_t, uint64_t>> id_map; // <addr, id>
    uint64_t id_num;

    std::map<uint64_t, uint64_t> id2core;
    MemorySwitch memory_switch;

    AddressStackMaper mapper;
};
