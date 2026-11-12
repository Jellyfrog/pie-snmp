/*
 * Checks that php_snmp_mib_switch() (php_snmp_mib.c) gives the same MIB
 * tree as a fresh build, for every step of a sequence of module lists.
 *
 * Input lines: MIBDIRS <TAB> MIBS (module list, "-" for none) <TAB>
 * modules loaded on demand afterwards (":" separated, "-" for none).
 *
 * Build, after ./configure (which builds the bundled Net-SNMP):
 *   cc -O2 -Inetsnmp-build/include -I. -o mib-switch-check \
 *      tools/mib-switch-check.c php_snmp_mib.c netsnmp-build/lib/libnetsnmp.a \
 *      -lssl -lcrypto -lm
 *
 * Run both modes and compare the hashes, which cover every node (OID,
 * label, owning modules, type, access, status, textual convention, hint,
 * units, enums, ranges, indexes, augments, varbinds) and which node a
 * lookup by name finds:
 *   ./mib-switch-check fresh SEQ > fresh.txt
 *   ./mib-switch-check incr SEQ > incr.txt
 *   paste -d' ' fresh.txt incr.txt | awk '$2 != $5 { print "step", $1, "differs" }'
 * Output per step: "step hash switched_in_place". With a step number as
 * third argument, the tree of that step is printed instead (diff those).
 * NODUMP=1 skips the dumps, for timing.
 *
 * Sibling order and the numbers in anonymous#N labels are left out: the
 * order of siblings only matters between siblings with the same subid
 * (kept), and the anonymous counter is never reset by Net-SNMP, so it
 * already differs between fresh builds in one process.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/parse.h>

#include "php_snmp_mib.h"

static FILE *out;
static uint64_t h;

static void emit(const char *fmt, ...)
{
	char buf[8192];
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	for (char *a = strstr(buf, "anonymous#"); a; a = strstr(a + 1, "anonymous#")) {
		char *d = a + 10, *e = d;
		while (*e >= '0' && *e <= '9') e++;
		memmove(d, e, strlen(e) + 1);
	}
	n = strlen(buf);
	for (int i = 0; i < n && i < (int) sizeof(buf); i++) {
		h ^= (unsigned char) buf[i];
		h *= 1099511628211ULL;
	}
	if (out) fputs(buf, out);
}

static const char *modname(int modid, char *buf)
{
	struct module *mp = find_module(modid);
	snprintf(buf, 256, "%s", mp ? mp->name : "#");
	return buf;
}

static int cmpstr(const void *a, const void *b)
{
	return strcmp(*(char *const *) a, *(char *const *) b);
}

static void path(struct tree *tp, char *buf, size_t len)
{
	char tmp[2048] = "";
	struct tree *p;
	buf[0] = 0;
	for (p = tp; p; p = p->parent) {
		snprintf(tmp, sizeof(tmp), ".%lu%s", p->subid, buf);
		snprintf(buf, len, "%s", tmp);
	}
}

static void dump_children(struct tree *first, int depth);

static void dump_node(struct tree *tp, int depth)
{
	char m[256], p[2048], q[2048];
	struct enum_list *ep;
	struct range_list *rp;
	struct index_list *ip;
	struct varbind_list *vp;
	struct tree *found;
	char *mods[256];
	int i, n;

	path(tp, p, sizeof(p));
	emit("%s %s", p, tp->label ? tp->label : "-");
	n = tp->number_modules < 256 ? tp->number_modules : 256;
	for (i = 0; i < n; i++) mods[i] = strdup(modname(tp->module_list[i], m));
	qsort(mods, n, sizeof(char *), cmpstr);
	emit(" mods=");
	for (i = 0; i < n; i++) { emit("%s,", mods[i]); free(mods[i]); }
	emit(" own=%s t=%d a=%d s=%d", modname(tp->modid, m), tp->type, tp->access, tp->status);
	if (tp->tc_index >= 0) emit(" tc=%s", get_tc_descriptor(tp->tc_index));
	if (tp->hint) emit(" hint=%s", tp->hint);
	if (tp->units) emit(" units=%s", tp->units);
	if (tp->augments) emit(" aug=%s", tp->augments);
	if (tp->defaultValue) emit(" def=%s", tp->defaultValue);
	if (tp->enums) { emit(" enums="); for (ep = tp->enums; ep; ep = ep->next) emit("%d:%s,", ep->value, ep->label); }
	if (tp->ranges) { emit(" ranges="); for (rp = tp->ranges; rp; rp = rp->next) emit("%d-%d,", rp->low, rp->high); }
	if (tp->indexes) { emit(" idx="); for (ip = tp->indexes; ip; ip = ip->next) emit("%s%s,", ip->isimplied ? "*" : "", ip->ilabel); }
	if (tp->varbinds) { emit(" vb="); for (vp = tp->varbinds; vp; vp = vp->next) emit("%s,", vp->vblabel); }
	emit(" po=%d", tp->printomat != NULL);
	if (tp->label) {
		/* which node a lookup by name finds depends on insertion order */
		found = find_tree_node(tp->label, -1);
		if (found) { path(found, q, sizeof(q)); emit(" lookup=%s", q); }
	}
	emit("\n");
	dump_children(tp->child_list, depth + 1);
}

