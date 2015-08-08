/******************************************************************
  ������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
  
        ��ӭ�����ҵ�USBר����http://group.ednchina.com/93/
        ��ӭ�����ҵ�blog��   http://www.ednchina.com/blog/computer00
                             http://computer00.21ic.org
                             
 FileSystem.c  file
 
���ߣ�Computer-lov
��������: 2005-05-31
�޸�����: 2009-04-12
�汾��V1.2
��Ȩ���У�����ؾ���
Copyright(C) Computer-lov 2009-2019
All rights reserved
*******************************************************************/


#include "FileSystem.h"
#include "Flash.H"
#include "MyType.h"
#include "Key.h"

FILE  File;

uint16 DirLength;     //�ļ�������
uint16 FolderLength;  //�ļ���������

//���һ�ζ�ȡFATʱ���߼����ַ�������жϱ��ζ�ȡ�Ƿ���Ҫ���´�Ӳ���ж�ȡ���Լӿ��ٶ�
uint32 LastFatLba=0xFFFFFFFF; 

uint32 DispCount=0;
uint8 DispTime;

uint16 NotRootDir;   //�����ֵ��Ϊ0����ʾ���Ǹ�Ŀ¼

DIR  CurrentDir;
uint16  DirName[200];

uint16  ItemName[13];
uint16 ItemCount;
DIR  TempDir;

uint8 FileOrFolder;

uint8 DriverNumber=0;             //����������

DPT Dpt[4];   //���̷�����
DBR Dbr[4];   //DOS������¼

uint32 FatStartSector[4];   //FAT��ʼ����
uint32 DataStartSector[4];  //���ݿ�ʼ����
uint32 MaxClusterNumber[4];  //�ܹ��ﵽ�����غ�

uint8 AudioFileExist=0; //�ļ�����

uint8 FatBuffer[512];         //�ļ������FAT����

uint32 PlayCount=0; //��ǰ���ŵ���Ŀ��
uint8 Status;  //��ǰ����״̬

uint8 Black;  //�Ƿ���

uint8 CurrentFileType;

