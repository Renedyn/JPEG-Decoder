#pragma once

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include "structures.h"
#include "image.h"
#include <glog/logging.h>
#include "fft.h"

template <typename T>
std::vector<T> ZigZagRead(const Matrix88 &mat) {
    int sz = 8;
    std::vector<T> result;
    result.reserve(64);
    for (int sum = 0; sum <= 2 * (sz - 1); ++sum) {
        if (sum % 2 == 1) {
            for (int id = std::max<int>(0, sum - sz + 1); id < sz && sum - id >= 0; ++id) {
                result.push_back(mat.Get(id, sum - id));
            }
        } else {
            for (int id = std::max<int>(0, sum - sz + 1); id < sz && sum - id >= 0; ++id) {
                result.push_back(mat.Get(sum - id, id));
            }
        }
    }
    return result;
}

Matrix88 ZigZagConvert(const std::vector<int> &seq) {
    if (seq.size() != 64) {
        throw std::runtime_error("Bad size of sequence in ZigZagConvert");
    }
    Matrix88 res;
    size_t cur_pos = 0;
    res.Get(0, 0) = seq[cur_pos++];
    res.Get(0, 1) = seq[cur_pos++];
    res.Get(1, 0) = seq[cur_pos++];
    res.Get(2, 0) = seq[cur_pos++];
    res.Get(1, 1) = seq[cur_pos++];
    res.Get(0, 2) = seq[cur_pos++];
    res.Get(0, 3) = seq[cur_pos++];
    res.Get(1, 2) = seq[cur_pos++];
    res.Get(2, 1) = seq[cur_pos++];
    res.Get(3, 0) = seq[cur_pos++];
    res.Get(4, 0) = seq[cur_pos++];
    res.Get(3, 1) = seq[cur_pos++];
    res.Get(2, 2) = seq[cur_pos++];
    res.Get(1, 3) = seq[cur_pos++];
    res.Get(0, 4) = seq[cur_pos++];
    res.Get(0, 5) = seq[cur_pos++];
    res.Get(1, 4) = seq[cur_pos++];
    res.Get(2, 3) = seq[cur_pos++];
    res.Get(3, 2) = seq[cur_pos++];
    res.Get(4, 1) = seq[cur_pos++];
    res.Get(5, 0) = seq[cur_pos++];
    res.Get(6, 0) = seq[cur_pos++];
    res.Get(5, 1) = seq[cur_pos++];
    res.Get(4, 2) = seq[cur_pos++];
    res.Get(3, 3) = seq[cur_pos++];
    res.Get(2, 4) = seq[cur_pos++];
    res.Get(1, 5) = seq[cur_pos++];
    res.Get(0, 6) = seq[cur_pos++];
    res.Get(0, 7) = seq[cur_pos++];
    res.Get(1, 6) = seq[cur_pos++];
    res.Get(2, 5) = seq[cur_pos++];
    res.Get(3, 4) = seq[cur_pos++];
    res.Get(4, 3) = seq[cur_pos++];
    res.Get(5, 2) = seq[cur_pos++];
    res.Get(6, 1) = seq[cur_pos++];
    res.Get(7, 0) = seq[cur_pos++];
    res.Get(7, 1) = seq[cur_pos++];
    res.Get(6, 2) = seq[cur_pos++];
    res.Get(5, 3) = seq[cur_pos++];
    res.Get(4, 4) = seq[cur_pos++];
    res.Get(3, 5) = seq[cur_pos++];
    res.Get(2, 6) = seq[cur_pos++];
    res.Get(1, 7) = seq[cur_pos++];
    res.Get(2, 7) = seq[cur_pos++];
    res.Get(3, 6) = seq[cur_pos++];
    res.Get(4, 5) = seq[cur_pos++];
    res.Get(5, 4) = seq[cur_pos++];
    res.Get(6, 3) = seq[cur_pos++];
    res.Get(7, 2) = seq[cur_pos++];
    res.Get(7, 3) = seq[cur_pos++];
    res.Get(6, 4) = seq[cur_pos++];
    res.Get(5, 5) = seq[cur_pos++];
    res.Get(4, 6) = seq[cur_pos++];
    res.Get(3, 7) = seq[cur_pos++];
    res.Get(4, 7) = seq[cur_pos++];
    res.Get(5, 6) = seq[cur_pos++];
    res.Get(6, 5) = seq[cur_pos++];
    res.Get(7, 4) = seq[cur_pos++];
    res.Get(7, 5) = seq[cur_pos++];
    res.Get(6, 6) = seq[cur_pos++];
    res.Get(5, 7) = seq[cur_pos++];
    res.Get(6, 7) = seq[cur_pos++];
    res.Get(7, 6) = seq[cur_pos++];
    res.Get(7, 7) = seq[cur_pos++];

    return res;
}

