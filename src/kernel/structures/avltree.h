#ifndef AVLTREE_H
#define AVLTREE_H

#include <stddef.h>

typedef struct AVLNode {
    int key;
    void *value;

    struct AVLNode *left;
    struct AVLNode *right;
    int height;
} AVLNode;

typedef struct AVLTree {
    AVLNode *root;
} AVLTree;

// management
void avl_init(AVLTree *tree);
void avl_free(AVLTree *tree);

// add and remove
void avl_insert(AVLTree *tree, int key, void *value);
void avl_remove(AVLTree *tree, int key);

// searching
void *avl_search(AVLTree *tree, int key);

// traversal
void avl_inorder(AVLTree *tree, void (*visit)(int key, void *value));

#endif // AVLTREE_H