#include <libpmem.h>
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>

namespace fp_tree {
using namespace std;

class pmem_stream {
protected:
    char * addr;
    char * cur;
    size_t mapped_len;
    int    is_pmem;
    string path;

public:
    pmem_stream() : addr(nullptr), cur(nullptr) {}
    pmem_stream(pmem_stream const &) = delete;
    pmem_stream &operator=(pmem_stream const &) = delete;

    pmem_stream(const string &path, size_t len) : path(path) {
        addr = (char *)pmem_map_file(path.c_str(), len, PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        cur  = addr;
    }

    pmem_stream(pmem_stream &&b) noexcept : addr(nullptr), cur(nullptr) {
        swap(b);
    }

    pmem_stream &operator=(pmem_stream &&b) noexcept {
        swap(b);
        return *this;
    }

    void swap(pmem_stream &b) noexcept {
        std::swap(path, b.path);
        std::swap(addr, b.addr);
        std::swap(cur, b.cur);
        std::swap(mapped_len, b.mapped_len);
        std::swap(is_pmem, b.is_pmem);
    }

    ~pmem_stream() {
        close();
    }

    void flush() const {
        if (!addr) return;
        if (is_pmem)
            pmem_persist(addr, mapped_len);
        else
            pmem_msync(addr, mapped_len);
    }

    void close() {
        if (!addr) return;
        flush();
        pmem_unmap(addr, mapped_len);
    }

    char *get_addr() const {
        return addr;
    }

    explicit operator bool() const {
        return bool(addr);
    }

    template <typename T>
    T peek() const {
        return *((T *)cur);
    }

    template <typename T>
    pmem_stream &operator<<(const T &value) {
        *((T *)cur) = value;
        cur += sizeof(T);
        return *this;
    }

    template <typename T>
    pmem_stream &operator>>(T &value) {
        value = *((T *)cur);
        cur += sizeof(T);
        return *this;
    }
};

template <typename T>
class pmem_ptr : public pmem_stream {
public:
    pmem_ptr() : pmem_stream() {}
    explicit pmem_ptr(const string &path) : pmem_stream(path, sizeof(T)) {}

    T &operator*() { return *((T *)addr); }
    T *operator->() { return (T *)addr; }
};

}  // namespace fp_tree