void FillWithCnt(std::vector<int> &result, const std::pair<int, int> &pr) {
    auto [cnt, val] = pr;
    if (cnt == -1 && val == -1) {
        while (result.size() != 64) {
            result.push_back(0);
        }
        return;
    }
    while (cnt--) {
        result.push_back(0);
    }
    result.push_back(val);
    if (result.size() > 64) {
        throw std::runtime_error("Size of matrix exceeded 64 in FillWithCnt, actual size = " +
                                 std::to_string(result.size()));
    }
}

RGB ConvertYCbCrToRGB(const YCbCr &pix) {
    RGB res;
    res.r = (pix.y + 1.402 * (pix.cr - 128));
    res.g = (pix.y - (0.114 * 1.772 * (pix.cb - 128) + 0.299 * 1.402 * (pix.cr - 128)) / 0.587);
    res.b = (pix.y + 1.772 * (pix.cb - 128));

    res.r = std::min(std::max(0, res.r), 255);
    res.g = std::min(std::max(0, res.g), 255);
    res.b = std::min(std::max(0, res.b), 255);
    return res;
}

void PrintQuantTableInfo(const QuantTable &tab) {
    std::cout << std::string(100, '-') << std::endl;
    std::cout << "Table id = " << tab.table_dest << std::endl;
    std::cout << "Table size = " << tab.kTableSize << std::endl;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            std::cout << tab.table.Get(i, j) << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << std::string(100, '-') << std::endl;
}

void MultiplyMatrixByElements(Matrix88 &fir, const Matrix88 &sec) {
    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            fir.Get(y, x) *= sec.Get(y, x);
        }
    }
}

void ConvertMatrix(Matrix88 &mat, DctCalculator &dct_calc, std::vector<double> *input,
                   std::vector<double> *output) {
    std::vector<double> els(64);
    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            els[y * 8 + x] = mat.Get(y, x);
        }
    }
    *input = els;
    dct_calc.Inverse();

    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            mat.Get(y, x) = (*output)[y * 8 + x] + 128;
            mat.Get(y, x) = std::min<double>(255, std::max<double>(0, mat.Get(y, x)));
        }
    }
}

void PrintFrameInfo(const FrameParametrs &frame) {
    std::cout << std::string(100, '-') << std::endl;
    std::cout << "Quant table dest = " << frame.qtable_dest << std::endl;
    std::cout << "Table AC dest = " << frame.ac_huff_dest << std::endl;
    std::cout << "Table DC dest = " << frame.dc_huff_dest << std::endl;
    std::cout << "Horizontal sampling mode = " << frame.hor_sampling << std::endl;
    std::cout << "Vertical sampling mode = " << frame.vert_sampling << std::endl;
    std::cout << "Frame label = " << frame.label << std::endl;
    std::cout << std::string(100, '-') << std::endl;
}

void PrintHuffmanInfo(const HuffTabParametrs &huff) {
    std::cout << std::string(100, '-') << std::endl;
    std::cout << "Huff table_ID = " << huff.table_id << std::endl;
    std::cout << "Huff table_class = " << huff.table_class << std::endl;
    std::cout << "Huff code count = " << huff.values.size() << std::endl;
    for (auto el : huff.values) {
        std::cout << static_cast<uint32_t>(el) << " ";
    }
    std::cout << std::endl;
    std::cout << std::string(100, '-') << std::endl;
}

template <typename T>
void PrintMatrix(const std::vector<std::vector<T>> &mat) {
    for (size_t y = 0; y < mat.size(); ++y) {
        for (size_t x = 0; x < mat[0].size(); ++x) {
            std::cout << mat[y][x] << ' ';
        }
        std::cout << std::endl;
    }
}