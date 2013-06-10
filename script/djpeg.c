//////////////////////////////////////////////////////////////////////////////
// JPEG�f�R�[�h
// 2005�N10��26�� V1.0 
// All Rights Reserved, Copyright (c) Hidemi Ishihara
//////////////////////////////////////////////////////////////////////////////
//
// JPEG����͂����Bitmap���o�͂��܂��B
// 
// % gcc -o djpeg fjpeg.c
// % djpeg ���̓t�@�C�����@�o�̓t�@�C����
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>

unsigned int BuffIndex;  // JPEG�f�[�^�̈ʒu
unsigned int BuffSize;   // JPEG�f�[�^�̑傫��
unsigned int BuffX;      // �摜�̉��T�C�Y
unsigned int BuffY;      // �摜�̏c�T�C�Y
unsigned int BuffBlockX; // MCU�̉���
unsigned int BuffBlockY; // MCU�̏c��
unsigned char *Buff;     // �L�������f�[�^������o�b�t�@

unsigned char  TableDQT[4][64];  // �ʎq���e�[�u��
unsigned short TableDHT[4][162]; // �n�t�}���e�[�u��

unsigned short TableHT[4][16]; // �n�t�}���X�^�[�g�e�[�u��
unsigned char  TableHN[4][16]; // �n�t�}���X�^�[�g�ԍ�

unsigned char BitCount = 0; // ���k�f�[�^�̓ǂݍ��݈ʒu
unsigned int LineData;      // �L���Ɏg���f�[�^
unsigned int NextData;      // �L���Ɏg���f�[�^

unsigned int PreData[3]; // DC�����p�̒��߃o�b�t�@

unsigned char CompGray;  // �O���[�X�P�[���Ȃ�1
unsigned char CompNum[3];  // �R���|�[�l���g��DQT�e�[�u���ԍ�

// �W�O�U�O�e�[�u��
int zigzag_table[]={
     0, 1, 8, 16,9, 2, 3,10,
    17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,
    27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,
    29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,
    53,60,61,54,47,55,62,63,
    0
};

typedef unsigned short WORD;
//typedef unsigned long DWORD;
//typedef long LONG;
typedef unsigned int DWORD;
typedef int LONG;

