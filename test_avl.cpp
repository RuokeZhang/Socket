#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include "avl.cpp" // lazy

template <class P, class M>
size_t my_offsetof(const M P::*member)
{
    return (size_t) & (reinterpret_cast<P *>(0)->*member);
}

template <class P, class M>
P *my_container_of_impl(M *ptr, const M P::*member)
{
    return (P *)((char *)ptr - my_offsetof(member));
}

#define my_container_of(ptr, type, member) \
    my_container_of_impl(ptr, &type::member)

struct Data
{
    AVLNode node;
    uint32_t val = 0;
};

struct Container
{
    AVLNode *root = NULL;
};

static void add(Container &c, uint32_t val)
{
    Data *data = new Data(); // allocate the data
    avl_init(&data->node);
    data->val = val;

    AVLNode *cur = NULL;      // current node
    AVLNode **from = &c.root; // the incoming pointer to the next node
    while (*from)
    { // tree search
        cur = *from;
        uint32_t node_val = my_container_of(cur, Data, node)->val;
        from = (val < node_val) ? &cur->left : &cur->right;
    }
    *from = &data->node; // attach the new node
    data->node.parent = cur;
    c.root = avl_fix(&data->node);
}

static bool del(Container &c, uint32_t val)
{
    AVLNode *cur = c.root;
    while (cur)
    {
        uint32_t node_val = my_container_of(cur, Data, node)->val;
        if (val == node_val)
        {
            break;
        }
        cur = val < node_val ? cur->left : cur->right;
    }
    if (!cur)
    {
        return false;
    }

    c.root = avl_del(cur);
    delete my_container_of(cur, Data, node);
    return true;
}

static void avl_verify(AVLNode *parent, AVLNode *node)
{
    if (!node)
    {
        return;
    }

    assert(node->parent == parent);
    avl_verify(node, node->left);
    avl_verify(node, node->right);

    assert(node->cnt == 1 + avl_cnt(node->left) + avl_cnt(node->right));

    uint32_t l = avl_depth(node->left);
    uint32_t r = avl_depth(node->right);
    assert(l == r || l + 1 == r || l == r + 1);
    assert(node->depth == 1 + max(l, r));

    uint32_t val = my_container_of(node, Data, node)->val;
    if (node->left)
    {
        assert(node->left->parent == node);
        assert(my_container_of(node->left, Data, node)->val <= val);
    }
    if (node->right)
    {
        assert(node->right->parent == node);
        assert(my_container_of(node->right, Data, node)->val >= val);
    }
}

static void extract(AVLNode *node, std::multiset<uint32_t> &extracted)
{
    if (!node)
    {
        return;
    }
    extract(node->left, extracted);
    extracted.insert(my_container_of(node, Data, node)->val);
    extract(node->right, extracted);
}

static void container_verify(
    Container &c, const std::multiset<uint32_t> &ref)
{
    avl_verify(NULL, c.root);
    assert(avl_cnt(c.root) == ref.size());
    std::multiset<uint32_t> extracted;
    extract(c.root, extracted);
    assert(extracted == ref);
}

static void dispose(Container &c)
{
    while (c.root)
    {
        AVLNode *node = c.root;
        c.root = avl_del(c.root);
        delete my_container_of(node, Data, node);
    }
}

static void test_insert(uint32_t sz)
{
    for (uint32_t val = 0; val < sz; ++val)
    {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i)
        {
            if (i == val)
            {
                continue;
            }
            add(c, i);
            ref.insert(i);
        }
        container_verify(c, ref);

        add(c, val);
        ref.insert(val);
        container_verify(c, ref);
        dispose(c);
    }
}

static void test_insert_dup(uint32_t sz)
{
    for (uint32_t val = 0; val < sz; ++val)
    {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i)
        {
            add(c, i);
            ref.insert(i);
        }
        container_verify(c, ref);

        add(c, val);
        ref.insert(val);
        container_verify(c, ref);
        dispose(c);
    }
}

static void test_remove(uint32_t sz)
{
    for (uint32_t val = 0; val < sz; ++val)
    {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i)
        {
            add(c, i);
            ref.insert(i);
        }
        container_verify(c, ref);

        assert(del(c, val));
        ref.erase(val);
        container_verify(c, ref);
        dispose(c);
    }
}

int main()
{
    Container c;

    // sequential insertion
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < 1000; i += 3)
    {
        add(c, i);
        ref.insert(i);
        container_verify(c, ref);
    }

    dispose(c);
    return 0;
}
