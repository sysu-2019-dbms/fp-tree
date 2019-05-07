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

    FPTree* getTree() const { return tree; }

    bool    ifLeaf() const { return isLeaf; }

    virtual KeyNode insert(const Key& k, const Value& v) = 0;
    virtual KeyNode split() = 0;
    virtual bool remove(const Key& k, int index, InnerNode* parent, bool &ifDelete) = 0;
    virtual bool update(const Key& k, const Value& v) = 0;
    virtual Value find(const Key& k) const = 0;
    virtual Key  getMinKey() const = 0;

    virtual void printNode() const = 0;
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
| nKeys | Keys = {k1,...,kn} | Children = {c1,...,cn} |
---------------------------------------------------------
*/
class InnerNode : public Node {
private:
    friend class FPTree;

    bool   isRoot;     // judge whether the node is root
    int    n;          // amount of children
    Key*   keys;       // max (2 * d + 2) keys
    Node** childrens;  // max (2 * d + 2) node pointers

    int findIndex(const Key& k) const;

    void getBrother(int index, InnerNode* parent, InnerNode* &leftBro, InnerNode* &rightBro);
    void redistributeRight(int index, InnerNode* rightBro, InnerNode* parent);
    void redistributeLeft(int index, InnerNode* leftBro, InnerNode* parent);

    void mergeParentRight(InnerNode* parent, InnerNode* rightBro);
    void mergeParentLeft(InnerNode* parent, InnerNode* leftBro);

    void mergeLeft(InnerNode* LeftBro, const Key& k);
    void mergeRight(InnerNode* rightBro, const Key& k);
public:
    InnerNode(int d, FPTree* tree, bool _ifRoot = false);
    ~InnerNode();

    KeyNode  insert(const Key& k, const Value& v) override;
    void     insertNonFull(const Key& k, Node* node);
    KeyNode  insertLeaf(const KeyNode& leaf);
    bool     remove(const Key& k, int index, InnerNode* parent, bool &ifDelete) override;
    bool     update(const Key& k, const Value& v) override;
    Value    find(const Key& k) const override;
    Key      getMinKey() const override;
    
    KeyNode  split() override;
    void     removeChild(int KeyIdx, int childIdx);

    Node*    getChild(int idx);
    Key      getKey(int idx);
    int      getKeyNum() const;
    int      getChildNum() const;
    bool     getIsRoot() const;
    void     printNode() const override;

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

    fp_tree::pmem_ptr<leaf_group> &get_pmem_ptr() const;

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

    KeyNode     insert(const Key& k, const Value& v) override;
    void        insertNonFull(const Key& k, const Value& v);
    bool        remove(const Key& k, int index, InnerNode* parent, bool &ifDelete) override;
    bool        update(const Key& k, const Value& v) override;
    Value       find(const Key& k) const override;
    int         findIndex(const Key& k) const;
    Key         getMinKey() const override;

    // used by insert()
    KeyNode     split() override;
    Key         findSplitKey() const;


    void        printNode() const override;

    int         findFirstZero() const;
    int         getBit(int idx) const;
    Key         getKey(int idx) const;
    Value       getValue(int idx) const;
    PPointer    getPPointer() const;

    // interface with NVM
    void        persist() const;
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