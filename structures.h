#pragma once

#include <fftw3.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "huffman.h"

struct YCbCr {
    double y;
    double cb;
    double cr;
};

class Matrix88 {
public:
    double mat[8][8];
    inline double &Get(size_t y, size_t x) {
        return mat[y][x];
    }

    inline const double &Get(size_t y, size_t x) const {
        return mat[y][x];
    }
};

struct FrameParametrs {
    size_t label;
    size_t hor_sampling;
    size_t vert_sampling;
    size_t qtable_dest;
    size_t dc_huff_dest;
    size_t ac_huff_dest;
};

struct HuffTabParametrs {
    static const int kMaxSize = 16;
    size_t table_class;
    size_t table_id;
    std::vector<uint8_t> lenghts;
    std::vector<uint8_t> values;
    HuffmanTree huffman;
};

struct QuantTable {
    static const int kTableSize = 8;
    size_t table_dest;
    Matrix88 table;
};

struct MCUBlock {
    std::vector<Matrix88> y_table;
    Matrix88 cb_table;
    Matrix88 cr_table;
};