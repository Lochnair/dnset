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

#include <linux/rculist.h>
#include <linux/slab.h>

#include "storage.h"
#include "util.h"

LIST_HEAD(group_list_head);

static bool addNode(struct node *root, char *word) {
	struct node *curr = root;

	WARN_ON(root == NULL);
	WARN_ON(word == NULL);

	while (*word) {
		WARN_ON(*word < 0);

		if (curr->children[*word] == NULL) {
			curr->children[*word] =
			    (struct node *)kzalloc(sizeof(struct node), GFP_KERNEL);

			// Make sure the memory we asked for got allocated
			if (curr->children[*word] == NULL)
				return false;

			curr->children[*word]->key = *word;
			curr->children[*word]->parent = curr;
		}

		curr = curr->children[*word];
		++word;
	}

	curr->isLeaf = true;

	return true;
}

static void destroyNode(struct node *curr) {
	int i;

	for (i = 0; i < TRIE_MAX_WIDTH; i++) {
		if (curr->children[i])
			destroyNode(curr->children[i]);
	}

	kfree(curr);
}

static struct node *findNode(struct node *root, char *word, bool wildcardMatch) {
	struct node *curr = root;

	while (*word != '\0') {
		WARN_ON(*word < 0);

		if (curr->children['*'] != NULL && wildcardMatch) {
			return curr->children['*'];
		} else if (curr->children[*word] != NULL) {
			curr = curr->children[*word];
			++word;
		} else {
			break;
		}
	}

	if (*word == '\0' && curr->isLeaf)
		return curr;

	return NULL;
}

static inline bool removeNode(struct node *curr) {
	int i;
	int childCount = 0;
	bool noChild = true;
	struct node *parentNode;

	if (curr == NULL)
		return false;


	for (i = 0; i < TRIE_MAX_WIDTH; i++)
	{
		if (curr->children[i] != NULL) {
			noChild = false;
			childCount++;
		}
	}

	curr->isLeaf = false;

	if (!noChild)
		return true;

	while (!curr->isLeaf && curr->parent != NULL && childCount == 0)
	{
		childCount = 0;
		parentNode = curr->parent;

		for (i = 0; i < TRIE_MAX_WIDTH; ++i)
		{
			if (parentNode->children[i] != NULL)
			{
				if (curr == parentNode->children[i])
				{
					// this is the child node we're deleting
					parentNode->children[i] = NULL;
					kfree(curr);
					curr = parentNode;
				}
				else
				{
					++childCount;
				}
			}
		}
	}

	return 0;
}

static void traverseNode(struct node *curr, char **dst)
{
	int i;

	if (curr->isLeaf)
	{
		struct node * ptr = curr;
		char * word = kzalloc(sizeof(char), GFP_KERNEL);
		unsigned int len = 1, dst_len = 0, word_len = 0;

		while (ptr)
		{

			word = krealloc(word, (len * sizeof(char)) + sizeof(char), GFP_KERNEL);
			word[word_len] = ptr->key;

			++len;
			++word_len;

			if (ptr->key == '\0')
				break;

			ptr = ptr->parent;
		}

		dst_len = strlen(*dst) + word_len + 1;

		if (dst_len - len == 0)
		{
			*dst = word;
		}
		else
		{
			*dst = krealloc(*dst, dst_len, GFP_KERNEL);
			strncat(strncat(*dst, "\n", 1), word, word_len);
		}
	}
	
	for (i = 0; i < TRIE_MAX_WIDTH; i++) {
		if (curr->children[i])
			traverseNode(curr->children[i], dst);
	}
}

bool addDomain(struct group *group, char *name) {
	return addNode(group->root_node, strrev(name));
}

bool delDomain(struct group *group, char *name) {
	struct node *node = findNode(group->root_node, strrev(name), false);
	return removeNode(node);
}

char *listDomains(struct group *group) {
	char *ret = kzalloc(sizeof(char), GFP_KERNEL);
	WARN_ON(group->root_node == NULL);

	traverseNode(group->root_node, &ret);

	return ret;
}

bool matchDomain(struct group *group, char *name) {
	return findNode(group->root_node, strrev(name), true) != NULL;
}

bool addGroup(char *group_name) {
	struct group *group = kzalloc(sizeof(struct group), GFP_KERNEL);

	if (group == NULL)
		return false;

	group->name = group_name;
	group->root_node = (struct node *)kzalloc(sizeof(struct node), GFP_KERNEL);

	if (group->root_node == NULL)
	{
		kfree(group->name);
		kfree(group);
		return false;
	}

	group->root_node->key = '\0';

	INIT_LIST_HEAD(&group->list);
	list_add_rcu(&group->list, &group_list_head);

	return true;
}

bool delGroup(char *name) {
	struct group *group = getGroup(name);

	if (!group)
		return false;

	list_del_rcu(&group->list);
	synchronize_rcu();
	destroyNode(group->root_node);
	kfree(group->name);
	kfree(group);

	return true;
}

void destroyGroups(void) {
	struct group *group;

	list_for_each_entry_rcu(group, &group_list_head, list) {
		list_del_rcu(&group->list);
		synchronize_rcu();
		destroyNode(group->root_node);
		kfree(group->name);
		kfree(group);
	}
}

struct group *getGroup(char *name) {
	struct group *group = NULL, *tmp = NULL;

	if (name == NULL) {
		printk(KERN_ERR "dnset: received null pointer for group_name");
		return NULL;
	}

	rcu_read_lock();
	list_for_each_entry_rcu(tmp, &group_list_head, list) {

		if (tmp == NULL) {
			printk(KERN_ERR "dnset: group is null");
			continue;
		}

		if (tmp->name == NULL) {
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

char *listGroups(void) {
	struct group *group;
	char *list = NULL;
	unsigned int len = 0;

	rcu_read_lock();
	list_for_each_entry_rcu(group, &group_list_head, list) {
		unsigned int curr_len = 0;

		if (group == NULL) {
			printk(KERN_ERR "dnset: group is null");
			continue;
		}

		if (group->name == NULL) {
			printk(KERN_ERR "dnset: group->name is null");
			continue;
		}

		curr_len = strlen(group->name);

		if (len == 0) {
			len += curr_len + 1; /* +1 for null-terminator */
			list = kzalloc(len, GFP_KERNEL);
			memcpy(list, group->name, len);
		} else {
			curr_len = strlen(group->name);
			len += curr_len + 1; /* +1 for \n */
			list = krealloc(list, len, GFP_KERNEL);
			strncat(strncat(list, "\n", 1), group->name, curr_len);
		}
	}
	rcu_read_unlock();

	return list;
}
