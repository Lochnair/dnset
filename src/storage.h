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
	u8 key;
	struct domain_node * next;
	struct domain_node * prev;
	struct domain_node * parent;
	struct domain_node * children;
	bool is_word;
} domain_node;

typedef struct domain_group {
	u8 * name;
	domain_node * root_node;
	struct list_head list;
} domain_group;

int domain_add(domain_group * group, u8 * name);
int domain_del(domain_group * group, u8 * name);
domain_node * domain_search(domain_group * group, u8 * name);

int group_add(u8 * name);
int group_del(u8 * name);
void group_destroy(void);
domain_group * group_get(u8 * name);
u8 * group_list(void);

static inline u8 * strrev(u8 * str)
{
	const int l = strlen(str);
	u8* rs = kmalloc(l + 1, GFP_KERNEL);
	int i;

	for(i = 0; i < l; i++) {
		rs[i] = str[l - 1 - i];
	}

	rs[l] = '\0';

	return rs;
}
