
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dhara/nand.h"
#ifndef FSIM_H
#define FSIM_H
typedef unsigned char bool;

int Flash_Init();

int Flash_DeInit();

int Flash_Erase(int blockno);

int Flash_Write(int pageno);

int Flash_WriteToCacheRandom(int offset, int size, void *buffer);

int Flash_WriteToCache(int offset, int size, void *buffer);

int Flash_ReadFromCache(int offset, int size, void *buffer);

int Flash_Read(int pageno);

extern long long int time;
#define FLASH_PAGE_SIZE_LOG2    11
#define FLASH_SPARE_SIZE_LOG2   6
#define FLASH_BLOCK_SIZE_LOG2   6
#define FLASH_DEVICE_SIZE_LOG2  12
#endif // FSIM_H

const struct dhara_nand sim_nand = {
	.log2_page_size		= 11,
	.log2_ppb		= 6,
	.num_blocks		= 4092
};

/*
    2KB per page + 64 spare bytes
    64 pages per block (128KB)
    Device has 4096 blocks
*/
FILE *flashfp;

int Erase_Count[1 << FLASH_DEVICE_SIZE_LOG2];
bool BadBlock_Table[1 << FLASH_DEVICE_SIZE_LOG2];
char MagicNumber[]="FSIM";

uint8_t Page_Buffer[(1 << FLASH_PAGE_SIZE_LOG2) + (1 << FLASH_SPARE_SIZE_LOG2)];


#define REAL_DATA_OFFSET     sizeof(Erase_Count) + sizeof(MagicNumber) - 1 + sizeof(BadBlock_Table)

void Format_SimFile()
{
    int i;
    fseek(flashfp, 0, SEEK_SET);
    fwrite(MagicNumber, 1, 4, flashfp);
    memset(Erase_Count, 0x00, sizeof(Erase_Count));
    fwrite(Erase_Count, sizeof(int), 1 << FLASH_DEVICE_SIZE_LOG2, flashfp);
    memset(BadBlock_Table, 0x00, sizeof(Erase_Count));
    fwrite(BadBlock_Table, sizeof(bool), 1 << FLASH_DEVICE_SIZE_LOG2, flashfp);
    for(i = 0; i < (1 << FLASH_DEVICE_SIZE_LOG2); i++)
        Flash_Erase(i);
}
int Flash_Init()
{
    char HDR[]="FLOW";

    flashfp = fopen("flashsim.dat", "rb+");
    if(!flashfp)
    {
        printf("Create a new flashsim.dat\r\n");
        flashfp = fopen("flashsim.dat", "wb+");
        Format_SimFile();
        if(!flashfp)
        {
            printf("Cannot create flashsim.dat\r\nAborting\r\n");
            exit(0);
        }
    }
    fseek(flashfp, 0, SEEK_SET);
    fread(HDR, 1, 4, flashfp);
    if(strcmp(MagicNumber, HDR))
    {
        printf("Simlator file has bad magic number, reformatting\r\n");
        Format_SimFile();
    }
    fread(Erase_Count, sizeof(int), 1 << FLASH_DEVICE_SIZE_LOG2, flashfp);
    fread(BadBlock_Table, sizeof(bool), 1 << FLASH_DEVICE_SIZE_LOG2, flashfp);
    printf("Successfully init the flash sim layer\r\n");
    printf("Page_Size %d, Block size %d, Device size %d\r\n", sizeof(Page_Buffer), 1 << FLASH_BLOCK_SIZE_LOG2, 1 << FLASH_DEVICE_SIZE_LOG2);
    return 0;
}

int Flash_DeInit()
{
    fseek(flashfp, 4, SEEK_SET);
    fwrite(Erase_Count, sizeof(int), 1 << FLASH_DEVICE_SIZE_LOG2, flashfp);
    fwrite(BadBlock_Table, sizeof(bool), 1 << FLASH_DEVICE_SIZE_LOG2, flashfp);
    fclose(flashfp);
    return 0;
}

int Flash_Erase(int blockno)
{
    int i;
    if(blockno >= (1 << FLASH_DEVICE_SIZE_LOG2))
        return -1;
    if(BadBlock_Table[blockno])
        return -1;

    Erase_Count[blockno]++;
    fseek(flashfp, blockno * sizeof(Page_Buffer) * (1 << FLASH_BLOCK_SIZE_LOG2) + REAL_DATA_OFFSET, SEEK_SET);
    memset(Page_Buffer, 0xff, sizeof(Page_Buffer));

    for(i = 0; i < (1 << FLASH_BLOCK_SIZE_LOG2); i++)
        fwrite(Page_Buffer, 1, sizeof(Page_Buffer), flashfp);

    return 0;
}

