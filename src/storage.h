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

#define TRIE_MAX_WIDTH 255

struct node {
	char key;
	struct node *parent;
	struct node *children[TRIE_MAX_WIDTH];
	bool isLeaf;
};

struct group {
	char *name;
	struct node *root_node;
	struct list_head list;
};

bool addDomain(struct group *group, char *domain);
bool delDomain(struct group *group, char *domain);
char *listDomains(struct group *group);
bool matchDomain(struct group *group, char *domain);

bool addGroup(char *name);
bool delGroup(char *name);
void destroyGroups(void);
struct group *getGroup(char *name);
char *listGroups(void);