#ifndef NFFTL_TYPE_H
#define NFFTL_TYPE_H
#include <stdint.h>
#include <stddef.h>

typedef uint32_t nfftl_page_t;

typedef uint32_t nfftl_block_t;


struct nfftl_nand{
	uint8_t		log2_page_size;

	uint8_t		log2_ppb;

	unsigned int	num_blocks;
};

typedef enum {
	NFFTL_E_NONE = 0,
	NFFTL_E_BAD_BLOCK, //Bad block
	NFFTL_E_ECC, //ECC error
	NFFTL_W_ECC, //ECC warning
	NFFTL_E_MAX,
} nfftl_error_t;
#endif
