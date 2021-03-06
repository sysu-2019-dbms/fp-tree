#include "utility/utility.h"
#include <fstream>
#include <string>
#include "utility/clhash.h"

using namespace std;

uint64_t calLeafSize() {
    int n         = LEAF_DEGREE * 2;
    int bitArrNum = (n + 7) / 8;
    // Leaf : | bitmap | pNext | fingerprints array | KV array |
    uint64_t size = bitArrNum + sizeof(PPointer) + n * sizeof(Byte) + n * (sizeof(Key) + sizeof(Value));
    return size;
}

uint64_t countOneBits(Byte b) {
    return __builtin_popcount(b);
}

// func that generates the fingerprints
Byte keyHash(Key k) {
    static clhasher h(UINT64_C(0x23a23cf5033c3c81), UINT64_C(0xb3816f6a2c68e530));
    string          kStr = std::to_string(k);
    uint64_t        temp = h(kStr);
    Byte            result;
    memcpy(&result, &temp, sizeof(Byte));
    return result;
}

bool PPointer::operator==(const PPointer p) const {
    return this->fileId == p.fileId && this->offset == p.offset;
}

// get the pNext of the leaf in the leaf file
PPointer getPNext(PPointer p) {
    string   leafPath = DATA_DIR + to_string(p.fileId);
    ifstream file(leafPath.c_str(), ios::in | ios::binary);
    PPointer t_p;
    t_p.fileId = 0;
    t_p.offset = 0;
    if (!file.is_open()) {
        return t_p;
    }
    int len = (LEAF_DEGREE * 2 + 7) / 8 + p.offset;
    file.seekg(len, ios::beg);
    file.read((char *)&(t_p), sizeof(PPointer));
    return t_p;
}

int find_first_zero(Byte bitmap[], size_t n) {
    size_t i = 0, len = (n + 7) / 8;
    while (i < len && bitmap[i] == 255) ++i;
    if (i == len) return -1;
    i *= 8;
    while (get_bit(bitmap, i)) ++i;
    return i >= n ? -1 : i;
}

void set_bit(Byte bitmap[], size_t n) {
#if defined(__GNUC__) && defined(__x86_64__)
#define _X86_SETBIT_
    asm volatile("bts %1, %0"
                 : "=m"(*(volatile long *)bitmap)
                 : "Ir"(n));
#endif

#ifndef _X86_SETBIT_
    bitmap[n / 8] |= 1 << (n % 8);
#endif
}

void clear_bit(Byte bitmap[], size_t n) {
#if defined(__GNUC__) && defined(__x86_64__)
#define _X86_CLEARBIT_
    asm volatile("btr %1, %0"
                 : "=m"(*(volatile long *)bitmap)
                 : "Ir"(n));
#endif

#ifndef _X86_CLEARBIT_
    bitmap[n / 8] |= 1 << (n % 8);
#endif
}


int get_bit(Byte bitmap[], size_t n) {
#if defined(__GNUC__) && defined(__x86_64__)
#define _X86_GETBIT_
    int oldbit;
    asm volatile("bt %2, %1; sbbl %0,%0"
                 : "=r"(oldbit)
                 : "m"(*(volatile long *)bitmap), "Ir"(n));
    return oldbit != 0;
#endif

#ifndef _X86_GETBIT_
    return (bitmap[n / 8] >> (n % 8)) & 1;
#endif
}


void clear_bit_since(Byte bitmap[], size_t len, size_t n) {
    bitmap[n / 8] &= 255 >> (8 - n % 8);
    bzero(bitmap + n / 8 + 1, len - n / 8 - 1);
}

void clear_bit_until(Byte bitmap[], size_t /*len*/, size_t n) {
    bitmap[n / 8] &= 255 << (n % 8);
    bzero(bitmap, n / 8);
}
