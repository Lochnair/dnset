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
 * GNU General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
