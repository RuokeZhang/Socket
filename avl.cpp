#include <stddef.h>
#include <stdint.h>

struct AVLNode
{
    uint32_t depth = 0; // subtree  height
    uint32_t cnt = 0;   // subtree size
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    AVLNode *parent = NULL;
};

static void avl_init(AVLNode *node)
{
    node->depth = 1;
    node->cnt = 1;
    node->left = node->right = node->parent = NULL;
}

static uint32_t avl_depth(AVLNode *node)
{
    return node ? node->depth : 0;
}

static uint32_t avl_cnt(AVLNode *node)
{
    return node ? node->cnt : 0;
}

static uint32_t max(uint32_t l, uint32_t r)
{
    return l < r ? r : l;
}

static void avl_update(AVLNode *node)
{
    node->depth = 1 + max(avl_depth(node->left), avl_depth(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}

/*
  2           4
 / \         / \
1   4  ==>  2   5
   / \     / \
  3   5   1   3
*/
static AVLNode *rot_left(AVLNode *node)
{
    // its right child will become the new root
    AVLNode *new_node = node->right;
    // new_node's left child will become node's child
    if (new_node->left)
    {
        new_node->left->parent = node;
    }
    node->right = new_node->left;
    // new_node's left child is current root node
    new_node->left = node;
    node->parent = new_node;
    // give node's parent to new_node
    new_node->parent = node->parent;
    // but in this step, we have no idea node is the left/right child of its parent!!!
    //  only update these two nodes bc only their subtrees' structure has been changed
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

static AVLNode *rot_right(AVLNode *node)
{
    AVLNode *new_node = node->left;
    if (new_node->right)
    {
        new_node->right->parent = node;
    }
    node->left = new_node->right;
    new_node->right = node;
    new_node->parent = node->parent;
    node->parent = new_node;
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

// if the left subtree is too deep, do a right rotation。 But after we finish this function, root's parent's left/right child have not be updated.
/*
This is an example of L-L imbalance, in this case, do a right rotation directly
    root
    /
   A
  /
 B

 This is L-R imbalance, we want promote B to be the root. DO a left rotation to A first, then do a right rotation to root
        root(4)
       /
      A(2)
       \
        B(3)

        root(4)
        /
    B(3)
    /
A(2)

*/
static AVLNode *avl_fix_left(AVLNode *root)
{ // determine L-L or L-R imbalance
    if (avl_depth(root->left->left) < avl_depth(root->left->right))
    {
        // L-R case
        // After calling this function, we assign root to be B's new parent, but we didn't assign B as the root's new left child
        AVLNode *B = rot_left(root->left);
        root->left = B;
    }
    return rot_right(root);
}

static AVLNode *avl_fix_right(AVLNode *root)
{
    if (avl_depth(root->right->right) < avl_depth(root->right->left))
    {
        AVLNode *B = rot_right(root->right);
        root->right = B;
    }
    return rot_left(root);
}

// we need fixing imbalance once we do an insertion or deletion
// Unlike BST, we need to fix the imbalance from the inserted node, up to the root
static AVLNode *avl_fix(AVLNode *node)
{
    while (true)
    {
        avl_update(node);
        // evert time we update its subtree structure, we need to get its left depth and right depth, to check if it's imbalanced
        int32_t l = avl_depth(node->left);
        int32_t r = avl_depth(node->right);
        AVLNode **from = NULL;
        // if node has a parent, let from point to it's parent's child pointer
        if (node->parent)
        {
            from = (node->parent->left == node)
                       ? &node->parent->left
                       : &node->parent->right;
        }
        if (l > r + 1)
        {
            node = avl_fix_left(node);
        }
        else if (r > l + 1)
        {
            node = avl_fix_right(node);
        }
        // if node has no parent(base case)
        if (!from)
        {
            return node;
        }
        // let parent's child pointer point to the new node
        *from = node;
        node = node->parent;
    }
}

static AVLNode *avl_del(AVLNode *node)
{
    // if the right subtree is empty, the node's left child will become the root, and we should link it to the parent
    if (node->right == NULL)
    {
        AVLNode *parent = node->parent;
        if (node->left)
        {
            node->left->parent = parent;
        }
        if (parent)
        {
            // check node is parent's left or right child
            if (parent->left == node)
            {
                parent->left = node->left;
            }
            else
            {
                parent->right = node->left;
            }
            return avl_fix(parent);
        }
        else
        { // if there's no parent
            return node->left;
        }
    }
    else
    { // if the right subtree isn't empty. We first swap node with the successor node in the right subtree, then remove the successor node (which is at a lower position).
        AVLNode *victim = node->right;
        while (victim->left)
        {
            victim = victim->left;
        }
        AVLNode *root = avl_del(victim);
        // swap node's and victim's value, not swapping the pointer
        *victim = *node;
        if (victim->left)
        {
            victim->left->parent = victim;
        }
        if (victim->right)
        {
            victim->right->parent = victim;
        }
        AVLNode *parent = node->parent;
        if (parent)
        {
            (parent->left == node ? parent->left : parent->right) = victim;
            return root;
        }
        else
        {
            // removing root?
            return victim;
        }
    }
    /* if we want to delete 50. the victim is 55
        50（node)
       /  \
     30    70
    /  \   /  \
  20   40 60  80
         /
        55
         \
          58

        50（node)
       /  \
     30    70
    /  \   /  \
  20   40 60  80
         /
        58
victim->55

现在要进行*victim = *node;了
        50(victim)
       /  \
     30    70
    /  \   /  \
  20   40 60  80
         /
        58
node->55
我想知道树的结构变了吗？因为node是有 left, right的，他们里面存着30 和 70 的地址值.是不是变成我画的这样了？
    */
}