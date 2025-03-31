#include <decoder.h>
#include <glog/logging.h>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include "bitreader.h"
#include "markers.h"
#include "structures.h"
#include "marker_readers.h"
#include "image.h"
#include "huffman.h"
#include "fft.h"
#include "util_funcs.h"

std::optional<std::pair<int, int>> GetValueInTable(BitReader &reader, HuffTabParametrs &tab,
                                                   bool is_dc) {
    while (true) {
        int value;
        bool ok = tab.huffman.Move(reader.GetBit(), value);
        if (ok) {
            if (value == 0 && !is_dc) {
                return std::nullopt;
            } else if (value == 0 && is_dc) {
                return std::pair<int, int>{0, 0};
            }
            int cnt_zeros = value >> 4;
            int ans = 0;
            bool neg = false;
            for (size_t id = 0; id < (value & 15); ++id) {
                ans <<= 1;
                ans ^= (reader.GetBit()) ^ neg;
                if (id == 0 && ans == 0) {
                    neg = true;
                    ans = 1;
                }
            }
            if (neg) {
                ans *= -1;
            }
            return std::pair<int, int>{cnt_zeros, ans};
        }
    }
}

Matrix88 ExtractTable(BitReader &reader, HuffTabParametrs &dc_huff, HuffTabParametrs &ac_huff) {
    std::vector<int> seq;
    seq.reserve(64);
    for (bool is_first = true; seq.size() < 64; is_first = false) {
        auto pr = GetValueInTable(reader, is_first ? dc_huff : ac_huff, is_first);
        if (!pr.has_value()) {
            FillWithCnt(seq, {-1, -1});
            break;
        } else {
            FillWithCnt(seq, pr.value());
        }
    }
    if (seq.size() > 64) {
        throw std::runtime_error("Size of matrix exceeded 64 in ExtractTable");
    }
    return ZigZagConvert(seq);
}

void ReadEncodedData(BitReader &reader,
                     const std::unordered_map<size_t, FrameParametrs> &frames_pars,
                     const std::vector<QuantTable> &quant_tables,
                     std::vector<HuffTabParametrs> &huffman_tables, Image &image) {
    if (image.Height() == 0 || image.Width() == 0) {
        throw std::runtime_error("Empty jpeg");
    }

    size_t hor_sampling = frames_pars.at(1).hor_sampling;
    size_t vert_sampling = frames_pars.at(1).vert_sampling;
    if (!(hor_sampling >= 1 && hor_sampling <= 2 && vert_sampling >= 1 && vert_sampling <= 2)) {
        throw std::runtime_error("Bad sampling in ReadEncodedData");
    }
    if (!(frames_pars.size() == 1 || frames_pars.size() == 3)) {
        throw std::runtime_error("Bad frames count in ReadEncodedData");
    }
    if (!(huffman_tables.size() == 4 || huffman_tables.size() == 2)) {
        throw std::runtime_error("Bad huffman_tables count in ReadEncodedData");
    }

    if (frames_pars.size() == 3) {
        if (frames_pars.at(2).hor_sampling != 1 || frames_pars.at(2).vert_sampling != 1) {
            throw std::runtime_error("Bad sampling in Cb in ReadEncodedData");
        }
        if (frames_pars.at(3).hor_sampling != 1 || frames_pars.at(3).vert_sampling != 1) {
            throw std::runtime_error("Bad sampling in Cr in ReadEncodedData");
        }
    }

    std::vector<MCUBlock> image_blocks;

    size_t dc_y_pos;
    size_t ac_y_pos;
    size_t dc_ch_pos;
    size_t ac_ch_pos;
    for (size_t id = 0; id < huffman_tables.size(); ++id) {
        if (huffman_tables[id].table_id == 0) {
            if (huffman_tables[id].table_class == 0) {
                dc_y_pos = id;
            } else {
                ac_y_pos = id;
            }
        } else {
            if (huffman_tables[id].table_class == 0) {
                dc_ch_pos = id;
            } else {
                ac_ch_pos = id;
            }
        }
    }

    int pref_sum_dc = 0;
    int pref_sum_dc_sec[2] = {0, 0};
    std::vector<double> input(64);
    std::vector<double> output(64);
    DctCalculator dct_calc(8, &input, &output);

    do {
        MCUBlock block;
        for (size_t iter = 0; iter < hor_sampling * vert_sampling; ++iter) {
            auto mat = ExtractTable(reader, huffman_tables[dc_y_pos], huffman_tables[ac_y_pos]);
            pref_sum_dc += mat.Get(0, 0);
            mat.Get(0, 0) = pref_sum_dc;
            MultiplyMatrixByElements(mat, quant_tables[0].table);
            block.y_table.push_back(mat);
            ConvertMatrix(block.y_table.back(), dct_calc, &input, &output);
        }
        for (size_t iter = 0; iter < 2 && frames_pars.size() == 3; ++iter) {
            auto mat = ExtractTable(reader, huffman_tables[dc_ch_pos], huffman_tables[ac_ch_pos]);
            pref_sum_dc_sec[iter] += mat.Get(0, 0);
            mat.Get(0, 0) = pref_sum_dc_sec[iter];
            MultiplyMatrixByElements(mat, quant_tables[1].table);
            if (iter == 0) {
                block.cb_table = mat;
                ConvertMatrix(block.cb_table, dct_calc, &input, &output);
            } else {
                block.cr_table = mat;
                ConvertMatrix(block.cr_table, dct_calc, &input, &output);
            }
        }
        image_blocks.push_back(std::move(block));
    } while (!reader.CheckEndOfJpeg());

    size_t mcu_height = 8 * vert_sampling;
    size_t mcu_width = 8 * hor_sampling;

    size_t mcu_tab_width = (image.Width() + mcu_width - 1) / mcu_width;

    for (size_t y = 0; y < image.Height(); ++y) {
        for (size_t x = 0; x < image.Width(); ++x) {
            if (mcu_tab_width * (y / mcu_height) + x / mcu_width >= image_blocks.size()) {
                break;
            }
            const auto &mcu = image_blocks[mcu_tab_width * (y / mcu_height) + x / mcu_width];
            size_t pos_in_y_table = y % mcu_height / 8 * 2 + x % mcu_width / 8;
            if ((vert_sampling == 2) ^ (hor_sampling == 2)) {
                pos_in_y_table = y % mcu_height / 8 + x % mcu_width / 8;
            }
            double y_val = mcu.y_table[pos_in_y_table].Get(y % 8, x % 8);
            size_t pos_y = (y % mcu_height) / vert_sampling;
            size_t pos_x = (x % mcu_width) / hor_sampling;
            RGB pix;
            if (frames_pars.size() == 3) {
                pix = ConvertYCbCrToRGB(
                    YCbCr{y_val, mcu.cb_table.Get(pos_y, pos_x), mcu.cr_table.Get(pos_y, pos_x)});
            } else {
                pix = ConvertYCbCrToRGB(YCbCr{y_val, 128, 128});
            }
            image.SetPixel(y, x, pix);
        }
    }
}

