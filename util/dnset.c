#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/genetlink.h>
#include "netlink.h"

#define DNSET_GENL_FAMILY_NAME "dnset"
#define DNSET_GENL_VERSION 0x1

struct nl_sock* sock;

static inline int add_domain(char * group, char * domain)
{
	return 0;
}

static inline int add_group(char * group)
{
	return 0;
}

int main(int argc, char **argv)
{
	int family_id, ret;
	struct nl_msg *msg;

	printf("Domain: %s\n", argv[2]);
	printf("Group: %s\n", argv[1]);

	sock = nl_socket_alloc();

	ret = genl_connect(sock);

	if (ret != 0) {
		printf("Couldn't connect to the NETLINK_GENERIC Netlink protocol");
		nl_socket_free(sock);
		return 1;
	}

	family_id = genl_ctrl_resolve(sock, DNSET_GENL_FAMILY_NAME);

	if (!(msg = nlmsg_alloc())) {
		printf("Couldn't allocate space for the message");
		nl_socket_free(sock);
		return 2;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, DNSET_C_ADD_GROUP, DNSET_GENL_VERSION);

	ret = nla_put_string(msg, DNSET_A_GROUP, argv[1]);

	if (ret != 0) {
		printf("Couldn't add group attribute to message.\n");
		nl_socket_free(sock);
		return 3;
	}

	ret = nl_send_auto(sock, msg);

	if (ret < 0) {
		printf("Couldn't send message.\n");
		nl_socket_free(sock);
		return 4;
	}

	free(msg);

	if (!(msg = nlmsg_alloc())) {
		printf("Couldn't allocate space for the message");
		nl_socket_free(sock);
		return 5;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, DNSET_C_ADD_DOMAIN, DNSET_GENL_VERSION);

	ret = nla_put_string(msg, DNSET_A_GROUP, argv[1]);

	if (ret != 0) {
		printf("Couldn't add group attribute to message.\n");
		nl_socket_free(sock);
		return 7;
	}

	ret = nla_put_string(msg, DNSET_A_DOMAIN, argv[2]);

	if (ret != 0) {
		printf("Couldn't add domain attribute to message.\n");
		nl_socket_free(sock);
		return 8;
	}

	ret = nl_send_auto(sock, msg);

	if (ret < 0) {
		printf("Couldn't send message.\n");
		nl_socket_free(sock);
		return 9;
	}

	nl_socket_free(sock);
	return 0;
}
