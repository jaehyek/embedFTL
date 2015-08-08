#ifndef NFFTL_DRIVER_H
#define NFFTL_DRIVER_H
#include "type.h"
/* Is the given block bad? */
int nfftl_nand_is_bad(const struct nfftl_nand *n, nfftl_block_t b);

/* Mark bad the given block (or attempt to). No return value is
 * required, because there's nothing that can be done in response.
 */
void nfftl_nand_mark_bad(const struct nfftl_nand *n, nfftl_block_t b);

/* Erase the given block. This function should return 0 on success or -1
 * on failure.
 *
 * The status reported by the chip should be checked. If an erase
 * operation fails, return -1 and set err to E_BAD_BLOCK.
 */
int nfftl_nand_erase(const struct nfftl_nand *n, nfftl_block_t b,
		     nfftl_error_t *err);

/* Program the given page. The data pointer is a pointer to an entire
 * page ((1 << log2_page_size) bytes). The operation status should be
 * checked. If the operation fails, return -1 and set err to
 * E_BAD_BLOCK.
 *
 * Pages will be programmed sequentially within a block, and will not be
 * reprogrammed.
 */
int nfftl_nand_prog(const struct nfftl_nand *n, nfftl_page_t p,
		    const uint8_t *data,
		    nfftl_error_t *err);

/* Check that the given page is erased */
int nfftl_nand_is_free(const struct nfftl_nand *n, nfftl_page_t p);

/* Read a portion of a page. ECC must be handled by the NAND
 * implementation. Returns 0 on sucess or -1 if an error occurs. If an
 * uncorrectable ECC error occurs, return -1 and set err to E_ECC.
 */
int nfftl_nand_read(const struct nfftl_nand *n, nfftl_page_t p,
		    size_t offset, size_t length,
		    uint8_t *data,
		    nfftl_error_t *err);

/* Read a page from one location and reprogram it in another location.
 * This might be done using the chip's internal buffers, but it must use
 * ECC.
 */
int nfftl_nand_copy(const struct nfftl_nand *n,
		    nfftl_page_t src, nfftl_page_t dst,
		    nfftl_error_t *err);

#endif
