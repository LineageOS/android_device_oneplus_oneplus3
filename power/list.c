/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *    * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include <utils/Log.h>

int init_list_head(struct list_node *head)
{
    if (head == NULL)
        return -1;

    memset(head, 0, sizeof(*head));

    return 0;
}

struct list_node *add_list_node(struct list_node *head, void *data)
{
    /* Create a new list_node. And put 'data' into it. */
    struct list_node *new_node;

    if (head == NULL) {
        return NULL;
    }

    if (!(new_node = malloc(sizeof(struct list_node)))) {
        return NULL;
    }

    new_node->data = data;
    new_node->next = head->next;
    new_node->compare = head->compare;
    new_node->dump = head->dump;
    head->next = new_node;

    return new_node;
}

int is_list_empty(struct list_node *head)
{
    return (head == NULL || head->next == NULL);
}

/*
 * Delink and de-allocate 'node'.
 */
int remove_list_node(struct list_node *head, struct list_node *del_node)
{
    struct list_node *current_node;
    struct list_node *saved_node;

    if (head == NULL || head->next == NULL) {
        return -1;
    }

    current_node = head->next;
    saved_node = head;

    while (current_node && current_node != del_node) {
        saved_node = current_node;
        current_node = current_node->next;
    }

    if (saved_node) {
        if (current_node) {
            saved_node->next = current_node->next;
        } else {
            /* Node not found. */
            return -1;
        }
    }

    if (del_node) {
        free(del_node);
    }

    return 0;
}

void dump_list(struct list_node *head)
{
    struct list_node *current_node = head;

    if (head == NULL)
        return;

    printf("List:\n");

    while ((current_node = current_node->next)) {
        if (current_node->dump) {
            current_node->dump(current_node->data);
        }
    }
}

struct list_node *find_node(struct list_node *head, void *comparison_data)
{
    struct list_node *current_node = head;

    if (head == NULL)
        return NULL;

    while ((current_node = current_node->next)) {
        if (current_node->compare) {
            if (current_node->compare(current_node->data,
                    comparison_data) == 0) {
                /* Match found. Return current_node. */
                return current_node;
            }
        }
    }

    /* No match found. */
    return NULL;
}
