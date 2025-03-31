#include <fft.h>

#include <fftw3.h>
#include <memory>
#include <cmath>
#include "structures.h"

class DctCalculator::Impl {
public:
    Impl(size_t width, std::vector<double> *input, std::vector<double> *output)
        : size_(width), input_(input) {
        if (input->size() != width * width || output->size() != width * width) {
            throw std::invalid_argument("Incorrect sizes");
        }

        plan_ = fftw_plan_r2r_2d(width, width, input->data(), output->data(), FFTW_REDFT01,
                                 FFTW_REDFT01, FFTW_ESTIMATE);
    }

    void Inverse() {
        for (size_t i = 0; i < size_; ++i) {
            (*input_)[i] *= std::sqrt(2);
            (*input_)[i * 8] *= std::sqrt(2);
        }
        for (auto &i : *input_) {
            i /= 16;
        }
        fftw_execute(plan_);
    }

    ~Impl() {
        fftw_destroy_plan(plan_);
    }

private:
    size_t size_;
    fftw_plan plan_;
    std::vector<double> *input_;
    std::unique_ptr<fftw_complex> res_;
};

DctCalculator::DctCalculator(size_t width, std::vector<double> *input, std::vector<double> *output)
    : impl_(std::make_unique<Impl>(width, input, output)) {
}

void DctCalculator::Inverse() {
    impl_->Inverse();
}

DctCalculator::~DctCalculator() = default;
