# Net-SNMP patches

`*.patch` files in this directory are applied, in name order, to a copy of
the `net-snmp/` submodule before it is built (`patch -p1`, from the Net-SNMP
source root). Name them with a numeric prefix, e.g.
`0001-libsnmp-avoid-two-syscalls-per-udp-message.patch`.

Generate one from a Net-SNMP branch with:

```sh
git -C /path/to/net-snmp format-patch -o "$PWD/patches" master..my-branch
```

A patch that does not apply fails `./configure`. Adding, removing or editing
a patch (or moving the submodule) makes the next `./configure` rebuild
Net-SNMP from scratch.
