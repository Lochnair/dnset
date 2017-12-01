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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include "dnset.h"
#include "netlink.h"
#include "storage.h"

static struct genl_family dnset_gnl_family;

/* Attribute policy */
static struct nla_policy dnset_genl_policy[DNSET_A_MAX + 1] = {
	[DNSET_A_DOMAIN]	= { .type = NLA_STRING },
	[DNSET_A_GROUP]		= { .type = NLA_STRING },
	[DNSET_A_LIST]		= { .type = NLA_STRING },
	[DNSET_A_RESULT]	= { .type = NLA_STRING },
};

static int add_domain(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	u8 *domain, *group_name;
	domain_group * group;

	printk(KERN_INFO "dnset: Received Netlink message.\n");

	#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
	#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
	#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	domain = kmalloc(strlen(nla_data(tb[DNSET_A_DOMAIN]) + 1), GFP_KERNEL);
	strcpy(domain, nla_data(tb[DNSET_A_DOMAIN]));

	group_name = kmalloc(strlen(nla_data(tb[DNSET_A_GROUP]) + 1), GFP_KERNEL);
	strcpy(group_name, nla_data(tb[DNSET_A_GROUP]));

	if (domain == NULL || group_name == NULL)
	{
		printk(KERN_ERR "dnset: how about that null-pointer?");
		return -1;
	}

	printk(KERN_INFO "dnset: domain: %s", domain);
	printk(KERN_INFO "dnset: group: %s", group_name);

	group = group_get(group_name);

	if (group == NULL)
	{
		// Non-existant group
		printk(KERN_INFO "dnset: attempted to add domain to non-existant group: %s", group_name);
		return -1;
	}

	if (strlen(domain) > 253)
	{
		printk(KERN_ERR "dnset: domain too long");
		return -1;
	}

	if (strlen(domain) < 1)
	{
		printk(KERN_ERR "dnset: domain too short");
		return -1;
	}

	if (domain_search(group, domain))
	{
		printk(KERN_ERR "dnset: domain %s already in group %s", domain, (u8 *)nla_data(tb[DNSET_A_GROUP]));
		return -1;
	}

	if (domain_add(group, domain) != 0)
	{
		printk(KERN_ERR "dnset: Something went horribly wrong.");
		return -1;
	}

	if (domain_search(group, domain) == NULL)
	{
		printk(KERN_ERR "dnset: just added domain not found.");
		return -1;
	}

	return 0;
}

static int add_group(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	u8 *group_name;

	#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
	#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
	#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	group_name = kmalloc(strlen(nla_data(tb[DNSET_A_GROUP]) + 1), GFP_KERNEL);
	strcpy(group_name, nla_data(tb[DNSET_A_GROUP]));

	if (strlen(group_name) < 1)
	{
		return -1;
	}

	if (group_get(group_name) != NULL)
	{
		printk(KERN_ERR "dnset: group %s already exists", group_name);
		return -1;
	}

	if (group_add(group_name) != 0)
	{
		printk(KERN_ERR "dnset: something went wrong while adding group: %s", group_name);
		return -1;
	}

	return 0;
}

static int del_domain(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

static int del_group(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

static int list_domains(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	domain_group * group;
	u8 *group_name;
	char * list;

	#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
	#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
	#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	group_name = kmalloc(strlen(nla_data(tb[DNSET_A_GROUP]) + 1), GFP_KERNEL);
	strcpy(group_name, nla_data(tb[DNSET_A_GROUP]));

	if (strlen(group_name) < 1)
	{
		return -1;
	}

	if ((group = group_get(group_name)) == NULL)
	{
		printk(KERN_ERR "dnset: group %s doesn't exist", group_name);
		return -1;
	}

	list = domain_list(group);

	kfree(group_name);
	return 0;
}

static int list_groups(struct sk_buff *skb, struct genl_info *info)
{
	char * result = group_list();
	printk(KERN_INFO "dnset: list: %s", result);
	kfree(result);
	return 0;
}

static int match_domain(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	u8 *domain, *group_name;

	printk(KERN_INFO "dnset: Received Netlink message.\n");

	#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
	#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
	#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	domain = kmalloc(strlen(nla_data(tb[DNSET_A_DOMAIN]) + 1), GFP_KERNEL);
	strcpy(domain, nla_data(tb[DNSET_A_DOMAIN]));

	group_name = kmalloc(strlen(nla_data(tb[DNSET_A_GROUP]) + 1), GFP_KERNEL);
	strcpy(group_name, nla_data(tb[DNSET_A_GROUP]));

	if (!dnset_match(group_name, domain))
		return -1;

	return 0;
}

/* Operation definitions */
static struct genl_ops dnset_gnl_ops[] = {
	{
		.cmd = DNSET_C_ADD_DOMAIN,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = add_domain,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_ADD_GROUP,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = add_group,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_DEL_DOMAIN,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = del_domain,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_DEL_GROUP,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = del_group,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_LIST_DOMAINS,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = list_domains,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_LIST_GROUPS,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = list_groups,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_MATCH_DOMAIN,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = match_domain,
		.dumpit = NULL,
	}
};

static struct genl_family dnset_gnl_family = {
	.hdrsize	= 0,
	.name		= DNSET_GENL_FAMILY_NAME,
	.version	= DNSET_GENL_VERSION,
	.module		= THIS_MODULE,
	.ops		= dnset_gnl_ops,
	.n_ops		= ARRAY_SIZE(dnset_gnl_ops),
	.maxattr	= DNSET_A_MAX,
};

static int __init dnset_init (void)
{
	int rc;
	rc = genl_register_family(&dnset_gnl_family);
	if (rc != 0) {
		printk(KERN_CRIT "dnset: Couldn't register genl family\n");
		return -EINVAL;
	}

	printk(KERN_INFO "dnset: Loaded dnset module.\n");

	return 0;
}

static void __exit dnset_exit (void)
{
	printk(KERN_INFO "Unloading dnset module.\n");
	genl_unregister_family(&dnset_gnl_family);
}

bool dnset_match(u8 * group_name, u8 * domain_name)
{
	domain_group * group;

	if (domain_name == NULL || group_name == NULL)
	{
		printk(KERN_ERR "dnset: how about that null-pointer?");
		return -1;
	}

	group = group_get(group_name);

	if (group == NULL)
	{
		// Non-existant group
		printk(KERN_INFO "dnset: attempted to add domain to non-existant group: %s", group_name);
		return -1;
	}

	if (strlen(domain_name) > 253)
	{
		printk(KERN_ERR "dnset: domain too long");
		return -1;
	}

	if (strlen(domain_name) < 1)
	{
		printk(KERN_ERR "dnset: domain too short");
		return -1;
	}

	return domain_search(group, domain_name);
}

module_init(dnset_init);
module_exit(dnset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nils Andreas Svee <nils@stokkdalen.no>");
MODULE_DESCRIPTION("Domain sets");
