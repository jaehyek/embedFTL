/******************************************************************
  ������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
  
        ��ӭ�����ҵ�USBר����http://group.ednchina.com/93/
        ��ӭ�����ҵ�blog��   http://www.ednchina.com/blog/computer00
                             http://computer00.21ic.org
                             
 FileSystem.c  file
 
���ߣ�Computer-lov
��������: 2005-05-31
�޸�����: 2009-04-11
�汾��V1.2
��Ȩ���У�����ؾ���
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
 uint8 Name[11];                      //11�ֽڵ��ļ���
 uint8 Attribute;                     //�ļ�����
 uint32  FirstClusterNumber;     //��ʼ�غ�
 uint32  NextClusterNumber;      //��һ�شغ�
 uint32 Length;                    //�ļ�����
}FILE,*pFILE;

typedef struct _DIR                           //�ļ��нṹ
{
 uint16 Name[13];                       //�ļ�����
 uint32 FirstClusterNumber;      //�ļ��п�ʼ�غ�
 uint32 CurrentClusterNumber;    //�ļ��е�ǰ�غ�
 uint32 FatherDirClusterNumber;     //�ļ��и�Ŀ¼�غţ��������Ϊ0����ʾ��Ŀ¼Ϊ��Ŀ¼
}DIR,*pDIR;

typedef struct
{
 uint8 FileSystemType;
 uint32 StartSector;
 uint32 TotalSectors;
}DPT,*pDPT;

typedef struct
{
 uint16 BytesPerSector;  //�����ֽ���  ƫ�Ƶ�ַ: 0x0B
 uint8 SectorsPerCluster;//ÿ�������� ƫ�Ƶ�ַ: 0x0D
 uint16 ReserveSectors;     //���������� ƫ�Ƶ�ַ: 0x0E
 uint8 NumOfFat;           //FAT������      ƫ�Ƶ�ַ: 0x10
 uint16 Fat16RootNum;      //��Ŀ¼������ֻ��FAT12/16ʹ�ã�FAT32���ֶα���Ϊ0 ƫ�Ƶ�ַ: 0x11
 uint16 SmallSectors;      //С��������ֻ��FAT12/16ʹ�ã�FAT32���ֶα���Ϊ0   ƫ�Ƶ�ַ: 0x13
 uint16 SectorsPerFat16;    //ÿFAT��������ֻ��FAT12/16ʹ�ã�FAT32���ֶα���Ϊ0  ƫ�Ƶ�ַ: 0x16
 uint32 HiddenSectors;      //����������             ƫ�Ƶ�ַ: 0x1C
 uint32 LargeSectors; //�����������÷�����FAT32��������  ƫ�Ƶ�ַ: 0x20
 uint32 SectorsPerFat32;  //ÿFAT��������ֻ��FAT32ʹ��  ƫ�Ƶ�ַ: 0x24
 uint32 RootClusterNum;   //��Ŀ¼�غ�                  ƫ�Ƶ�ַ: 0x2C
 uint8	SystemId[8];      	//�ļ�ϵͳID��
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
extern DBR Dbr[4];   //DOS������¼
extern uint8 DriverNumber;    //����������
extern uint32 DataStartSector[4];  //���ݿ�ʼ����
extern uint8 AudioFileExist; //�ļ�����
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
