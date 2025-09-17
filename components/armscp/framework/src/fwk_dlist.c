/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Intrusive circular doubly-linked list.
 */

#include <fwk_dlist.h>
#include <fwk_slist.h>

#include <stddef.h>

void __fwk_dlist_push_head(
    struct fwk_dlist *list,
    struct fwk_dlist_node *new)
{
    new->prev = (struct fwk_dlist_node *)list;
    list->head->prev = new;

    __fwk_slist_push_head(
        (struct fwk_slist *)list,
        (struct fwk_slist_node *)new);
}

void __fwk_dlist_push_tail(
    struct fwk_dlist *list,
    struct fwk_dlist_node *new)
{
    new->prev = list->tail;

    __fwk_slist_push_tail(
        (struct fwk_slist *)list,
        (struct fwk_slist_node *)new);
}

struct fwk_dlist_node *__fwk_dlist_pop_head(struct fwk_dlist *list)
{
    struct fwk_dlist_node *popped;

    popped = (struct fwk_dlist_node *)__fwk_slist_pop_head(
        (struct fwk_slist *)list);

    list->head->prev = (struct fwk_dlist_node *)list;

    if (popped != NULL) {
        popped->prev = NULL;
    }

    return popped;
}

void __fwk_dlist_remove(
    struct fwk_dlist *list,
    struct fwk_dlist_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->prev = NULL;
    node->next = NULL;
}

void __fwk_dlist_insert(
    struct fwk_dlist *list,
    struct fwk_dlist_node *restrict new,
    struct fwk_dlist_node *restrict node)
{
    if (node == NULL) {
        __fwk_dlist_push_tail(list, new);

        return;
    }

    node->prev->next = new;
    new->prev = node->prev;
    new->next = node;
    node->prev = new;
}
