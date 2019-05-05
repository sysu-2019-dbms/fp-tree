#include <map>
#include <vector>
#include "utility/pmem.h"
#include "utility/utility.h"

using std::map;
using std::string;
using std::vector;

struct allocator_catalog {
    uint64_t maxFileId;
    uint64_t freeNum;
    PPointer startLeaf;
} __attribute__((packed));

struct empty_free_list {
} __attribute__((packed));

struct key_value {
    Key   key;
    Value value;
} __attribute__((packed));

struct leaf {
    Byte      bitmap[(LEAF_DEGREE * 2 + 7) / 8];
    PPointer  pNext;
    Byte      fingerprints[LEAF_DEGREE * 2];
    key_value kv[LEAF_DEGREE * 2];
} __attribute__((packed));

struct leaf_group {
    uint64_t usedNum;
    Byte     bitmap[LEAF_GROUP_AMOUNT];
    leaf     leaves[LEAF_GROUP_AMOUNT];

    bool  valid(PPointer p);
    Byte& used(PPointer p);
} __attribute__((packed));

// Use this to allocate or free a leaf node in NVM
class PAllocator {
private:
    static PAllocator* pAllocator;  // singleton
    PPointer           startLeaf;   // first leaf's PPointer of fptree
    uint64_t           maxFileId;   // current fileId not used
    uint64_t           freeNum;     // free leaves amount
    vector<PPointer>   freeList;    // leaves list: the leaf that has been allocatored but is free

    map<uint64_t, fp_tree::pmem_ptr<leaf_group>> fId2PmAddr;  // the map of fileId to pmem address

    void initFilePmemAddr();  // initial the fId2PmAddr
public:
    static PAllocator* getAllocator();  // singleton pattern

    PAllocator();
    ~PAllocator();

    char* getLeafPmemAddr(PPointer p);
    bool  getLeaf(PPointer& p, char*& pmem_addr);  // get and use a free leaf
    bool  freeLeaf(PPointer p);                    // free the used leaf
    bool  newLeafGroup();                          // allocate a new group of leaves
    bool  ifLeafUsed(PPointer p);                  // judge whether the leaf is used
    bool  ifLeafFree(PPointer p);                  // judge whether the leaf is free
    bool  ifLeafExist(PPointer p);                 // judge whether the leaf exists

    bool persistCatalog();  // persist the catalog file in NVM/SSD

    PPointer getUsedLeaf(int idx);
    PPointer getFreeLeaf(int idx);
    PPointer getStartPointer() { return this->startLeaf; }
    uint64_t getMaxFileId() { return this->maxFileId; }
    uint64_t getFreeNum() { return this->freeNum; }
};
