#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <vector>
#include <string>
#include <functional>

class BloomFilter {
public:
    BloomFilter(size_t size, size_t num_hashes);
    void add(const std::string& item);
    bool probably_contains(const std::string& item) const;

private:
    std::vector<bool> bits_;
    size_t num_hashes_;
    std::hash<std::string> hash_func_;

    size_t hash(const std::string& item, size_t index) const;
};

#endif // BLOOMFILTER_H

