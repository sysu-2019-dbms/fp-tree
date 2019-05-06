#include "utility/pmem.h"
#include <libpmem.h>
#include <algorithm>

namespace fp_tree {
using namespace std;

pmem_stream::pmem_stream() : addr(nullptr), cur(nullptr) {}

pmem_stream::pmem_stream(const string &path, size_t len) : path(path) {
    addr = (char *)pmem_map_file(path.c_str(), len, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
    cur  = addr;
}

pmem_stream::pmem_stream(pmem_stream &&b) noexcept : addr(nullptr), cur(nullptr) {
    swap(b);
}

pmem_stream::~pmem_stream() {
    close();
}

pmem_stream &pmem_stream::operator=(pmem_stream &&b) noexcept {
    swap(b);
    return *this;
}

void pmem_stream::swap(pmem_stream &b) noexcept {
    std::swap(path, b.path);
    std::swap(addr, b.addr);
    std::swap(cur, b.cur);
    std::swap(mapped_len, b.mapped_len);
    std::swap(is_pmem, b.is_pmem);
}

void pmem_stream::flush(void *addr, size_t len) const {
    if (is_pmem)
        pmem_persist(addr, len);
    else
        pmem_msync(addr, len);
}

void pmem_stream::flush() const {
    if (!addr) return;
    flush(addr, mapped_len);
}

void pmem_stream::close() {
    if (!addr) return;
    flush();
    pmem_unmap(addr, mapped_len);
}

char *pmem_stream::get_addr() const {
    return addr;
}

pmem_stream::operator bool() const {
    return bool(addr);
}

}  // namespace fp_tree
