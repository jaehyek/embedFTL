/******************************************************************
  本程序只供学习使用，未经作者许可，不得用于其它任何用途
  
        欢迎访问我的USB专区：http://group.ednchina.com/93/
        欢迎访问我的blog：   http://www.ednchina.com/blog/computer00
                             http://computer00.21ic.org
                             
 FileSystem.c  file
 
作者：Computer-lov
建立日期: 2005-05-31
修改日期: 2009-04-11
版本：V1.2
版权所有，盗版必究。
Copyright(C) Computer-lov 2009-2019
All rights reserved
*******************************************************************/

#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "MyType.h"

uint8 IsAudioFile(void);
uint32 GetNextSector(void);
void GetPreFile(void);
void GetNextFile(void);
void GetNstFile(void);
void GetNstItem(void);
void GetPreItem(void);
void GetNextItem(void);
void EnterDir(void);
void ExitDir(void);
uint32 FileSystemInit(void);

typedef struct _FILE
{
 uint8 Name[11];                      //11字节的文件名
 uint8 Attribute;                     //文件属性
 uint32  FirstClusterNumber;     //起始簇号
 uint32  NextClusterNumber;      //下一簇簇号
 uint32 Length;                    //文件长度
}FILE,*pFILE;

typedef struct _DIR                           //文件夹结构
{
 uint16 Name[13];                       //文件夹名
 uint32 FirstClusterNumber;      //文件夹开始簇号
 uint32 CurrentClusterNumber;    //文件夹当前簇号
 uint32 FatherDirClusterNumber;     //文件夹父目录簇号，如果该项为0，表示父目录为根目录
}DIR,*pDIR;

typedef struct
{
 uint8 FileSystemType;
 uint32 StartSector;
 uint32 TotalSectors;
}DPT,*pDPT;

typedef struct
{
 uint16 BytesPerSector;  //扇区字节数  偏移地址: 0x0B
 uint8 SectorsPerCluster;//每簇扇区数 偏移地址: 0x0D
 uint16 ReserveSectors;     //保留扇区数 偏移地址: 0x0E
 uint8 NumOfFat;           //FAT副本数      偏移地址: 0x10
 uint16 Fat16RootNum;      //根目录项数，只被FAT12/16使用，FAT32该字段必须为0 偏移地址: 0x11
 uint16 SmallSectors;      //小扇区数，只被FAT12/16使用，FAT32该字段必须为0   偏移地址: 0x13
 uint16 SectorsPerFat16;    //每FAT扇区数，只被FAT12/16使用，FAT32该字段必须为0  偏移地址: 0x16
 uint32 HiddenSectors;      //隐藏扇区数             偏移地址: 0x1C
 uint32 LargeSectors; //总扇区数，该分区中FAT32的扇区数  偏移地址: 0x20
 uint32 SectorsPerFat32;  //每FAT扇区数，只被FAT32使用  偏移地址: 0x24
 uint32 RootClusterNum;   //根目录簇号                  偏移地址: 0x2C
 uint8	SystemId[8];      	//文件系统ID号
}DBR,*pDBR;

extern FILE File;
extern DIR CurrentDir;
extern DIR TempDir;
extern uint16 DirName[200];
extern uint16 DirLength;
extern uint16 FolderLength;
extern uint16 ItemName[13];
extern uint16 ItemCount;
extern uint8 FileOrFolder;
extern uint16 NotRootDir;
extern DBR Dbr[4];   //DOS引导记录
extern uint8 DriverNumber;    //磁盘驱动器
extern uint32 DataStartSector[4];  //数据开始扇区
extern uint8 AudioFileExist; //文件存在
extern uint8 CurrentFileType;

#define IS_AUDIO_FILE  0
#define IS_FOLDER    1
#define IS_EMPTY     2

#define MP3_FILE 1
#define WMA_FILE 2
#define WAV_FILE 3
#define MID_FILE 4

#define START  0
#define PLAY   1
#define PAUSE  2
#define STOP   3

#define KEY_FUN KEY4

#endif
