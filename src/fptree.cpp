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
InnerNode::InnerNode(size_t d, FPTree* t, bool _isRoot)
    : Node(t, false) {
    degree   = t->degree;
    isRoot   = _isRoot;
    n        = 0;
    keys     = new Key[2 * d + 2];
    children = new Node*[2 * d + 2];
}

// delete the InnerNode
InnerNode::~InnerNode() {
    delete[] keys;
    delete[] children;
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
    memmove(children + pos + 1, children + pos, sizeof(Node*) * (n - pos));
    memmove(keys + pos + 1, keys + pos, sizeof(Key) * (n - pos));
    children[pos] = node;
    keys[pos]     = k;
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
        return {k, nullptr};
    }

    int     pos = findIndex(k);
    KeyNode childSplitKey;
    if (pos == 0) {
        childSplitKey = children[0]->insert(k, v);
        keys[0]       = children[0]->getMinKey();
    } else {
        childSplitKey = children[pos - 1]->insert(k, v);
    }
    if (childSplitKey.node) {
        if (n < 2 * degree + 1) {
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
    return {k, nullptr};
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode InnerNode::insertLeaf(const KeyNode& leaf) {
    keys[n]       = leaf.key;
    children[n++] = leaf.node;
    return leaf;
}

KeyNode InnerNode::split() {
    InnerNode* right = new InnerNode(degree, tree);

    right->n = n / 2;
    n -= right->n;

    copy(keys + n, keys + n + right->n, right->keys);
    copy(children + n, children + n + right->n, right->children);

    return {keys[n], right};
}

bool InnerNode::remove(const Key& k, int index, InnerNode* parent, bool& ifDelete) {
    // the InnerNode need to be redistributed or merged after deleting one of its child node.

    int pos = findIndex(k);
    if (pos == 0) return false;
    bool ifRemove = children[pos - 1]->remove(k, pos - 1, this, ifDelete);
    if (ifDelete) {
        removeChild(pos - 1, pos - 1);
        if (isRoot && n == 1 && !children[0]->ifLeaf()) {
            tree->root         = dynamic_cast<InnerNode*>(children[0]);
            tree->root->isRoot = true;
            ifDelete           = true;
            return ifRemove;
        }

        if (n < degree + 1 && !isRoot) {
            // The node does not satisfy constraints any more.
            // Need to do a lot of work:
            InnerNode *leftBro = nullptr, *rightBro = nullptr;
            getBrother(index, parent, leftBro, rightBro);

            if (parent->isRoot && parent->getChildNum() == 2) {
                // Case 6: the parent is root
                if (leftBro) {
                    mergeParentLeft(parent, leftBro);
                    ifDelete = true;
                } else if (rightBro) {
                    mergeParentRight(parent, rightBro);
                    ifDelete = true;
                }
                return ifRemove;
            }

            if (rightBro && rightBro->n > degree + 1) {
                // Case 2: the right brother has enough elements
                //         for redistribution
                redistributeRight(index, rightBro, parent);
                ifDelete = false;
                return ifRemove;

            } else if (leftBro && leftBro->n > degree + 1) {
                // Case 3: the left brother has enough elements
                //         for redistribution
                redistributeLeft(index, leftBro, parent);
                ifDelete = false;
                return ifRemove;
            }

            if (rightBro) {
                // Case 4: the right brother has to merge with this node
                mergeRight(rightBro, k);
                parent->keys[index + 1] = rightBro->getMinKey();
                ifDelete                = true;
                return ifRemove;
            } else if (leftBro) {
                // Case 5: the left brother has to merge with this node
                mergeLeft(leftBro, k);
                parent->keys[index - 1] = leftBro->getMinKey();
                ifDelete                = true;
                return ifRemove;
            }

            return false;
        }

        // Case 1: The tree still satisfies the constraints
        ifDelete = false;
        return ifRemove;
    }
    keys[0] = children[0]->getMinKey();
    return ifRemove;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(int index, InnerNode* parent, InnerNode*& leftBro, InnerNode*& rightBro) {
    if (parent && index > 0) leftBro = dynamic_cast<InnerNode*>(parent->children[index - 1]);
    if (parent && index + 1 < (int)parent->n) rightBro = dynamic_cast<InnerNode*>(parent->children[index + 1]);
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* /*parent*/, InnerNode* leftBro) {
    memmove(leftBro->children + leftBro->n, children, sizeof(Node*) * n);
    memmove(leftBro->keys + leftBro->n, keys, sizeof(Key) * n);
    leftBro->n += n;
    tree->root      = leftBro;
    leftBro->isRoot = true;
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* /*parent*/, InnerNode* rightBro) {
    memmove(rightBro->children + this->n, rightBro->children, sizeof(Node*) * rightBro->n);
    memmove(rightBro->keys + this->n, rightBro->keys, sizeof(Key) * rightBro->n);
    memmove(rightBro->children, children, sizeof(Node*) * n);
    memmove(rightBro->keys, keys, sizeof(Key) * n);
    rightBro->n += n;
    tree->root       = rightBro;
    rightBro->isRoot = true;
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(int index, InnerNode* leftBro, InnerNode* parent) {
    // TODO
    // leftBro's max key becomes my min key

    // spare space
    memmove(children + 1, children, sizeof(Node*) * n);
    memmove(keys + 1, keys, sizeof(Key) * n);

    // move the key
    --leftBro->n;
    keys[0]     = leftBro->keys[leftBro->n];
    children[0] = leftBro->children[leftBro->n];
    ++this->n;

    // update parent
    parent->keys[index] = this->getMinKey();
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(int index, InnerNode* rightBro, InnerNode* parent) {
    // rightBro's min key becomes my max key

    // move the key
    --rightBro->n;
    keys[n]     = rightBro->keys[0];
    children[n] = rightBro->children[0];
    ++this->n;

    memmove(rightBro->keys, rightBro->keys + 1, sizeof(Key) * rightBro->n);
    memmove(rightBro->children, rightBro->children + 1, sizeof(Node*) * rightBro->n);

    // update parent
    parent->keys[index + 1] = rightBro->getMinKey();
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* leftBro, const Key& /*k*/) {
    memmove(leftBro->children + leftBro->n, children, sizeof(Node*) * n);
    memmove(leftBro->keys + leftBro->n, keys, sizeof(Key) * n);
    leftBro->n += n;
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* rightBro, const Key& /*k*/) {
    memmove(rightBro->children + this->n, rightBro->children, sizeof(Node*) * rightBro->n);
    memmove(rightBro->keys + this->n, rightBro->keys, sizeof(Key) * rightBro->n);
    memmove(rightBro->children, children, sizeof(Node*) * n);
    memmove(rightBro->keys, keys, sizeof(Key) * n);
    rightBro->n += n;
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(int /*keyIdx*/, int childIdx) {
    delete children[childIdx];

    --n;
    // Simply move the keys and children
    for (size_t i = childIdx; i < n; ++i) {
        keys[i]     = keys[i + 1];
        children[i] = children[i + 1];
    }
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    int pos = findIndex(k);
    if (pos == 0)
        return false;
    else
        return children[pos - 1]->update(k, v);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) const {
    int pos = findIndex(k) - 1;
    if (pos < 0) return MAX_VALUE;
    return children[pos]->find(k);
}

Key InnerNode::getMinKey() const {
    return keys[0];
}

// get the children node of this InnerNode
Node* InnerNode::getChild(size_t idx) {
    return children[idx];
}

// get the key of this InnerNode
Key InnerNode::getKey(size_t idx) {
    if (idx < this->n) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() const {
    cout << "||#|";
    for (size_t i = 0; i < this->n; i++) {
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
    for (size_t i = 0; i < 2 * this->degree; i++) {
        if (this->getBit(i)) {
            cout << " " << this->pmem->kv[i].key << " : " << this->pmem->kv[i].value << " |";
        }
    }
    cout << "|"
         << " ====>> ";
}

// new a empty leaf and set the valuable of the LeafNode
LeafNode::LeafNode(FPTree* t) : Node(t, true) {
    char* pmemaddr;
    PAllocator::getAllocator()->getLeaf(pPointer, pmemaddr);
    pmem   = reinterpret_cast<leaf*>(pmemaddr);
    degree = LEAF_DEGREE;
    n      = 0;
    prev = next = nullptr;
    filePath    = PAllocator::getAllocator()->getLeafGroupFilePath(pPointer.fileId);
    bitmapSize  = 2 * degree;
}

// reload the leaf with the specific Persistent Pointer
// need to call the PAllocator
LeafNode::LeafNode(PPointer pPointer, FPTree* t) : Node(t, true) {
    LeafNode* cur = this;
    do {
        cur->pPointer = pPointer;
        cur->pmem     = (leaf*)PAllocator::getAllocator()->getLeafPmemAddr(pPointer);
        cur->degree   = LEAF_DEGREE;
        cur->n        = 0;
        for (size_t i = 0; i < sizeof(cur->pmem->bitmap); ++i)
            cur->n += countOneBits(cur->pmem->bitmap[i]);
        cur->prev = cur->next = nullptr;
        cur->filePath         = PAllocator::getAllocator()->getLeafGroupFilePath(pPointer.fileId);
        cur->bitmapSize       = 2 * cur->degree;
    } while (cur->pmem->pNext.fileId && ({
                 cur->next       = new LeafNode(*cur);
                 pPointer        = cur->pmem->pNext;
                 cur->next->prev = cur;
                 cur             = cur->next;
             }));
}

LeafNode::~LeafNode() {
}

// insert an entry into the leaf, need to split it if it is full
KeyNode LeafNode::insert(const Key& k, const Value& v) {
    KeyNode newChild{k, nullptr};
    if (update(k, v)) return newChild;

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
    pmem->kv[pos]           = {k, v};
    pmem->fingerprints[pos] = keyHash(k);

    get_pmem_ptr().flush_part(&pmem->kv[pos]);
    get_pmem_ptr().flush_part(&pmem->bitmap[pos / 8]);
    get_pmem_ptr().flush_part(&pmem->fingerprints[pos]);
}

// split the leaf node
// here, if we call leafNode::split, this node must be full
// so bitmap must be all one-bit.
KeyNode LeafNode::split() {
    LeafNode* newNode  = new LeafNode(tree);
    Key       splitKey = findSplitKey();

    newNode->n = n / 2;
    n -= newNode->n;
    *newNode->pmem = *pmem;

    size_t j = 0;
    for (size_t i = 0; i < n; ++i, ++j)
        for (; !get_bit(pmem->bitmap, j); ++j)
            ;
    clear_bit_since(pmem->bitmap, sizeof(pmem->bitmap), j);
    clear_bit_until(newNode->pmem->bitmap, sizeof(newNode->pmem->bitmap), j);

    pmem->pNext = newNode->pPointer;
    get_pmem_ptr().flush_part(pmem->bitmap);
    newNode->persist();

    next          = newNode;
    newNode->prev = this;

    return {splitKey, newNode};
}

// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find
Key LeafNode::findSplitKey() const {
    struct kvf {
        key_value kv;
        Byte      fingerprint;
    };
    static kvf arr[LEAF_DEGREE * 2];
    size_t     len = 0;
    for (size_t i = 0; i < bitmapSize; ++i)
        if (getBit(i)) {
            arr[len++] = {pmem->kv[i], pmem->fingerprints[i]};
        }
    assert(len == n);
    nth_element(arr, arr + (n + 1) / 2, arr + n, [](const kvf& a, const kvf& b) { return a.kv.key < b.kv.key; });
    bzero(pmem->bitmap, sizeof(pmem->bitmap));
    for (size_t i = 0; i < n; ++i) {
        set_bit(pmem->bitmap, i);
        pmem->kv[i]           = arr[i].kv;
        pmem->fingerprints[i] = arr[i].fingerprint;
    }
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
bool LeafNode::remove(const Key& k, int /*index*/, InnerNode* /*parent*/, bool& ifDelete) {
    int idx = findIndex(k);
    if (idx == -1) return false;
    clear_bit(pmem->bitmap, idx);
    if (--n == 0) {
        if (prev) {
            prev->next = next;
            if (next) {
                prev->pmem->pNext = next->pPointer;
            } else {
                prev->pmem->pNext = PPointer{0, 0};
            }
            prev->get_pmem_ptr().flush_part(&(prev->pmem->pNext));
        } else {
            PAllocator::getAllocator()->setStartPointer(pmem->pNext);
        }
        if (next) {
            next->prev = prev;
        }
        PAllocator::getAllocator()->freeLeaf(this->pPointer);
        ifDelete = true;
    } else {
        get_pmem_ptr().flush_part(&(pmem->bitmap[idx / 8]));
        ifDelete = false;
    }

    return true;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
    int pos = findIndex(k);
    if (pos == -1) return false;
    get_pmem_ptr().modify(&pmem->kv[pos].value, v);
    return true;
}

int LeafNode::findIndex(const Key& k) const {
    const Byte hashval = keyHash(k);
    for (size_t i = 0; i < bitmapSize; ++i)
        if (getBit(i) && pmem->fingerprints[i] == hashval && pmem->kv[i].key == k)
            return i;
    return -1;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) const {
    int idx = findIndex(k);
    return idx == -1 ? MAX_VALUE : pmem->kv[idx].value;
}

Key LeafNode::getMinKey() const {
    Key  ans = MAX_VALUE;
    bool met = false;
    for (size_t i = 0; i < bitmapSize; ++i)
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
    return find_first_zero(pmem->bitmap, bitmapSize);
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() const {
    get_pmem_ptr().flush_part(pmem);
}

void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        InnerNode* node = dynamic_cast<InnerNode*>(n);
        for (size_t i = 0; i < node->n; i++) {
            recursiveDelete(node->children[i]);
        }
        delete n;
    }
}

FPTree::FPTree(size_t t_degree) {
    this->degree = t_degree;
    bulkLoading();
}

FPTree::~FPTree() {
    recursiveDelete(this->root);
}

InnerNode* FPTree::getRoot() {
    return this->root;
}

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
        InnerNode* temp     = root;
        bool       ifRemove = root->remove(k, -1, nullptr, ifDelete);
        if (ifDelete) delete temp;
        return ifRemove;
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
    return MAX_VALUE;
}

// call the InnerNode and LeafNode print func to print the whole tree
void FPTree::printTree() {
    queue<Node*> q;
    q.push(root);
    while (!q.empty()) {
        Node* cur = q.front();
        q.pop();
        cur->printNode();
        if (!cur->isLeaf) {
            InnerNode* node = dynamic_cast<InnerNode*>(cur);
            for (size_t i = 0; i < node->n; ++i) {
                q.push(node->children[i]);
            }
        }
        cout << endl;
    }
}

bool FPTree::bulkLoading() {
    PPointer start = PAllocator::getAllocator()->getStartPointer();
    if (start.fileId == 0) {
        this->root = new InnerNode(degree, this, true);
        return false;
    }
    LeafNode*    startLeaf = new LeafNode(start, this);
    queue<Node*> q;
    size_t       lthis = 0, lupper = 0;
    for (; startLeaf; startLeaf = startLeaf->next) {
        q.push(startLeaf);
        ++lthis;
    }

    while (q.size() > 1 || q.front()->ifLeaf()) {
        InnerNode* node = new InnerNode(degree, this);
        size_t     sz   = lthis < 2 * degree + 1 ? lthis : degree;
        for (size_t i = 0; i < sz; ++i) {
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
