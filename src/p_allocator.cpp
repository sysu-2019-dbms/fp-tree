#include "utility/p_allocator.h"
#include <fstream>
#include <iostream>
using namespace std;
using namespace fp_tree;

const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath    = DATA_DIR + "free_list";

bool key_value::operator<(const key_value &b) const {
    return key < b.key;
}

bool leaf_group::valid(PPointer p) const {
    return p.offset >= 8 + LEAF_GROUP_AMOUNT &&
           p.offset < LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * sizeof(leaf);
}

const Byte &leaf_group::used(PPointer p) const {
    return bitmap[(p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf)];
}

Byte &leaf_group::used(PPointer p) {
    return bitmap[(p.offset - 8 - LEAF_GROUP_AMOUNT) / sizeof(leaf)];
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
        catalog = std::move(pmem_ptr<allocator_catalog>{catalogPath});

        pmem_stream freelist(freePath, sizeof(PPointer) * getFreeNum());
        for (uint64_t i = 0; i < getFreeNum(); i++) {
            PPointer p;
            freelist >> p;
            freeList.push_back(p);
        }
    } else {
        // simply create file
        pmem_ptr<empty_free_list> list(freePath);
        catalog = std::move(pmem_ptr<allocator_catalog>{catalogPath});
        catalog->maxFileId        = 1;
        catalog->freeNum          = 0;
        catalog->startLeaf.fileId = 0;
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
    for (uint64_t i = 1; i < getMaxFileId(); i++) {
        fId2PmAddr.emplace(i, pmem_ptr<leaf_group>(getLeafGroupFilePath(i)));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
leaf *PAllocator::getLeafPmemAddr(PPointer p) const {
    return fId2PmAddr.count(p.fileId) ? reinterpret_cast<leaf *>(fId2PmAddr.at(p.fileId).get_addr() + p.offset) : nullptr;
}

// get and use a leaf for the fptree leaf allocation
// return false if no free leaf is available
bool PAllocator::getLeaf(PPointer &p, char *&pmem_addr) {
    if (getFreeNum() == 0) {
        if (!newLeafGroup()) return false;
    }
    // need not to flush freelist, we just pop back
    p = freeList.back();
    freeList.pop_back();
    catalog->freeNum--;

    pmem_ptr<leaf_group> &group = fId2PmAddr[p.fileId];
    group->usedNum++;
    group->used(p) = 1;
    pmem_addr = group.get_addr() + p.offset;

    if (!catalog->startLeaf.fileId) {
        catalog->startLeaf = p;
    }
    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) const {
    return ifLeafExist(p) && fId2PmAddr.at(p.fileId)->used(p);
}

bool PAllocator::ifLeafFree(PPointer p) const {
    return ifLeafExist(p) && !fId2PmAddr.at(p.fileId)->used(p);
}

// judge whether the leaf with specific PPointer exists.
bool PAllocator::ifLeafExist(PPointer p) const {
    return fId2PmAddr.count(p.fileId) && fId2PmAddr.at(p.fileId)->valid(p);
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    if (ifLeafUsed(p)) {
        fId2PmAddr[p.fileId]->used(p) = 0;
        freeList.push_back(p);
        catalog->freeNum++;
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog() {
    pmem_stream freelist(freePath, sizeof(PPointer) * catalog->freeNum);
    if (!freelist) return false;
    for (uint64_t i = 0; i < catalog->freeNum; i++) {
        freelist << freeList[i];
    }
    return true;
}

// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    pmem_ptr<leaf_group> group(getLeafGroupFilePath(getMaxFileId()));
    if (!group) return false;
    for (size_t i = 0; i < LEAF_GROUP_AMOUNT; i++) {
        freeList.push_back((PPointer){getMaxFileId(), (uint64_t) & ((leaf_group *)0)->leaves[i]});
    }
    fId2PmAddr.emplace(getMaxFileId(), std::move(group));
    catalog->freeNum += LEAF_GROUP_AMOUNT;
    catalog->maxFileId++;
    return true;
}

string PAllocator::getLeafGroupFilePath(uint64_t fileId) const {
    return DATA_DIR + to_string(fileId);
}

pmem_ptr<leaf_group> &PAllocator::getLeafGroup(PPointer p) {
    return fId2PmAddr[p.fileId];
}

PPointer PAllocator::getStartPointer() const { return catalog->startLeaf; }
void     PAllocator::setStartPointer(PPointer startPointer) {
    catalog->startLeaf = startPointer;
}
uint64_t PAllocator::getMaxFileId() const { return catalog->maxFileId; }
uint64_t PAllocator::getFreeNum() const { return catalog->freeNum; }