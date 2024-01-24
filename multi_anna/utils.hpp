#pragma once

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

template <class T>
void LoadList(const std::string& list_path, std::vector<T>& arr)
{
    std::string line;
    const std::string delimiter = ",";
    std::ifstream raw_list(list_path);
    while (std::getline(raw_list, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        T formatted;
        iss >> formatted;
        arr.push_back(formatted);
    }
    raw_list.close();
}

template <class T>
void LoadCSV(const std::string& csv_path, std::vector<std::vector<T>>& matrix, int max_row, int max_col = -1)
{
    std::string line;
    const std::string delimiter = ",";
    std::ifstream raw_csv(csv_path);
    int row_cnt = 0;
    while (std::getline(raw_csv, line) && row_cnt < max_row) {
        row_cnt++;
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string value;

        std::vector<T> row;
        while (getline(ss, value, ',')) {
            if (value.empty()) {
                continue;
            }
            std::istringstream iss(value);
            T formatted;
            iss >> formatted;
            row.push_back(formatted);
            if (max_col != -1 and row.size() == max_col) {
                break;
            }
        }
        matrix.push_back(row);
    }
}

class AddressMapper {
public:
    AddressMapper(int devices, int channels, int ranks, int bankgroups, int banks, int offset)
        : devices(devices)
        , channels(channels)
        , ranks(ranks)
        , bankgroups(bankgroups)
        , banks(banks)
        , offset(offset)
    {
    }
    static int LogBase2(int power_of_two)
    {
        int i = 0;
        while (power_of_two > 1) {
            power_of_two /= 2;
            i++;
        }
        return i;
    }
    void makeMapper(const std::string& x)
    {
        std::map<std::string, int> field_widths;
        field_widths["de"] = LogBase2(devices);
        field_widths["ch"] = LogBase2(channels);
        field_widths["ra"] = LogBase2(ranks);
        field_widths["bg"] = LogBase2(bankgroups);
        field_widths["ba"] = LogBase2(banks);
        field_widths["of"] = LogBase2(offset);

        // get address mapping position fields from config
        // each field must be 2 chars
        std::vector<std::string> fields;
        for (size_t i = 0; i < x.size(); i += 2) {
            std::string token = x.substr(i, 2);
            fields.push_back(token);
        }
        fields.push_back("of");

        std::map<std::string, int> field_pos;
        int pos = 0;
        while (!fields.empty()) {
            auto token = fields.back();
            fields.pop_back();
            if (field_widths.find(token) == field_widths.end()) {
                std::cerr << "Unrecognized field: " << token << std::endl;
                exit(-1);
            }
            field_pos[token] = pos;
            pos += field_widths[token];
        }

        de_pos = field_pos.at("de");
        ch_pos = field_pos.at("ch");
        ra_pos = field_pos.at("ra");
        bg_pos = field_pos.at("bg");
        ba_pos = field_pos.at("ba");
        of_pos = field_pos.at("of");

        de_width = field_widths.at("de");
        ch_width = field_widths.at("ch");
        ra_width = field_widths.at("ra");
        bg_width = field_widths.at("bg");
        ba_width = field_widths.at("ba");
        of_width = field_widths.at("of");

        de_mask = (1 << field_widths.at("de")) - 1;
        ch_mask = (1 << field_widths.at("ch")) - 1;
        ra_mask = (1 << field_widths.at("ra")) - 1;
        bg_mask = (1 << field_widths.at("bg")) - 1;
        ba_mask = (1 << field_widths.at("ba")) - 1;
        of_mask = (1 << field_widths.at("of")) - 1;
    }
    std::pair<uint64_t, uint64_t> map(uint64_t x)
    {
        uint64_t res = 0;
        int de = (x >> de_pos) & de_mask;
        int ch = (x >> ch_pos) & ch_mask;
        int ra = (x >> ra_pos) & ra_mask;
        int bg = (x >> bg_pos) & bg_mask;
        int ba = (x >> ba_pos) & ba_mask;
        int of = (x >> of_pos) & of_mask;
        std::cout << de << std::endl;
        std::cout << ch << std::endl;
        std::cout << ra << std::endl;
        std::cout << bg << std::endl;
        std::cout << ba << std::endl;
        std::cout << of << std::endl;
        res += ch;
        res <<= ra_width;
        res += ra;
        res <<= bg_width;
        res += bg;
        res <<= ba_width;
        res += ba;
        res <<= of_width;
        res += of;
        return std::make_pair(de, res);
    }

private:
    int devices;
    int channels;
    int ranks;
    int bankgroups;
    int banks;
    int offset;

    int de_pos, ch_pos, ra_pos, bg_pos, ba_pos, of_pos;
    int de_width, ch_width, ra_width, bg_width, ba_width, of_width;
    uint64_t de_mask, ch_mask, ra_mask, bg_mask, ba_mask, of_mask;
};

// for remove stack level interleave from address
class AddressStackMaper {
public:
    AddressStackMaper(int l_pos, int r_pos)
        : l_pos(l_pos)
        , r_pos(r_pos)
    {
        assert(l_pos <= r_pos);
        l_mask = (1 << l_pos) - 1;
        st_mask = (1 << (r_pos - l_pos)) - 1;
    }

    // return <st, true addr for one st>
    std::pair<uint64_t, uint64_t> map(uint64_t x)
    {
        return std::make_pair((x >> l_pos) & st_mask, (x & l_mask) | ((x >> (r_pos - l_pos)) & (~l_mask)));
    }

private:
    int l_pos, r_pos;
    uint64_t l_mask;
    uint64_t st_mask;
};
