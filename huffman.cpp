#include "include/huffman.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>

class HuffmanTree::Impl {
public:
    Impl() : nodes_(1, {UINT32_MAX, 0, 0}), cur_vert_(0) {
    }

    Impl(std::vector<uint8_t> code_lengths, const std::vector<uint8_t> &values) : cur_vert_(0) {
        Build(code_lengths, values);
    }

    void Build(std::vector<uint8_t> code_lengths, const std::vector<uint8_t> &values) {
        cur_vert_ = 0;
        if (code_lengths.size() > 16) {
            throw std::invalid_argument("Too deep");
        }
        if (std::accumulate(code_lengths.begin(), code_lengths.end(), static_cast<size_t>(0)) !=
            values.size()) {
            throw std::invalid_argument("values.size() != sum(code_length)");
        }
        nodes_.push_back({UINT32_MAX, 0, 0});
        int cur_pos = 0;
        DfsBuild(0, 0, cur_pos, code_lengths, values);
        if (std::accumulate(code_lengths.begin(), code_lengths.end(), static_cast<size_t>(0)) !=
            0) {
            throw std::invalid_argument("Bad code_length");
        }
    }

    bool Move(bool bit, int &value) {
        cur_vert_ = bit ? nodes_[cur_vert_].right : nodes_[cur_vert_].left;
        if (cur_vert_ == 0) {
            throw std::invalid_argument("Moved to empty leaf");
        }
        if (nodes_[cur_vert_].val != UINT32_MAX) {
            value = nodes_[cur_vert_].val;
            cur_vert_ = 0;
            return true;
        }
        return false;
    }

private:
    void DfsBuild(int vert, int hei, int &pos, std::vector<uint8_t> &code_lengths,
                  const std::vector<uint8_t> &values) {
        if (hei - 1 >= static_cast<int>(code_lengths.size())) {
            return;
        }
        if (hei > 0 && code_lengths[hei - 1] > 0) {
            nodes_[vert].val = values[pos++];
            --code_lengths[hei - 1];
            return;
        }
        nodes_[vert].left = nodes_.size();
        nodes_.push_back({UINT32_MAX, 0, 0});
        DfsBuild(nodes_[vert].left, hei + 1, pos, code_lengths, values);

        nodes_[vert].right = nodes_.size();
        nodes_.push_back({UINT32_MAX, 0, 0});
        DfsBuild(nodes_[vert].right, hei + 1, pos, code_lengths, values);
    }

    struct Node {
        uint32_t val;
        uint32_t left;
        uint32_t right;
    };
    std::vector<Node> nodes_;
    size_t cur_vert_;
};

HuffmanTree::HuffmanTree() {
    impl_ = std::make_unique<Impl>();
}

void HuffmanTree::Build(const std::vector<uint8_t> &code_lengths,
                        const std::vector<uint8_t> &values) {
    impl_ = std::make_unique<Impl>(code_lengths, values);
}

bool HuffmanTree::Move(bool bit, int &value) {
    return impl_->Move(bit, value);
}

HuffmanTree::HuffmanTree(HuffmanTree &&) = default;

HuffmanTree &HuffmanTree::operator=(HuffmanTree &&) = default;

HuffmanTree::~HuffmanTree() = default;