/* Children in subid order; children with the same subid keep their list
 * order, which decides which one an OID lookup finds. */
static void dump_children(struct tree *first, int depth)
{
	struct tree *c, **v;
	int n = 0, i, j;
	for (c = first; c; c = c->next_peer) n++;
	if (!n) return;
	v = malloc(n * sizeof(*v));
	for (i = 0, c = first; c; c = c->next_peer) v[i++] = c;
	for (i = 1; i < n; i++) { /* stable insertion sort by subid */
		struct tree *t = v[i];
		for (j = i - 1; j >= 0 && v[j]->subid > t->subid; j--) v[j + 1] = v[j];
		v[j + 1] = t;
	}
	for (i = 0; i < n; i++) dump_node(v[i], depth);
	free(v);
}

static void dump(void)
{
	char m[256];
	struct module *mp;
	emit("loaded:");
	for (int i = 0; (mp = find_module(i)); i++) if (mp->no_imports != -1) emit(" %s", modname(i, m));
	emit("\n");
	dump_children(get_tree_head(), 0);
}

int main(int argc, char **argv)
{
	char line[65536];
	int step = 0, dumpstep = argc > 3 ? atoi(argv[3]) : -1;
	int incr = strcmp(argv[1], "incr") == 0;
	FILE *f = fopen(argv[2], "r");

	setenv("MIBS", "", 1);
	unsetenv("MIBFILES");
	init_snmp("snmpapp");
	unsetenv("MIBS");
	snmp_disable_log();

	while (fgets(line, sizeof(line), f)) {
		char *dirs = strtok(line, "\t\n");
		char *mibs = strtok(NULL, "\t\n");
		char *od = strtok(NULL, "\t\n");
		bool in_place = false;
		if (!mibs || !strcmp(mibs, "-")) mibs = "";
		netsnmp_ds_set_string(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_MIBDIRS, dirs);
		if (incr) {
			in_place = php_snmp_mib_switch(mibs);
		} else {
			char *saved = NULL;
			shutdown_mib();
			setenv("MIBS", mibs, 1);
			netsnmp_init_mib();
			unsetenv("MIBS");
			(void) saved;
		}
		if (od && strcmp(od, "-")) {
			char *st = NULL;
			for (char *e = strtok_r(od, ":", &st); e; e = strtok_r(NULL, ":", &st)) netsnmp_read_module(e);
		}
		h = 1469598103934665603ULL;
		out = (step == dumpstep) ? stdout : NULL;
		if (!getenv("NODUMP")) dump();
		if (dumpstep < 0) printf("%d %016llx %d\n", step, (unsigned long long) h, in_place);
		if (step == dumpstep) break;
		step++;
	}
	return 0;
}
