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

typedef struct domain_node {
	char key;
	struct domain_node * next;
	struct domain_node * prev;
	struct domain_node * parent;
	struct domain_node * children;
	bool is_word;
	spinlock_t lock;
} domain_node;

typedef struct domain_group {
	char * name;
	domain_node * root_node;
	struct list_head list;
	struct rcu_head	rcu;
} domain_group;

int domain_add(domain_group * group, char * name);
int domain_del(domain_group * group, char * name);
char * domain_list(domain_group * group);
domain_node * domain_search(domain_group * group, char * name);

int group_add(char * name);
int group_del(char * name);
void group_destroy(void);
domain_group * group_get(char * name);
char * group_list(void);

static inline char * strrev(char * str)
{
	const int l = strlen(str);
	char* rs = kmalloc(l + 1, GFP_KERNEL);
	int i;

	for(i = 0; i < l; i++) {
		rs[i] = str[l - 1 - i];
	}

	rs[l] = '\0';

	return rs;
}
