#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "structures.h"
#include "bitreader.h"
#include "util_funcs.h"
#include <glog/logging.h>

std::unordered_map<size_t, FrameParametrs> ReadSOF0(BitReader &reader, uint16_t &height,
                                                    uint16_t &width) {
    std::unordered_map<size_t, FrameParametrs> frames_pars;
    uint16_t size = reader.GetDoubleByte() - 2;
    reader.GetByte();                     // P
    height = reader.GetDoubleByte();      // Y
    width = reader.GetDoubleByte();       // X
    size_t frame_cnt = reader.GetByte();  // Nf
    if (size != 3 * frame_cnt + 6) {
        throw std::runtime_error("Bad parametrs in SOF0");
    }
    for (size_t id = 0; id < frame_cnt; ++id) {
        FrameParametrs pars;
        pars.label = reader.GetByte();  // C_i
        reader.GetByte();               // H_i|V_i
        pars.hor_sampling = reader.CheckLastByte() >> 4;
        pars.vert_sampling = reader.CheckLastByte() & 15;
        pars.qtable_dest = reader.GetByte();  // Tq_i
        if ((pars.hor_sampling < 1 || pars.hor_sampling > 4) ||
            (pars.vert_sampling < 1 || pars.vert_sampling > 4) || pars.qtable_dest > 3 ||
            frames_pars.count(pars.label)) {
            throw std::runtime_error("Bad parametrs in SOF0");
        }

        frames_pars[pars.label] = pars;
    }
    return frames_pars;
}

std::vector<HuffTabParametrs> ReadDHT(BitReader &reader) {
    int size = reader.GetDoubleByte() - 2;
    std::vector<HuffTabParametrs> tables;
    while (size > 0) {
        tables.push_back({});
        HuffTabParametrs &tab = tables.back();
        reader.GetByte();  // Tc|Th
        size -= 1;
        tab.table_class = reader.CheckLastByte() >> 4;
        tab.table_id = reader.CheckLastByte() & 15;
        if (!(tab.table_class <= 1 && tab.table_id <= 1)) {
            throw std::runtime_error("Bad parametrs in DHT");
        }

        for (size_t id = 0; id < HuffTabParametrs::kMaxSize; ++id) {
            tab.lenghts.push_back(reader.GetByte());
            size -= reader.CheckLastByte() + 1;
        }
        for (size_t id = 0; id < HuffTabParametrs::kMaxSize; ++id) {
            std::vector<uint8_t> vals(tab.lenghts[id]);
            for (auto &val : vals) {
                val = reader.GetByte();
            }
            for (const auto &val : vals) {
                tab.values.push_back(val);
            }
        }
        tab.huffman.Build(tab.lenghts, tab.values);
    }

    if (size != 0) {
        throw std::runtime_error("Bad parametrs in DHT");
    }
    return tables;
}

std::vector<QuantTable> ReadDQT(BitReader &reader) {
    uint16_t size = reader.GetDoubleByte() - 2;
    if (size == 0 || size % 65 != 0) {
        throw std::runtime_error("Bad size in DQT");
    }
    std::vector<QuantTable> tabs(size / 65);
    for (size_t cnt = 0; cnt < size / 65; ++cnt) {
        QuantTable &tab = tabs[cnt];
        reader.GetByte();  // Pq|Tq
        if ((reader.CheckLastByte() >> 4) != 0) {
            throw std::runtime_error("Bad Pq in DQT");
        }
        tab.table_dest = reader.CheckLastByte() & 15;
        if (tab.table_dest > 3) {
            throw std::runtime_error("Bad Tq in DQT");
        }
        std::vector<int> all_els;
        for (size_t x = 0; x < QuantTable::kTableSize; ++x) {
            for (size_t y = 0; y < QuantTable::kTableSize; ++y) {
                all_els.push_back(reader.GetByte());
            }
        }
        tab.table = ZigZagConvert(all_els);
    }
    return tabs;
}

void ReadSOS(BitReader &reader, std::unordered_map<size_t, FrameParametrs> &frame_pars) {
    uint16_t size = reader.GetDoubleByte() - 2;  // NOLINT
    uint8_t comp_cnt = reader.GetByte();
    size -= 1;
    if (frame_pars.size() != comp_cnt) {
        throw std::runtime_error("Bad count of components in ReadSOS");
    }
    for (size_t id = 0; id < comp_cnt; ++id) {
        uint8_t comp_num = reader.GetByte();  // Cs
        reader.GetByte();                     // Td_j|Ta_j
        size -= 2;
        uint8_t td = reader.CheckLastByte() >> 4;
        uint8_t ta = reader.CheckLastByte() & 15;
        if (!(td <= 1 && ta <= 1)) {
            throw std::runtime_error("Bad TD and TA in ReadSOS");
        }
        if (!frame_pars.count(comp_num)) {
            throw std::runtime_error("No component in frames in ReadSOS");
        }
        frame_pars[comp_num].dc_huff_dest = td;
        frame_pars[comp_num].ac_huff_dest = ta;
    }
    if (reader.GetByte() != 0) {  // Ss
        throw std::runtime_error("Bad Ss in ReadSOS");
    }
    if (reader.GetByte() != 63) {  // Se
        throw std::runtime_error("Bad Se in ReadSOS");
    }
    if (reader.GetByte() != 0) {  // Ah|Al
        throw std::runtime_error("Bad Ah|Al in ReadSOS");
    }
    size -= 3;
    if (size != 0) {
        throw std::runtime_error("Bad size in ReadSOS");
    }
}

void ReadAPPn(BitReader &reader) {
    uint16_t size = reader.GetDoubleByte() - 2;
    for (size_t id = 0; id < size; ++id) {
        reader.GetByte();
    }
}

void ReadDRI(BitReader &reader) {
    uint16_t size = reader.GetDoubleByte() - 2;          // NOLINT
    uint16_t restart_interval = reader.GetDoubleByte();  // NOLINT
    assert(0);
}

void ReadRSTn(BitReader &reader) {  // NOLINT
    assert(0);
}

std::string ReadCOM(BitReader &reader) {
    uint16_t size = reader.GetDoubleByte() - 2;
    std::string comment;
    for (size_t id = 0; id < size; ++id) {
        comment += static_cast<char>(reader.GetByte());
    }
    return comment;
}