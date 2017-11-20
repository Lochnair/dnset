#define DNSET_GENL_FAMILY_NAME "dnset"
#define DNSET_GENL_VERSION 0x1

/* Attributes */
enum {
	DNSET_A_UNSPEC,
	DNSET_A_DOMAIN,
	DNSET_A_GROUP,
	DNSET_A_LIST,
	__DNSET_A_MAX,
};

#define DNSET_A_MAX (__DNSET_A_MAX - 1)

/* Commands: enumeration off all commands
 * used by userspace applications to identify command to be executed
 */
enum {
	DNSET_C_UNSPEC,
	DNSET_C_ADD_DOMAIN,
	DNSET_C_ADD_GROUP,
	DNSET_C_DEL_DOMAIN,
	DNSET_C_DEL_GROUP,
	DNSET_C_LIST_GROUPS,
	DNSET_C_LIST_DOMAINS,
	DNSET_C_MATCH_DOMAIN,
	__DNSET_C_MAX,
};
#define DNSET_C_MAX (__DNSET_C_MAX - 1)
