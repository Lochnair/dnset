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

#include <linux/slab.h>
#include <linux/rculist.h>

#include "storage.h"

LIST_HEAD(group_list_head);

static domain_node * node_create(char key) {
	domain_node * node = (domain_node *) kmalloc(sizeof(domain_node), GFP_KERNEL);

	WARN_ON(node == NULL);

	node->key = key;
	node->next = NULL;
	node->children = NULL;
	node->parent = NULL;
	node->prev = NULL;
	spin_lock_init(&node->lock);
	return node;
}

static domain_node * node_create_root(void) {
	return node_create('\0');
}

static domain_node * node_lookup(domain_node * root, char * key) {
	domain_node * level = root;
	unsigned int lvl = 0;

	if (root == NULL)
		return NULL;

	if (root->children == NULL)
		return NULL;

	if (key == NULL)
		return NULL;

	level = root->children;

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

static int node_add(domain_node * root, char * key) {
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
			spin_lock(&pTrav->children->lock);
			pTrav->children->parent = pTrav;
			spin_unlock(&pTrav->children->lock);
			key++;
		}

		pTrav->children = node_create('\0');
		spin_lock(&pTrav->children->lock);
		pTrav->children->parent = pTrav;
		spin_unlock(&pTrav->children->lock);
		return 0;
	}

	if (node_lookup(pTrav, key)) {
		printk(KERN_ERR "dnset: Duplicate key: %c", *key);
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
	spin_lock(&pTrav->next->lock);
	pTrav->next->parent = pTrav->parent;
	pTrav->next->prev = pTrav;
	spin_unlock(&pTrav->next->lock);

	if (!(*key)) {
		return 0;
	}

	key++;

	for (pTrav = pTrav->next; *key; pTrav = pTrav->children) {
		pTrav->children = node_create(*key);
		spin_lock(&pTrav->children->lock);
		pTrav->children->parent = pTrav;
		spin_unlock(&pTrav->children->lock);
		key++;
	}

	pTrav->children = node_create('\0');
	spin_lock(&pTrav->children->lock);
	pTrav->children->parent = pTrav;
	spin_unlock(&pTrav->children->lock);
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

static int node_remove(domain_node * root, char * key) {
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
			spin_lock(&ptr->lock);
			ptr->next->prev = ptr->prev;
			ptr->prev->next = ptr->next;
			spin_unlock(&ptr->lock);

			kfree(tmp);
			break;
		}
		else if (ptr->prev && !ptr->next) {
			spin_lock(&ptr->lock);
			ptr->prev->next = NULL;
			spin_unlock(&ptr->lock);
			kfree(tmp);
			break;
		}
		else if (!ptr->prev && ptr->next) {
			spin_lock(&ptr->lock);
			ptr->parent->children = ptr->next;
			spin_unlock(&ptr->lock);
			kfree(tmp);
			break;
		}
		else {
			ptr = ptr->parent;
			spin_lock(&ptr->lock);
			ptr->children = NULL;
			spin_unlock(&ptr->lock);
			kfree(tmp);
		}
	}

	return 0;
}

int domain_add(domain_group * group, char * name)
{
	return node_add(group->root_node, strrev(name));
}

int domain_del(domain_group * group, char * name)
{
	return node_remove(group->root_node, strrev(name));
}

void traverse_node(domain_node * ptr, char ** dst)
{

	for (; ptr != NULL; ptr = ptr->children)
	{
		if (ptr->key == '\0')
		{
			domain_node * tmp = ptr->parent;
			char * word = kmalloc(sizeof(char), GFP_KERNEL);
			unsigned int buff_len, len = 0;

			while (tmp)
			{
				word = krealloc(word, sizeof(char) * (1 + len), GFP_KERNEL);
				word[len] = tmp->key;

				if (len > 0 && tmp->key == '\0')
				{
					// root node
					break;
				}

				tmp = tmp->parent;
				len++;
			}


			buff_len = strlen(*dst) + strlen(word) + 1;

			if (strlen(*dst) == 0)
			{
				*dst = krealloc(*dst, buff_len, GFP_KERNEL);
				memcpy(*dst, word, strlen(word) + 1);
			} else {
				*dst = krealloc(*dst, buff_len, GFP_KERNEL);
				strncat(strncat(*dst, "\n", 1), word, strlen(word));
			}
		}

		traverse_node(ptr->next, dst);
	}

}

char * domain_list(domain_group * group)
{
	domain_node * ptr = group->root_node->children;
	char * ret = kmalloc(sizeof(char), GFP_KERNEL);
	ret[0] = '\0';
	traverse_node(ptr, &ret);

	return ret;
}

domain_node * domain_search(domain_group * group, char * name)
{
	return node_lookup(group->root_node, strrev(name));
}

int group_add(char * group_name)
{
	domain_group * group = kmalloc(sizeof(domain_group), GFP_KERNEL);

	WARN_ON(group == NULL);

	group->name = group_name;
	group->root_node = node_create_root();

	if (group->root_node == NULL) {
		return -1;
	}

	INIT_LIST_HEAD(&group->list);
	list_add_rcu(&group->list, &group_list_head);

	return 0;
}

int group_del(char * name)
{
	domain_group * group = group_get(name);

	if (!group)
		return -1;

	list_del_rcu(&group->list);
	synchronize_rcu();
	node_destroy(group->root_node);
	kfree(group->name);
	kfree(group);

	return 0;
}

void group_destroy()
{
	domain_group * group;

	list_for_each_entry_rcu(group, &group_list_head, list) {
		list_del_rcu(&group->list);
		synchronize_rcu();
		node_destroy(group->root_node);
		kfree(group->name);
		kfree(group);
	}
}

domain_group * group_get(char * name)
{
        domain_group * group = NULL, * tmp = NULL;

        if (name == NULL)
        {
                printk(KERN_ERR "dnset: received null pointer for group_name");
                return NULL;
        }

	rcu_read_lock();
        list_for_each_entry_rcu(tmp, &group_list_head, list) {

                if (tmp == NULL)
                {
                        printk(KERN_ERR "dnset: group is null");
                        continue;
                }

                if (tmp->name == NULL)
                {
                        printk(KERN_ERR "dnset: group->name is null");
                        continue;
                }

                if (strcmp(name, tmp->name) == 0) {
                        group = tmp;
			goto unlock;
                }
        }

unlock:
	rcu_read_unlock();
	return group;
}

char * group_list(void)
{
	domain_group * group;
	char * list = NULL;
	unsigned int len = 0;

	rcu_read_lock();
	list_for_each_entry_rcu(group, &group_list_head, list) {
		unsigned int curr_len = 0;

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

		curr_len = strlen(group->name);

		if (len == 0)
		{
			len += curr_len + 1; /* +1 for null-terminator */
			list = kmalloc(len, GFP_KERNEL);
			memcpy(list, group->name, len);
		} else {
			curr_len = strlen(group->name);
			len += curr_len + 1;  /* +1 for \n */
			list = krealloc(list, len, GFP_KERNEL);
			strncat(strncat(list, "\n", 1), group->name, curr_len);
		}
	}
	rcu_read_unlock();

	return list;
}
