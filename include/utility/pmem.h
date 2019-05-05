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
    pmem_stream();
    pmem_stream(pmem_stream const &) = delete;
    pmem_stream &operator=(pmem_stream const &) = delete;
    pmem_stream(const string &path, size_t len);
    ~pmem_stream();

    pmem_stream(pmem_stream &&b) noexcept;
    pmem_stream &operator=(pmem_stream &&b);

    void swap(pmem_stream &b) noexcept;

    void flush() const;

    void close();

    char *get_addr() const;

    explicit operator bool() const;

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