typedef struct tagBITMAPFILEHEADER {
  WORD    bfType;
  DWORD   bfSize;
  WORD    bfReserved1;
  WORD    bfReserved2;
  DWORD   bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
  DWORD  biSize;
  LONG   biWidth;
  LONG   biHeight;
  WORD   biPlanes;
  WORD   biBitCount;
  DWORD  biCompression;
  DWORD  biSizeImage;
  LONG   biXPelsPerMeter;
  LONG   biYPelsPerMeter;
  DWORD  biClrUsed;
  DWORD  biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

//////////////////////////////////////////////////////////////////////////////
// Bitmap���o�͂���
// file: �t�@�C����
// x,y:  �摜�̃T�C�Y
// b:    �o�C�g�J�E���g(1�h�b�g�ӂ�̃o�C�g��)
//////////////////////////////////////////////////////////////////////////////
void BmpSave(unsigned char *file,unsigned char *buff,
             unsigned int x,unsigned int y,unsigned int b){
  BITMAPFILEHEADER lpBf;
  BITMAPINFOHEADER lpBi;
  unsigned char tbuff[4];
  FILE *fp;
  unsigned char str;
  int i,k;

  if((fp = fopen(file,"wb")) == NULL){
    perror(0);
    exit(0);
  }

  // �t�@�C���w�b�_�̐ݒ�
  tbuff[0] = 'B';
  tbuff[1] = 'M';
  fwrite(tbuff,2,1,fp);
  tbuff[3] = ((14 +40 +x *y *b) >> 24) & 0xff;
  tbuff[2] = ((14 +40 +x *y *b) >> 16) & 0xff;
  tbuff[1] = ((14 +40 +x *y *b) >>  8) & 0xff;
  tbuff[0] = ((14 +40 +x *y *b) >>  0) & 0xff;
  fwrite(tbuff,4,1,fp);
  tbuff[1] = 0;
  tbuff[0] = 0;
  fwrite(tbuff,2,1,fp);
  fwrite(tbuff,2,1,fp);
  tbuff[3] = 0;
  tbuff[2] = 0;
  tbuff[1] = 0;
  tbuff[0] = 54;
  fwrite(tbuff,4,1,fp);

  // �C���t�H���[�V�����̐ݒ�
  lpBi.biSize            = 40;
  lpBi.biWidth           = x;
  lpBi.biHeight          = y;
  lpBi.biPlanes          = 1;
  lpBi.biBitCount        = b*8;
  lpBi.biCompression     = 0;
  lpBi.biSizeImage       = x*y*b;
  lpBi.biXPelsPerMeter   = 300;
  lpBi.biYPelsPerMeter   = 300;
  lpBi.biClrUsed         = 0;
  lpBi.biClrImportant    = 0;
  fwrite(&lpBi,1,40,fp);

  // �㉺���]
  for(k=0;k<y/2;k++){
    for(i=0;i<x*3;i++){
      str = buff[k*x*3+i];
      buff[k*x*3+i] = buff[((y-1)*x*3 -k*x*3) +i];
      buff[((y-1)*x*3-k*x*3) +i] = str;
    }
  }

  fwrite(buff,1,x*y*b,fp);

  fclose(fp);
}

//////////////////////////////////////////////////////////////////////////////
// 1Byte�擾
unsigned char get_byte(unsigned char *buff){
  if(BuffIndex >= BuffSize) return 0;
  return buff[BuffIndex++];
}
//////////////////////////////////////////////////////////////////////////////
// 2Byte�擾
unsigned short get_word(unsigned char *buff){
  unsigned char h,l;
  h = get_byte(buff);
  l = get_byte(buff);
  return (h<<8)|l;
}

//////////////////////////////////////////////////////////////////////////////
// 32bit�f�[�^�擾(�L�����̂ݎg�p����)
unsigned int get_data(unsigned char *buff){
  unsigned char str = 0;
  unsigned int data = 0;
  str = get_byte(buff);
  if(str ==0xff) if(get_byte(buff)== 0x00) str = 0xFF; else str = 0x00;
  data = str;
  str = get_byte(buff);
  if(str ==0xff) if(get_byte(buff)== 0x00) str = 0xFF; else str = 0x00;
  data = (data << 8) | str;
  str = get_byte(buff);
  if(str ==0xff) if(get_byte(buff)== 0x00) str = 0xFF; else str = 0x00;
  data = (data << 8) | str;
  str = get_byte(buff);
  if(str ==0xff) if(get_byte(buff)== 0x00) str = 0xFF; else str = 0x00;
  data = (data << 8) | str;
  //printf(" Get Data: %08x\n",data);
  return data;
}

//////////////////////////////////////////////////////////////////////////////
// APP0����
void GetAPP0(unsigned char *buff){
  unsigned short data;
  unsigned char str;
  unsigned int i;
  
  data = get_word(buff); // Lp(�����O�X)
  // APP0�͓ǂ܂Ȃ��Ă������̂Ŏ�荇���������O�X���X�L�b�v����
  for(i=0;i<data-2;i++){
    str = get_byte(buff);
  }
  /*
  str = get_byte(buff);  // ���ʎq(5����,"JFIF"��[00])
  str = get_byte(buff);
  str = get_byte(buff);
  str = get_byte(buff);
  str = get_byte(buff);
  data = get_word(buff); // �o�[�W����
  str = get_byte(buff);  // �𑜓x�̒P��
  data = get_word(buff); // �������̉𑜓x
  data = get_word(buff); // �c�����̉𑜓x
  data = get_word(buff); // �T���l�C���̉��s�N�Z����
  data = get_word(buff); // �T���l�C���̏c�s�N�Z����
  data = get_word(buff); // �T���l�C���f�[�^(����ꍇ����)
  */
}

//////////////////////////////////////////////////////////////////////////////
// DQT����
void GetDQT(unsigned char *buff){
  unsigned short data;
  unsigned char str;
  unsigned int i;
  unsigned int tablenum;

  data = get_word(buff);
  str = get_byte(buff); // �e�[�u���ԍ�
  
  printf("*** DQT Table %d\n",str);
  for(i=0;i<64;i++){
    TableDQT[str][i] = get_byte(buff);
    printf(" %2d: %2x\n",i,TableDQT[str][i]);
  }
}

//////////////////////////////////////////////////////////////////////////////
// DHT����
void GetDHT(unsigned char *buff){
  unsigned short data;
  unsigned char str;
  unsigned int i;
  unsigned char max,count;
  unsigned short ShiftData = 0x8000,HuffmanData =0x0000;
  unsigned int tablenum;

  data = get_word(buff);
  str = get_byte(buff);

  switch(str){
  case 0x00:
    // Y��������
    tablenum = 0x00;
    break;
  case 0x10:
    // Y�𗬐���
    tablenum = 0x01;
    break;
  case 0x01:
    // CbCr��������
    tablenum = 0x02;
    break;
  case 0x11:
    // CbCr�𗬐���
    tablenum = 0x03;
    break;
  }

  printf("*** DHT Table/Number %d\n",tablenum);
  // �e�[�u�����쐬����
  max = 0;
  for(i=0;i<16;i++){
    count = get_byte(buff);
    TableHT[tablenum][i] = HuffmanData;
    TableHN[tablenum][i] = max;
    printf(" %2d: %4x,%2x\n",i,TableHT[tablenum][i],TableHN[tablenum][i]);
    max = max + count;
    while(!(count==0)){
      HuffmanData += ShiftData;
      count--;
    }
    ShiftData = ShiftData >> 1; // �E��1bit�V�t�g����
  }

  printf("*** DHT Table %d\n",tablenum);
  for(i=0;i<max;i++){
    TableDHT[tablenum][i] = get_byte(buff);
    printf(" %2d: %2x\n",i,TableDHT[tablenum][i]);
  }
}

//////////////////////////////////////////////////////////////////////////////
// SOF����
void GetSOF(unsigned char *buff){
  unsigned short data;
  unsigned char str;
  unsigned int i;
  unsigned char count;

  data = get_word(buff);
  str = get_byte(buff);
  BuffY = get_word(buff); // �摜�̉��T�C�Y
  BuffX = get_word(buff); // �摜�̏c�T�C�Y
  count = get_byte(buff); // �f�[�^�̃R���|�[�l���g��
  switch(count){
    1: CompGray = 1; break;  // �O���[�X�P�[��
    3: CompGray = 0; break;  // YCbYr or YIQ
    default: CompGray = 0; break;
  }
  printf(" CompNum: %d\n", count);
  for(i=0;i<count;i++){
    str = get_byte(buff); // �R���|�[�l���g�ԍ�
    printf(" Comp[%d]: %02X\n", i, str);
    str = get_byte(buff); // �T���v�����O�䗦
    printf(" Sample[%d]: %02X\n", i, str);
    str = get_byte(buff); // DQT�e�[�u���ԍ�
    printf(" DQT[%d]: %02X\n", i, str);
  }

  // MCU�̃T�C�Y���Z�o����
  BuffBlockX = (int)(BuffX /16);
  if(BuffX % 16 >0) BuffBlockX++;
  BuffBlockY = (int)(BuffY /16);
  if(BuffY % 16 >0) BuffBlockY++;
  Buff = (unsigned char*)malloc(BuffBlockY*16*BuffBlockX*16*3);

  printf(" size : %d x %d,(%d x %d)\n",BuffX,BuffY,BuffBlockX,BuffBlockY);
}

//////////////////////////////////////////////////////////////////////////////
// SOS����
void GetSOS(unsigned char *buff){
  unsigned short data;
  unsigned char str;
  unsigned int i;
  unsigned char count;

  data = get_word(buff);
  count = get_byte(buff);
  for(i=0;i<count;i++){
    str = get_byte(buff);
    printf(" CompNum[%d]: %02X\n", i, str);
    str = get_byte(buff);
    printf(" CompDHT[%d]: %02X\n", i, str);
  }
  str = get_byte(buff);
  str = get_byte(buff);
  str = get_byte(buff);
}

//////////////////////////////////////////////////////////////////////////////
// �n�t�}���f�R�[�h�{�t�ʎq���{�t�W�O�U�O
void HuffmanDecode(unsigned char *buff, unsigned char table, int *BlockData){
  unsigned int data;
  unsigned char zero;
  unsigned short code,huffman;
  unsigned char count =0;
  unsigned int BitData;
  unsigned int i;
  unsigned char tabledqt,tabledc,tableac,tablen;
  unsigned char ZeroCount,DataCount;
  int DataCode;

  for(i=0;i<64;i++) BlockData[i] = 0x0; // �f�[�^�̃��Z�b�g

  // �e�[�u���ԍ���ݒ肷��
  if(table ==0x00){
    tabledqt =0x00;
    tabledc =0x00;
    tableac =0x01;
  }else if(table ==0x01){
    tabledqt =0x01;
    tabledc  =0x02;
    tableac  =0x03;
  }else{
    tabledqt =0x01;
    tabledc  =0x02;
    tableac  =0x03;
  }

  count = 0; // �O�̂��߂�
  while(count <64){
    // �r�b�g�J�E���g�̈ʒu��32���z�����ꍇ�A�V���Ƀf�[�^���擾����
    if(BitCount >=32){
      LineData = NextData;
      NextData = get_data(buff);
      BitCount -= 32;
    }
    // Huffman�f�R�[�h�Ŏg�p����f�[�^�ɒu��������
    if(BitCount >0){
      BitData = (LineData << BitCount) | (NextData >> (32 - BitCount));
    }else{
      BitData = LineData;
    }
    printf(" Haffuman BitData(%2d,%2d): %8x\n",table,count,BitData);

    // �g�p����e�[�u���̃Z���N�g
    if(count ==0) tablen = tabledc; else tablen = tableac;
    code = (unsigned short)(BitData >> 16); // �R�[�h��16�r�b�g�g�p����
    // �n�t�}���R�[�h���ǂ̃r�b�g���ɂ��邩����o��
    for(i=0;i<16;i++) {
      printf(" Haff hit(%2d:%2d): %8x,%8x\n",table,i,TableHT[tablen][i],code);
      if(TableHT[tablen][i]>code) break;
    }
    i--;

    code    = (unsigned short)(code >> (15 - i)); // �R�[�h�̉��ʂ𑵂���
    huffman = (unsigned short)(TableHT[tablen][i] >> (15 - i));

    printf(" PreUse Dht Number(%2d): %8x,%8x,%8x\n",i,code,huffman,TableHN[tablen][i]);

    // �n�t�}���e�[�u���̏ꏊ���Z�o����
    code = code - huffman + TableHN[tablen][i];

    printf(" Use Dht Number: %8x\n",code);

    ZeroCount = (TableDHT[tablen][code] >> 4) & 0x0F; // �[�������O�X�̌�
    DataCount = (TableDHT[tablen][code]) & 0x0F;      // �����f�[�^�̃r�b�g��
    printf(" Dht Table: %8x,%8x\n",ZeroCount,DataCount);
    // �n�t�}���R�[�h�𔲂��A�����f�[�^���擾����
    DataCode  = (BitData << (i + 1)) >> (16 + (16 - DataCount));
    // �擪�r�b�g��"0"�ł���Ε��̃f�[�^�A��ʃr�b�g�ɂP�𗧂ĂāA�P�𑫂�
    //if(!(DataCode & (1<<(DataCount-1)))) DataCode=DataCode-(1<<DataCount)+1;
    if(!(DataCode & (1<<(DataCount-1))) && DataCount !=0){
      DataCode |= (~0) << DataCount;
      DataCode += 1;
    }

    printf(" Use Bit: %d\n",(i + DataCount +1));
    BitCount += (i + DataCount +1); // �g�p�����r�b�g�������Z����

    if(count ==0){
      // DC�����̏ꍇ�A�f�[�^�ƂȂ�
      if(DataCount ==0) DataCode =0x0; // DataCount��0�Ȃ�f�[�^��0�ł���
      PreData[table] += DataCode; // DC�����͉��Z���Ȃ���΂Ȃ�Ȃ�
      // �t�ʎq���{�W�O�U�O
      BlockData[zigzag_table[count]] =PreData[table]*TableDQT[tabledqt][count];
      count ++;
    }else{
      if(ZeroCount == 0x0 && DataCount == 0x0){
        // AC������EOB�����������ꍇ�͏I������
        break;
      }else if(ZeroCount ==0xF && DataCount == 0x0){
        // ZRL�����������ꍇ�A15�̃[���f�[�^�Ƃ݂Ȃ�
        count += 15;
      }else{
        count += ZeroCount;
        // �t�ʎq���{�W�O�U�O
        BlockData[zigzag_table[count]] = DataCode * TableDQT[tabledqt][count];
      }
      count ++;
    }
  }
}

const int C1_16 = 4017; // cos( pi/16) x4096
const int C2_16 = 3784; // cos(2pi/16) x4096
const int C3_16 = 3406; // cos(3pi/16) x4096
const int C4_16 = 2896; // cos(4pi/16) x4096
const int C5_16 = 2276; // cos(5pi/16) x4096
const int C6_16 = 1567; // cos(6pi/16) x4096
const int C7_16 = 799;  // cos(7pi/16) x4096

//////////////////////////////////////////////////////////////////////////////
// �tDCT
void DctDecode(int *BlockIn, int *BlockOut){
  int i;
  int s0,s1,s2,s3,s4,s5,s6,s7;
  int t0,t1,t2,t3,t4,t5,t6,t7;

  /*
  printf("-----------------------------\n");
  printf(" iDCT(In)\n");
  printf("-----------------------------\n");
  for(i=0;i<64;i++){
    printf("%2d: %8x\n",i,BlockIn[i]);
  }
  */

  for(i=0;i<8;i++) {
    s0 = (BlockIn[0] + BlockIn[4]) * C4_16;
    s1 = (BlockIn[0] - BlockIn[4]) * C4_16;
    s3 = (BlockIn[2] * C2_16) + (BlockIn[6] * C6_16);
    s2 = (BlockIn[2] * C6_16) - (BlockIn[6] * C2_16);
    s7 = (BlockIn[1] * C1_16) + (BlockIn[7] * C7_16);
    s4 = (BlockIn[1] * C7_16) - (BlockIn[7] * C1_16);
    s6 = (BlockIn[5] * C5_16) + (BlockIn[3] * C3_16);
    s5 = (BlockIn[5] * C3_16) - (BlockIn[3] * C5_16);

    /*
    printf("s0:%8x\n",s0);
    printf("s1:%8x\n",s1);
    printf("s2:%8x\n",s2);
    printf("s3:%8x\n",s3);
    printf("s4:%8x\n",s4);
    printf("s5:%8x\n",s5);
    printf("s6:%8x\n",s6);
    printf("s7:%8x\n",s7);
    */

    t0 = s0 + s3;
    t3 = s0 - s3;
    t1 = s1 + s2;
    t2 = s1 - s2;
    t4 = s4 + s5;
    t5 = s4 - s5;
    t7 = s7 + s6;
    t6 = s7 - s6;

    /*    
    printf("t0:%8x\n",t0);
    printf("t1:%8x\n",t1);
    printf("t2:%8x\n",t2);
    printf("t3:%8x\n",t3);
    printf("t4:%8x\n",t4);
    printf("t5:%8x\n",t5);
    printf("t6:%8x\n",t6);
    printf("t7:%8x\n",t7);
    */

    s6 = (t5 + t6) * 181 / 256; // 1/sqrt(2)
    s5 = (t6 - t5) * 181 / 256; // 1/sqrt(2)

    /*    
    printf("s5:%8x\n",s5);
    printf("s6:%8x\n",s6);
    */

    *BlockIn++ = (t0 + t7) >> 11;
    *BlockIn++ = (t1 + s6) >> 11;
    *BlockIn++ = (t2 + s5) >> 11;
    *BlockIn++ = (t3 + t4) >> 11;
    *BlockIn++ = (t3 - t4) >> 11;
    *BlockIn++ = (t2 - s5) >> 11;
    *BlockIn++ = (t1 - s6) >> 11;
    *BlockIn++ = (t0 - t7) >> 11;
  }

  BlockIn -= 64;

  /*
  printf("-----------------------------\n");
  printf(" iDCT(Middle)\n");
  printf("-----------------------------\n");
  for(i=0;i<64;i++){
    printf("%2d: %8x\n",i,BlockIn[i]);
  }
  */

  for(i=0;i<8;i++){
    s0 = (BlockIn[ 0] + BlockIn[32]) * C4_16;
    s1 = (BlockIn[ 0] - BlockIn[32]) * C4_16;
    s3 = BlockIn[16] * C2_16 + BlockIn[48] * C6_16;
    s2 = BlockIn[16] * C6_16 - BlockIn[48] * C2_16;
    s7 = BlockIn[ 8] * C1_16 + BlockIn[56] * C7_16;
    s4 = BlockIn[ 8] * C7_16 - BlockIn[56] * C1_16;
    s6 = BlockIn[40] * C5_16 + BlockIn[24] * C3_16;
    s5 = BlockIn[40] * C3_16 - BlockIn[24] * C5_16;

    /*
    printf("s0:%8x\n",s0);
    printf("s1:%8x\n",s1);
    printf("s2:%8x\n",s2);
    printf("s3:%8x\n",s3);
    printf("s4:%8x\n",s4);
    printf("s5:%8x\n",s5);
    printf("s6:%8x\n",s6);
    printf("s7:%8x\n",s7);
    */

    t0 = s0 + s3;
    t1 = s1 + s2;
    t2 = s1 - s2;
    t3 = s0 - s3;
    t4 = s4 + s5;
    t5 = s4 - s5;
    t6 = s7 - s6;
    t7 = s6 + s7;

    /*
    printf("t0:%8x\n",t0);
    printf("t1:%8x\n",t1);
    printf("t2:%8x\n",t2);
    printf("t3:%8x\n",t3);
    printf("t4:%8x\n",t4);
    printf("t5:%8x\n",t5);
    printf("t6:%8x\n",t6);
    printf("t7:%8x\n",t7);
    */

    s5 = (t6 - t5) * 181 / 256; // 1/sqrt(2)
    s6 = (t5 + t6) * 181 / 256; // 1/sqrt(2)

    /*
    printf("s5:%8x\n",s5);
    printf("s6:%8x\n",s6);
    */

    BlockOut[ 0] = ((t0 + t7) >> 15);
    BlockOut[56] = ((t0 - t7) >> 15);
    BlockOut[ 8] = ((t1 + s6) >> 15);
    BlockOut[48] = ((t1 - s6) >> 15);
    BlockOut[16] = ((t2 + s5) >> 15);
    BlockOut[40] = ((t2 - s5) >> 15);
    BlockOut[24] = ((t3 + t4) >> 15);
    BlockOut[32] = ((t3 - t4) >> 15);
    
    BlockIn++;
    BlockOut++;
  }
  BlockOut-=8;
  /*
  printf("-----------------------------\n");
  printf(" iDCT(Out)\n");
  printf("-----------------------------\n");
  for(i=0;i<8;i++){
    printf(" %2d: %04x;\n",i+ 0,BlockOut[i+ 0]&0xFFFF);
    printf(" %2d: %04x;\n",i+56,BlockOut[i+56]&0xFFFF);
    printf(" %2d: %04x;\n",i+ 8,BlockOut[i+ 8]&0xFFFF);
    printf(" %2d: %04x;\n",i+48,BlockOut[i+48]&0xFFFF);
    printf(" %2d: %04x;\n",i+16,BlockOut[i+16]&0xFFFF);
    printf(" %2d: %04x;\n",i+40,BlockOut[i+40]&0xFFFF);
    printf(" %2d: %04x;\n",i+24,BlockOut[i+24]&0xFFFF);
    printf(" %2d: %04x;\n",i+32,BlockOut[i+32]&0xFFFF);
  }
  */
}

//////////////////////////////////////////////////////////////////////////////
// 4:1:1�̃f�R�[�h����
void Decode411(unsigned char *buff, int *BlockY, int *BlockCb, int *BlockCr){
  int BlockHuffman[64];
  int BlockYLT[64];
  int BlockYRT[64];
  int BlockYLB[64];
  int BlockYRB[64];
  unsigned int i;

  // �P�x(����)
  printf("Block:00\n");
  HuffmanDecode(buff,0x00,BlockHuffman);
  DctDecode(BlockHuffman,BlockYLT);
  // �P�x(�E��)
  printf("Block:01\n");
  HuffmanDecode(buff,0x00,BlockHuffman);
  DctDecode(BlockHuffman,BlockYRT);
  // �P�x(����)
  printf("Block:02\n");
  HuffmanDecode(buff,0x00,BlockHuffman);
  DctDecode(BlockHuffman,BlockYLB);
  // �P�x(�E��)
  printf("Block:03\n");
  HuffmanDecode(buff,0x00,BlockHuffman);
  DctDecode(BlockHuffman,BlockYRB);
  // �F��
  printf("Block:10\n");
  HuffmanDecode(buff,0x01,BlockHuffman);
  DctDecode(BlockHuffman,BlockCb);
  // �ԐF��
  printf("Block:11\n");
  HuffmanDecode(buff,0x02,BlockHuffman);
  DctDecode(BlockHuffman,BlockCr);
  
  // �u���b�N�T�C�Y��16x16�ɂ���
  for(i=0;i<64;i++){
    BlockY[(int)(i/8) *16 +(i % 8)] = BlockYLT[i];
    BlockY[(int)(i/8) *16 +(i % 8)+8] = BlockYRT[i];
    BlockY[(int)(i/8) *16 +(i % 8)+128] = BlockYLB[i];
    BlockY[(int)(i/8) *16 +(i % 8)+128+8] = BlockYRB[i];
  }
}

//////////////////////////////////////////////////////////////////////////////
// YUV��RGB�ɕϊ�
void DecodeYUV(int *y, int *cb, int *cr, unsigned char *rgb){
  int r,g,b;
  int p,i;

  printf("----RGB----\n");
  for(i=0;i<256;i++){
    p = ((int)(i/32) * 8) + ((int)((i % 16)/2));
    r = 128 + y[i] + cr[p]*1.402;
    r = (r & 0xffffff00) ? (r >> 24) ^ 0xff : r;
    g = 128 + y[i] - cb[p]*0.34414 - cr[p]*0.71414;
    g = (g & 0xffffff00) ? (g >> 24) ^ 0xff : g;
    b = 128 + y[i] + cb[p]*1.772;
    b = (b & 0xffffff00) ? (b >> 24) ^ 0xff : b;
    rgb[i*3+0] = b;
    rgb[i*3+1] = g;
    rgb[i*3+2] = r;
    /*    
    printf("[RGB]%3d: %3x,%3x,%3x = %2x,%2x,%2x\n",i,
           y[i]&0x1FF,cr[p]&0x1FF,cb[p]&0x1FF,
           rgb[i*3+2],rgb[i*3+1],rgb[i*3+0]);
    */
  }
}

//////////////////////////////////////////////////////////////////////////////
// �C���[�W�̃f�R�[�h
void Decode(unsigned char *buff,unsigned char *rgb){
  int BlockY[256];
  int BlockCb[256];
  int BlockCr[256];
  int x,y,i,p;

  for(y=0;y<BuffBlockY;y++){
    for(x=0;x<BuffBlockX;x++){
      Decode411(buff,BlockY,BlockCb,BlockCr); // 4:1:1�̃f�R�[�h
      DecodeYUV(BlockY,BlockCb,BlockCr,rgb);  // YUV��RGB�ϊ�
      for(i=0;i<256;i++){
        if((x*16+(i%16)<BuffX) && (y*16+i/16<BuffY)){
          p=y*16*BuffX*3+x*16*3+(int)(i/16)*BuffX*3+(i%16)*3;
          Buff[p+0] = rgb[i*3+0];
          Buff[p+1] = rgb[i*3+1];
          Buff[p+2] = rgb[i*3+2];
          
          printf("RGB[%4d,%4d]: %2x,%2x,%2x\n",x*16+(i%16),y*16+i/16,
                 rgb[i*3+2],rgb[i*3+1],rgb[i*3+0]);
              
        }
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// �f�R�[�h
void JpegDecode(unsigned char *buff){
  unsigned short data;
  unsigned int i;
  unsigned int Image =0;
  unsigned char RGB[256*3];
  while(!(BuffIndex >= BuffSize)){
    if(Image ==0){
      data = get_word(buff);
      switch(data){
      case 0xFFD8: // SOI
		printf("Header: SOI\n");
        break;
      case 0xFFE0: // APP0
		printf("Header: APP0\n");
        GetAPP0(buff);
        break;
      case 0xFFDB: // DQT
		printf("Header: DQT\n");
        GetDQT(buff);
        break;
      case 0xFFC4: // DHT
		printf("Header: DHT\n");
        GetDHT(buff);
        break;
      case 0xFFC0: // SOF
		printf("Header: SOF\n");
        GetSOF(buff);
        break;
      case 0xFFDA: // SOS
		printf("Header: SOS\n");
        GetSOS(buff);
        Image = 1;
        // �f�[�^�̏���
        PreData[0] = 0x00;
        PreData[1] = 0x00;
        PreData[2] = 0x00;
        LineData = get_data(buff);
        NextData = get_data(buff);
        BitCount =0;
        break;
      case 0xFFD9: // EOI
		printf("Header: EOI\n");
        break;
      default:
        // ���ʂł��Ȃ��w�b�_�[�͓ǂݔ�΂�
		printf("Header: other(%X)\n", data);
        if((data & 0xFF00) == 0xFF00 && !(data == 0xFF00)){
          data = get_word(buff);
          for(i=0;i<data-2;i++){
            get_byte(buff);
          }
        }
        break;
      }
    }else{
      // �L��(SOS�����Ă���)
      printf("/****Image****/\n");
      Decode(buff,RGB);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// ���C���֐�
//////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  unsigned char *buff;
  FILE *fp;

//
printf( " sizeof(char):           %02d\n", sizeof( char ) );
printf( " sizeof(unsigned char):  %02d\n", sizeof( unsigned char ) );
printf( " sizeof(short):          %02d\n", sizeof( short ) );
printf( " sizeof(unsigned short): %02d\n", sizeof( unsigned short ) );
printf( " sizeof(int):            %02d\n", sizeof( int ) );
printf( " sizeof(unsigned int):   %02d\n", sizeof( unsigned int ) );
printf( " sizeof(long):           %02d\n", sizeof( long ) );
printf( " sizeof(unsigned long):  %02d\n", sizeof( unsigned long ) );

printf( " sizeof:                 %02d\n", sizeof( BITMAPFILEHEADER ) );
printf( " sizeof:                 %02d\n", sizeof( BITMAPINFOHEADER ) );

  if((fp = fopen(argv[1],"rb")) == NULL){
    perror(0);
    exit(0);
  }

  // �t�@�C���T�C�Y���擾����
  BuffSize = 0;
  while(!feof(fp)){
    fgetc(fp);
    BuffSize ++;
  }
  BuffSize--;
  rewind(fp); // �t�@�C���|�C���^���ŏ��ɖ߂�

  buff = (unsigned char *)malloc(BuffSize); // �o�b�t�@���m�ۂ���
  fread(buff,1,BuffSize,fp);                // �o�b�t�@�ɓǂݍ���
  BuffIndex = 0;
  JpegDecode(buff);                         // JPEG�f�R�[�h����
  BmpSave(argv[2],Buff,BuffX,BuffY,3);      // Bitmap�ɕۑ�����

  // �S�ĊJ�����܂�
  fclose(fp);
  free(buff);
  free(Buff);

  return 0;
}
