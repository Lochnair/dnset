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

static int send_reply_to_userspace(struct genl_info *info, DN_ATTR attr, void * payload, size_t payload_size)
{
	struct sk_buff * msg = genlmsg_new(payload_size, GFP_KERNEL);
	void * reply;

	if (!msg)
		return -ENOMEM;

	reply = genlmsg_put_reply(msg, info, &dnset_gnl_family, 0, info->genlhdr->cmd);

	if (!reply)
		return -EMSGSIZE;

	nla_put(msg, attr, payload_size, payload);
	genlmsg_end(msg, reply);
	return genlmsg_reply(msg, info);
}

static int add_domain(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	char *domain, *group_name;
	char domain_len, group_len;
	struct group * group;

#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	domain_len = strlen(nla_data(tb[DNSET_A_DOMAIN])) + 1;
	group_len = strlen(nla_data(tb[DNSET_A_GROUP])) + 1;

	domain = kzalloc(domain_len, GFP_KERNEL);
	group_name = kzalloc(group_len, GFP_KERNEL);

	if (domain == NULL || group_name == NULL)
	{
		printk(KERN_ERR "dnset: how about that null-pointer?");
		send_reply_to_userspace(info, DNSET_A_RESULT, "There's a null-pointer in my message.", strlen("There's a null-pointer in my message.") + 1);
		return -1;
	}

	memcpy(domain, nla_data(tb[DNSET_A_DOMAIN]), domain_len);
	memcpy(group_name, nla_data(tb[DNSET_A_GROUP]), group_len);


	

	if (domain_len > 253)
	{
		printk(KERN_ERR "dnset: domain too long");
		send_reply_to_userspace(info, DNSET_A_RESULT, "The specified domain is too long.", strlen("The specified domain is too long.") + 1);
		goto error;
	}

	group = getGroup(group_name);

	if (group == NULL)
	{
		// Non-existant group
		printk(KERN_INFO "dnset: attempted to add domain to non-existant group: %s", group_name);
		send_reply_to_userspace(info, DNSET_A_RESULT, "The specified group does not exist.", strlen("The specified group does not exist.") + 1);
		goto error;
	}

	if (!addDomain(group, domain))
	{
		printk(KERN_ERR "dnset: Something went horribly wrong.");
		send_reply_to_userspace(info, DNSET_A_RESULT, "Couldn't add the domain.", strlen("Couldn't add the domain.") + 1);
		goto error;
	}

	kfree(domain);
	kfree(group_name);

	send_reply_to_userspace(info, DNSET_A_RESULT, "OK.", strlen("OK.") + 1);
	return 0;

error:
	kfree(domain);
	kfree(group_name);
	return -1;
}

static int add_group(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	char *group_name;
	char group_len;

#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	group_len = strlen(nla_data(tb[DNSET_A_GROUP])) + 1;
	group_name = kzalloc(group_len, GFP_KERNEL);
	WARN_ON(group_name == NULL);
	memcpy(group_name, nla_data(tb[DNSET_A_GROUP]), group_len);

	if (group_len < 1)
	{
		return -1;
	}

	if (getGroup(group_name) != NULL)
	{
		printk(KERN_ERR "dnset: group %s already exists", group_name);
		return -1;
	}

	if (!addGroup(group_name))
	{
		printk(KERN_ERR "dnset: something went wrong while adding group: %s", group_name);
		return -1;
	}

	send_reply_to_userspace(info, DNSET_A_RESULT, "OK.", strlen("OK.") + 1);
	return 0;
}

static int del_domain(struct sk_buff *skb, struct genl_info *info)
{
	int ret = 0;
	struct nlattr *tb[__DNSET_A_MAX];
	char *domain, *group_name;
	char domain_len, group_len;
	struct group * group;

#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	domain_len = strlen(nla_data(tb[DNSET_A_DOMAIN])) + 1;
	group_len = strlen(nla_data(tb[DNSET_A_GROUP])) + 1;

	domain = kzalloc(domain_len, GFP_KERNEL);
	group_name = kzalloc(group_len, GFP_KERNEL);

	WARN_ON(domain == NULL);
	WARN_ON(group_name == NULL);

	memcpy(domain, nla_data(tb[DNSET_A_DOMAIN]), domain_len);
	memcpy(group_name, nla_data(tb[DNSET_A_GROUP]), group_len);

	ret = 0;

	if (domain == NULL || group_name == NULL)
	{
		printk(KERN_ERR "dnset: how about that null-pointer?");
		return -1;
	}

	if (domain_len > 253)
	{
		printk(KERN_ERR "dnset: domain too long");
		ret = -1;
		goto free;
	}

	group = getGroup(group_name);

	if (group == NULL)
	{
		// Non-existant group
		printk(KERN_INFO "dnset: attempted to add domain to non-existant group: %s", group_name);
		ret = -1;
		goto free;
	}

	if (!delDomain(group, domain)) {
		printk(KERN_ERR "dnset: Something went wrong");
		ret = -1;
		goto free;
	}

free:
	kfree(domain);
	kfree(group_name);
	return 0;

}

