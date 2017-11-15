#include <linux/list.h>
#include <linux/radix-tree.h>
#include <linux/slab.h>

#include "storage.h"

LIST_HEAD(group_list);

domain_node * domain_add(domain_group * group, char * name)
{
	return node_add(group->root_node, strrev(name));
}

void domain_del(domain_group * group, char * name)
{
	return node_remove(group->root_node, strrev(name));
}

domain_node * domain_search(domain_group * group, char * name)
{
	printk(KERN_INFO "dnset: domain_search root pointer: %p", group->root_node);
	return node_lookup(group->root_node, strrev(name));
}

int group_del(char * name)
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

	list_for_each_prev(pos, &group_list) {
		group = list_entry(pos, domain_group, list);
		list_del(&group->list);
		node_destroy(group->root_node);
		kfree(group->name);
		kfree(group);
	}
}

domain_group * group_get(char * name)
{
        domain_group * group;
        struct list_head * pos;

        if (name == NULL)
        {
                printk(KERN_ERR "dnset: received null pointer for group_name");
                return NULL;
        }

        printk(KERN_INFO "dnset: looking for group: %s", name);

        list_for_each(pos, &group_list) {
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

void group_new(char * group_name)
{
	domain_group * group = kmalloc(sizeof(domain_group), GFP_KERNEL);
	group->name = group_name;
	group->root_node = node_create_root();

	INIT_LIST_HEAD(&group->list);
	list_add(&group->list, &group_list);
}

noinline domain_node * node_add(domain_node * root, char * key) {
	domain_node * pTrav = NULL;

	if (root == NULL) {
		printk(KERN_ERR "dnset: Tried to add node to null tree");
		return NULL;
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
		return pTrav->children;
	}

	if (node_lookup(pTrav, key)) {
		printk(KERN_ERR "dnset: Duplicate key");
		return NULL;
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
			return node_add(pTrav->next, key);
		}
		pTrav = pTrav->next;
	}

	pTrav->next = node_create(*key);
	pTrav->next->parent = pTrav->parent;
	pTrav->next->prev = pTrav;

	if (!(*key)) {
		return pTrav;
	}

	key++;

	for (pTrav = pTrav->next; *key; pTrav = pTrav->children) {
		pTrav->children = node_create(*key);
		pTrav->children->parent = pTrav;
		key++;
	}

	pTrav->children = node_create('\0');
	pTrav->children->parent = pTrav;
	return pTrav;
}

noinline domain_node * node_create(u8 key) {
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

domain_node * node_create_root() {
	return node_create('\0');
}

noinline void node_destroy(domain_node * root) {
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

noinline domain_node * node_lookup(domain_node * root, char * key) {
	domain_node * level = root;
	int lvl = 0;

	printk(KERN_INFO "dnset: level root pointer: %p", level);

	if (root == NULL)
		return NULL;

	if (key == NULL)
		return NULL;

	printk(KERN_INFO "dnset: Looking up: %s", key);

	while (1)
	{
		domain_node * found = NULL;
		domain_node * curr;

		printk(KERN_INFO "dnset: recurse level: %d", lvl);

		for(curr = level; curr != NULL; curr = curr->next) {
			printk(KERN_INFO "dnset: curr pointer: %p", curr);
			printk(KERN_INFO "dnset: Current key: %d", curr->key);
			if (curr->key == *key) {
				found = curr;
				lvl++;
				break;
			}
		}

		if (found == NULL) {
			return NULL;
		}

		if (curr->key == '*') {
			domain_node * nulled = curr->children;
			if (nulled == NULL || nulled->key != '\0')
				printk(KERN_WARNING "dnset: Something weird happening here: %s", key);

			return nulled;
		}

		if (*key == '\0') {
			return curr;
		}

		level = found->children;
		key++;
	}
}

noinline void node_remove(domain_node * root, char * key) {
	domain_node * ptr = NULL;
	domain_node * tmp = NULL;

	if (root == NULL || key == NULL)
		return;

	ptr = node_lookup(root->children, key);

	if (ptr == NULL) {
		printk(KERN_ERR "dnset: Key [%s] not found in trie\n", key);
		return;
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
}
