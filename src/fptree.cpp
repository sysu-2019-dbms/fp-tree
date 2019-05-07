#include "fptree/fptree.h"
#include <algorithm>
#include <cassert>
using namespace std;
using namespace fp_tree;
Node::Node(FPTree* tree, bool isLeaf) : tree(tree), isLeaf(isLeaf) {
}

FPTree* Node::getTree() const { return tree; }

bool Node::ifLeaf() const { return isLeaf; }

// Initial the new InnerNode
InnerNode::InnerNode(int d, FPTree* t, bool _isRoot)
    : Node(t, false) {
    degree    = t->degree;
    isRoot    = _isRoot;
    n         = 0;
    keys      = new Key[2 * d + 2];
    childrens = new Node*[2 * d + 2];
}

// delete the InnerNode
InnerNode::~InnerNode() {
    delete[] keys;
    delete[] childrens;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(const Key& k) const {
    int pos = upper_bound(keys, keys + n, k) - keys;
    return pos;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(const Key& k, Node* node) {
    int pos = findIndex(k);
    memmove(childrens + pos + 1, childrens + pos, sizeof(Node*) * (n - pos));
    memmove(keys + pos + 1, keys + pos, sizeof(Key) * (n - pos));
    childrens[pos] = node;
    keys[pos]      = k;
    n++;
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode InnerNode::insert(const Key& k, const Value& v) {
    // 1.insertion to the first leaf(only one leaf)
    if (this->isRoot && this->n == 0) {
        LeafNode* node = new LeafNode(tree);
        node->insert(k, v);
        insertNonFull(k, node);
        return (KeyNode){k, nullptr};
    }

    int     pos = findIndex(k);
    KeyNode childSplitKey;
    if (pos == 0) {
        childSplitKey = childrens[0]->insert(k, v);
        keys[0]       = childrens[0]->getMinKey();
    } else {
        childSplitKey = childrens[pos - 1]->insert(k, v);
    }
    if (childSplitKey.node) {
        if (n < 2 * degree + 2) {
            insertNonFull(childSplitKey.key, childSplitKey.node);
        } else {
            KeyNode    selfSplitKey = split();
            InnerNode* splitNode    = (InnerNode*)selfSplitKey.node;
            if (childSplitKey.key < selfSplitKey.key) {
                insertNonFull(childSplitKey.key, childSplitKey.node);
            } else {
                splitNode->insertNonFull(childSplitKey.key, childSplitKey.node);
            }

            if (this->isRoot) {
                InnerNode* newRoot = new InnerNode(tree->degree, tree, true);
                isRoot             = false;
                splitNode->isRoot  = false;
                newRoot->insertNonFull(getMinKey(), this);
                newRoot->insertNonFull(splitNode->getMinKey(), splitNode);
                tree->root = newRoot;
            }
            return selfSplitKey;
        }
    }
    return (KeyNode){k, nullptr};
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode InnerNode::insertLeaf(const KeyNode& leaf) {
    keys[n]        = leaf.key;
    childrens[n++] = leaf.node;
    return leaf;
}

KeyNode InnerNode::split() {
    InnerNode* right = new InnerNode(degree, tree);

    right->n = n / 2;
    n -= right->n;

    copy(keys + n, keys + n + right->n, right->keys);
    copy(childrens + n, childrens + n + right->n, right->childrens);

    return KeyNode{keys[n], right};
}

// remove the target entry
// return TRUE if the children node is deleted after removement.
// the InnerNode need to be redistributed or merged after deleting one of its children node.
bool InnerNode::remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) {
    bool ifRemove = false;
    // only have one leaf
    // TODO

    // recursive remove
    // TODO
    return ifRemove;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(int index, InnerNode* parent, InnerNode*& leftBro, InnerNode*& rightBro) {
    if (index > 0) leftBro = dynamic_cast<InnerNode*>(parent->childrens[index - 1]);
    if (index + 1 < parent->n) rightBro = dynamic_cast<InnerNode*>(parent->childrens[index + 1]);
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* parent, InnerNode* leftBro) {
    // TODO
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* parent, InnerNode* rightBro) {
    // TODO
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(int index, InnerNode* leftBro, InnerNode* parent) {
    // TODO
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(int index, InnerNode* rightBro, InnerNode* parent) {
    // TODO
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* leftBro, const Key& k) {
    // TODO
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* rightBro, const Key& k) {
    // TODO
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(int keyIdx, int childIdx) {
    // TODO
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    int pos = findIndex(k);
    if (pos == 0)
        return false;
    else
        return childrens[pos - 1]->update(k, v);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) const {
    int pos = findIndex(k) - 1;
    return childrens[pos]->find(k);
}

Key InnerNode::getMinKey() const {
    return keys[0];
}

// get the children node of this InnerNode
Node* InnerNode::getChild(int idx) {
    return childrens[idx];
}

// get the key of this InnerNode
Key InnerNode::getKey(int idx) {
    if (idx < this->n) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() const {
    cout << "||#|";
    for (int i = 0; i < this->n; i++) {
        cout << " " << this->keys[i] << " |#|";
    }
    cout << "|"
         << "    ";
}

int  InnerNode::getKeyNum() const { return this->n - 1; }
int  InnerNode::getChildNum() const { return this->n; }
bool InnerNode::getIsRoot() const { return this->isRoot; }

pmem_ptr<leaf_group>& LeafNode::get_pmem_ptr() const {
    return PAllocator::getAllocator()->getLeafGroup(pPointer);
}

// print the LeafNode
void LeafNode::printNode() const {
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
    char *pmemaddr;
    PAllocator::getAllocator()->getLeaf(pPointer, pmemaddr);
    pmem = reinterpret_cast<leaf *>(pmemaddr);
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
    pmem           = (leaf*)PAllocator::getAllocator()->getLeafPmemAddr(pPointer);
    degree         = LEAF_DEGREE;
    n              = 0;
    for (int i = 0; i < sizeof(pmem->bitmap); ++i)
        n += countOneBits(pmem->bitmap[i]);
    prev = next = nullptr;
    if (pmem->pNext.fileId) {
        next       = new LeafNode(pmem->pNext, t);
        next->prev = this;
    }
    filePath   = PAllocator::getAllocator()->getLeafGroupFilePath(pPointer.fileId);
    bitmapSize = 0;  // TODO
}

LeafNode::~LeafNode() {
    persist();
}

// insert an entry into the leaf, need to split it if it is full
KeyNode LeafNode::insert(const Key& k, const Value& v) {
    KeyNode newChild = (KeyNode){k, nullptr};
    // TODO
    if (n >= 2 * degree - 1) {
        newChild = split();
        if (k < newChild.key) {
            insertNonFull(k, v);
        } else {
            dynamic_cast<LeafNode*>(newChild.node)->insertNonFull(k, v);
        }
    } else {
        insertNonFull(k, v);
    }
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

    get_pmem_ptr().flush_part(&(pmem->kv[pos]));
    get_pmem_ptr().flush_part(&(pmem->bitmap[pos / 8]));
    get_pmem_ptr().flush_part(&(pmem->fingerprints[pos]));
}

// split the leaf node
// here, if we call leafNode::split, this node must be full
// so bitmap must be all one-bit.
KeyNode LeafNode::split() {
    persist();
    LeafNode* newNode  = new LeafNode(tree);
    Key       splitKey = findSplitKey();

    newNode->n = n / 2;
    n -= newNode->n;
    memset(pmem->bitmap, 0, sizeof(leaf::bitmap));
    *newNode->pmem = *pmem;

    for (int i = 0; i < n; ++i)
        set_bit(pmem->bitmap, i);

    for (int i = 0; i < newNode->n; ++i)
        set_bit(newNode->pmem->bitmap, i);

    copy(pmem->kv + n, pmem->kv + n + newNode->n, newNode->pmem->kv);
    copy(pmem->fingerprints + n, pmem->fingerprints + n + newNode->n, newNode->pmem->fingerprints);
    pmem->pNext = newNode->pPointer;
    persist();
    newNode->persist();

    next          = newNode;
    newNode->prev = this;

    return (KeyNode){splitKey, newNode};
}

// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find
Key LeafNode::findSplitKey() const {
    nth_element(pmem->kv, pmem->kv + (n + 1) / 2, pmem->kv + n);
    transform(pmem->kv, pmem->kv + n, pmem->fingerprints, [](key_value const& kv) {
        return keyHash(kv.key);
    });
    return pmem->kv[(n + 1) / 2].key;
}

// get the targte bit in bitmap
// TIPS: bit operation
int LeafNode::getBit(int idx) const {
    return get_bit(pmem->bitmap, idx);
}

Key LeafNode::getKey(int idx) const {
    return this->pmem->kv[idx].key;
}

Value LeafNode::getValue(int idx) const {
    return this->pmem->kv[idx].value;
}

PPointer LeafNode::getPPointer() const {
    return this->pPointer;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) {
    bool ifRemove = false;
    // TODO
    --n;
    int idx = findIndex(k);
    assert(idx != -1);
    clear_bit(pmem->bitmap, idx);
    if (n == 0) {
        ifRemove = true;
        if (prev) {
            prev->next = next;
            if (next) {
                prev->pmem->pNext = next->pPointer;
            } else {
                prev->pmem->pNext = PPointer{0, 0};
            }
            prev->get_pmem_ptr().flush_part(&(prev->pmem->pNext));
        }
        if (next) {
            next->prev = prev;
        }
        PAllocator::getAllocator()->freeLeaf(pPointer);
    } else {
        get_pmem_ptr().flush_part(&(pmem->bitmap[idx / 8]));
    }

    return ifRemove;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
    int pos = findIndex(k);
    if (pos == -1) return false;
    pmem->kv[pos].value = v;
    get_pmem_ptr().flush_part(&pmem->kv[pos].value);
    return true;
}

int LeafNode::findIndex(const Key& k) const {
    for (int i = 0; i < n; ++i)
        if (getBit(i) && pmem->fingerprints[i] == keyHash(k) && pmem->kv[i].key == k)
            return i;
    return -1;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) const {
    int idx = findIndex(k);
    return idx == -1 ? MAX_VALUE : pmem->kv[idx].value;
}

Key LeafNode::getMinKey() const {
    Key  ans;
    bool met = false;
    for (int i = 0; i < n; ++i)
        if (getBit(i)) {
            if (!met)
                ans = getKey(i), met = true;
            else
                ans = min(ans, getKey(i));
        }
    return ans;
}

// find the first empty slot
int LeafNode::findFirstZero() const {
    for (int i = 0; i < n; ++i)
        if (!getBit(i)) return i;
    return -1;
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() const {
    PAllocator::getAllocator()->getLeafGroup(pPointer).flush_part(pmem);
}

// call by the ~FPTree(), delete the whole tree
void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        InnerNode* node = dynamic_cast<InnerNode*>(n);
        for (int i = 0; i < node->n; i++) {
            recursiveDelete(node->childrens[i]);
        }
        delete n;
    }
}

FPTree::FPTree(uint64_t t_degree) {
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
    queue<Node*> q;
    q.push(root);
    while (!q.empty()) {
        Node* cur = q.front();
        q.pop();
        cur->printNode();
        if (!cur->isLeaf) {
            InnerNode* node = dynamic_cast<InnerNode*>(cur);
            for (int i = 0; i < node->n; ++i) {
                q.push(node->childrens[i]);
            }
        }
    }
}

// bulkLoading the leaf files and reload the tree
// need to traverse leaves chain
// if no tree is reloaded, return FALSE
// need to call the PAllocator
bool FPTree::bulkLoading() {
    PPointer start = PAllocator::getAllocator()->getStartPointer();
    if (start.fileId == 0) {
        this->root = new InnerNode(degree, this, true);
        return false;
    }
    LeafNode*    startLeaf = new LeafNode(start, this);
    queue<Node*> q;
    int          lthis = 0, lupper = 0;
    for (; startLeaf; startLeaf = startLeaf->next) {
        q.push(startLeaf);
        ++lthis;
    }

    while (q.size() > 1 || q.front()->ifLeaf()) {
        InnerNode* node = new InnerNode(degree, this);
        int        sz   = lthis < 2 * degree + 2 ? lthis : degree;
        for (int i = 0; i < sz; ++i) {
            Node* qq = q.front();
            q.pop();
            --lthis;
            node->insertLeaf(KeyNode{qq->getMinKey(), qq});
        }
        q.push(node);
        ++lupper;
        if (lthis == 0) lthis = lupper, lupper = 0;
    }

    this->root         = dynamic_cast<InnerNode*>(q.front());
    this->root->isRoot = true;
    return true;
}
