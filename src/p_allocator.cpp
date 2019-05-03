#include "utility/p_allocator.h"
//#include "../include/utility/p_allocator.h"
#include <iostream>
using namespace std;
// the file that store the information of allocator
const string P_ALLOCATOR_CATALOG_NAME = "p_allocator_catalog";
// a list storing the free leaves
const string P_ALLOCATOR_FREE_LIST = "free_list";

PAllocator *PAllocator::pAllocator = new PAllocator();

PAllocator *PAllocator::getAllocator()
{
    if (PAllocator::pAllocator == NULL)
    {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

/* data storing structure of allocator
   In the catalog file, the data structure is listed below
   | maxFileId(8 bytes) | freeNum = m | treeStartLeaf(the PPointer) |
   In freeList file:
   | freeList{(fId, offset)1,...(fId, offset)m} |
*/
PAllocator::PAllocator()
{
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in | ios::binary);
    ifstream freeListFile(freeListPath, ios::in | ios::binary);
    // judge if the catalog exists
    char *pmemaddr;
	size_t mapped_len;
	int is_pmem;
    if (allocatorCatalog.is_open() && freeListFile.is_open())
    {
        // exist
        // allocatorCatalog.read((char *)&maxFileId, sizeof(uint64_t));
        // allocatorCatalog.read((char *)&freeNum, sizeof(uint64_t));
        // allocatorCatalog.read((char *)&startLeaf, sizeof(PPointer));
        
        pmemaddr=(char*)pmem_map_file(allocatorCatalogPath.c_str(), 
                            sizeof(u_int64_t)*2+sizeof(PPointer),
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
        maxFileId=*((uint64_t*)pmemaddr);
        pmemaddr+=8;
        freeNum=*((uint64_t*)pmemaddr);
        pmemaddr+=8;
        startLeaf=*((PPointer*)pmemaddr);

        pmemaddr=(char*)pmem_map_file(freeListPath.c_str(), 
                            sizeof(u_int64_t)*2*LEAF_DEGREE*100,
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
        freeList.clear();
        for (int i = 0; i < freeNum; i++)
        {
            // PPointer p;
            // freeListFile.read((char *)&p, sizeof(PPointer));
            PPointer p=*((PPointer*)pmemaddr);
            pmemaddr+=sizeof(PPointer);
            freeList.push_back(p);
        }
    }
    else
    {
        // ofstream ofcatalog;
        // ofcatalog.open(allocatorCatalogPath, ios::out | ios::binary);
        // ofstream offreelist;
        // offreelist.open(freeListPath, ios::out | ios::binary);
        //not exist, create catalog and free_list file, then open.
        pmem_map_file(allocatorCatalogPath.c_str(), 
                            sizeof(u_int64_t)*2+sizeof(PPointer),
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
        pmem_map_file(freeListPath.c_str(), 
                            sizeof(u_int64_t)*2*LEAF_DEGREE*100,
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator()
{
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    size_t mapped_len;
    int is_pmem;
    char *pmemaddr; 
    for (int i=1;i<maxFileId;i++){
        pmemaddr=(char *)pmem_map_file((DATA_DIR + std::to_string(i)).c_str(),
                                           8 + LEAF_GROUP_AMOUNT +
                                               LEAF_GROUP_AMOUNT * (sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE,
                                           PMEM_FILE_CREATE,
                                           0666,
                                           &mapped_len,
                                           &is_pmem);   
        pmem_unmap(pmemaddr,mapped_len);
    }
    pmemaddr=(char *)pmem_map_file(allocatorCatalogPath.c_str(), 
                            sizeof(u_int64_t)*2+sizeof(PPointer),
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
    pmem_unmap(pmemaddr,mapped_len);

    pmemaddr=(char *)pmem_map_file(freeListPath.c_str(), 
                            sizeof(u_int64_t)*2*LEAF_DEGREE*100,
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
    pmem_unmap(pmemaddr,mapped_len);
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr()
{
    fId2PmAddr.clear();
    size_t mapped_len;
    int is_pmem;
    char *pmemaddr;
    for (int i=1;i<maxFileId;i++){
        pmemaddr=(char *)pmem_map_file((DATA_DIR + std::to_string(maxFileId)).c_str(),
                                           8 + LEAF_GROUP_AMOUNT +
                                               LEAF_GROUP_AMOUNT * (sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE,
                                           PMEM_FILE_CREATE,
                                           0666,
                                           &mapped_len,
                                           &is_pmem);
        fId2PmAddr[i]=pmemaddr;
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char *PAllocator::getLeafPmemAddr(PPointer p)
{
    if (fId2PmAddr.count(p.fileId))
        return fId2PmAddr[p.fileId]+p.offset;
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return
bool PAllocator::getLeaf(PPointer &p, char *&pmem_addr)
{
    // TODO
    if (freeNum == 0)
    {
        if (!newLeafGroup()) return false; 
    }
    
    p = freeList.back();
    freeList.pop_back();
    freeNum--;
    pmem_addr = getLeafPmemAddr(p);
    if (pmem_addr != NULL) return true;
    
    return false;
}

bool PAllocator::ifLeafUsed(PPointer p)
{
    if (ifLeafExist(p))
    {
        int pos=(p.offset-8-LEAF_GROUP_AMOUNT)/ 
            ((sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE);
        if (*(fId2PmAddr[p.fileId]+8+pos)==1) return true;
    }
    return false;
}

bool PAllocator::ifLeafFree(PPointer p)
{
    if (ifLeafExist(p))
    {
        int pos=(p.offset-8-LEAF_GROUP_AMOUNT)/ 
            ((sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE);
        if (*(fId2PmAddr[p.fileId]+8+pos)==0) return true;
    }
    return false;
}

// judge whether the leaf with specific PPointer exists.
bool PAllocator::ifLeafExist(PPointer p)
{
    if (fId2PmAddr.count(p.fileId)&&p.offset>=8+LEAF_GROUP_AMOUNT&&
    p.offset<8 + LEAF_GROUP_AMOUNT +  LEAF_GROUP_AMOUNT* (sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE)
        return true;
    else
        return false;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p)
{
    if (ifLeafUsed(p))
    {
        freeList.push_back(p);
        freeNum++;
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog()
{
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    size_t mapped_len;
    int is_pmem;
    char *pmemaddr; 
    pmemaddr=(char*)pmem_map_file(allocatorCatalogPath.c_str(), 
                            sizeof(u_int64_t)*2+sizeof(PPointer),
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
    if (pmemaddr==NULL) return false;
    if (is_pmem)
		pmem_persist(pmemaddr, mapped_len);
	else
		pmem_msync(pmemaddr, mapped_len);

    pmemaddr=(char*)pmem_map_file(freeListPath.c_str(), 
                            sizeof(u_int64_t)*2*LEAF_DEGREE*100,
                            PMEM_FILE_CREATE,
				            0666,
                            &mapped_len,
                            &is_pmem);
    if (pmemaddr==NULL) return false;
    if (is_pmem)
		pmem_persist(pmemaddr, mapped_len);
	else
		pmem_msync(pmemaddr, mapped_len);
    return true;
}

/*
  Leaf group structure: (uncompressed)
  | usedNum(8b) | bitmap(n * byte) | leaf1 |...| leafn |
*/
// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup()
{
    // TODO
    size_t mapped_len;
    int is_pmem;
    char *pmemaddr = (char *)pmem_map_file((DATA_DIR + std::to_string(maxFileId)).c_str(),
                                           8 + LEAF_GROUP_AMOUNT +
                                               LEAF_GROUP_AMOUNT * (sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE,
                                           PMEM_FILE_CREATE,
                                           0666,
                                           &mapped_len,
                                           &is_pmem);
    for (int i = 0; i < LEAF_GROUP_AMOUNT; i++)
    {
        freeList.push_back((PPointer){maxFileId, 8 + LEAF_GROUP_AMOUNT + i * (sizeof(MAX_KEY) + sizeof(MAX_VALUE)) * LEAF_DEGREE});
    }
    fId2PmAddr[maxFileId] = pmemaddr;
    freeNum += LEAF_GROUP_AMOUNT;
    maxFileId++;
    return false;
}