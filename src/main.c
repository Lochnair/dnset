#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include "netlink.h"
#include "storage.h"

static struct genl_family dnset_gnl_family;

/* Attribute policy */
static struct nla_policy dnset_genl_policy[DNSET_A_MAX + 1] = {
	[DNSET_A_DOMAIN]	= { .type = NLA_STRING },
	[DNSET_A_GROUP]		= { .type = NLA_STRING },
};

static int dnset_add_domain(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	char *domain, *group_name;
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
		printk(KERN_ERR "dnset: domain %s already in group %s", domain, (char *)nla_data(tb[DNSET_A_GROUP]));
		return -1;
	}

	if (domain_add(group, domain) == NULL)
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

static int dnset_add_group(struct sk_buff *skb, struct genl_info *info)
{
	int ret;
	struct nlattr *tb[__DNSET_A_MAX];
	char *group_name;

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

	group_new(group_name);

	return 0;
}

static int dnset_del_domain(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

static int dnset_del_group(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}

/* Operation definitions */
static struct genl_ops dnset_gnl_ops[] = {
	{
		.cmd = DNSET_C_ADD_DOMAIN,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = dnset_add_domain,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_ADD_GROUP,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = dnset_add_group,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_DEL_DOMAIN,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = dnset_del_domain,
		.dumpit = NULL,
	},
	{
		.cmd = DNSET_C_DEL_GROUP,
		.flags = 0,
		.policy = dnset_genl_policy,
		.doit = dnset_del_group,
		.dumpit = NULL,
	},
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

module_init(dnset_init);
module_exit(dnset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nils Andreas Svee <nils@stokkdalen.no>");
MODULE_DESCRIPTION("Domain sets");
