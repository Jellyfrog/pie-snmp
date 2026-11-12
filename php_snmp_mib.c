/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   +----------------------------------------------------------------------+
*/

/*
 * Rebuilding the MIB tree parses every module again, including the ones
 * that every list shares (SNMPv2-SMI, SNMPv2-TC, IF-MIB, ...). When only
 * the list of modules changes, php_snmp_mib_switch() instead unloads the
 * modules the new list does not need and loads the missing ones.
 *
 * Modules kept: the requested modules that are loaded, plus everything they
 * import or read for MODULE-COMPLIANCE MODULE and AGENT-CAPABILITIES SUPPORTS
 * clauses (struct module deps, patches/0006), and the other modules in the
 * same file (reading a file loads every module in it), transitively. Everything else that is loaded (modules of the old
 * list, modules loaded on demand by MODULE::name lookups, files read with
 * read_mib()) is unloaded, so the tree matches a fresh build of the new list.
 *
 * A node that several modules define keeps the attributes of the module
 * that defined it first (last with MIB_REPLACE); unloading that module
 * leaves them in place. So it also rebuilds when a module to unload owns
 * the attributes of a node that a kept module still defines, and when, after
 * the switch, a node's attributes come from another module than in a fresh
 * build, which reads the modules in list order, each after its imports, and
 * when a node of an unloaded module stays behind as the parent of a kept one,
 * when a lookup by name finds another of several nodes with that name, and
 * when nodes with several names for one OID are in another order.
 *
 * It falls back to a full rebuild when anything other than the list differs
 * from the last full build: the MIB directories, the MIB parser options,
 * MIBFILES or PREFIX, and when the root module (SNMPv2-SMI) would not be
 * kept, since it owns the root nodes of the tree.
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/parse.h>

#include "php_snmp_mib.h"

static struct {
	bool valid;
	char *mibdirs;
	char *mibfiles;
	char *prefix;
	int parse_label, comment_term, replace, warnings, errors, save_descrs;
} last;

static char *dup_or_null(const char *s)
{
	return s ? strdup(s) : NULL;
}

static bool str_eq(const char *a, const char *b)
{
	return a == b || (a && b && strcmp(a, b) == 0);
}

static void read_options(int *parse_label, int *comment_term, int *replace, int *warnings, int *errors, int *save_descrs)
{
	*parse_label = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIB_PARSE_LABEL);
	*comment_term = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIB_COMMENT_TERM);
	*replace = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIB_REPLACE);
	*warnings = netsnmp_ds_get_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIB_WARNINGS);
	*errors = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIB_ERRORS);
	*save_descrs = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_SAVE_MIB_DESCRS);
}

void php_snmp_mib_forget(void)
{
	free(last.mibdirs);
	free(last.mibfiles);
	free(last.prefix);
	memset(&last, 0, sizeof(last));
}

static void remember(void)
{
	php_snmp_mib_forget();
	last.mibdirs = dup_or_null(netsnmp_get_mib_directory());
	last.mibfiles = dup_or_null(netsnmp_getenv("MIBFILES"));
	last.prefix = dup_or_null(netsnmp_getenv("PREFIX"));
	read_options(&last.parse_label, &last.comment_term, &last.replace,
		&last.warnings, &last.errors, &last.save_descrs);
	last.valid = true;
}

static bool same_as_last(void)
{
	int parse_label, comment_term, replace, warnings, errors, save_descrs;

	if (!last.valid) {
		return false;
	}
	read_options(&parse_label, &comment_term, &replace, &warnings, &errors, &save_descrs);
	return str_eq(last.mibdirs, netsnmp_get_mib_directory())
		&& str_eq(last.mibfiles, netsnmp_getenv("MIBFILES"))
		&& str_eq(last.prefix, netsnmp_getenv("PREFIX"))
		&& last.parse_label == parse_label && last.comment_term == comment_term
		&& last.replace == replace && last.warnings == warnings
		&& last.errors == errors && last.save_descrs == save_descrs;
}

static void set_mibs_env(const char *value)
{
#ifdef _WIN32
	_putenv_s("MIBS", value ? value : "");
#else
	if (value) {
		setenv("MIBS", value, 1);
	} else {
		unsetenv("MIBS");
	}
#endif
}

static void rebuild(const char *mibs)
{
	char *saved = dup_or_null(getenv("MIBS"));

	shutdown_mib();
	set_mibs_env(mibs);
	netsnmp_init_mib();
	set_mibs_env(saved);
	free(saved);
	remember();
}

static bool is_loaded(const struct module *mp)
{
	return mp && mp->no_imports != -1;
}

static void keep_with_imports(char *keep, int nmods, int modid)
{
	struct module *mp;
	int i;

	if (modid < 0 || modid >= nmods || keep[modid]) {
		return;
	}
	keep[modid] = 1;
	mp = find_module(modid);
	if (!is_loaded(mp)) {
		return;
	}
	for (i = 0; i < mp->no_imports; i++) {
		keep_with_imports(keep, nmods, mp->imports[i].modid);
	}
	for (i = 0; i < mp->no_deps; i++) {
		keep_with_imports(keep, nmods, mp->deps[i]);
	}
	for (i = 0; i < nmods; i++) {
		struct module *other = find_module(i);
		if (i != modid && is_loaded(other) && other->file && mp->file && strcmp(other->file, mp->file) == 0) {
			keep_with_imports(keep, nmods, i);
		}
	}
}

/* Would unloading the modules marked in drop leave a node with attributes
 * from an unloaded module? */
