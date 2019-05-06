#include<memory.h>
#include<iostream>
#include<stdlib.h>
#include<queue>

#include"utility/p_allocator.h"

// In Mac C++, it is little-endian
// 0x12345677 --> 78 56 34 12

typedef struct t_KeyNode KeyNode;
typedef struct t_NodeLevel NodeLevel;

class FPTree;
class InnerNode;
class LeafNode;

class Node {
protected:
    friend class FPTree;
    
    FPTree* tree;     // the tree that the node belongs to
    int     degree;   // the degree of the node
    bool    isLeaf;   // judge whether the node is leaf

public:
    Node(FPTree *tree, bool isLeaf);
    virtual ~Node() {}

    FPTree* getTree() { return tree; }

    bool    ifLeaf() { return isLeaf; }

    virtual KeyNode insert(const Key& k, const Value& v) = 0;
    virtual KeyNode split() = 0;
    virtual bool remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) = 0;
    virtual bool update(const Key& k, const Value& v) = 0;
    virtual Value find(const Key& k) = 0;

    virtual void printNode() = 0;
};

// used for node's recursive insertion and split
// only allocated in func split()
typedef struct t_KeyNode {
    Key key;
    Node* node;
} KeyNode;

// used by print func
typedef struct t_NodeLevel {
    Node* node;
    int level;
} NodeLevel;

// 
typedef struct t_KeyValue {
    Key k;
    Value v;
} KeyValue;

/*
<<struct of the InnerNode>>
---------------------------------------------------------
| nKeys | Keys = {k1,...,kn-1} | Children = {c1,...,cn} |
---------------------------------------------------------
*/
class InnerNode : public Node {
private:
    friend class FPTree;

    bool   isRoot;     // judge whether the node is root
    int    n;          // amount of children
    Key*   keys;       // max (2 * d + 1) keys
    Node** childrens;  // max (2 * d + 2) node pointers

    int findIndex(const Key& k);

    void getBrother(const int& index, InnerNode* const& parent, InnerNode* &leftBro, InnerNode* &rightBro);
    void redistributeRight(const int& index, InnerNode* const& rightBro, InnerNode* const& parent);
    void redistributeLeft(const int& index, InnerNode* const& leftBro, InnerNode* const& parent);

    void mergeParentRight(InnerNode* const& parent, InnerNode* const& rightBro);
    void mergeParentLeft(InnerNode* const& parent, InnerNode* const& leftBro);

    void mergeLeft(InnerNode* const& LeftBro, const Key& k);
    void mergeRight(InnerNode* const& rightBro, const Key& k);
public:

    InnerNode(const int& d, FPTree* const& tree, bool _ifRoot = false);
    ~InnerNode();

    KeyNode  insert(const Key& k, const Value& v);
    void     insertNonFull(const Key& k, Node* const& node);
    KeyNode  insertLeaf(const KeyNode& leaf);
    bool     remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete);
    bool     update(const Key& k, const Value& v);
    Value    find(const Key& k);
    
    KeyNode  split();
    void     removeChild(const int& KeyIdx, const int& childIdx);

    Node*    getChild(const int& idx);
    Key      getKey(const int& idx);
    int      getKeyNum() { return this->n - 1; }
    int      getChildNum() { return this->n; }
    bool     getIsRoot() { return this->isRoot; }
    void     printNode();

};

/*
-------------------------------------------------------------
| bitmap | pNext | fingerprints | KV = [(k1,v1),...(kn,vn)] | 
-------------------------------------------------------------
*/
// LeafNode structure is the leaf node in NVM that is buffered in the DRAM.
class LeafNode : public Node{
private:
    friend class FPTree;
    friend class InnerNode;

    // the NVM relative variables
    char*      pmem_addr;      // the pmem address of the leaf node

    // the pointer below are all pmem address based on pmem_addr
    // need to set the pointer pointed to NVM address
    leaf *pmem;

    fp_tree::pmem_ptr<leaf_group> &get_pmem_ptr();

    // the DRAM relative variables
    int        n;              // amount of entries
    LeafNode*  prev;           // the address of previous leafnode      
    LeafNode*  next;           // the address of next leafnode  
    PPointer   pPointer;        // the persistent pointer pointed to the leaf in NVM
    string     filePath;        // the file path of the leaf
    
    uint64_t   bitmapSize;      // the bitmap size of the leaf(bytes)

public:
    LeafNode(FPTree* tree);                // allocate a new leaf
    LeafNode(PPointer p, FPTree* t);       // read a leaf from NVM/SSD
    ~LeafNode();

    KeyNode     insert(const Key& k, const Value& v);
    void        insertNonFull(const Key& k, const Value& v);
    bool        remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete);
    bool        update(const Key& k, const Value& v);
    Value       find(const Key& k);

    // used by insert()
    KeyNode     split();
    Key         findSplitKey();

    void        printNode();

    int         findFirstZero();
    int         getBit(int idx);
    Key         getKey(int idx);
    Value       getValue(int idx);
    PPointer    getPPointer();

    // interface with NVM
    void        persist();
};

class FPTree {
private:
    friend class InnerNode;
    InnerNode* root;
    uint64_t   degree;

    void recursiveDelete(Node* n);
public:
    FPTree(uint64_t degree);
    ~FPTree();

    void       insert(Key k, Value v);
    bool       remove(Key k);
    bool       update(Key k, Value v);
    Value      find(Key k);
    LeafNode*  findLeaf(Key K);

    InnerNode* getRoot();
    void       changeRoot(InnerNode* newRoot);
    void       printTree();

    bool       bulkLoading();
};