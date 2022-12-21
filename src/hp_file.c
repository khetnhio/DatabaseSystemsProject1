#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define HP_ERROR -1

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

int HP_CreateFile(char *fileName){
  int fd;
  BF_Block *block0;
  BF_Block_Init(&block0);
  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fd));
  void *data;
  CALL_BF(BF_AllocateBlock(fd,block0));
  data = BF_Block_GetData(block0);
  HP_info *info=data;
  memcpy(info->filetype, "heap", strlen("heap")+1);
  info->fileDesc=fd;
  info->key=ID;
  info->lastBlock=0;
  info->maxRecords=6;
  BF_Block_SetDirty(block0);
  CALL_BF(BF_UnpinBlock(block0));
  CALL_BF(BF_CloseFile(fd));
  //CALL_BF(BF_Close());
  return 0;
}

HP_info* HP_OpenFile(char *fileName){
  HP_info *info;
  int fd;
  BF_Block *block;
  BF_Block_Init(&block);
  BF_ErrorCode err;
  err =BF_OpenFile(fileName,&fd);
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL ;
  }
  err=BF_GetBlock(fd,0,block); //παίρνω το block 0 που περιέχει την δομή HP_info απο το αρχείο
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL ;
  }  
  void *data;
  data = BF_Block_GetData(block);
  info = data;
  if (strcmp(info->filetype,"heap") != 0){
    fprintf(stderr,"File is not heap file\n");
    return NULL ;    
  }
  err=BF_UnpinBlock(block);
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL ;
  } 
  return info;
}


int HP_CloseFile( HP_info* hp_info ){
    int fd=hp_info->fileDesc;
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(fd,0,block));
    CALL_BF(BF_CloseFile(fd));
    BF_Block_Destroy(&block);
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
    int fd = hp_info->fileDesc;
    int blockId = hp_info->lastBlock;
    BF_Block *block;
    BF_Block_Init(&block);
    HP_block_info* b_info;
    fd = (hp_info->fileDesc);
    if (blockId == 0){ //περίπτωση που το αρχείο δέν έχει ακόμα blocks με δεδομένα
      printf("The file is empty\n");
      BF_Block_Init(&block);
      CALL_BF(BF_AllocateBlock(fd,block));
      void *data = BF_Block_GetData(block);
      Record *rec = data;
      rec[0]=record;   
      hp_info->lastBlock=1;
      blockId++;
      b_info = data+500;
      b_info->recordsNo=1;
      printf("HP_info: lastBlock: %d No of records in block:",hp_info->lastBlock);  
      printf("%d\n",b_info->recordsNo);
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      return blockId;
    }
    else{
     CALL_BF(BF_GetBlock(fd,blockId,block));// φορτώνω το τελευταίο block στην μνήμη
     void *data = BF_Block_GetData(block);
     b_info =data+500;    
     if ( (b_info->recordsNo) == (hp_info->maxRecords) ){ // περίπτωση που το τελευταίο block του σωρού είναι γεμάτο
       printf("block is full\n");
       CALL_BF(BF_AllocateBlock(fd,block));
       void *data1 = BF_Block_GetData(block);
       Record *rec = data1;
       rec[0] = record;
       blockId++;
       hp_info->lastBlock=blockId;
       b_info = data+500;
       b_info->recordsNo=1;
       printf("HP_info: lastBlock: %d No of records in block:",hp_info->lastBlock);  
       printf("%d\n",b_info->recordsNo);
       BF_Block_SetDirty(block);
       CALL_BF(BF_UnpinBlock(block));
       return blockId;      
     }
     else if( (b_info->recordsNo) < (hp_info->maxRecords) ){// περίπτωση που η νέα καταχώριση χωράει στο τελευταίο block
       printf("block not full\n");
       void *data1=BF_Block_GetData(block);
       Record *rec = data1;
       int i=b_info->recordsNo;
       printf("Insterting: (%d,%s,%s,%s)\n",record.id,record.name,record.surname,record.city);
       rec[0]=record;
       rec[1]=record;
       printf("i is %d\n",i);
       b_info->recordsNo++;
       printf("HP_info: lastBlock: %d No of records in block:",hp_info->lastBlock);      
       printf("%d\n",b_info->recordsNo);
       BF_Block_SetDirty(block);
       CALL_BF(BF_UnpinBlock(block));
       return blockId;
     }
    }


}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