/********************************************************************
�������ܣ�LCD����λ��
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void LcdGoHome(void)
{
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ���LCDдһ�ֽ����ݡ�
��ڲ�����
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void WriteLcdData(uint8 Dat)
{
 Dat=0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ���UNICODE����ת��ΪGB�롣
��ڲ�����ָ��Ҫת�����ݵĵ�ַ��
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void ChangeCode(uint16* pData)
{
 pData=0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ��ж��Ƿ�Ϊһ��֧�ֵ���Ƶ�ļ��ļ�
��ڲ������ޡ�
��    �أ�0�����ǣ�1���ǡ�
��    ע���ޡ�
********************************************************************/
uint8 IsAudioFile(void)
{
 if((File.Name[0]==0x00)||(File.Name[0]==0xE5)||(File.Name[0]=='.'))return 0;
 if(File.Attribute==0x0F)return 0;  //����ĿΪ���ļ�����Ŀ
 if(File.Attribute & 0x04)return 0;  //���ļ�Ϊϵͳ�ļ�
 if(File.Attribute & 0x08)return 0;  //����ĿΪϵͳ���
 if(File.Attribute & 0x10)return 0;  //����ĿΪ��Ŀ¼
 if((File.Name[8]=='M')&&(File.Name[9]=='P')&&(File.Name[10]=='3')) //MP3�ļ�
 {
  CurrentFileType=MP3_FILE;
  return CurrentFileType;
 }
 if((File.Name[8]=='W')&&(File.Name[9]=='M')&&(File.Name[10]=='A')) //WMA�ļ�
 {
  CurrentFileType=WMA_FILE;
  return CurrentFileType;
 }
 if((File.Name[8]=='W')&&(File.Name[9]=='A')&&(File.Name[10]=='V')) //WAV�ļ�
 {
  CurrentFileType=WAV_FILE;
  return CurrentFileType;
 }
 if((File.Name[8]=='M')&&(File.Name[9]=='I')&&(File.Name[10]=='D')) //MIDI�ļ�
 {
  CurrentFileType=MID_FILE;
  return CurrentFileType;
 }
 return 0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ����ݵ�ǰ�غţ���ȡ��һ��������ַ
��ڲ������ޡ�
��    �أ��ޡ�
��    ע����һ��������ַ��
********************************************************************/
uint32 GetNextSector(void)
{
 uint32 LBA;
 uint16 i;
 LBA=(File.NextClusterNumber)/((Dbr[DriverNumber].BytesPerSector)/4);  //����������ַ
 i=(File.NextClusterNumber)*4-LBA*Dbr[DriverNumber].BytesPerSector; //����ƫ�Ƶ�ַ
 if(LastFatLba!=LBA)    //������ݲ���FatBuffer[]�У�����Ҫ���¶�ȡ
 {
  LastFatLba=LBA;
  LBA=LBA+FatStartSector[DriverNumber];
  FlashReadOneSector(LBA*Dbr[DriverNumber].BytesPerSector,FatBuffer,0);
 }
 //��ȡ��һ�غ�
 ((uint8 *)&(File.NextClusterNumber))[0]=FatBuffer[i];
 i++;
 ((uint8 *)&(File.NextClusterNumber))[1]=FatBuffer[i];
 i++;
 ((uint8 *)&(File.NextClusterNumber))[2]=FatBuffer[i];
 i++;
 ((uint8 *)&(File.NextClusterNumber))[3]=FatBuffer[i];
 
 LBA=File.NextClusterNumber-(Dbr[DriverNumber].RootClusterNum); //����غ�
 LBA=LBA*(Dbr[0].SectorsPerCluster)+DataStartSector[0];        //ת����������ַ
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
�������ܣ���ȡ�ļ��е���һ������ַ
��ڲ������ޡ�
��    �أ��ޡ�
��    ע����һ������ַ��
********************************************************************/
uint32 GetDirNextSector(void)
{
 uint32 LBA,LBA2;
 uint16 i;
 LBA=CurrentDir.CurrentClusterNumber-(Dbr[DriverNumber].RootClusterNum);
 LBA=LBA*(Dbr[DriverNumber].SectorsPerCluster)+DataStartSector[DriverNumber];
 LBA2=(CurrentDir.CurrentClusterNumber)/((Dbr[DriverNumber].BytesPerSector)/4);  //����������ַ
 i=(CurrentDir.CurrentClusterNumber)*4-LBA2*Dbr[DriverNumber].BytesPerSector;    //����ƫ�Ƶ�ַ
 LBA2=LBA2+FatStartSector[DriverNumber];
 FlashReadOneSector(LBA2*Dbr[DriverNumber].BytesPerSector,FlashSectorBuf,0);
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[0]=FlashSectorBuf[i];
 i++;
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[1]=FlashSectorBuf[i];
 i++;
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[2]=FlashSectorBuf[i];
 i++;
 ((uint8 *)&(CurrentDir.CurrentClusterNumber))[3]=FlashSectorBuf[i];  //��ȡ��һ�غ�
 return LBA;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ���ȡ��n��MP3�ļ�
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
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
    //����ѵ����һ�����֮����
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
      //���Ϊ��һ����øոձ����һ���滻���һ��
      if(j==0)
      {
       for(k=0;k<32;k++)
       {
        FlashSectorBuf[k+512-32]=TempBuffer[k];
       }
       j=16;
      }
      j--;
      if((FlashSectorBuf[j*32+11]==0x0F)&&(FlashSectorBuf[j*32]!=0xE5))  //����ҵ���Ŀ¼�����ó�Ŀ¼���滻��Ŀ¼��
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
  &&(LBA<(Dpt[DriverNumber].StartSector+Dpt[DriverNumber].TotalSectors))); //ֱ���ļ��н���
 PlayCount=1;
 Status=STOP;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ�����һ��MP3�ļ����ļ���Ϣ������File�ṹ��
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void GetPreFile(void)
{
 if(PlayCount>=2)
 {
  PlayCount--;
 }
 else //����ѵ�����ļ�����ǰһ���ļ����򲥷�ԭ�ļ�
 {
  File.NextClusterNumber=File.FirstClusterNumber;
  return;
 }
 GetNstFile();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ�����һ��MP3�ļ�
��ڲ�����
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void GetNextFile(void)
{
 PlayCount++;
 GetNstFile();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ��ж��Ƿ�Ϊһ���ļ��� 
��ڲ������ޡ�
��    �أ�0�����ǣ�1���ǡ�
��    ע���ޡ�
********************************************************************/
uint8 IsFolder(void)
{
 if((File.Name[0]==0x00)||(File.Name[0]==0xE5)||(File.Name[0]=='.'))return 0;
 if(File.Attribute==0x0F)return 0;  //����ĿΪ���ļ�����Ŀ
 if(File.Attribute & 0x04)return 0;  //���ļ�Ϊϵͳ�ļ�
 if(File.Attribute & 0x08)return 0;  //����ĿΪϵͳ���
 if(File.Attribute & 0x10)return 1;  //����ĿΪ��Ŀ¼
 else return 0;
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ���ȡ��ǰĿ¼�� ��һ����Ч�ļ������ļ���
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void GetPreItem(void)
{
 if(ItemCount>=2)ItemCount--;
 if(ItemCount==0)ItemCount=1;
 GetNstItem();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ���ȡ��ǰĿ¼�� ��һ����Ч�ļ������ļ���
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
********************************************************************/
void GetNextItem(void)
{
 ItemCount++;
 GetNstItem();
}
/////////////////////////End of function/////////////////////////////

/********************************************************************
�������ܣ�����һ���ļ���
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
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
�������ܣ��ҳ����ļ�����ʼ�غš�
��ڲ������ޡ�
��    �أ��ޡ�
��    ע�����ļ��е���ʼ�غš�
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
     &&(FlashSectorBuf[j*32+11]&0x10))  //�ҵ���Ŀ¼��
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
  &&(LBA<(Dpt[DriverNumber].StartSector+Dpt[DriverNumber].TotalSectors))); //ֱ���ļ��н���
 //����Ҳ�����Ŀ¼���������ش����˻ص���Ŀ¼
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
�������ܣ��˵���һ��Ŀ¼�����Ϊ��Ŀ¼�����˲���
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
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
   if(((uint8 *)&(DirName[FolderLength/2]))[0]=='\\')break; //ֱ��������б��
   if(((uint8 *)&(DirName[FolderLength/2]))[1]=='\\')break; //ֱ��������б��
   DirName[FolderLength/2]=0x2020;
  }
  FolderLength++;
 }
 PlayCount=0;
 ItemCount=0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/********************************************************************
�������ܣ���ȡ��ǰ�ļ����У���n����Ч�ļ������ļ���
��ڲ������ޡ�
��    �أ��ޡ�
��    ע���ޡ�
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
    //����ѵ����һ�����֮����
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
       ItemName[k]=FlashSectorBuf[j*32+k];      //������ļ���
      }
      if(j==0) //���Ϊ��һ����øոձ����һ���滻���һ��
      {
       for(k=0;k<32;k++)
       {
        FlashSectorBuf[k+512-32]=TempBuffer[k];
       }
       j=16;
      }
      j--;
      //����ҵ���Ŀ¼�����ó�Ŀ¼���滻��Ŀ¼��
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
       ItemName[k]=FlashSectorBuf[j*32+k];     //������ļ���
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
  &&(LBA<(Dpt[DriverNumber].StartSector+Dpt[DriverNumber].TotalSectors))); //ֱ���ļ��н���

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
�������ܣ�����Ƿ�ΪFAT32�ļ�ϵͳ��
��ڲ������ޡ�
��    �أ�0���ɹ�����0��ʧ�ܡ�
��    ע���ޡ�
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
�������ܣ��ļ�ϵͳ��ʼ����
��ڲ������ޡ�
��    �أ�0���ɹ�����0��ʧ�ܡ�
��    ע���ޡ�
********************************************************************/
uint32 FileSystemInit(void)
{
 uint16 i;
 uint32 FileSystemIsOk;
 
 AudioFileExist=0; //��ʼ����Ƶ�ļ�������
 
 //��0�������ݣ���������MBR��Ҳ������DBR��
 //����U�̣���һ����DBR������Ӳ�̣�����MBR��
 FlashReadOneSector(0,FlashSectorBuf,0);
 
 //�жϽ����Ƿ�Ϊ0x55��0xAA��Ч������־
 if((FlashSectorBuf[510]!=0x55)||(FlashSectorBuf[511]!=0xAA))
 {
  return 1;
 }
 
 //����0������MBR
 for(i=0;i<4;i++)
 {
  //����1�ļ�ϵͳ����
  Dpt[i].FileSystemType=FlashSectorBuf[0x1C2+i*16];
  //����1��ʼ������
  Dpt[i].StartSector=FlashSectorBuf[0x1C9+i*16];
  Dpt[i].StartSector<<=8;
  Dpt[i].StartSector+=FlashSectorBuf[0x1C8+i*16];
  Dpt[i].StartSector<<=8;
  Dpt[i].StartSector+=FlashSectorBuf[0x1C7+i*16];
  Dpt[i].StartSector<<=8;
  Dpt[i].StartSector+=FlashSectorBuf[0x1C6+i*16];
  //������������
  Dpt[i].TotalSectors=FlashSectorBuf[0x1CD+i*16];
  Dpt[i].TotalSectors<<=8;
  Dpt[i].TotalSectors+=FlashSectorBuf[0x1CC+i*16];
  Dpt[i].TotalSectors<<=8;
  Dpt[i].TotalSectors+=FlashSectorBuf[0x1CB+i*16];
  Dpt[i].TotalSectors<<=8;
  Dpt[i].TotalSectors+=FlashSectorBuf[0x1CA+i*16];
 }
 
 //��������0��DBR
 //ÿ�����ֽ���
 Dbr[0].BytesPerSector=FlashSectorBuf[0x0C];
 Dbr[0].BytesPerSector<<=8;
 Dbr[0].BytesPerSector+=FlashSectorBuf[0x0B];
 //ÿ��������
 Dbr[0].SectorsPerCluster=FlashSectorBuf[0x0D];
 //����������
 Dbr[0].ReserveSectors=FlashSectorBuf[0x0F];
 Dbr[0].ReserveSectors<<=8;
 Dbr[0].ReserveSectors+=FlashSectorBuf[0x0E];
 //FAT������
 Dbr[0].NumOfFat=FlashSectorBuf[0x10];
 //FAT16�ĸ�Ŀ¼��
 Dbr[0].Fat16RootNum=FlashSectorBuf[0x12];
 Dbr[0].Fat16RootNum<<=8;
 Dbr[0].Fat16RootNum+=FlashSectorBuf[0x11];
 //С������
 Dbr[0].SmallSectors=FlashSectorBuf[0x14];
 Dbr[0].SmallSectors<<=8;
 Dbr[0].SmallSectors+=FlashSectorBuf[0x13];
 //FAT12/16 ÿFAT������
 Dbr[0].SectorsPerFat16=FlashSectorBuf[0x17];
 Dbr[0].SectorsPerFat16<<=8;
 Dbr[0].SectorsPerFat16+=FlashSectorBuf[0x16];
 //����������
 Dbr[0].HiddenSectors=FlashSectorBuf[0x1F];
 Dbr[0].HiddenSectors<<=8;
 Dbr[0].HiddenSectors+=FlashSectorBuf[0x1E];
 Dbr[0].HiddenSectors<<=8;
 Dbr[0].HiddenSectors+=FlashSectorBuf[0x1D];
 Dbr[0].HiddenSectors<<=8;
 Dbr[0].HiddenSectors+=FlashSectorBuf[0x1C];
 //��������
 Dbr[0].LargeSectors=FlashSectorBuf[0x23];
 Dbr[0].LargeSectors<<=8;
 Dbr[0].LargeSectors+=FlashSectorBuf[0x22];
 Dbr[0].LargeSectors<<=8;
 Dbr[0].LargeSectors+=FlashSectorBuf[0x21];
 Dbr[0].LargeSectors<<=8;
 Dbr[0].LargeSectors+=FlashSectorBuf[0x20];
 //FAT32ÿFAT������
 Dbr[0].SectorsPerFat32=FlashSectorBuf[0x27];
 Dbr[0].SectorsPerFat32<<=8;
 Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x26];
 Dbr[0].SectorsPerFat32<<=8;
 Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x25];
 Dbr[0].SectorsPerFat32<<=8;
 Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x24];
 //��Ŀ¼�غ�
 Dbr[0].RootClusterNum=FlashSectorBuf[0x2F];
 Dbr[0].RootClusterNum<<=8;
 Dbr[0].RootClusterNum+=FlashSectorBuf[0x2E];
 Dbr[0].RootClusterNum<<=8;
 Dbr[0].RootClusterNum+=FlashSectorBuf[0x2D];
 Dbr[0].RootClusterNum<<=8;
 Dbr[0].RootClusterNum+=FlashSectorBuf[0x2C];
 //ϵͳID��
 Dbr[0].SystemId[0]=FlashSectorBuf[0x52];
 Dbr[0].SystemId[1]=FlashSectorBuf[0x53];
 Dbr[0].SystemId[2]=FlashSectorBuf[0x54];
 Dbr[0].SystemId[3]=FlashSectorBuf[0x55];
 Dbr[0].SystemId[4]=FlashSectorBuf[0x56];
 Dbr[0].SystemId[5]=FlashSectorBuf[0x57];
 Dbr[0].SystemId[6]=FlashSectorBuf[0x58];
 Dbr[0].SystemId[7]=FlashSectorBuf[0x59];
 
 if(CheckFileSystemId(0)==0) //��FAT32�ļ�ϵͳ����˵����������DBR
 {
  if(Dbr[0].BytesPerSector==512)  //ֻ֧��ÿ����512�ֽڵ��ļ�ϵͳ
  {
   FatStartSector[0]=Dbr[0].ReserveSectors;
   DataStartSector[0]=FatStartSector[0]+(Dbr[0].NumOfFat)*(Dbr[0].SectorsPerFat32);
   FileSystemIsOk=1;
  }
  else  //����ÿ����512�ֽڣ����󣬿�����MBR
  {
  }
 }
 else //û�м�⵽FAT32�ļ�ϵͳǩ�����������MBR
 {
  if((Dpt[0].StartSector==0)||(Dpt[0].TotalSectors==0)||(Dpt[0].StartSector>200))
  {
   return 2;
  }     
 }
 
 if(!FileSystemIsOk)
 {
  //���е������˵��ǰ�������0����ΪMBR��
  //��ô������0��DBR�����Ƿ���ȷ�������֧��512�ֽ�ÿ����
  FlashReadOneSector(Dpt[0].StartSector*512,FlashSectorBuf,0);
  
  //�жϽ����Ƿ�Ϊ0x55��0xAA
  if((FlashSectorBuf[510]!=0x55)||(FlashSectorBuf[511]!=0xAA))
  {
   return 3;
  }
  
  //ÿ�����ֽ���
  Dbr[0].BytesPerSector=FlashSectorBuf[0x0C];
  Dbr[0].BytesPerSector<<=8;
  Dbr[0].BytesPerSector+=FlashSectorBuf[0x0B];
  //ÿ��������
  Dbr[0].SectorsPerCluster=FlashSectorBuf[0x0D];
  //����������
  Dbr[0].ReserveSectors=FlashSectorBuf[0x0F];
  Dbr[0].ReserveSectors<<=8;
  Dbr[0].ReserveSectors+=FlashSectorBuf[0x0E];
  //FAT������
  Dbr[0].NumOfFat=FlashSectorBuf[0x10];
  //FAT16�ĸ�Ŀ¼��
  Dbr[0].Fat16RootNum=FlashSectorBuf[0x12];
  Dbr[0].Fat16RootNum<<=8;
  Dbr[0].Fat16RootNum+=FlashSectorBuf[0x11];
  //С������
  Dbr[0].SmallSectors=FlashSectorBuf[0x14];
  Dbr[0].SmallSectors<<=8;
  Dbr[0].SmallSectors+=FlashSectorBuf[0x13];
  //FAT12/16 ÿFAT������
  Dbr[0].SectorsPerFat16=FlashSectorBuf[0x17];
  Dbr[0].SectorsPerFat16<<=8;
  Dbr[0].SectorsPerFat16+=FlashSectorBuf[0x16];
  //����������
  Dbr[0].HiddenSectors=FlashSectorBuf[0x1F];
  Dbr[0].HiddenSectors<<=8;
  Dbr[0].HiddenSectors+=FlashSectorBuf[0x1E];
  Dbr[0].HiddenSectors<<=8;
  Dbr[0].HiddenSectors+=FlashSectorBuf[0x1D];
  Dbr[0].HiddenSectors<<=8;
  Dbr[0].HiddenSectors+=FlashSectorBuf[0x1C];
  //��������
  Dbr[0].LargeSectors=FlashSectorBuf[0x23];
  Dbr[0].LargeSectors<<=8;
  Dbr[0].LargeSectors+=FlashSectorBuf[0x22];
  Dbr[0].LargeSectors<<=8;
  Dbr[0].LargeSectors+=FlashSectorBuf[0x21];
  Dbr[0].LargeSectors<<=8;
  Dbr[0].LargeSectors+=FlashSectorBuf[0x20];
  //FAT32ÿFAT������
  Dbr[0].SectorsPerFat32=FlashSectorBuf[0x27];
  Dbr[0].SectorsPerFat32<<=8;
  Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x26];
  Dbr[0].SectorsPerFat32<<=8;
  Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x25];
  Dbr[0].SectorsPerFat32<<=8;
  Dbr[0].SectorsPerFat32+=FlashSectorBuf[0x24];
  //��Ŀ¼�غ�
  Dbr[0].RootClusterNum=FlashSectorBuf[0x2F];
  Dbr[0].RootClusterNum<<=8;
  Dbr[0].RootClusterNum+=FlashSectorBuf[0x2E];
  Dbr[0].RootClusterNum<<=8;
  Dbr[0].RootClusterNum+=FlashSectorBuf[0x2D];
  Dbr[0].RootClusterNum<<=8;
  Dbr[0].RootClusterNum+=FlashSectorBuf[0x2C];
  //ϵͳID��
  Dbr[0].SystemId[0]=FlashSectorBuf[0x52];
  Dbr[0].SystemId[1]=FlashSectorBuf[0x53];
  Dbr[0].SystemId[2]=FlashSectorBuf[0x54];
  Dbr[0].SystemId[3]=FlashSectorBuf[0x55];
  Dbr[0].SystemId[4]=FlashSectorBuf[0x56];
  Dbr[0].SystemId[5]=FlashSectorBuf[0x57];
  Dbr[0].SystemId[6]=FlashSectorBuf[0x58];
  Dbr[0].SystemId[7]=FlashSectorBuf[0x59];
  
  if(CheckFileSystemId(0)==0) //��FAT32�ļ�ϵͳ
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
 
 DriverNumber=0;  //C��
 FlashReadOneSector(FatStartSector[DriverNumber]*Dbr[0].BytesPerSector,FlashSectorBuf,0);
 ((uint8 *)&MaxClusterNumber[DriverNumber])[0]=FlashSectorBuf[0];
 ((uint8 *)&MaxClusterNumber[DriverNumber])[1]=FlashSectorBuf[1];
 ((uint8 *)&MaxClusterNumber[DriverNumber])[2]=FlashSectorBuf[2];
 ((uint8 *)&MaxClusterNumber[DriverNumber])[3]=FlashSectorBuf[3];
 CurrentDir.FirstClusterNumber=Dbr[0].RootClusterNum;
 LastFatLba=0xFFFFFFFF;  //��Ϊ��ЧLBA
 return 0;
}
/////////////////////////End of function/////////////////////////////
