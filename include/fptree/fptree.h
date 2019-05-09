#include <memory.h>
#include <stdlib.h>
#include <iostream>
#include <queue>

#include "utility/p_allocator.h"

typedef struct t_KeyNode   KeyNode;
typedef struct t_NodeLevel NodeLevel;

class FPTree;
class InnerNode;
class LeafNode;

class Node {
protected:
    friend class FPTree;

    FPTree* tree;
    size_t  degree;  // the degree of the node
    bool    isLeaf;

public:
    Node(FPTree* tree, bool isLeaf);
    virtual ~Node() {}

    FPTree* getTree() const;  // the tree that the node belongs to

    bool ifLeaf() const;  // judge whether the node is leaf

    /**
     * \brief insert key-value into this node.
     * \param k key of the key-value pair to be inserted, may duplicate.
     * \param v value of the key-value pair to be inserted
     * \return nonnull KeyNode with key the minimum key of the splitted, node the node splitted out
     * \note The key-value will finally be inserted into one leaf node.
     */
    virtual KeyNode insert(const Key& k, const Value& v) = 0;

    /**
     * \brief split this node into two nodes.
     * \return KeyNode with key the medium key to split keys of this node, node the node splitted out
     * \note you should manually insert the new node into parent node of the old one.
     */
    virtual KeyNode split() = 0;

    /**
     * \brief remove the key and related value.
     * \param k the key to be removed.
     * \param index the child index in parent.
     * \param ifDelete set to true if this node is to be deleted.
     * \return true if the key exists, and remove operation succeeds.
     * \note caller node should delete this node if ifDelete is true.
     */
    virtual bool remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) = 0;

    /**
     * \brief update the value related to the key.
     * \param k the key whose related value to be modified
     * \param v the new value
     * \return true if the key exists and update operation succeeds
     */
    virtual bool update(const Key& k, const Value& v) = 0;

    /**
     * \brief find the value related to the key.
     * \param k the key whose value is to be identified.
     * \return the related value, MAX_VALUE if the key cannot be found
     */
    virtual Value find(const Key& k) const = 0;

    /**
     * \brief find the minimum key inside this node.
     * \return the minimum key
     */
    virtual Key getMinKey() const = 0;

    /**
     * \brief print information of this node to the screen
     */
    virtual void printNode() const = 0;
};

// used for node's recursive insertion and split
// only allocated in func split()
typedef struct t_KeyNode {
    Key   key;
    Node* node;
} KeyNode;

// used by print func
typedef struct t_NodeLevel {
    Node* node;
    int   level;
} NodeLevel;

typedef struct t_KeyValue {
    Key   k;
    Value v;
} KeyValue;

class InnerNode : public Node {
private:
    friend class FPTree;

    bool   isRoot;    // judge whether the node is root
    size_t n;         // amount of children
    Key*   keys;      // max (2 * d + 2) keys
    Node** children;  // max (2 * d + 2) node pointers

    int findIndex(const Key& k) const;

    void getBrother(int index, InnerNode* parent, InnerNode*& leftBro, InnerNode*& rightBro);
    void redistributeRight(int index, InnerNode* rightBro, InnerNode* parent);
    void redistributeLeft(int index, InnerNode* leftBro, InnerNode* parent);

    void mergeParentRight(InnerNode* parent, InnerNode* rightBro);
    void mergeParentLeft(InnerNode* parent, InnerNode* leftBro);

    void mergeLeft(InnerNode* LeftBro, const Key& k);
    void mergeRight(InnerNode* rightBro, const Key& k);

public:
    InnerNode(size_t d, FPTree* tree, bool _ifRoot = false);
    ~InnerNode();

    KeyNode insert(const Key& k, const Value& v) override;
    void    insertNonFull(const Key& k, Node* node);
    KeyNode insertLeaf(const KeyNode& leaf);
    bool    remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) override;
    bool    update(const Key& k, const Value& v) override;
    Value   find(const Key& k) const override;
    Key     getMinKey() const override;

    KeyNode split() override;
    void    removeChild(int KeyIdx, int childIdx);

    Node* getChild(size_t idx);
    Key   getKey(size_t idx);
    int   getKeyNum() const;
    int   getChildNum() const;
    bool  getIsRoot() const;
    void  printNode() const override;
};

// LeafNode structure is the leaf node in NVM that is buffered in the DRAM.
class LeafNode : public Node {
private:
    friend class FPTree;
    friend class InnerNode;

    // the pointer below are all pmem address based on pmem_addr
    // need to set the pointer pointed to NVM address
    leaf* pmem;

    fp_tree::pmem_ptr<leaf_group>& get_pmem_ptr() const;

    // the DRAM relative variables
    size_t    n;         // amount of entries
    LeafNode* prev;      // the address of previous leafnode
    LeafNode* next;      // the address of next leafnode
    PPointer  pPointer;  // the persistent pointer pointed to the leaf in NVM
    string    filePath;  // the file path of the leaf

    size_t bitmapSize;  // the bitmap size of the leaf(bytes)

public:
    LeafNode(FPTree* tree);           // allocate a new leaf
    LeafNode(PPointer p, FPTree* t);  // read a leaf from NVM/SSD
    ~LeafNode();

    KeyNode insert(const Key& k, const Value& v) override;
    void    insertNonFull(const Key& k, const Value& v);
    bool    remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) override;
    bool    update(const Key& k, const Value& v) override;
    Value   find(const Key& k) const override;
    int     findIndex(const Key& k) const;
    Key     getMinKey() const override;

    // used by insert()
    KeyNode split() override;
    Key     findSplitKey() const;

    void printNode() const override;

    int      findFirstZero() const;
    int      getBit(int idx) const;
    Key      getKey(int idx) const;
    Value    getValue(int idx) const;
    PPointer getPPointer() const;

    // interface with NVM
    void persist() const;
};

class FPTree {
private:
    friend class InnerNode;
    InnerNode* root;
    size_t     degree;

    void recursiveDelete(Node* n);  // call by the ~FPTree(), delete the whole tree

public:
    FPTree(size_t degree);
    ~FPTree();

    void      insert(Key k, Value v);
    bool      remove(Key k);
    bool      update(Key k, Value v);
    Value     find(Key k);
    LeafNode* findLeaf(Key K);

    InnerNode* getRoot();                       // get the root node of the tree
    void       changeRoot(InnerNode* newRoot);  // change the root of the tree

    void printTree();

    /**
     * \brief load data from persistent memory.
     * \return true if any data exists in the pmem.
     */
    bool bulkLoading();
};