static bool leaves_stale_nodes(struct tree *tp, const char *drop, int nmods, bool replace)
{
	for (; tp; tp = tp->next_peer) {
		if (tp->number_modules > 1) {
			int owner = tp->module_list[replace ? tp->number_modules - 1 : 0];
			if (owner >= 0 && owner < nmods && drop[owner]) {
				int i;
				for (i = 0; i < tp->number_modules; i++) {
					int m = tp->module_list[i];
					if (m < 0 || m >= nmods || !drop[m]) {
						return true;
					}
				}
			}
		}
		if (leaves_stale_nodes(tp->child_list, drop, nmods, replace)) {
			return true;
		}
	}
	return false;
}

/* Number the modules in the order a fresh build links their nodes into the
 * tree: the list in order, each module after its imports (in IMPORTS order)
 * and the modules its MODULE and SUPPORTS clauses read. */
static void rank_module(int modid, int *rank, int nmods, int *next)
{
	struct module *mp;
	int i;

	if (modid < 0 || modid >= nmods || rank[modid] != -1) {
		return;
	}
	rank[modid] = INT_MAX;  /* in progress, for import cycles */
	mp = find_module(modid);
	if (is_loaded(mp)) {
		for (i = 0; i < mp->no_imports; i++) {
			rank_module(mp->imports[i].modid, rank, nmods, next);
		}
		for (i = 0; i < mp->no_deps; i++) {
			rank_module(mp->deps[i], rank, nmods, next);
		}
	}
	rank[modid] = (*next)++;
}

/* Does every node that several modules define have the attributes of the
 * module a fresh build would read first (last with MIB_REPLACE)? */
struct sibling {
	u_long subid;
	int pos, rank;
};

static int sibling_cmp(const void *a, const void *b)
{
	const struct sibling *x = a, *y = b;
	if (x->subid != y->subid) {
		return x->subid < y->subid ? -1 : 1;
	}
	return x->pos - y->pos;
}

/* Siblings with the same subid (one OID, several names): a lookup by OID
 * finds the first. Each new node goes before the existing ones, so in a
 * fresh build the module read last comes first. */
static bool sibling_order_matches_fresh(struct tree *first, const int *rank, int nmods)
{
	struct tree *tp;
	struct sibling *v;
	int n = 0, i;
	bool ok = true;

	for (tp = first; tp; tp = tp->next_peer) {
		n++;
	}
	if (n < 2) {
		return true;
	}
	v = malloc(n * sizeof(*v));
	if (!v) {
		return false;
	}
	for (i = 0, tp = first; tp; tp = tp->next_peer, i++) {
		v[i].subid = tp->subid;
		v[i].pos = i;
		v[i].rank = tp->modid >= 0 && tp->modid < nmods ? rank[tp->modid] : -1;
	}
	qsort(v, n, sizeof(*v), sibling_cmp);
	for (i = 1; i < n && ok; i++) {
		if (v[i].subid == v[i - 1].subid
				&& (v[i].rank < 0 || v[i - 1].rank < 0 || v[i].rank == INT_MAX
					|| v[i - 1].rank == INT_MAX || v[i - 1].rank < v[i].rank)) {
			ok = false;
		}
	}
	free(v);
	return ok;
}

