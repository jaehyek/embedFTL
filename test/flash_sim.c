#include <stdio.h>
#include <string.h>
#include "driver.h"
const char filename[]="sim.dat";
const char magic[4]={'G','F','S','M'};
#define OOB_START 16
#define PAGE_START (64 * 64) * 2 + OOB_START
#define PAYLOAD_SIZE (64 * 64) * (2048 + 2)
#define perror(...) fprintf(stderr, __VA_ARGS__)
#define dbg(...) printf(__VA_ARGS__)
#define LOG2_PAGE_SIZE 11
#define LOG2_PAGES_PER_BLOCK 6

FILE *fp;
const struct nfftl_nand sim_nand = {
	.log2_page_size		= LOG2_PAGE_SIZE,
	.log2_ppb		= LOG2_PAGES_PER_BLOCK,
	.num_blocks		= 64
};
/* This source provide a Flash Simulator */
/* File format:
	* |MAGIC.NO|OOB data|page data|
	* This simulator provides 2048 per page, OOB is 2 bytes per page, badblock mark & Used Mark
	* 64 pages per block
	* and 64 blocks per device(4096pages per device)
*/
int nfftl_nand_is_bad(const struct nfftl_nand *n, nfftl_block_t b)
{
	uint8_t chk;
	if(b > 64)
		return -1;
	fseek(fp, OOB_START + b * 64 * 2, SEEK_SET);
	fread(&chk, 1, 1, fp);
	if(chk != 0xff)
		return -1;
	return 0;
}

/* Mark bad the given block (or attempt to). No return value is
 * required, because there's nothing that can be done in response.
 */
void nfftl_nand_mark_bad(const struct nfftl_nand *n, nfftl_block_t b)
{
	uint8_t id = 0x00;
	if(b > 64)
		return;
	fseek(fp, OOB_START + b * 64 * 2, SEEK_SET);
	fwrite(&id, 1, 1, fp);
}

/* Erase the given block. This function should return 0 on success or -1
 * on failure.
 *
 * The status reported by the chip should be checked. If an erase
 * operation fails, return -1 and set err to E_BAD_BLOCK.
 */
int nfftl_nand_erase(const struct nfftl_nand *n, nfftl_block_t b,
		     nfftl_error_t *err)
{
	size_t i;
	uint8_t tmp = 0xff;
	if(b > 64)
		return -1;
	if(nfftl_nand_is_bad(n, b))
	{
		*err = NFFTL_E_BAD_BLOCK;
		return -1;
	}
	fseek(fp, OOB_START + b * 64 * 2, SEEK_SET);
	for(i = 0; i < 64 * 2; i++)
		fwrite(&tmp, 1, 1, fp);//Erase OOB data
	fseek(fp, PAGE_START + b * 2048 * 64, SEEK_SET);
	for(i = 0; i < 2048 * 64; i++)
		fwrite(&tmp, 1, 1, fp);//Erase Page data
	return 0;
}

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
		    nfftl_error_t *err)
{
	uint8_t tmp = 0x00;//OOB 1 for free mark
	if(nfftl_nand_is_bad(n, p / 64))
		return -1;
	if(!nfftl_nand_is_free(n, p))
		return -1;
	fseek(fp, OOB_START + p * 2 + 1, SEEK_SET);
	fwrite(&tmp, 1, 1, fp);
	fseek(fp, PAGE_START + p * 2048, SEEK_SET);
	fwrite(data, 2048, 1, fp);
	return 0;
}


/* Check that the given page is erased */
int nfftl_nand_is_free(const struct nfftl_nand *n, nfftl_page_t p)
{
	uint8_t chk;
	if(p > 64 * 64)
		return -1;
	fseek(fp, OOB_START + p * 2 + 1, SEEK_SET);
	fread(&chk, 1, 1, fp);
	if(chk != 0xff)
		return 0;
	return 1;
}

/* Read a portion of a page. ECC must be handled by the NAND
 * implementation. Returns 0 on sucess or -1 if an error occurs. If an
 * uncorrectable ECC error occurs, return -1 and set err to E_ECC.
 */
int nfftl_nand_read(const struct nfftl_nand *n, nfftl_page_t p,
		    size_t offset, size_t length,
		    uint8_t *data,
		    nfftl_error_t *err)
{
	if(nfftl_nand_is_bad(n, p / 64))
		return -1;
	fseek(fp, PAGE_START + p * 2048 + offset, SEEK_SET);
	fread(data, length, 1, fp);
	return 0;
}

/* Read a page from one location and reprogram it in another location.
 * This might be done using the chip's internal buffers, but it must use
 * ECC.
 */
int nfftl_nand_copy(const struct nfftl_nand *n,
		    nfftl_page_t src, nfftl_page_t dst,
		    nfftl_error_t *err)
{
	uint8_t data[2048];
	uint8_t tmp = 0x00;
	if(!nfftl_nand_is_free(n, dst))
		return -1;
	if(nfftl_nand_is_bad(n, dst / 64))
		return -1;
	fseek(fp, PAGE_START + src * 2048, SEEK_SET);
	fread(data, 2048, 1, fp);
	fseek(fp, PAGE_START + dst * 2048, SEEK_SET);
	fwrite(data, 2048, 1, fp);
	fseek(fp, OOB_START + dst * 2 + 1, SEEK_SET);
	fwrite(&tmp, 1, 1, fp);
	return 0;
}

void init_simfile()
{
	uint8_t tmp = 0xff;
	size_t i;
	fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		perror("Can't create simlator file!\r\n");
	}
	fwrite(magic, 4, 1, fp);
	for(i = 0; i <  PAYLOAD_SIZE; i++)
		fwrite(&tmp, 1, 1, fp);
	fclose(fp);
}
int main()
{
	uint8_t chk[4];
	printf("Flash simulator\r\n");
	fp = fopen(filename, "rb+");
	if(fp == NULL)
	{
		dbg("I should init the simulator file first\r\n");
		init_simfile();
		fp = fopen(filename, "rb+");
	}
	fseek(fp, 0, SEEK_SET);
	fread(chk, 4, 1, fp);
	if(memcmp(chk, magic, 4))
	{
		fclose(fp);
		perror("Invaild magic number!Re-init simulator file!\r\n");
		init_simfile();
	}
	dbg("This NAND has 64 blocks, 64 pages per block and 2048 bytes per block\r\n");
	dbg("Page_start offset = 0x%x\r\n", PAGE_START);
	int i;
	nfftl_error_t err;
	uint8_t test[2048];
	memset(test, 0x00, 2048);
	
	if(nfftl_nand_prog(&sim_nand,0,test,&err))
		dbg("Program failed!\r\n");
	for(i = 0; i < 64; i++)
	{
		if(nfftl_nand_is_free(&sim_nand, i))
			dbg("Page %d is free\r\n", i);
		else
			dbg("Page %d is unfree\r\n", i);
	}
	nfftl_nand_copy(&sim_nand,0,1,&err);
	nfftl_nand_erase(&sim_nand,0,&err);
	fclose(fp);
	//init_simfile();
	return 0;
}
