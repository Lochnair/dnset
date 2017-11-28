/*
 * Copyright (c) 2017 by Nils Andreas Svee
 *
 * This file is part of dnset.
 *
 * dnset is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/list.h>
#include <linux/slab.h>

#include "storage.h"

LIST_HEAD(group_list_head);

static domain_node * node_create(u8 key) {
	domain_node * node = (domain_node *) kmalloc(sizeof(domain_node), GFP_KERNEL);

	if (node == NULL) {
		printk(KERN_ERR "dnset: Couldn't allocate memory for new node");
		return node;
	}

	node->key = key;
	node->next = NULL;
	node->children = NULL;
	node->parent = NULL;
	node->prev = NULL;
	return node;
}

static domain_node * node_create_root(void) {
	return node_create('\0');
}

static domain_node * node_lookup(domain_node * root, u8 * key) {
	domain_node * level = root;
	u32 lvl = 0;

	if (root == NULL)
		return NULL;

	if (root->children == NULL)
		return NULL;

	if (key == NULL)
		return NULL;

	level = root->children;

	printk(KERN_INFO "dnset: Looking up: %s", key);

	while (1)
	{
		domain_node * found = NULL;
		domain_node * curr;

		for(curr = level; curr != NULL; curr = curr->next) {
			if (curr->key == *key) {
				found = curr;
				lvl++;
				break;
			}

			if (curr->key == '*') {
				return curr;
			}
		}

		if (found == NULL) {
			return NULL;
		}

		if (*key == '\0') {
			return curr;
		}

		level = found->children;
		key++;
	}
}

static s32 node_add(domain_node * root, u8 * key) {
	domain_node * pTrav = NULL;

	if (root == NULL) {
		printk(KERN_ERR "dnset: Tried to add node to null tree");
		return -1;
	}

	pTrav = root->children;

	if (pTrav == NULL) {
		// First node
		for (pTrav = root; * key; pTrav = pTrav->children) {
			pTrav->children = node_create(*key);
			pTrav->children->parent = pTrav;
			key++;
		}

		pTrav->children = node_create('\0');
		pTrav->children->parent = pTrav;
		return 0;
	}

	if (node_lookup(pTrav, key)) {
		printk(KERN_ERR "dnset: Duplicate key");
		return -1;
	}

	while (*key != '\0'){
		if (*key == pTrav->key) {
			key++;
			pTrav = pTrav->children;
		} else {
			break;
		}
	}

	while (pTrav->next) {
		if (*key == pTrav->next->key) {
			key++;
			node_add(pTrav->next, key);
			return 0;
		}
		pTrav = pTrav->next;
	}

	pTrav->next = node_create(*key);
	pTrav->next->parent = pTrav->parent;
	pTrav->next->prev = pTrav;

	if (!(*key)) {
		return 0;
	}

	key++;

	for (pTrav = pTrav->next; *key; pTrav = pTrav->children) {
		pTrav->children = node_create(*key);
		pTrav->children->parent = pTrav;
		key++;
	}

	pTrav->children = node_create('\0');
	pTrav->children->parent = pTrav;
	return 0;
}

static void node_destroy(domain_node * root) {
	domain_node * ptr = root;
	domain_node * tmp = root;

	while (ptr)
	{
		while (ptr->children)
			ptr = ptr->children;

		tmp = ptr;

		if (ptr->prev && ptr->next)
		{
			ptr->next->prev = ptr->prev;
			ptr->prev->next = ptr->next;
			kfree(tmp);
		}
		else if (ptr->prev && !ptr->next)
		{
			ptr->prev->next = NULL;
			kfree(tmp);
		}
		else if(!ptr->prev && ptr->next)
		{
			ptr->parent->children = ptr->next;
			ptr->next->prev = NULL;
			ptr = ptr->next;
			kfree(tmp);
		}
		else
		{
			if (ptr->parent == NULL)
			{
				// We're now at the root node
				kfree(tmp);
				return;
			}

			ptr = ptr->parent;
			ptr->children = NULL;
			kfree(tmp);
		}
	}
}

static s32 node_remove(domain_node * root, u8 * key) {
	domain_node * ptr = NULL;
	domain_node * tmp = NULL;

	if (root == NULL || key == NULL)
		return -1;

	ptr = node_lookup(root->children, key);

	if (ptr == NULL) {
		printk(KERN_ERR "dnset: Key [%s] not found in trie\n", key);
		return -2;
	}

	while(1) {
		tmp = ptr;
		if (ptr->prev && ptr->next) {
			ptr->next->prev = ptr->prev;
			ptr->prev->next = ptr->next;

			kfree(tmp);
			break;
		}
		else if (ptr->prev && !ptr->next) {
			ptr->prev->next = NULL;
			kfree(tmp);
			break;
		}
		else if (!ptr->prev && ptr->next) {
			ptr->parent->children = ptr->next;
			kfree(tmp);
			break;
		}
		else {
			ptr = ptr->parent;
			ptr->children = NULL;
			kfree(tmp);
		}
	}

	return 0;
}

s32 domain_add(domain_group * group, u8 * name)
{
	return node_add(group->root_node, strrev(name));
}

s32 domain_del(domain_group * group, u8 * name)
{
	return node_remove(group->root_node, strrev(name));
}

u8 * domain_list(domain_group * group)
{
	return "";
}

domain_node * domain_search(domain_group * group, u8 * name)
{
	return node_lookup(group->root_node, strrev(name));
}

s32 group_add(u8 * group_name)
{
	domain_group * group = kmalloc(sizeof(domain_group), GFP_KERNEL);

	if (group == NULL) {
		printk(KERN_ERR "dnset: Couldn't allocate memory for new group.");
		return -1;
	}

	group->name = group_name;
	group->root_node = node_create_root();

	if (group->root_node == NULL) {
		return -1;
	}

	INIT_LIST_HEAD(&group->list);
	list_add(&group->list, &group_list_head);

	return 0;
}

s32 group_del(u8 * name)
{
	domain_group * group = group_get(name);

	if (!group)
	return -1;

	list_del(&group->list);
	node_destroy(group->root_node);
	kfree(group->name);
	kfree(group);

	return 0;
}

void group_destroy()
{
	domain_group * group;
	struct list_head * pos;

	list_for_each_prev(pos, &group_list_head) {
		group = list_entry(pos, domain_group, list);
		list_del(&group->list);
		node_destroy(group->root_node);
		kfree(group->name);
		kfree(group);
	}
}

domain_group * group_get(u8 * name)
{
        domain_group * group;
        struct list_head * pos;

        if (name == NULL)
        {
                printk(KERN_ERR "dnset: received null pointer for group_name");
                return NULL;
        }

        printk(KERN_INFO "dnset: looking for group: %s", name);

        list_for_each(pos, &group_list_head) {
                group = list_entry(pos, domain_group, list);

                if (group == NULL)
                {
                        printk(KERN_ERR "dnset: group is null");
                        continue;
                }

                if (group->name == NULL)
                {
                        printk(KERN_ERR "dnset: group->name is null");
                        continue;
                }

                if (strcmp(name, group->name) == 0) {
                        return group;
                }
        }

        return NULL;
}

u8 * group_list(void)
{
	domain_group * group;
	struct list_head * pos;
	u8 * list = NULL;
	u32 len = 0;

	list_for_each(pos, &group_list_head) {
		u32 curr_len = 0;
		group = list_entry(pos, domain_group, list);

		if (group == NULL)
		{
			printk(KERN_ERR "dnset: group is null");
			continue;
		}

		if (group->name == NULL)
		{
			printk(KERN_ERR "dnset: group->name is null");
			continue;
		}

		if (len == 0)
		{
			curr_len = strlen(group->name);
			len += curr_len + 1; /* +1 for null-terminator */
			list = krealloc(list, len, GFP_KERNEL);
			strncat(list, group->name, curr_len);
		} else {
			curr_len = strlen(group->name);
			len += curr_len + 1;  /* +1 for \n */
			list = krealloc(list, len, GFP_KERNEL);
			strncat(strncat(list, "\n", 1), group->name, curr_len);
		}
	}

	return list;
}