static int del_group(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	char *group_name;
	char group_len;

#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	group_len = strlen(nla_data(tb[DNSET_A_GROUP])) + 1;
	group_name = kzalloc(group_len, GFP_KERNEL);
	WARN_ON(group_name == NULL);
	memcpy(group_name, nla_data(tb[DNSET_A_GROUP]), group_len);

	if (group_len < 1)
	{
		kfree(group_name);
		return -1;
	}

	if (!delGroup(group_name))
	{
		printk(KERN_ERR "dnset: Something wong.");
		kfree(group_name);
		return -1;
	}

	kfree(group_name);

	return 0;
}

static int list_domains(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	struct group * group;
	char *group_name;
	char group_len;
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

	group_len = strlen(nla_data(tb[DNSET_A_GROUP])) + 1;
	group_name = kzalloc(group_len, GFP_KERNEL);
	WARN_ON(group_name == NULL);
	memcpy(group_name, nla_data(tb[DNSET_A_GROUP]), group_len);

	if (group_len < 1)
	{
		kfree(group_name);
		return -1;
	}

	if ((group = getGroup(group_name)) == NULL)
	{
		printk(KERN_ERR "dnset: group %s doesn't exist", group_name);
		kfree(group_name);
		return -1;
	}

	list = listDomains(group);
	printk("list: %s", list);

	kfree(group_name);
	return 0;
}

static int list_groups(struct sk_buff *skb, struct genl_info *info)
{
	char * result = listGroups();
	printk(KERN_INFO "dnset: list: %s", result);
	kfree(result);
	return 0;
}

static int match_domain(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	char *domain, *group_name;
	char domain_len, group_len;

#if KERNEL_VERSION(4, 12, 0) > LINUX_VERSION_CODE
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy);
#else
	ret = genlmsg_parse(info->nlhdr, &dnset_gnl_family, tb, DNSET_A_MAX, dnset_genl_policy, NULL);
#endif

	if (ret != 0) {
		printk(KERN_ERR "dnset: Couldn't parse message.");
		return ret;
	}

	domain_len = strlen(nla_data(tb[DNSET_A_DOMAIN])) + 1;
	group_len = strlen(nla_data(tb[DNSET_A_GROUP])) + 1;

	domain = kzalloc(domain_len, GFP_KERNEL);
	group_name = kzalloc(group_len, GFP_KERNEL);

	WARN_ON(domain == NULL);
	WARN_ON(group_name == NULL);

	memcpy(domain, nla_data(tb[DNSET_A_DOMAIN]), domain_len);
	memcpy(group_name, nla_data(tb[DNSET_A_GROUP]), group_len);

	if (!dnset_match(group_name, domain)) {
		kfree(domain);
		kfree(group_name);
		return -1;
	}

	kfree(domain);
	kfree(group_name);

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

bool dnset_match(char * group_name, char * domain_name)
{
	struct group * group;

	if (domain_name == NULL || group_name == NULL)
	{
		printk(KERN_ERR "dnset: how about that null-pointer?");
		return false;
	}

	if (strlen(domain_name) > 253)
	{
		printk(KERN_ERR "dnset: domain too long");
		return false;
	}

	group = getGroup(group_name);

	if (group == NULL)
	{
		// Non-existant group
		printk(KERN_INFO "dnset: attempted to match domain with non-existant group: %s", group_name);
		return false;
	}

	printk("dnset: %s in %s: %s", domain_name, group_name, matchDomain(group, domain_name) ? "true" : "false");

	return matchDomain(group, domain_name);
}

module_init(dnset_init);
module_exit(dnset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nils Andreas Svee <nils@stokkdalen.no>");
MODULE_DESCRIPTION("Domain sets");
