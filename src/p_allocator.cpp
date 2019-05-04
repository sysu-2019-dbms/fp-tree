#include "utility/p_allocator.h"
#include <iostream>
using namespace std;
using namespace fp_tree;

const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath    = DATA_DIR + "free_list";

struct allocator_catalog {
    uint64_t maxFileId;
    uint64_t freeNum;
    PPointer startLeaf;
} __attribute__((packed));

struct free_list {
    PPointer list[LEAF_DEGREE * 100];
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
} __attribute__((packed));

PAllocator *PAllocator::pAllocator = new PAllocator();

PAllocator *PAllocator::getAllocator() {
    if (PAllocator::pAllocator == NULL) {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

#define UPDATE_CATALOG(catalog)          \
    do {                                 \
        maxFileId = (catalog).maxFileId; \
        freeNum   = (catalog).freeNum;   \
        startLeaf = (catalog).startLeaf; \
    } while (0)

/* data storing structure of allocator
   In the catalog file, the data structure is listed below
   | maxFileId(8 bytes) | freeNum = m | treeStartLeaf(the PPointer) |
   In freeList file:
   | freeList{(fId, offset)1,...(fId, offset)m} |
*/
PAllocator::PAllocator() {
    ifstream allocatorCatalog(catalogPath, ios::in | ios::binary);
    ifstream freeListFile(freePath, ios::in | ios::binary);
    // judge if the catalog exists
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        pmem_ptr<allocator_catalog> catalog(catalogPath);
        UPDATE_CATALOG(*catalog);

        pmem_ptr<free_list> freelist(freePath);
        freeList.clear();
        for (uint64_t i = 0; i < freeNum; i++) {
            freeList.push_back(freelist->list[i]);
        }
    } else {
        // simply create file
        pmem_stream(catalogPath, sizeof(allocator_catalog));
        pmem_stream(freePath, sizeof(free_list));
        maxFileId = 1;
        freeNum   = 0;
        freeList.clear();
    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
    persistCatalog();
    fId2PmAddr.clear();
    freeList.clear();
    maxFileId              = 1;
    freeNum                = 0;
    PAllocator::pAllocator = nullptr;
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() {
    fId2PmAddr.clear();
    for (uint64_t i = 1; i < maxFileId; i++) {
        pmem_ptr<leaf_group> pmem(DATA_DIR + to_string(maxFileId));
        fId2PmAddr.emplace(i, std::move(pmem));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char *PAllocator::getLeafPmemAddr(PPointer p) {
    if (fId2PmAddr.count(p.fileId)) return fId2PmAddr[p.fileId].get_addr() + p.offset;
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return
bool PAllocator::getLeaf(PPointer &p, char *&pmem_addr) {
    if (freeNum == 0) {
        if (!newLeafGroup()) return false;
    }
    p = freeList.back();
    freeList.pop_back();
    freeNum--;

    pmem_ptr<leaf_group> group(DATA_DIR + to_string(p.fileId));
    if (!group) return false;
    group->usedNum++;
    int pos            = (p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf);
    group->bitmap[pos] = 1;

    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) {
    if (ifLeafExist(p)) {
        int pos = (p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf);
        if (*(fId2PmAddr[p.fileId].get_addr() + 8 + pos) == 1) return true;
    }
    return false;
}

bool PAllocator::ifLeafFree(PPointer p) {
    if (ifLeafExist(p)) {
        int pos = (p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf);
        if (*(fId2PmAddr[p.fileId].get_addr() + 8 + pos) == 0) return true;
    }
    return false;
}

// judge whether the leaf with specific PPointer exists.
bool PAllocator::ifLeafExist(PPointer p) {
    if (fId2PmAddr.count(p.fileId) && p.offset >= 8 + LEAF_GROUP_AMOUNT &&
        p.offset < LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * sizeof(leaf))
        return true;
    else
        return false;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    if (ifLeafUsed(p)) {
        int pos = (p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf);

        *(fId2PmAddr[p.fileId].get_addr() + 8 + pos) = 0;
        freeList.push_back(p);
        freeNum++;
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog() {
    pmem_ptr<allocator_catalog> catalog(catalogPath);
    if (!catalog) return false;
    *catalog = (allocator_catalog){maxFileId, freeNum, startLeaf};

    pmem_ptr<free_list> freelist(freePath);
    if (!freelist) return false;
    for (uint64_t i = 0; i < freeNum; i++) {
        PPointer p        = freeList[i];
        freelist->list[i] = p;
    }
    return true;
}

// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    pmem_ptr<leaf_group> group(DATA_DIR + to_string(maxFileId));
    if (!group) return false;
    for (size_t i = 0; i < LEAF_GROUP_AMOUNT; i++) {
        freeList.push_back((PPointer){maxFileId, LEAF_GROUP_HEAD + i * sizeof(leaf)});
    }
    fId2PmAddr.emplace(maxFileId, std::move(group));
    freeNum += LEAF_GROUP_AMOUNT;
    maxFileId++;
    return true;
}