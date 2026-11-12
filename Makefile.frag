# Rebuild the bundled Net-SNMP if it is missing (e.g. after "make clean",
# which deletes every *.a in the build tree).
$(SNMP_SHARED_DEPENDENCIES):
	$(SHELL) $(SNMP_BUILD_SCRIPT) $(SNMP_BUILD_SRCDIR) $(SNMP_BUILD_DIR)
