#include "fptree/fptree.h"
#include <algorithm>
using namespace std;
using namespace fp_tree;
Node::Node(FPTree* tree, bool isLeaf) : tree(tree), isLeaf(isLeaf) {
}

// Initial the new InnerNode
InnerNode::InnerNode(const int& d, FPTree* const& t, bool _isRoot)
    : Node(t, false) {
    degree    = t->degree;
    isRoot    = _isRoot;
    n         = 0;
    keys      = new Key[2 * d + 1];
    childrens = new Node*[2 * d + 2];
}

// delete the InnerNode
InnerNode::~InnerNode() {
    delete[] keys;
    delete[] childrens;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(const Key& k) {
    int pos = upper_bound(keys, keys + n - 1, k) - keys;
    return pos;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(const Key& k, Node* const& node) {
    // TODO
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode InnerNode::insert(const Key& k, const Value& v) {
    // 1.insertion to the first leaf(only one leaf)
    if (this->isRoot && this->n == 0) {
        LeafNode* node = new LeafNode(tree);
        node->insert(k, v);
        childrens[n++] = node;
        return (KeyNode){k, node};
    }
    if (n < 2 * degree + 1) {
        int pos = findIndex(k);
        childrens[pos]->insert(k, v);
        return (KeyNode){k, nullptr};
    } else {
    }
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode InnerNode::insertLeaf(const KeyNode& leaf) { /*
    KeyNode* newChild = NULL;
    // first and second leaf insertion into the tree
    if (this->isRoot && this->n == 0) {
        // TODO
        return newChild;
    }

    // recursive insert
    // Tip: please judge whether this InnerNode is full
    // next level is not leaf, just insertLeaf
    // TODO

    // next level is leaf, insert to childrens array
    // TODO

    return newChild;*/
}

KeyNode InnerNode::split() {
    InnerNode* left  = new InnerNode(degree, tree);
    InnerNode* right = new InnerNode(degree, tree);

    right->n = left->n = n / 2;

    copy(keys, keys + (n - 1) / 2, left->keys);
    copy(keys + (n + 1) / 2, keys + (n - 1), right->keys);
    copy(childrens, childrens + n / 2, left->childrens);
    copy(childrens + n / 2, childrens + n, right->childrens);

    n            = 2;
    keys[0]      = keys[n / 2];
    childrens[0] = left;
    childrens[1] = right;

    return KeyNode{keys[0], this};
}

// remove the target entry
// return TRUE if the children node is deleted after removement.
// the InnerNode need to be redistributed or merged after deleting one of its children node.
bool InnerNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool& ifDelete) {
    bool ifRemove = false;
    // only have one leaf
    // TODO

    // recursive remove
    // TODO
    return ifRemove;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(const int& index, InnerNode* const& parent, InnerNode*& leftBro, InnerNode*& rightBro) {
    if (index > 0) leftBro = reinterpret_cast<InnerNode*>(parent->childrens[index - 1]);
    if (index + 1 < parent->n) rightBro = reinterpret_cast<InnerNode*>(parent->childrens[index + 1]);
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* const& parent, InnerNode* const& leftBro) {
    // TODO
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* const& parent, InnerNode* const& rightBro) {
    // TODO
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(const int& index, InnerNode* const& leftBro, InnerNode* const& parent) {
    // TODO
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(const int& index, InnerNode* const& rightBro, InnerNode* const& parent) {
    // TODO
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* const& leftBro, const Key& k) {
    // TODO
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* const& rightBro, const Key& k) {
    // TODO
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(const int& keyIdx, const int& childIdx) {
    // TODO
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    // TODO
    return false;
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) {
    int pos = findIndex(k);
    return childrens[pos]->find(k);
}

// get the children node of this InnerNode
Node* InnerNode::getChild(const int& idx) {
    return childrens[idx];
}

// get the key of this InnerNode
Key InnerNode::getKey(const int& idx) {
    if (idx < this->n) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() {
    cout << "||#|";
    for (int i = 0; i < this->n; i++) {
        cout << " " << this->keys[i] << " |#|";
    }
    cout << "|"
         << "    ";
}

pmem_ptr<leaf_group>& LeafNode::get_pmem_ptr() {
    return PAllocator::getAllocator()->getLeafGroup(pPointer);
}

// print the LeafNode
void LeafNode::printNode() {
    cout << "||";
    for (int i = 0; i < 2 * this->degree; i++) {
        if (this->getBit(i)) {
            cout << " " << this->pmem->kv[i].key << " : " << this->pmem->kv[i].value << " |";
        }
    }
    cout << "|"
         << " ====>> ";
}

// new a empty leaf and set the valuable of the LeafNode
LeafNode::LeafNode(FPTree* t) : Node(t, true) {
    PAllocator::getAllocator()->getLeaf(pPointer, pmem_addr);
    pmem   = &leaf_group::get_leaf(pmem_addr, pPointer);
    degree = LEAF_DEGREE;
    n      = 0;
    prev = next = nullptr;
    filePath    = PAllocator::getAllocator()->getLeafGroupFilePath(pPointer.fileId);
    bitmapSize  = 0;  // TODO
}

// reload the leaf with the specific Persistent Pointer
// need to call the PAllocator
LeafNode::LeafNode(PPointer pPointer, FPTree* t) : Node(t, true) {
    this->pPointer = pPointer;
    pmem_addr      = PAllocator::getAllocator()->getLeafPmemAddr(pPointer);
    pmem           = &leaf_group::get_leaf(pmem_addr, pPointer);
    degree         = LEAF_DEGREE;
    n              = PAllocator::getAllocator()->getLeafGroup(pPointer)->usedNum;
    prev = next = nullptr;
    filePath    = PAllocator::getAllocator()->getLeafGroupFilePath(pPointer.fileId);
    bitmapSize  = 0;  // TODO
}

LeafNode::~LeafNode() {
    persist();
}

// insert an entry into the leaf, need to split it if it is full
KeyNode LeafNode::insert(const Key& k, const Value& v) {
    KeyNode newChild;
    // TODO
    if (n == 2 * degree) {
        newChild = split();
    }
    insertNonFull(k, v);
    persist();
    return newChild;
}

// insert into the leaf node that is assumed not full
void LeafNode::insertNonFull(const Key& k, const Value& v) {
    // TODO
    n++;
    int pos = findFirstZero();
    set_bit(pmem->bitmap, pos);
    pmem->kv[pos]           = (key_value){k, v};
    pmem->fingerprints[pos] = keyHash(k);
    get_pmem_ptr().flush();
/*    get_pmem_ptr().flush_part(&(pmem->kv[pos]));
    get_pmem_ptr().flush_part(&(pmem->bitmap[pos / 8]));
    get_pmem_ptr().flush_part(&(pmem->fingerprints[pos]));*/
}

// split the leaf node
// here, if we call leafNode::split, this node must be full
// so bitmap must be all one-bit.
KeyNode LeafNode::split() {
    persist();
    LeafNode* newNode  = new LeafNode(tree);
    Key       splitKey = findSplitKey();

    for (int i = 0; i < n / 2; ++i) {
        set_bit(pmem->bitmap, i);
    }
    for (int i = n / 2; i < n; ++i) {
        clear_bit(pmem->bitmap, i);
    }
    *newNode->pmem = *pmem;
    memcpy(newNode->pmem->kv, pmem->kv + n / 2, sizeof(key_value) * n / 2);
    newNode->n = n = n / 2;
    pmem->pNext    = newNode->pPointer;
    persist();
    newNode->persist();

    next          = newNode;
    newNode->prev = this;

    return (KeyNode){splitKey, newNode};
}

// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find
Key LeafNode::findSplitKey() {
    Key midKey = 0;
    nth_element(pmem->kv, pmem->kv + n / 2, pmem->kv + n);
    return pmem->kv[n / 2].key;
}

// get the targte bit in bitmap
// TIPS: bit operation
int LeafNode::getBit(int idx) {
    return get_bit(pmem->bitmap, idx);
}

Key LeafNode::getKey(int idx) {
    return this->pmem->kv[idx].key;
}

Value LeafNode::getValue(int idx) {
    return this->pmem->kv[idx].value;
}

PPointer LeafNode::getPPointer() {
    return this->pPointer;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool& ifDelete) {
    bool ifRemove = false;
    // TODO
    return ifRemove;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
    bool ifUpdate = false;
    // TODO
    return ifUpdate;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) {
    for (int i = 0; i < n; ++i)
        if (getBit(i) && pmem->kv[i].key == k)
            return pmem->kv[i].value;
    return MAX_VALUE;
}

// find the first empty slot
int LeafNode::findFirstZero() {
    for (int i = 0; i < n; ++i)
        if (!getBit(i)) return i;
    return -1;
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() {
    PAllocator::getAllocator()->getLeafGroup(pPointer).flush_part(pmem);
}

// call by the ~FPTree(), delete the whole tree
void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        for (int i = 0; i < ((InnerNode*)n)->n; i++) {
            recursiveDelete(((InnerNode*)n)->childrens[i]);
        }
        delete n;
    }
}

FPTree::FPTree(uint64_t t_degree) {
    FPTree* temp = this;
    this->root   = new InnerNode(t_degree, temp, true);
    this->degree = t_degree;
    bulkLoading();
}

FPTree::~FPTree() {
    recursiveDelete(this->root);
}

// get the root node of the tree
InnerNode* FPTree::getRoot() {
    return this->root;
}

// change the root of the tree
void FPTree::changeRoot(InnerNode* newRoot) {
    this->root = newRoot;
}

void FPTree::insert(Key k, Value v) {
    if (root != NULL) {
        root->insert(k, v);
    }
}

bool FPTree::remove(Key k) {
    if (root != NULL) {
        bool       ifDelete = false;
        InnerNode* temp     = NULL;
        return root->remove(k, -1, temp, ifDelete);
    }
    return false;
}

bool FPTree::update(Key k, Value v) {
    if (root != NULL) {
        return root->update(k, v);
    }
    return false;
}

Value FPTree::find(Key k) {
    if (root != NULL) {
        return root->find(k);
    }
}

// call the InnerNode and LeafNode print func to print the whole tree
// TIPS: use Queue
void FPTree::printTree() {
    // TODO
}

// bulkLoading the leaf files and reload the tree
// need to traverse leaves chain
// if no tree is reloaded, return FALSE
// need to call the PALlocator
bool FPTree::bulkLoading() {
    // TODO
    return false;
}