static bool owners_match_fresh(struct tree *tp, const int *rank, int nmods, bool replace)
{
	if (!sibling_order_matches_fresh(tp, rank, nmods)) {
		return false;
	}
	for (; tp; tp = tp->next_peer) {
		/* Unloading keeps a node without modules while it has children; a
		 * fresh build never has one (it would make the parent anonymous). */
		if (tp->number_modules == 0) {
			return false;
		}
		/* A node left without modules and then defined again keeps the
		 * modid -1 that unloading gave it. */
		if (tp->modid != tp->module_list[0]) {
			return false;
		}
		/* A lookup by name returns the node read last (each new node goes
		 * to the front of its hash chain): a fresh build finds the one of
		 * the module it reads last, a switched tree may find a kept one. */
		if (tp->label) {
			struct tree *found = find_tree_node(tp->label, -1);
			if (found && found != tp) {
				int a = tp->modid, b = found->modid;
				if (a < 0 || a >= nmods || b < 0 || b >= nmods || rank[a] < 0 || rank[b] < 0
						|| rank[a] == INT_MAX || rank[b] == INT_MAX || rank[a] > rank[b]) {
					return false;
				}
			}
		}
		if (tp->number_modules > 1) {
			int i, best = -1;
			for (i = 0; i < tp->number_modules; i++) {
				int m = tp->module_list[i];
				if (m < 0 || m >= nmods || rank[m] < 0 || rank[m] == INT_MAX) {
					return false;
				}
				if (best < 0 || (replace ? rank[m] > rank[best] : rank[m] < rank[best])) {
					best = m;
				}
			}
			if (tp->module_list[replace ? tp->number_modules - 1 : 0] != best) {
				return false;
			}
		}
		if (!owners_match_fresh(tp->child_list, rank, nmods, replace)) {
			return false;
		}
	}
	return true;
}

/* Switch the loaded tree to mibs in place. Returns false, without changing
 * anything, if that is not possible. */
static bool switch_in_place(const char *mibs)
{
	char *list, *entry, *st = NULL, *keep;
	int nmods, modid, root, *rank, next = 0;
	bool replace = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIB_REPLACE);

	if (!same_as_last() || *mibs == '+' || *mibs == '-') {
		return false;
	}

	/* Module ids are assigned in sequence and stay valid until the tree is
	 * shut down. */
	for (nmods = 0; find_module(nmods); nmods++);
	keep = calloc(nmods ? nmods : 1, 1);
	list = strdup(mibs);
	if (!keep || !list) {
		free(keep);
		free(list);
		return false;
	}

	for (entry = strtok_r(list, ENV_SEPARATOR, &st); entry; entry = strtok_r(NULL, ENV_SEPARATOR, &st)) {
		if (strcasecmp(entry, "ALL") == 0) {
			free(keep);
			free(list);
			return false;
		}
		modid = which_module(entry);
		if (modid >= 0 && is_loaded(find_module(modid))) {
			keep_with_imports(keep, nmods, modid);
		}
	}
	free(list);

	root = which_module("SNMPv2-SMI");
	if (root < 0 || root >= nmods || !keep[root]) {
		free(keep);
		return false;
	}

	/* From here on keep[] marks the modules to unload. */
	for (modid = 0; modid < nmods; modid++) {
		keep[modid] = !keep[modid] && is_loaded(find_module(modid));
	}
	if (leaves_stale_nodes(get_tree_head(), keep, nmods, replace)) {
		free(keep);
		return false;
	}
	for (modid = 0; modid < nmods; modid++) {
		if (keep[modid]) {
			netsnmp_unload_module(find_module(modid)->name);
		}
	}
	free(keep);

	list = strdup(mibs);
	if (!list) {
		/* The tree is half switched: start over. */
		php_snmp_mib_forget();
		return false;
	}
	st = NULL;
	for (entry = strtok_r(list, ENV_SEPARATOR, &st); entry; entry = strtok_r(NULL, ENV_SEPARATOR, &st)) {
		netsnmp_read_module(entry);
	}
	adopt_orphans();

	/* The tree is switched; check that shared nodes got the attributes a
	 * fresh build would give them. If not, the caller rebuilds. */
	for (nmods = 0; find_module(nmods); nmods++);
	rank = malloc((nmods ? nmods : 1) * sizeof(int));
	if (!rank) {
		free(list);
		php_snmp_mib_forget();
		return false;
	}
	for (modid = 0; modid < nmods; modid++) {
		rank[modid] = -1;
	}
	strcpy(list, mibs);
	st = NULL;
	for (entry = strtok_r(list, ENV_SEPARATOR, &st); entry; entry = strtok_r(NULL, ENV_SEPARATOR, &st)) {
		rank_module(which_module(entry), rank, nmods, &next);
	}
	free(list);
	if (!owners_match_fresh(get_tree_head(), rank, nmods, replace)) {
		free(rank);
		php_snmp_mib_forget();
		return false;
	}
	free(rank);

	return true;
}

bool php_snmp_mib_switch(const char *mibs)
{
	if (switch_in_place(mibs)) {
		return true;
	}
	rebuild(mibs);
	return false;
}
