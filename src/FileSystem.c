/******************************************************************
  本程序只供学习使用，未经作者许可，不得用于其它任何用途
  
        欢迎访问我的USB专区：http://group.ednchina.com/93/
        欢迎访问我的blog：   http://www.ednchina.com/blog/computer00
                             http://computer00.21ic.org
                             
 FileSystem.c  file
 
作者：Computer-lov
建立日期: 2005-05-31
修改日期: 2009-04-12
版本：V1.2
版权所有，盗版必究。
Copyright(C) Computer-lov 2009-2019
All rights reserved
*******************************************************************/


#include "FileSystem.h"
#include "Flash.H"
#include "MyType.h"
#include "Key.h"

FILE  File;

uint16 DirLength;     //文件名长度
uint16 FolderLength;  //文件夹名长度

//最后一次读取FAT时的逻辑块地址，用来判断本次读取是否需要重新从硬盘中读取，以加快速度
uint32 LastFatLba=0xFFFFFFFF; 

uint32 DispCount=0;
uint8 DispTime;

uint16 NotRootDir;   //如果该值不为0，表示不是根目录

DIR  CurrentDir;
uint16  DirName[200];

uint16  ItemName[13];
uint16 ItemCount;
DIR  TempDir;

uint8 FileOrFolder;

uint8 DriverNumber=0;             //磁盘驱动器

DPT Dpt[4];   //磁盘分区表
DBR Dbr[4];   //DOS引导记录

uint32 FatStartSector[4];   //FAT开始扇区
uint32 DataStartSector[4];  //数据开始扇区
uint32 MaxClusterNumber[4];  //能够达到的最大簇号

uint8 AudioFileExist=0; //文件存在

uint8 FatBuffer[512];         //文件分配表FAT缓冲

uint32 PlayCount=0; //当前播放的曲目号
uint8 Status;  //当前播放状态

uint8 Black;  //是否反显

uint8 CurrentFileType;

