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
	int family_id, ret;
	struct nl_msg *msg;

	sock = nl_socket_alloc();

	ret = genl_connect(sock);

	if (ret != 0) {
		printf("Couldn't connect to the NETLINK_GENERIC Netlink protocol");
		nl_socket_free(sock);
		return 1;
	}

	family_id = genl_ctrl_resolve(sock, DNSET_GENL_FAMILY_NAME);

	/* First domain */
	if (!(msg = nlmsg_alloc())) {
		printf("Couldn't allocate space for the message");
		nl_socket_free(sock);
		return 5;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, DNSET_C_ADD_DOMAIN, DNSET_GENL_VERSION);

	ret = nla_put_string(msg, DNSET_A_GROUP, group);

	if (ret != 0) {
		printf("Couldn't add group attribute to message.\n");
		nl_socket_free(sock);
		return 7;
	}

	ret = nla_put_string(msg, DNSET_A_DOMAIN, domain);

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

	free(msg);
	nl_socket_free(sock);

	return 0;
}

static inline int add_group(char * group)
{
	int family_id, ret;
	struct nl_msg *msg;

	sock = nl_socket_alloc();

	ret = genl_connect(sock);

	if (ret != 0) {
		printf("Couldn't connect to the NETLINK_GENERIC Netlink protocol");
		nl_socket_free(sock);
		return 1;
	}

	family_id = genl_ctrl_resolve(sock, DNSET_GENL_FAMILY_NAME);

	/* Add group */
	if (!(msg = nlmsg_alloc())) {
		printf("Couldn't allocate space for the message");
		nl_socket_free(sock);
		return 2;
	}

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, DNSET_C_ADD_GROUP, DNSET_GENL_VERSION);

	ret = nla_put_string(msg, DNSET_A_GROUP, group);

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
	nl_socket_free(sock);

	return 0;
}

void print_usage()
{
	printf("Usage: \n");
	printf("dnset \n");
	printf("      add <group> [domain]\n");
	printf("      del <group> [domain]\n");
	printf("      list [group]\n");
	printf("      match <group> <domain>\n");
}

int main(int argc, char **argv)
{
	printf("argc = %d\n", argc);
	if (argc < 2) {
		print_usage();
		return 1;
	}

	if(strcmp("add", argv[1]) == 0) {
		if (argc == 3) {
			add_group(argv[2]);
		}
		else if (argc == 4) {
			add_domain(argv[2], argv[3]);
		} else {
			print_usage();
			return 1;
		}
	} else if (strcmp("del", argv[1]) == 0) {
		// TODO: Impl delete
	} else {
		print_usage();
		return 1;
	}

	return 0;
}
