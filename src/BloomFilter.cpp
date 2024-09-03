#include "BloomFilter.h"

BloomFilter::BloomFilter(size_t size, size_t num_hashes)
    : bits_(size, false), num_hashes_(num_hashes) {}

void BloomFilter::add(const std::string& item) {
    for (size_t i = 0; i < num_hashes_; ++i) {
        bits_[hash(item, i)] = true;
    }
}

bool BloomFilter::probably_contains(const std::string& item) const {
    for (size_t i = 0; i < num_hashes_; ++i) {
        if (!bits_[hash(item, i)]) {
            return false;
        }
    }
    return true;
}

size_t BloomFilter::hash(const std::string& item, size_t index) const {
    return (hash_func_(item) + index) % bits_.size();
}
