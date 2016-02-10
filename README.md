NAND Flash F**king translation layer
======

A FTL layer to remapping the NAND device as legacy 512b/sector block device for small embedded device.

Features
===

Working in Progress. The goal of this FTL is to make a stable storage driver for industrial or embedded device usage.

- [ ] Wear leveling
- [x] Bad block managing
- [ ] TRIM/GC
- [ ] MLC/TLC support (Maybe it will be never implemented)
- [ ] SLC mode support (It still requires a strong software ECC)
- [ ] RS ECC parity support (Maybe it will be never implemented)
- [ ] BCH ECC parity support
- [x] Hamming ECC parity support
- [x] GD5F series Error-free NAND support
- [ ] STM32 NAND controller hardware ECC support
- [ ] Tiny device support (e.g. RAM less than 2KB device, e.g. AVR or 8051) (Maybe it will be never implemented)
- [ ] NOR Flash support (Maybe it will be never implemented)
 
Stub Functions
===

is_bad(nanddevice_t *this, block_t block) check a block 's quality (We will realize a BBT and internal BBM, it doesn't rely to factory Bad Block Mark)

read(nanddevice_t *this, page_t page, void *buffer) read data from the NAND by PAGE (if *buffer function is defined, this function is nolonger need. Besides, using Flash's internal buffer can reduce RAM usage)

write(nanddevice_t *this, page_t page, void *buffer) write data to the NAND by PAGE (if *buffer function is defined, this function is nolonger need. Besides, using Flash's internal buffer can reduce RAM usage)

erase(nanddevice_t *this, block_t block) erase a NAND block

/******* As optional function *******/

read_to_buffer(nanddevice_t *this, page_t page) Read a page to flash's internal buffer

write_from_buffer(nanddevice_t *this, page_t page) Write a page from flash's internal buffer

read_from_buffer(nanddevice_t *this, size_t offset, size_t length, void *buffer) Read data from flash's internal buffer

write_to_buffer(nanddevice_t *this, size_t offset, size_t length, bool keepdata, void *buffer) Write data to flash's internal buffer, if keepdata is not set, the buffer will be cleaned by it's internal logic)

copy(nanddevice_t *this, page_t page_original, page_t page_target) (some devices doesn't support that) Copy a page to another page using NAND's internal buffer (Maybe doesn't need)

copy_modify(nanddevice_t *this, page_t page_original, page_t page_target, (copy_callback *)modify_data_callback()) Copy a page to another page using NAND's internal buffer with some modified data by callback function. (e.g ECC parity data or LPN, this stub function maybe doesn't need too)

To upper layer 's interface
===

On small device, the ftl_t struct will not be compiled, and the buffer or table will be static allocated.

ftl_status ftl_status(ftl_t *this) The disk_status function returns the current drive status, e.g, NOINIT/WP/READY

ftl_status ftl_initialize(ftl_t *this) Initialize a FTL layer and allocate memory or CPU time to it. 

ftl_status ftl_read(ftl_t *this, void *buffer, offset_t sector, size_t count) Read sectors

ftl_status ftl_write(ftl_t *this, const void *buffer, offset_t sector, size_t count) Write sectors

ftl_status ftl_gc(ftl_t *this, const void *buffer) Garbage collection for small device.

ftl_status ftl_ioctl(ftl_t *this, uint8_t cmd, void *buffer) IOCTL interface, on small device, it will not be compiled, e.g. 8051.

Supported ioctl commands:

CTRL_SYNC Flush buffer immediately

GET_SECTOR_COUNT Returns number of available sectors

GET_SECTOR_SIZE Returns 512

GET_BLOCK_SIZE Returns page size or block size

CTRL_TRIM Remove unused sectors for garbage collection

CTRL_FORMAT Pending a low-level format on the NAND Flash, it doesn't clean BBT or BBM, it just clean the mapping table and clean all data blocks and log blocks.

CTRL_WIPE Pending a low-level format on the NAND Flash, it will clean all blocks on nand with BBT and blocks, this function is very dangerous, be caution to use!

GET_BBT Returns Bad-block table in bitmap

CTRL_SET_BBT Re-write bad_block table, this function is also very dangerous!

GET_FTL_PARAMENTERS Returns media type (SLC or MLC), ECC type, total erase cycle, average block erace cycle, ECC error rate. You can use it to except the flash's lifetime. and FTL also can monitoring the flash status, if the FTL except the flash will reaches it 's life time, it will use more ECC codes, and probably makes the device write-protected, and notice upper to replace media or copy out data.

CTRL_GC_DEEP Pending a deep garbage collection on device. If it is running in process, write data will be write to FTL's buffer if it is a interrupt triggered write, if the FTL's buffer is full, write() will returns FTL_WAIT.

CTRL_GC_MILD Pending a mild garbage collection on device, it will not take long, if the write() function detected the free log pages is low than thresold, it will be triggered.


License
===

BSD


