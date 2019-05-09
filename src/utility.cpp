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
    clhasher h(UINT64_C(0x23a23cf5033c3c81), UINT64_C(0xb3816f6a2c68e530));
    string   kStr = std::to_string(k);
    uint64_t temp = h(kStr);
    Byte     result;
    memcpy(&result, &temp, sizeof(Byte));
    return result;
}

bool PPointer::operator==(const PPointer p) const {
    if (this->fileId == p.fileId && this->offset == p.offset) {
        return true;
    } else {
        return false;
    }
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
    file.read((char*)&(t_p), sizeof(PPointer));
    return t_p;
}

int find_first_zero(Byte bitmap[], size_t n) {
    size_t i = 0, byte, len = (n + 7) / 8;
    while (i < len && bitmap[i] == 255) ++i;
    if (i == len) return -1;
    byte = bitmap[i], i *= 8;
    while (byte & 1) byte >>= 1, ++i;
    return i >= n ? -1 : i;
}

void set_bit(Byte bitmap[], size_t n) {
    bitmap[n / 8] |= 1 << (n % 8);
}

void clear_bit(Byte bitmap[], size_t n) {
    bitmap[n / 8] &= ~(1 << (n % 8));
}

void clear_bit_since(Byte bitmap[], size_t len, size_t n) {
    bitmap[n / 8] &= 255 >> (8 - n % 8);
    bzero(bitmap + n / 8 + 1, len - n / 8 - 1);
}

void clear_bit_until(Byte bitmap[], size_t /*len*/, size_t n) {
    bitmap[n / 8] &= 255 << (n % 8);
    bzero(bitmap, n / 8);
}

int get_bit(Byte bitmap[], size_t n) {
    return (bitmap[n / 8] >> (n % 8)) & 1;
}