/********************************************************************
函数功能：LCD光标归位。
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void LcdGoHome(void)
{
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：往LCD写一字节数据。
入口参数：
返    回：无。
备    注：无。
********************************************************************/
void WriteLcdData(uint8 Dat)
{
 Dat=0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：将UNICODE编码转换为GB码。
入口参数：指向要转换数据的地址。
返    回：无。
备    注：无。
********************************************************************/
void ChangeCode(uint16* pData)
{
 pData=0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：判断是否为一个支持的音频文件文件
入口参数：无。
返    回：0：不是；1：是。
备    注：无。
********************************************************************/
uint8 IsAudioFile(void)
{
 if((File.Name[0]==0x00)||(File.Name[0]==0xE5)||(File.Name[0]=='.'))return 0;
 if(File.Attribute==0x0F)return 0;  //此项目为长文件名项目
 if(File.Attribute & 0x04)return 0;  //此文件为系统文件
 if(File.Attribute & 0x08)return 0;  //此项目为系统标卷
 if(File.Attribute & 0x10)return 0;  //此项目为子目录
 if((File.Name[8]=='M')&&(File.Name[9]=='P')&&(File.Name[10]=='3')) //MP3文件
 {
  CurrentFileType=MP3_FILE;
  return CurrentFileType;
 }
 if((File.Name[8]=='W')&&(File.Name[9]=='M')&&(File.Name[10]=='A')) //WMA文件
 {
  CurrentFileType=WMA_FILE;
  return CurrentFileType;
 }
 if((File.Name[8]=='W')&&(File.Name[9]=='A')&&(File.Name[10]=='V')) //WAV文件
 {
  CurrentFileType=WAV_FILE;
  return CurrentFileType;
 }
 if((File.Name[8]=='M')&&(File.Name[9]=='I')&&(File.Name[10]=='D')) //MIDI文件
 {
  CurrentFileType=MID_FILE;
  return CurrentFileType;
 }
 return 0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：根据当前簇号，获取下一个扇区地址
入口参数：无。
返    回：无。
备    注：下一个扇区地址。
********************************************************************/
uint32 GetNextSector(void)
{
 uint32 LBA;
 uint16 i;
 LBA=(File.NextClusterNumber)/((Dbr[DriverNumber].BytesPerSector)/4);  //计算扇区地址
 i=(File.NextClusterNumber)*4-LBA*Dbr[DriverNumber].BytesPerSector; //计算偏移地址
 if(LastFatLba!=LBA)    //如果数据不在FatBuffer[]中，则需要重新读取
 {
  LastFatLba=LBA;
  LBA=LBA+FatStartSector[DriverNumber];
  FlashReadOneSector(LBA*Dbr[DriverNumber].BytesPerSector,FatBuffer,0);
 }
 //获取下一簇号
 ((uint8 *)&(File.NextClusterNumber))[0]=FatBuffer[i];
 i++;
 ((uint8 *)&(File.NextClusterNumber))[1]=FatBuffer[i];
 i++;
 ((uint8 *)&(File.NextClusterNumber))[2]=FatBuffer[i];
 i++;
 ((uint8 *)&(File.NextClusterNumber))[3]=FatBuffer[i];
 
 LBA=File.NextClusterNumber-(Dbr[DriverNumber].RootClusterNum); //保存簇号
 LBA=LBA*(Dbr[0].SectorsPerCluster)+DataStartSector[0];        //转换成扇区地址
 DispTime++;
 if(DispTime>3)
 {
  DispTime=0; 
  LcdGoHome();
  DispCount+=2;
  for(i=0;i<14;i++)
  {
   WriteLcdData(((uint8 *)DirName)[(i+DispCount)%(DirLength+4)]);
  }
 }
 return LBA;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：获取文件夹的下一扇区地址
入口参数：无。
返    回：无。
备    注：下一扇区地址。
********************************************************************/
uint32 GetDirNextSector(void)
{
 uint32 LBA,LBA2;
 uint16 i;
 LBA=CurrentDir.CurrentClusterNumber-(Dbr[DriverNumber].RootClusterNum);
 LBA=LBA*(Dbr[DriverNumber].SectorsPerCluster)+DataStartSector[DriverNumber];
 LBA2=(CurrentDir.CurrentClusterNumber)/((Dbr[DriverNumber].BytesPerSector)/4);  //计算扇区地址
 i=(CurrentDir.CurrentClusterNumber)*4-LBA2*Dbr[DriverNumber].BytesPerSector;    //计算偏移地址
 LBA2=LBA2+FatStartSector[DriverNumber];
 FlashReadOneSector(LBA2*Dbr[DriverNumber].BytesPerSector,FlashSectorBuf,0);
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[0]=FlashSectorBuf[i];
 i++;
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[1]=FlashSectorBuf[i];
 i++;
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[2]=FlashSectorBuf[i];
 i++;
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[3]=FlashSectorBuf[i];  //获取下一簇号
 return LBA;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：获取第n个MP3文件
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void GetNstFile(void)
{
 uint16 i,j,k,TempCount;
 uint32 LBA;
 uint8  TempBuffer[32];
 DispCount=0;
 TempCount=0;
 CurrentDir.CurrentClusterNumber=CurrentDir.FirstClusterNumber;
 do
 {
  LBA=GetDirNextSector();
  for(i=0;i<Dbr[DriverNumber].SectorsPerCluster;i++)
  {
   FlashReadOneSector(LBA*Dbr[DriverNumber].BytesPerSector,FlashSectorBuf,0);
   for(j=0;j<Dbr[DriverNumber].BytesPerSector/32;j++)
   {
    //如果已到最后一个项，则将之保存
    if(j==15)
    {
     for(k=0;k<32;k++)
     {
      TempBuffer[k]=FlashSectorBuf[k+512-32];
     }
    }
    for(k=0;k<11;k++)
    {
     File.Name[k]=FlashSectorBuf[j*32+k];
    }
    File.Attribute=FlashSectorBuf[j*32+11];
    if(IsAudioFile())
    {
     TempCount++;
     if(TempCount==PlayCount)
     {
      ((uint8 *)&(File.FirstClusterNumber))[0]=FlashSectorBuf[j*32+0x1A];     
      ((uint8 *)&(File.FirstClusterNumber))[1]=FlashSectorBuf[j*32+0x1B];     
      ((uint8 *)&(File.FirstClusterNumber))[2]=FlashSectorBuf[j*32+0x14];      
      ((uint8 *)&(File.FirstClusterNumber))[3]=FlashSectorBuf[j*32+0x15];
      ((uint8 *)&(File.Length))[0]=FlashSectorBuf[j*32+0x1C];
      ((uint8 *)&(File.Length))[1]=FlashSectorBuf[j*32+0x1D];
      ((uint8 *)&(File.Length))[2]=FlashSectorBuf[j*32+0x1E];
      ((uint8 *)&(File.Length))[3]=FlashSectorBuf[j*32+0x1F];
      File.NextClusterNumber=File.FirstClusterNumber;
      DirLength=FolderLength;
      //如果为第一项，则用刚刚保存的一项替换最后一项
      if(j==0)
      {
       for(k=0;k<32;k++)
       {
        FlashSectorBuf[k+512-32]=TempBuffer[k];
       }
       j=16;
      }
      j--;
      if((FlashSectorBuf[j*32+11]==0x0F)&&(FlashSectorBuf[j*32]!=0xE5))  //如果找到长目录，则用长目录名替换短目录名
      {
       for(k=0;k<10;k++)
       {
        ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x02+k];
        k++;
        DirLength++;
        ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+k];
        DirLength++;
       }
       for(k=0;k<12;k++)
       {
        ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x0F+k];
        DirLength++;
        k++;
        ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x0D+k];
        DirLength++;
       }
       ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x1D];
       DirLength++;
       ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x1C];
       DirLength++;
       ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x1F];
       DirLength++;
       ((uint8 *)DirName)[DirLength]=FlashSectorBuf[j*32+0x1E];
       DirLength++;
      }
      for(k=FolderLength/2;k<13+FolderLength/2;k++)
      {
       if((DirName[k]==0x0000)||(DirName[k]==0xFF00)||(DirName[k]==0xFFFF))break;
       ChangeCode(&(DirName[k]));
      }
      DirLength=FolderLength+(k-FolderLength/2)*2;
      for(;k<100;k++)
      {
       DirName[k]=0x2020;
      }
      return;
     }
    }
   }
   LBA++;
  }
 }while((!(CurrentDir.CurrentClusterNumber>=MaxClusterNumber[DriverNumber]))
  &&(LBA<(Dpt[DriverNumber].StartSector+Dpt[DriverNumber].TotalSectors))); //直到文件夹结束
 PlayCount=1;
 Status=STOP;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：打开上一个MP3文件，文件信息保存在File结构中
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void GetPreFile(void)
{
 if(PlayCount>=2)
 {
  PlayCount--;
 }
 else //如果已到达该文件夹最前一个文件，则播放原文件
 {
  File.NextClusterNumber=File.FirstClusterNumber;
  return;
 }
 GetNstFile();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：打开下一个MP3文件
入口参数：
返    回：无。
备    注：无。
********************************************************************/
void GetNextFile(void)
{
 PlayCount++;
 GetNstFile();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：判断是否为一个文件夹 
入口参数：无。
返    回：0：不是；1：是。
备    注：无。
********************************************************************/
uint8 IsFolder(void)
{
 if((File.Name[0]==0x00)||(File.Name[0]==0xE5)||(File.Name[0]=='.'))return 0;
 if(File.Attribute==0x0F)return 0;  //此项目为长文件名项目
 if(File.Attribute & 0x04)return 0;  //此文件为系统文件
 if(File.Attribute & 0x08)return 0;  //此项目为系统标卷
 if(File.Attribute & 0x10)return 1;  //此项目为子目录
 else return 0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：获取当前目录下 下一个有效文件或者文件夹
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void GetPreItem(void)
{
 if(ItemCount>=2)ItemCount--;
 if(ItemCount==0)ItemCount=1;
 GetNstItem();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：获取当前目录下 下一个有效文件或者文件夹
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void GetNextItem(void)
{
 ItemCount++;
 GetNstItem();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：进入一个文件夹
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void EnterDir(void)
{
 uint8 i;
 CurrentDir.FatherDirClusterNumber=CurrentDir.FirstClusterNumber;
 CurrentDir.FirstClusterNumber=TempDir.FirstClusterNumber;
 CurrentDir.CurrentClusterNumber=CurrentDir.FirstClusterNumber;
 NotRootDir++;
 for(i=0;i<13;i++)
 {
  if(ItemName[i]==0x2020)break;
  DirName[FolderLength/2]=ItemName[i];
  FolderLength+=2;
 }
 DirName[FolderLength/2]=0x5C20;
 FolderLength+=2;
 ItemCount=0;
 PlayCount=0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：找出父文件夹起始簇号。
入口参数：无。
返    回：无。
备    注：父文件夹的起始簇号。
********************************************************************/
//////////////////////////////////     ////////////////////////////////////
uint32 FindFatherDir(void)
{
 uint32 ClusterNumber;
 uint16 i,j;
 uint32 LBA;
 CurrentDir.CurrentClusterNumber=CurrentDir.FirstClusterNumber;
 do
 {
  LBA=GetDirNextSector();
  for(i=0;i<Dbr[DriverNumber].SectorsPerCluster;i++)
  {
   FlashReadOneSector(LBA*Dbr[DriverNumber].BytesPerSector,FlashSectorBuf,0);
   for(j=0;j<Dbr[DriverNumber].BytesPerSector/32;j++)
   {
    if((FlashSectorBuf[j*32+0]=='.')
     &&(FlashSectorBuf[j*32+1]=='.')
     &&(FlashSectorBuf[j*32+11]&0x10))  //找到父目录项
    {
     ((uint8 *)&(ClusterNumber))[0]=FlashSectorBuf[j*32+0x1A];   
     ((uint8 *)&(ClusterNumber))[1]=FlashSectorBuf[j*32+0x1B];  
     ((uint8 *)&(ClusterNumber))[2]=FlashSectorBuf[j*32+0x14];         
     ((uint8 *)&(ClusterNumber))[3]=FlashSectorBuf[j*32+0x15];
     if(ClusterNumber==0)
     {
      ClusterNumber=Dbr[0].RootClusterNum;
     }
     return ClusterNumber;
    }
   }
   LBA++;
  }
 }while((!(CurrentDir.CurrentClusterNumber>=MaxClusterNumber[DriverNumber]))
  &&(LBA<(Dpt[DriverNumber].StartSector+Dpt[DriverNumber].TotalSectors))); //直到文件夹结束
 //如果找不到父目录，则发生严重错误，退回到根目录
 FolderLength=4;
 for(i=2;i<200;i++)
 {
  DirName[i]=0x2020;
 }
 NotRootDir=0;
 CurrentDir.FirstClusterNumber=Dbr[0].RootClusterNum;
 CurrentDir.CurrentClusterNumber=CurrentDir.FirstClusterNumber;
 return CurrentDir.FirstClusterNumber;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：退到上一级目录，如果为根目录，则退不动
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void ExitDir(void)
{
 uint8 i;
 if(NotRootDir)
 {
  NotRootDir--;
  CurrentDir.FirstClusterNumber=CurrentDir.FatherDirClusterNumber;
  CurrentDir.CurrentClusterNumber=CurrentDir.FirstClusterNumber;
  if(NotRootDir)CurrentDir.FatherDirClusterNumber=FindFatherDir();
  FolderLength--;
  DirName[FolderLength/2]=0x2020;
  for(i=0;i<13;i++)
  {
   FolderLength-=2;
   if(((uint8 *)&(DirName[FolderLength/2]))[0]=='\\')break; //直到遇到反斜杠
   if(((uint8 *)&(DirName[FolderLength/2]))[1]=='\\')break; //直到遇到反斜杠
   DirName[FolderLength/2]=0x2020;
  }
  FolderLength++;
 }
 PlayCount=0;
 ItemCount=0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/********************************************************************
函数功能：获取当前文件夹中，第n个有效文件或者文件夹
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void GetNstItem(void)
{
 uint16 i,j,k,TempCount;
 uint32 LBA;
 uint8  TempBuffer[32];

 TempCount=0;
 PlayCount=0;
 CurrentDir.CurrentClusterNumber=CurrentDir.FirstClusterNumber;
 do
 {
  LBA=GetDirNextSector();
  for(i=0;i<Dbr[DriverNumber].SectorsPerCluster;i++)
  {
   FlashReadOneSector(LBA*Dbr[DriverNumber].BytesPerSector,FlashSectorBuf,0);
   for(j=0;j<Dbr[DriverNumber].BytesPerSector/32;j++)
   {
    //如果已到最后一个项，则将之保存
    if(j==15)
    {
     for(k=0;k<32;k++)
     {
      TempBuffer[k]=FlashSectorBuf[k+512-32];
     }
    }
    for(k=0;k<11;k++)
    {
     File.Name[k]=FlashSectorBuf[j*32+k];
    }
    File.Attribute=FlashSectorBuf[j*32+11];
    if(IsAudioFile())
    {
     if(Black)
     {
      Black=0;
     }
     TempCount++;
     PlayCount++;
     if(TempCount==ItemCount)
     {
      for(k=0;k<11;k++)
      {
       ItemName[k]=FlashSectorBuf[j*32+k];      //保存短文件名
      }
      if(j==0) //如果为第一项，则用刚刚保存的一项替换最后一项
      {
       for(k=0;k<32;k++)
       {
        FlashSectorBuf[k+512-32]=TempBuffer[k];
       }
       j=16;
      }
      j--;
      //如果找到长目录，则用长目录名替换短目录名
      if((FlashSectorBuf[j*32+11]==0x0F)&&(FlashSectorBuf[j*32]!=0xE5))
      {
       for(k=0;k<10;k++)
       {
        ((uint8 *)ItemName)[k]=FlashSectorBuf[j*32+0x02+k];
        k++;
        ((uint8 *)ItemName)[k]=FlashSectorBuf[j*32+k];
       }
       for(k=0;k<12;k++)
       {
        ((uint8 *)ItemName)[10+k]=FlashSectorBuf[j*32+0x0F+k];
        k++;
        ((uint8 *)ItemName)[10+k]=FlashSectorBuf[j*32+0x0D+k];
       }
       ((uint8 *)ItemName)[22]=FlashSectorBuf[j*32+0x1D];
       ((uint8 *)ItemName)[23]=FlashSectorBuf[j*32+0x1C];
       ((uint8 *)ItemName)[24]=FlashSectorBuf[j*32+0x1F];
       ((uint8 *)ItemName)[25]=FlashSectorBuf[j*32+0x1E];
      }
      for(k=0;k<13;k++)
      {
       if((ItemName[k]==0x0000)||(ItemName[k]==0xFF00)||(ItemName[k]==0xFFFF))break;
       ChangeCode(&(ItemName[k]));
      }
      for(;k<13;k++)
      {
       ItemName[k]=0x2020;
      }
      FileOrFolder=IS_AUDIO_FILE;
      return;
     }
    }
    if(IsFolder())
    {
     if(!Black)
     {
      Black=1;
     }
     TempCount++;
     if(TempCount==ItemCount)
     {
      ((uint8 *)&(TempDir.FirstClusterNumber))[0]=FlashSectorBuf[j*32+0x1A];  
      ((uint8 *)&(TempDir.FirstClusterNumber))[1]=FlashSectorBuf[j*32+0x1B]; 
      ((uint8 *)&(TempDir.FirstClusterNumber))[2]=FlashSectorBuf[j*32+0x14];              
      ((uint8 *)&(TempDir.FirstClusterNumber))[3]=FlashSectorBuf[j*32+0x15];
      TempDir.CurrentClusterNumber=TempDir.FirstClusterNumber;
      for(k=0;k<11;k++)
      {
       ItemName[k]=FlashSectorBuf[j*32+k];     //保存短文件名
      }
      if(j==0)
      {
       for(k=0;k<32;k++)
       {
        FlashSectorBuf[k+512-32]=TempBuffer[k];
       }
       j=16;
      }
      j--;
      if((FlashSectorBuf[j*32+11]==0x0F)&&(FlashSectorBuf[j*32]!=0xE5))
      {
       for(k=0;k<10;k++)
       {
        ((uint8 *)ItemName)[k]=FlashSectorBuf[j*32+0x02+k];
        k++;
        ((uint8 *)ItemName)[k]=FlashSectorBuf[j*32+k];
       }
       for(k=0;k<12;k++)
       {
        ((uint8 *)ItemName)[10+k]=FlashSectorBuf[j*32+0x0F+k];
        k++;
        ((uint8 *)ItemName)[10+k]=FlashSectorBuf[j*32+0x0D+k];
       }
       ((uint8 *)ItemName)[22]=FlashSectorBuf[j*32+0x1D];
       ((uint8 *)ItemName)[23]=FlashSectorBuf[j*32+0x1C];
       ((uint8 *)ItemName)[24]=FlashSectorBuf[j*32+0x1F];
       ((uint8 *)ItemName)[25]=FlashSectorBuf[j*32+0x1E];
      }
      for(k=0;k<13;k++)
      {
       if((ItemName[k]==0x0000)||(ItemName[k]==0xFF00)||(ItemName[k]==0xFFFF))break;
       ChangeCode(&(ItemName[k]));
      }
      for(;k<13;k++)
      {
       ItemName[k]=0x2020;
      }
      FileOrFolder=IS_FOLDER;
      return;
     }
    }
   }
  LBA++;
  if(KeyDown)
  {
   if(KeyDown==KEY_FUN)
   {
    KeyDown&=~KEY_FUN;
    goto Exit;
   }
   else KeyDown=0;
   }
  }
 }while((!(CurrentDir.CurrentClusterNumber>=MaxClusterNumber[DriverNumber]))
  &&(LBA<(Dpt[DriverNumber].StartSector+Dpt[DriverNumber].TotalSectors))); //直到文件夹结束

 Exit:
 ItemCount=0;
 PlayCount=0;
 FileOrFolder=IS_EMPTY;
 for(k=0;k<13;k++)
 {
  ItemName[k]=0x2020;
 }
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：检查是否为FAT32文件系统。
入口参数：无。
返    回：0：成功；非0：失败。
备    注：无。
********************************************************************/
uint32 CheckFileSystemId(uint8 Index)
{
 if(Dbr[Index].SystemId[0]!='F')return 1;
 if(Dbr[Index].SystemId[1]!='A')return 2;
 if(Dbr[Index].SystemId[2]!='T')return 3;
 if(Dbr[Index].SystemId[3]!='3')return 4;
 if(Dbr[Index].SystemId[4]!='2')return 5;
 if(Dbr[Index].SystemId[5]!=' ')return 6;
 if(Dbr[Index].SystemId[6]!=' ')return 7;
 if(Dbr[Index].SystemId[7]!=' ')return 8;
 return 0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
函数功能：文件系统初始化。
入口参数：无。
返    回：0：成功；非0：失败。
备    注：无。
********************************************************************/
uint32 FileSystemInit(void)
{
 uint16 i;
 uint32 FileSystemIsOk;
 
 AudioFileExist=0; //初始化音频文件不存在
 
 //读0扇区数据，它可能是MBR，也可能是DBR。
 //对于U盘，它一般是DBR。对于硬盘，它是MBR。
 FlashReadOneSector(0,FlashSectorBuf,0);
 
 //判断结束是否为0x55和0xAA有效结束标志
 if((FlashSectorBuf[510]!=0x55)||(FlashSectorBuf[511]!=0xAA))
 {
  return 1;
 }
 
 //假设0扇区是MBR
 for(i=0;i<4;i++)
 {
  //分区1文件系统类型
  Dpt[i].FileSystemType=FlashSectorBuf[0x1C2+i*16];
  //分区1启始扇区号
  Dpt[i].StartSector=FlashSectorBuf[0x1C9+i*16];
  Dpt[i].StartSector<<=8;
  Dpt[i].StartSector+=FlashSectorBuf[0x1C8+i*16];
  Dpt[i].StartSector<<=8;
  Dpt[i].StartSector+=FlashSectorBuf[0x1C7+i*16];
  Dpt[i].StartSector<<=8;
  Dpt[i].StartSector+=FlashSectorBuf[0x1C6+i*16];
  //分区总扇区数
  Dpt[i].TotalSectors=FlashSectorBuf[0x1CD+i*16];
  Dpt[i].TotalSectors<<=8;
  Dpt[i].TotalSectors+=FlashSectorBuf[0x1CC+i*16];
  Dpt[i].TotalSectors<<=8;
  Dpt[i].TotalSectors+=FlashSectorBuf[0x1CB+i*16];
  Dpt[i].TotalSectors<<=8;
  Dpt[i].TotalSectors+=FlashSectorBuf[0x1CA+i*16];
 }
 
 //假设扇区0是DBR
 //每扇区字节数
 Dbr[0].BytesPerSector=FlashSectorBuf[0x0C];
 Dbr[0].BytesPerSector<<=8;
 Dbr[0].BytesPerSector+=FlashSectorBuf[0x0B];
 //每簇扇区数
 Dbr[0].SectorsPerCluster=FlashSectorBuf[0x0D];
 //保留扇区数
 Dbr[0].ReserveSectors=FlashSectorBuf[0x0F];
 Dbr[0].ReserveSectors<<=8;
 Dbr[0].ReserveSectors+=FlashSectorBuf[0x0E];
 //FAT副本数
 Dbr[0].NumOfFat=FlashSectorBuf[0x10];
 //FAT16的根目录数
 Dbr[0].Fat16RootNum=FlashSectorBuf[0x12];
 Dbr[0].Fat16RootNum<<=8;
 Dbr[0].Fat16RootNum+=FlashSectorBuf[0x11];
 //小扇区数
 Dbr[0].SmallSectors=FlashSectorBuf[0x14];
 Dbr[0].SmallSectors<<=8;
 Dbr[0].SmallSectors+=FlashSectorBuf[0x13];
 //FAT12/16 每FAT扇区数
 Dbr[0].SectorsPerFat16=FlashSectorBuf[0x17];
 Dbr[0].SectorsPerFat16<<=8;
 Dbr[0].SectorsPerFat16+=FlashSectorBuf[0x16];
 //隐藏扇区数
 Dbr[0].HiddenSectors=FlashSectorBuf[0x1F];
 Dbr[0].HiddenSectors<<=8;
 Dbr[0].HiddenSectors+=FlashSectorBuf[0x1E];
 Dbr[0].HiddenSectors<<=8;
 Dbr[0].HiddenSectors+=FlashSectorBuf[0x1D];
 Dbr[0].HiddenSectors<<=8;
 Dbr[0].HiddenSectors+=FlashSectorBuf[0x1C];
 //总扇区数
 Dbr[0].LargeSectors=FlashSectorBuf[0x23];
 Dbr[0].LargeSectors<<=8;
 Dbr[0].LargeSectors+=FlashSectorBuf[0x22];
 Dbr[0].LargeSectors<<=8;
 Dbr[0].LargeSectors+=FlashSectorBuf[0x21];
 Dbr[0].LargeSectors<<=8;
 Dbr[0].LargeSectors+=FlashSectorBuf[0x20];
 //FAT32每FAT扇区数
 Dbr[0].SectorsPerFat32=FlashSectorBuf[0x27];
 Dbr[0].SectorsPerFat32<<=8;
 Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x26];
 Dbr[0].SectorsPerFat32<<=8;
 Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x25];
 Dbr[0].SectorsPerFat32<<=8;
 Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x24];
 //根目录簇号
 Dbr[0].RootClusterNum=FlashSectorBuf[0x2F];
 Dbr[0].RootClusterNum<<=8;
 Dbr[0].RootClusterNum+=FlashSectorBuf[0x2E];
 Dbr[0].RootClusterNum<<=8;
 Dbr[0].RootClusterNum+=FlashSectorBuf[0x2D];
 Dbr[0].RootClusterNum<<=8;
 Dbr[0].RootClusterNum+=FlashSectorBuf[0x2C];
 //系统ID号
 Dbr[0].SystemId[0]=FlashSectorBuf[0x52];
 Dbr[0].SystemId[1]=FlashSectorBuf[0x53];
 Dbr[0].SystemId[2]=FlashSectorBuf[0x54];
 Dbr[0].SystemId[3]=FlashSectorBuf[0x55];
 Dbr[0].SystemId[4]=FlashSectorBuf[0x56];
 Dbr[0].SystemId[5]=FlashSectorBuf[0x57];
 Dbr[0].SystemId[6]=FlashSectorBuf[0x58];
 Dbr[0].SystemId[7]=FlashSectorBuf[0x59];
 
 if(CheckFileSystemId(0)==0) //是FAT32文件系统，则说明它可能是DBR
 {
  if(Dbr[0].BytesPerSector==512)  //只支持每扇区512字节的文件系统
  {
   FatStartSector[0]=Dbr[0].ReserveSectors;
   DataStartSector[0]=FatStartSector[0]+(Dbr[0].NumOfFat)*(Dbr[0].SectorsPerFat32);
   FileSystemIsOk=1;
  }
  else  //不是每扇区512字节，错误，可能是MBR
  {
  }
 }
 else //没有检测到FAT32文件系统签名，则可能是MBR
 {
  if((Dpt[0].StartSector==0)||(Dpt[0].TotalSectors==0)||(Dpt[0].StartSector>200))
  {
   return 2;
  }     
 }
 
 if(!FileSystemIsOk)
 {
  //运行到这里，则说明前面读到的0扇区为MBR，
  //那么读分区0的DBR，看是否正确，这里仅支持512字节每扇区
  FlashReadOneSector(Dpt[0].StartSector*512,FlashSectorBuf,0);
  
  //判断结束是否为0x55和0xAA
  if((FlashSectorBuf[510]!=0x55)||(FlashSectorBuf[511]!=0xAA))
  {
   return 3;
  }
  
  //每扇区字节数
  Dbr[0].BytesPerSector=FlashSectorBuf[0x0C];
  Dbr[0].BytesPerSector<<=8;
  Dbr[0].BytesPerSector+=FlashSectorBuf[0x0B];
  //每簇扇区数
  Dbr[0].SectorsPerCluster=FlashSectorBuf[0x0D];
  //保留扇区数
  Dbr[0].ReserveSectors=FlashSectorBuf[0x0F];
  Dbr[0].ReserveSectors<<=8;
  Dbr[0].ReserveSectors+=FlashSectorBuf[0x0E];
  //FAT副本数
  Dbr[0].NumOfFat=FlashSectorBuf[0x10];
  //FAT16的根目录数
  Dbr[0].Fat16RootNum=FlashSectorBuf[0x12];
  Dbr[0].Fat16RootNum<<=8;
  Dbr[0].Fat16RootNum+=FlashSectorBuf[0x11];
  //小扇区数
  Dbr[0].SmallSectors=FlashSectorBuf[0x14];
  Dbr[0].SmallSectors<<=8;
  Dbr[0].SmallSectors+=FlashSectorBuf[0x13];
  //FAT12/16 每FAT扇区数
  Dbr[0].SectorsPerFat16=FlashSectorBuf[0x17];
  Dbr[0].SectorsPerFat16<<=8;
  Dbr[0].SectorsPerFat16+=FlashSectorBuf[0x16];
  //隐藏扇区数
  Dbr[0].HiddenSectors=FlashSectorBuf[0x1F];
  Dbr[0].HiddenSectors<<=8;
  Dbr[0].HiddenSectors+=FlashSectorBuf[0x1E];
  Dbr[0].HiddenSectors<<=8;
  Dbr[0].HiddenSectors+=FlashSectorBuf[0x1D];
  Dbr[0].HiddenSectors<<=8;
  Dbr[0].HiddenSectors+=FlashSectorBuf[0x1C];
  //总扇区数
  Dbr[0].LargeSectors=FlashSectorBuf[0x23];
  Dbr[0].LargeSectors<<=8;
  Dbr[0].LargeSectors+=FlashSectorBuf[0x22];
  Dbr[0].LargeSectors<<=8;
  Dbr[0].LargeSectors+=FlashSectorBuf[0x21];
  Dbr[0].LargeSectors<<=8;
  Dbr[0].LargeSectors+=FlashSectorBuf[0x20];
  //FAT32每FAT扇区数
  Dbr[0].SectorsPerFat32=FlashSectorBuf[0x27];
  Dbr[0].SectorsPerFat32<<=8;
  Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x26];
  Dbr[0].SectorsPerFat32<<=8;
  Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x25];
  Dbr[0].SectorsPerFat32<<=8;
  Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x24];
  //根目录簇号
  Dbr[0].RootClusterNum=FlashSectorBuf[0x2F];
  Dbr[0].RootClusterNum<<=8;
  Dbr[0].RootClusterNum+=FlashSectorBuf[0x2E];
  Dbr[0].RootClusterNum<<=8;
  Dbr[0].RootClusterNum+=FlashSectorBuf[0x2D];
  Dbr[0].RootClusterNum<<=8;
  Dbr[0].RootClusterNum+=FlashSectorBuf[0x2C];
  //系统ID号
  Dbr[0].SystemId[0]=FlashSectorBuf[0x52];
  Dbr[0].SystemId[1]=FlashSectorBuf[0x53];
  Dbr[0].SystemId[2]=FlashSectorBuf[0x54];
  Dbr[0].SystemId[3]=FlashSectorBuf[0x55];
  Dbr[0].SystemId[4]=FlashSectorBuf[0x56];
  Dbr[0].SystemId[5]=FlashSectorBuf[0x57];
  Dbr[0].SystemId[6]=FlashSectorBuf[0x58];
  Dbr[0].SystemId[7]=FlashSectorBuf[0x59];
  
  if(CheckFileSystemId(0)==0) //是FAT32文件系统
  {
   if(Dbr[0].BytesPerSector==512)
   {
    FatStartSector[0]=Dpt[0].StartSector+Dbr[0].ReserveSectors;
    DataStartSector[0]=FatStartSector[0]+(Dbr[0].NumOfFat)*(Dbr[0].SectorsPerFat32);
   }
   else
   {
    return 5;
   }
  }
  else
  {
   return 6;
  }
 }
 
 DirName[0]='C'+0xA380;
 ((unsigned char *)DirName)[2]=':';
 ((unsigned char *)DirName)[3]='\\';
 FolderLength=4;
 DirLength=FolderLength;
 NotRootDir=0;
 
 DriverNumber=0;  //C盘
 FlashReadOneSector(FatStartSector[DriverNumber]*Dbr[0].BytesPerSector,FlashSectorBuf,0);
 ((uint8 *)&MaxClusterNumber[DriverNumber])[0]=FlashSectorBuf[0];
 ((uint8 *)&MaxClusterNumber[DriverNumber])[1]=FlashSectorBuf[1];
 ((uint8 *)&MaxClusterNumber[DriverNumber])[2]=FlashSectorBuf[2];
 ((uint8 *)&MaxClusterNumber[DriverNumber])[3]=FlashSectorBuf[3];
 CurrentDir.FirstClusterNumber=Dbr[0].RootClusterNum;
 LastFatLba=0xFFFFFFFF;  //置为无效LBA
 return 0;
}
/////////////////////////End of function/////////////////////////////
