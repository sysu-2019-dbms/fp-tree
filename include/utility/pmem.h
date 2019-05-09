#include <string>
#include <iostream>
#include <algorithm>

namespace fp_tree {
using namespace std;

class pmem_stream {
protected:
    char * addr;
    char * cur;
    size_t mapped_len;
    int    is_pmem;
    string path;

    void map(size_t len);
public:
    pmem_stream();
    pmem_stream(pmem_stream const &) = delete;
    pmem_stream &operator=(pmem_stream const &) = delete;
    pmem_stream(const string &path, size_t len);
    ~pmem_stream();

    pmem_stream(pmem_stream &&b) noexcept;
    pmem_stream &operator=(pmem_stream &&b) noexcept;

    void swap(pmem_stream &b) noexcept;
    void flush(const void *addr, size_t len) const;
    void flush() const;

    void close();

    char *get_addr() const;

    explicit operator bool() const;

    void seekg(std::streampos pos, std::ios::seekdir dir);

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

    T &operator*() const { return *((T *)addr); }
    T *operator->() const { return (T *)addr; }

    // Partially flush
    template <typename V>
    void flush_part(const V *addr) {
        flush(addr, sizeof(V));
    }

    template <typename V, size_t size>
    void flush_part(const V (&arr)[size]) {
        flush(arr, sizeof(V) * size);
    }

    template <typename V>
    void modify(V *addr, V&& newValue) {
        *addr = newValue;
        flush_part(addr);
    }
};

template <typename T>
class pmem_stack : pmem_stream {
    size_t sz;
public:
    pmem_stack() : pmem_stream() {}
    explicit pmem_stack(const string &path, size_t initial_num)
        : pmem_stream(path, std::max(sizeof(T) * initial_num * 2, 10UL)), sz(initial_num) {}

    void push(const T& element) {
        if (mapped_len < (sz + 1) * sizeof(T)) {
            close();
            map((sz + 1) * sizeof(T) * 2);
        }

        reinterpret_cast<T*>(addr)[sz] = element;
        flush(addr + sz * sizeof(T), sizeof(T));
        ++sz;
    }

    T &back() const {
        return reinterpret_cast<T*>(addr)[sz - 1];
    }

    void pop() {
        --sz;
    }
};

}  // namespace fp_tree
