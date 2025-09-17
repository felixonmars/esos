/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Intrusive circular singly-linked list.
 */

#include <fwk_list.h>
#include <fwk_slist.h>

#include <stdbool.h>
#include <stddef.h>

void __fwk_slist_init(struct fwk_slist *list)
{
    list->head = (struct fwk_slist_node *)list;
    list->tail = (struct fwk_slist_node *)list;
}

struct fwk_slist_node *__fwk_slist_head(const struct fwk_slist *list)
{
    if (__fwk_slist_is_empty(list)) {
        return NULL;
    }

    return list->head;
}

bool __fwk_slist_is_empty(const struct fwk_slist *list)
{
    bool is_empty;

    is_empty = list->head == (struct fwk_slist_node *)list;

    return is_empty;
}

void __fwk_slist_push_head(
    struct fwk_slist *list,
    struct fwk_slist_node *new)
{
    new->next = list->head;

    list->head = new;
    if (list->tail == (struct fwk_slist_node *)list) {
        list->tail = new;
    }
}

void __fwk_slist_push_tail(
    struct fwk_slist *list,
    struct fwk_slist_node *new)
{
    new->next = (struct fwk_slist_node *)list;

    list->tail->next = new;
    list->tail = new;
}

struct fwk_slist_node *__fwk_slist_pop_head(struct fwk_slist *list)
{
    struct fwk_slist_node *popped;

    if (__fwk_slist_is_empty(list)) {
        return NULL;
    }

    popped = list->head;
    if (popped->next == (struct fwk_slist_node *)list) {
        list->tail = (struct fwk_slist_node *)list;
    }

    list->head = popped->next;

    popped->next = NULL;

    return popped;
}

struct fwk_slist_node *__fwk_slist_next(
    const struct fwk_slist *list,
    const struct fwk_slist_node *node)
{
    return (node->next == (struct fwk_slist_node *)list) ? NULL : node->next;
}

void __fwk_slist_remove(
    struct fwk_slist *list,
    struct fwk_slist_node *node)
{
    struct fwk_slist_node *node_iter = (struct fwk_slist_node *)list;

    while (node_iter->next != (struct fwk_slist_node *)list) {
        if (node_iter->next == node) {
            node_iter->next = node->next;

            if (node->next == (struct fwk_slist_node *)list) {
                list->tail = (struct fwk_slist_node *)node_iter;
            }

            node->next = NULL;

            return;
        }
        node_iter = node_iter->next;
    }
}

bool __fwk_slist_contains(
    const struct fwk_slist *list,
    const struct fwk_slist_node *node)
{
    const struct fwk_slist_node *node_iter;

    node_iter = (struct fwk_slist_node *)list;

    while (node_iter->next != (struct fwk_slist_node *)list) {
        if (node_iter->next == node) {
            return true;
        }

        node_iter = node_iter->next;
    }

    return false;
}