Image Decode(std::istream &input) {
    std::unordered_map<size_t, FrameParametrs> frames_pars;
    std::vector<QuantTable> quant_tables;
    std::vector<HuffTabParametrs> huffman_tables;
    Image result;
    BitReader reader(input);

    {
        auto marker = IdentMarker(reader.GetDoubleByte());
        if (marker != JpegMarkers::SOI) {
            throw std::runtime_error("Bad jpeg, no SOI at start");
        }
    }

    bool was_sof0 = false;
    while (true) {
        uint16_t bts = reader.GetDoubleByte();
        auto marker = IdentMarker(bts);
        if (marker == JpegMarkers::NOTAMARKER) {
            throw std::runtime_error("Bad jpeg, unknown marker");
        }
        if (marker == JpegMarkers::SOI) {
            throw std::runtime_error("Bad jpeg, second SOI");
        }
        if (marker == JpegMarkers::SOF0) {
            if (was_sof0) {
                throw std::runtime_error("Second SOF0");
            }
            was_sof0 = true;
            uint16_t height;
            uint16_t width;
            frames_pars = ReadSOF0(reader, height, width);
            result.SetSize(width, height);
        }
        if (marker == JpegMarkers::SOF2) {
            throw std::runtime_error("Progressive type, not implemented");
        }
        if (marker == JpegMarkers::DHT) {
            for (HuffTabParametrs &tb : ReadDHT(reader)) {
                huffman_tables.push_back(std::move(tb));
            }
        }
        if (marker == JpegMarkers::DQT) {
            for (auto tb : ReadDQT(reader)) {
                quant_tables.push_back(tb);
            }
        }
        if (marker == JpegMarkers::DRI) {
            ReadDRI(reader);
        }
        if (marker == JpegMarkers::SOS) {
            ReadSOS(reader, frames_pars);
            ReadEncodedData(reader, frames_pars, quant_tables, huffman_tables, result);
            return result;
        }
        if (marker == JpegMarkers::RSTn) {
            ReadRSTn(reader);
        }
        if (marker == JpegMarkers::APPn) {
            ReadAPPn(reader);
        }
        if (marker == JpegMarkers::COM) {
            result.SetComment(ReadCOM(reader));
        }
        if (marker == JpegMarkers::EOI) {
            if (!input.eof()) {
                throw std::runtime_error("Bad jpeg, not empty tail");
            }
            return result;
        }
    }
}
