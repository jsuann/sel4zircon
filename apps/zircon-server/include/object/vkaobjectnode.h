#pragma once

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

extern "C" {
#include <sel4/sel4.h>
#include <vka/object.h>
#include <zircon/types.h>
#include "debug.h"
}

/*
 * VkaObjectNodes are used to track vka objects,
 * primarily page table objects.
 */

struct VkaObjectNode;

struct VkaObjectNode {
    vka_object_t obj;
    VkaObjectNode *next;
};

static inline VkaObjectNode *
new_vka_node(VkaObjectNode *head, vka_object_t new_obj)
{
    VkaObjectNode *n = (VkaObjectNode *)malloc(sizeof(VkaObjectNode));
    if (n == NULL) {
        return NULL;
    }
    n->obj = new_obj;
    n->next = head;
    dprintf(SPEW, "Added new node at %p, next %p\n", n, n->next);
    return n;
}

static inline void
free_vka_nodes(vka_t *vka, VkaObjectNode *head)
{
    VkaObjectNode *curr = head;
    while (curr != NULL) {
        dprintf(SPEW, "Deleting node at %p\n", curr);
        head = curr;
        curr = curr->next;
        vka_free_object(vka, &head->obj);
        free(head);
    }
}