int Flash_Write(int pageno)
{
    uint8_t Page_Read_Buffer[(1 << FLASH_PAGE_SIZE_LOG2) + (1 << FLASH_SPARE_SIZE_LOG2)];
    int i;
    if(pageno >= ((1 << FLASH_DEVICE_SIZE_LOG2) * (1 << FLASH_BLOCK_SIZE_LOG2)))
        return -1;
    if(BadBlock_Table[pageno / (1 << FLASH_BLOCK_SIZE_LOG2)])
        return -1;
    fseek(flashfp, sizeof(Page_Buffer) * pageno + REAL_DATA_OFFSET, SEEK_SET);
    fread(Page_Read_Buffer, 1, sizeof(Page_Buffer), flashfp);

    for(i = 0; i < sizeof(Page_Buffer); i++)
    {
        Page_Buffer[i] = Page_Read_Buffer[i] & Page_Buffer[i];
    }

    fseek(flashfp, sizeof(Page_Buffer) * pageno + REAL_DATA_OFFSET, SEEK_SET);
    fwrite(Page_Buffer, 1, sizeof(Page_Buffer), flashfp);
    return 0;
}

int Flash_Read(int pageno)
{
    if(pageno >= ((1 << FLASH_DEVICE_SIZE_LOG2) * (1 << FLASH_BLOCK_SIZE_LOG2)))
        return -1;
    if(BadBlock_Table[pageno / (1 << FLASH_BLOCK_SIZE_LOG2)])
        return -1;
    fseek(flashfp, sizeof(Page_Buffer) * pageno + REAL_DATA_OFFSET, SEEK_SET);
    fread(Page_Buffer, 1, sizeof(Page_Buffer), flashfp);
    return 0;
}

int Flash_WriteToCache(int offset, int size, void *buffer)
{
    /*if((offset + size) > sizeof(Page_Buffer))
        return -1;*/
    memset(Page_Buffer, 0xff, sizeof(Page_Buffer));
    memcpy(Page_Buffer + offset, buffer, size);
    return 0;
}

int Flash_WriteToCacheRandom(int offset, int size, void *buffer)
{
    /*if((offset + size) > sizeof(Page_Buffer))
        return -1;*/
    memcpy(Page_Buffer + offset, buffer, size);
    return 0;
}

int Flash_ReadFromCache(int offset, int size, void *buffer)
{
    /*if((offset + size) > sizeof(Page_Buffer))
        return -1;*/
    memcpy(buffer, Page_Buffer + offset, size);
    return 0;
}

/* Is the given block bad? */
int dhara_nand_is_bad(const struct dhara_nand *n, dhara_block_t b)
{
    //printf("checking block %d\r\n", b);
    return 0;
}

void DumpEraseCycle(int n)
{
    int i;
    for(i = 0; i < (1 << FLASH_DEVICE_SIZE_LOG2); i++)
    {
        printf("Block %d erase cycle %d\r\n", i, Erase_Count[i]);
        if(n)
            if((i + 1) % n == 0) getchar();
    }

}
/* Mark bad the given block (or attempt to). No return value is
 * required, because there's nothing that can be done in response.
 */
void dhara_nand_mark_bad(const struct dhara_nand *n, dhara_block_t b)
{

}

/* Erase the given block. This function should return 0 on success or -1
 * on failure.
 *
 * The status reported by the chip should be checked. If an erase
 * operation fails, return -1 and set err to E_BAD_BLOCK.
 */
int dhara_nand_erase(const struct dhara_nand *n, dhara_block_t b,
		     dhara_error_t *err)
{
    time += 3500;
    printf("Erase block %d\r\n", b);
    Flash_Erase(b + 4);
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
int dhara_nand_prog(const struct dhara_nand *n, dhara_page_t p,
		    const uint8_t *data,
		    dhara_error_t *err)
{
    uint8_t flag = 0x00;
    time += 400;
    printf("Write page %d\r\n", p);
    Flash_WriteToCache(0, 2048, (uint8_t *)data);
    Flash_WriteToCacheRandom(0x801, 1, &flag);
    Flash_Write(p + 64 * 4);
    return 0;
}

/* Check that the given page is erased */
int dhara_nand_is_free(const struct dhara_nand *n, dhara_page_t p)
{
    uint8_t flag = 0x00;
    time += 120;
    //printf("Is free %d\r\n", p);
    Flash_Read(p + 64 * 4);
    Flash_ReadFromCache(0x801, 1, &flag);
    if(flag != 0xff)
        return 0;
    return 1;
}

/* Read a portion of a page. ECC must be handled by the NAND
 * implementation. Returns 0 on sucess or -1 if an error occurs. If an
 * uncorrectable ECC error occurs, return -1 and set err to E_ECC.
 */
int dhara_nand_read(const struct dhara_nand *n, dhara_page_t p,
		    size_t offset, size_t length,
		    uint8_t *data,
		    dhara_error_t *err)
{
    time += 120;
    //printf("Read page %d\r\n", p);
    Flash_Read(p+ 64 * 4);
    Flash_ReadFromCache(offset, length, data);
    return 0;
}

/* Read a page from one location and reprogram it in another location.
 * This might be done using the chip's internal buffers, but it must use
 * ECC.
 */
int dhara_nand_copy(const struct dhara_nand *n,
		    dhara_page_t src, dhara_page_t dst,
		    dhara_error_t *err)
{
    time += 120 + 400;
    printf("copy page %d to %d\r\n", src, dst);
	Flash_Read(src+ 64 * 4);
	Flash_Write(dst+ 64 * 4);
	return 0;
}
