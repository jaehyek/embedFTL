NAND Flash F**king translation layer
======

A FTL layer to remapping the NAND device as legacy 512b/sector block device.

Features
===

Working in Progress. I don't think this layer is suitable for valuable use!

- [ ] Wear leveling 
- [ ] Bad block managing
- [ ] TRIM/GC

Stub Functions
===

is_bad() check a block 's quality

mark_bad() (some devices doesn't need that)

is_free() return 1 if a page is empty

read() read data from the NAND

write() write data to the NAND by PAGE

erase() erase a NAND block

copy() (some devices doesn't support that) copy a page to another page uses NAND's internal buffer

License
===

BSD


