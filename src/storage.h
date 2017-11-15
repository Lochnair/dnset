#include <linux/radix-tree.h>

typedef struct domain_node {
	u8 key;
	struct domain_node * next;
	struct domain_node * prev;
	struct domain_node * parent;
	struct domain_node * children;
	bool is_word;
} domain_node;

typedef struct domain_group {
	char * name;
	domain_node * root_node;
	struct list_head list;
} domain_group;

domain_node * domain_add(domain_group * group, char * name);
void domain_del(domain_group * group, char * name);
domain_node * domain_search(domain_group * group, char * name);

int group_del(char * name);
void group_destroy(void);
domain_group * group_get(char * name);
void group_new(char * name);


domain_node * node_add(domain_node * root, char * key);
domain_node * node_create(u8 key);
domain_node * node_create_root(void);
void node_destroy(domain_node * root);
domain_node * node_lookup(domain_node * root, char * key);
void node_remove(domain_node * root, char * key);

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
