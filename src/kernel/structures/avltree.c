#include "avltree.h"
#include <memory/heap/kheap.h>

static int height(AVLNode *node) {
    return node ? node->height : 0;
}

static int max(int a, int b) {
    return (a > b) ? a : b;
}

static AVLNode *create_node(int key, void *value) {
    AVLNode *node = kmalloc(sizeof(AVLNode));
    if (!node)
        return NULL;
    node->key   = key;
    node->value = value;
    node->left = node->right = NULL;
    node->height             = 1;
    return node;
}

static void free_node(AVLNode *node) {
    if (!node)
        return;
    free_node(node->left);
    free_node(node->right);
    kfree(node);
}

static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x  = y->left;
    AVLNode *T2 = x->right;

    x->right = y;
    y->left  = T2;

    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x;
}

static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y  = x->right;
    AVLNode *T2 = y->left;

    y->left  = x;
    x->right = T2;

    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    return y;
}

static int get_balance(AVLNode *node) {
    return node ? height(node->left) - height(node->right) : 0;
}

static AVLNode *insert_node(AVLNode *node, int key, void *value) {
    if (!node)
        return create_node(key, value);

    if (key < node->key)
        node->left = insert_node(node->left, key, value);
    else if (key > node->key)
        node->right = insert_node(node->right, key, value);
    else {
        node->value = value;
        return node;
    }

    node->height = 1 + max(height(node->left), height(node->right));
    int balance  = get_balance(node);

    // Balancing
    if (balance > 1 && key < node->left->key)
        return rotate_right(node);

    if (balance < -1 && key > node->right->key)
        return rotate_left(node);

    if (balance > 1 && key > node->left->key) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    if (balance < -1 && key < node->right->key) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

static AVLNode *min_value_node(AVLNode *node) {
    AVLNode *current = node;
    while (current->left)
        current = current->left;
    return current;
}

static AVLNode *remove_node(AVLNode *root, int key) {
    if (!root)
        return NULL;

    if (key < root->key)
        root->left = remove_node(root->left, key);
    else if (key > root->key)
        root->right = remove_node(root->right, key);
    else {
        if (!root->left || !root->right) {
            AVLNode *temp = root->left ? root->left : root->right;
            kfree(root);
            return temp;
        } else {
            AVLNode *temp = min_value_node(root->right);
            root->key     = temp->key;
            root->value   = temp->value;
            root->right   = remove_node(root->right, temp->key);
        }
    }

    root->height = 1 + max(height(root->left), height(root->right));
    int balance  = get_balance(root);

    if (balance > 1 && get_balance(root->left) >= 0)
        return rotate_right(root);

    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }

    if (balance < -1 && get_balance(root->right) <= 0)
        return rotate_left(root);

    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }

    return root;
}

static void *search_node(AVLNode *node, int key) {
    if (!node)
        return NULL;
    if (key == node->key)
        return node->value;
    else if (key < node->key)
        return search_node(node->left, key);
    else
        return search_node(node->right, key);
}

static void inorder_traverse(AVLNode *node, void (*visit)(int, void *)) {
    if (!node)
        return;
    inorder_traverse(node->left, visit);
    visit(node->key, node->value);
    inorder_traverse(node->right, visit);
}

// API implementations
void avl_init(AVLTree *tree) {
    tree->root = NULL;
}

void avl_free(AVLTree *tree) {
    free_node(tree->root);
    tree->root = NULL;
}

void avl_insert(AVLTree *tree, int key, void *value) {
    tree->root = insert_node(tree->root, key, value);
}

void avl_remove(AVLTree *tree, int key) {
    tree->root = remove_node(tree->root, key);
}

void *avl_search(AVLTree *tree, int key) {
    return search_node(tree->root, key);
}

void avl_inorder(AVLTree *tree, void (*visit)(int key, void *value)) {
    inorder_traverse(tree->root, visit);
}