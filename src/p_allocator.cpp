#include "utility/p_allocator.h"
#include <fstream>
using namespace std;
using namespace fp_tree;

const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath    = DATA_DIR + "free_list";

bool key_value::operator<(const key_value &b) const {
    return key < b.key;
}

bool leaf_group::valid(PPointer p) {
    return p.offset >= 8 + LEAF_GROUP_AMOUNT &&
           p.offset < LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * sizeof(leaf);
}

Byte &leaf_group::used(PPointer p) {
    return bitmap[(p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf)];
}

leaf &leaf_group::get_leaf(char *pmemaddr, PPointer p) {
    leaf_group *group = (leaf_group *)pmemaddr;
    return group->leaves[(p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf)];
}

PAllocator *PAllocator::pAllocator = new PAllocator();

PAllocator *PAllocator::getAllocator() {
    if (PAllocator::pAllocator == NULL) {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

PAllocator::PAllocator() {
    ifstream allocatorCatalog(catalogPath, ios::in | ios::binary);
    ifstream freeListFile(freePath, ios::in | ios::binary);
    // judge if the catalog exists
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        allocatorCatalog.close();
        freeListFile.close();
        pmem_ptr<allocator_catalog> catalog(catalogPath);
        maxFileId = catalog->maxFileId;
        freeNum   = catalog->freeNum;
        startLeaf = catalog->startLeaf;

        pmem_stream freelist(freePath, sizeof(PPointer) * freeNum);
        for (uint64_t i = 0; i < freeNum; i++) {
            PPointer p;
            freelist >> p;
            freeList.push_back(p);
        }
    } else {
        // simply create file
        pmem_ptr<allocator_catalog> catalog(catalogPath);
        pmem_ptr<empty_free_list>   list(freePath);
        maxFileId = 1;
        freeNum   = 0;
    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
    persistCatalog();
    PAllocator::pAllocator = nullptr;
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() {
    fId2PmAddr.clear();
    for (uint64_t i = 1; i < maxFileId; i++) {
        fId2PmAddr.emplace(i, pmem_ptr<leaf_group>(getLeafGroupFilePath(i)));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char *PAllocator::getLeafPmemAddr(PPointer p) {
    return fId2PmAddr.count(p.fileId) ? fId2PmAddr[p.fileId].get_addr() + p.offset : nullptr;
}

// get and use a leaf for the fptree leaf allocation
// return false if no free leaf is available
bool PAllocator::getLeaf(PPointer &p, char *&pmem_addr) {
    if (freeNum == 0) {
        if (!newLeafGroup()) return false;
    }
    p = freeList.back();
    freeList.pop_back();
    freeNum--;

    pmem_ptr<leaf_group> &group = fId2PmAddr[p.fileId];
    group->usedNum++;
    group->used(p) = 1;
    group.flush();
    pmem_addr = group.get_addr();

    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) {
    return ifLeafExist(p) && fId2PmAddr[p.fileId]->used(p);
}

bool PAllocator::ifLeafFree(PPointer p) {
    return ifLeafExist(p) && !fId2PmAddr[p.fileId]->used(p);
}

// judge whether the leaf with specific PPointer exists.
bool PAllocator::ifLeafExist(PPointer p) {
    return fId2PmAddr.count(p.fileId) && fId2PmAddr[p.fileId]->valid(p);
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    if (ifLeafUsed(p)) {
        fId2PmAddr[p.fileId]->used(p) = 0;
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

    pmem_stream freelist(freePath, sizeof(PPointer) * freeNum);
    if (!freelist) return false;
    for (uint64_t i = 0; i < freeNum; i++) {
        freelist << freeList[i];
    }
    return true;
}

// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    pmem_ptr<leaf_group> group(getLeafGroupFilePath(maxFileId));
    if (!group) return false;
    for (size_t i = 0; i < LEAF_GROUP_AMOUNT; i++) {
        freeList.push_back((PPointer){maxFileId, (uint64_t) & ((leaf_group *)0)->leaves[i]});
    }
    fId2PmAddr.emplace(maxFileId, std::move(group));
    freeNum += LEAF_GROUP_AMOUNT;
    maxFileId++;
    return true;
}

string PAllocator::getLeafGroupFilePath(uint64_t fileId) {
    return DATA_DIR + to_string(fileId);
}

pmem_ptr<leaf_group> &PAllocator::getLeafGroup(PPointer p) {
    return fId2PmAddr[p.fileId];
}