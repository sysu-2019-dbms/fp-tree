#include <stdlib.h>
#include <string>

#ifndef UTILITY_VALUE

#define UTILITY_VALUE
#define MAX_DEGREE 256
#define MIN_DEGREE 2
#define LEAF_DEGREE 56
#define INNER_DEGREE 4096

#define MAX_KEY UINT64_MAX
#define MAX_VALUE UINT64_MAX

#define LEAF_GROUP_AMOUNT 16
#define ILLEGAL_FILE_ID   0

#endif

using std::string;

typedef unsigned char Byte;

typedef uint64_t  Key;    // key(8 byte)
typedef uint64_t  Value;  // value(8 byte)

//leaves file and pallocator data storing place
const string DATA_DIR =  FPTREE_DB_PATH; // TODO

// leaf header length, the bitmap is simply one byte for a leaf
const uint64_t LEAF_GROUP_HEAD = sizeof(uint64_t) + LEAF_GROUP_AMOUNT;

struct PPointer
{
    /* data */
    uint64_t fileId;
    uint64_t offset;

    bool operator==(const PPointer p) const;
} __attribute__((packed));

uint64_t calLeafSize();

uint64_t countOneBits(Byte b);

/**
 * \brief find first zero bit in bitmap.
 * \param len bit length of the bitmap.
 * \return first zero bit index or -1 if not found.
 */
int  find_first_zero(Byte bitmap[], size_t len);
void set_bit(Byte bitmap[], size_t n);
void clear_bit(Byte bitmap[], size_t n);
void clear_bit_since(Byte bitmap[], size_t len, size_t n);
void clear_bit_until(Byte bitmap[], size_t len, size_t n);

int get_bit(Byte bitmap[], size_t n);

Byte keyHash(Key k);

PPointer getPNext(PPointer p);