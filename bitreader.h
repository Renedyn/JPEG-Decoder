#pragma once

#include <istream>
#include <stdexcept>
#include <glog/logging.h>

class BitReader {
public:
    BitReader() = delete;
    BitReader(std::istream& inp) : inp_(inp), bit_pos_(0) {
    }

    bool GetBit() {
        if (bit_pos_ % 8 == 0) {
            if (inp_.eof()) {
                throw std::runtime_error("EOF in GetBit");
            }
            last_byte_ = inp_.get();
            if (last_byte_ == 0xff) {
                if (inp_.eof()) {
                    throw std::runtime_error("EOF after 0xFF in GetBit");
                }
                if (inp_.peek() == 0) {
                    inp_.get();
                    bit_pos_ += 8;  // Will cause strange behavior
                } else if (inp_.peek() == 0xd9) {
                    throw std::runtime_error("0xFFD9 in GetBit");
                } else {
                    throw std::runtime_error("Bad byte after 0xFF in GetBit");
                }
            }
        }
        bool res = last_byte_ & (1u << (7 - bit_pos_ % 8));
        ++bit_pos_;
        return res;
    }

    bool CheckEndOfJpeg() {
        if (inp_.eof()) {
            throw std::runtime_error("EOF in CheckEndOfJpeg");
        }
        if (inp_.peek() == 0xff) {
            inp_.get();
            if (inp_.eof()) {
                throw std::runtime_error("EOF in CheckEndOfJpeg");
            }
            auto byte = inp_.peek();
            inp_.putback(static_cast<char>(0xff));
            return byte == 0xd9;
        } else {
            return false;
        }
    }

    uint8_t GetByte() {
        if (bit_pos_ % 8 != 0) {
            throw std::runtime_error("Bad bit pos in GetByte");
        }
        if (inp_.eof()) {
            throw std::runtime_error("EOF in GetByte");
        }
        last_byte_ = inp_.get();
        bit_pos_ += 8;
        return last_byte_;
    }

    uint8_t CheckLastByte() {
        return last_byte_;
    }

    uint16_t GetDoubleByte() {
        if (bit_pos_ % 8 != 0) {
            throw std::runtime_error("Bad bit pos in GetDoubleByte");
        }
        uint16_t res = 0;
        for (int cn = 0; cn < 2; ++cn) {
            if (inp_.eof()) {
                throw std::runtime_error("EOF in GetByte");
            }
            res <<= 8;
            last_byte_ = inp_.get();
            bit_pos_ += 8;
            res ^= last_byte_;
        }
        return res;
    }

    size_t GetCurPos() {
        return bit_pos_;
    }

private:
    std::istream& inp_;
    size_t bit_pos_;
    unsigned char last_byte_;
};