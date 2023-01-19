#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
#include "ht_table.h"
#include "record.h"

#define SHT_ERROR -1
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }
#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return SHT_ERROR;        \
  }                         \
}

int sHashFunction (char *name,int buckets){
  char *namePtr = name;
  char letter;
  int sum = 0;
  memcpy(&letter,namePtr,sizeof(char));
  while (letter != '\0') {
    //printf("letter =%d\n",letter);
    sum += (letter)*(letter)*0.938181;
    namePtr++;
    memcpy(&letter,namePtr,sizeof(char));
  }
  //printf("The sum is: %d\n",sum);
  return sum % buckets;
}

int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName){
  if (buckets>12) {
    printf("exceeded maximum number of buckets for this implementation (12)\n");
    return SHT_ERROR;
  }
  int fd;
  int blockId;
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_CreateFile(sfileName));
  CALL_BF(BF_OpenFile(sfileName,&fd));
  CALL_BF(BF_AllocateBlock(fd,block));
  void *data = BF_Block_GetData(block);
  SHT_info *info=data;
  memcpy(info->filetype,"SHT File",strlen("SHT File")+1);
  info->fileDesc=fd;
  info->bucketsNum=buckets;
  info->key=NAME;
  info->maxRecords=BF_BLOCK_SIZE/sizeof(shtRecord);
  //memcpy(info->primaryFileName,fileName,25);
  //printf("The block id for the SHT metadata is:%d\n",blockId);
  for (int i=0;i<buckets;i++){ //αρχικοποίηση του πίνακα κατακερματισμού για το δευτερεύον ευρετήριο
    info->hashTable[i]=0; // οταν η τιμή είναι μηδέν, δεν υπάρχει κανένα block στον κάδο
  }
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  CALL_BF(BF_CloseFile(fd));
  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
  SHT_info *info;
  int fd;
  BF_Block *block;
  BF_Block_Init(&block);
  BF_ErrorCode err;
  err=BF_OpenFile(indexName,&fd);
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL;
  }
  err=BF_GetBlock(fd,0,block);// εδω ίσως δεν λειτουργεί γιατι ενδέχεται να μην είναι στο block 0 το SHT_info??
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL;
  }
  void *data =BF_Block_GetData(block);
  info = data;
  if (strcmp(info->filetype,"SHT File")!=0){
    printf("The file type is: %s\n",info->filetype);
    fprintf(stderr,"File is not secondary hash table file what?\n");
  }
  return info;
}


int SHT_CloseSecondaryIndex( SHT_info* SHT_info ){
  int fd = SHT_info->fileDesc;
  BF_Block *block;
  CALL_BF(BF_GetBlock(fd,0,block));
  CALL_BF(BF_UnpinBlock(block));// το block γίνεται unpin μόνο όταν κλείσει το αρχείο
  CALL_BF(BF_CloseFile(fd));
  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record *record, int block_id){
  shtRecord *shtRec = (shtRecord*)malloc(sizeof(shtRecord));  // η πλειάδα που θα μπεί στο block
  shtRec->blockid=block_id; 
  memcpy(shtRec->name,record->name,15);
  int fd= sht_info->fileDesc;
  int bkt = sHashFunction(record->name,sht_info->bucketsNum); // ο κάδος που πρέπει να γίνει η καταχώρηση
  int blockId = sht_info->hashTable[bkt]; // id του τελευταίου block στον κάδο
  void *data;
  BF_Block *block;
  BF_Block_Init(&block);
  SHT_block_info *b_info;
  if (blockId==0){ // περίπτωση που ο κάδος είναι άδειος
    CALL_BF(BF_AllocateBlock(fd,block)); // δημιουργία νέου block
    CALL_BF(BF_GetBlockCounter(fd,&blockId));
    blockId--;// το id είναι ο αριθμός των block -1
    sht_info->hashTable[bkt]=blockId;
    data = BF_Block_GetData(block);
    shtRecord *rec =data;
    memcpy(rec,shtRec,sizeof(shtRecord));
    b_info = (SHT_block_info*)(rec+(sht_info->maxRecords));
    b_info->recordsNum=1;
    b_info->prevBlock=0; // 0 όταν δέν υπάρχει προηγούμενο block στον κάδο
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    return blockId;
  }
  CALL_BF(BF_GetBlock(fd,blockId,block));
  data = BF_Block_GetData(block);
  shtRecord *rec =data;
  b_info = (SHT_block_info*)(rec+(sht_info->maxRecords));
  if (b_info->recordsNum ==sht_info->maxRecords){ //περίπτωση που το block είναι γεμάτο
    int newBlockId;
    CALL_BF(BF_UnpinBlock(block)); // ξεκαρφιτσώνω το προηγούμενο block πριν πάρω το καινούργιο
    CALL_BF(BF_AllocateBlock(fd,block));
    CALL_BF(BF_GetBlockCounter(fd,&newBlockId));
    newBlockId--; // το id είναι ο αριθμός των block -1
    sht_info->hashTable[bkt]=newBlockId; // ενημέρωση του πίνακα
    data = BF_Block_GetData(block);
    rec = data;
    memcpy(rec,shtRec,sizeof(shtRecord));
    b_info = (SHT_block_info*)(rec+(sht_info->maxRecords));
    b_info->recordsNum=1;
    b_info->prevBlock=blockId;
    CALL_BF(BF_UnpinBlock(block));
    return newBlockId;
  }
  else if (b_info->recordsNum < sht_info->maxRecords){
    int recNum = b_info->recordsNum;
    memcpy(rec+recNum,shtRec,sizeof(shtRecord));
    b_info->recordsNum++;
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    return blockId;
  }
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  int sht_fd = sht_info->fileDesc;
  int fd = ht_info->fileDesc;
  int bkt = sHashFunction(name,sht_info->bucketsNum);
  int sht_blockId=sht_info->hashTable[bkt];
  int blockId;
  int blocksRead=0;
  void *data;
  BF_Block *sht_block;
  BF_Block *block;
  BF_Block_Init(&sht_block);
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(sht_fd,sht_blockId,sht_block));
  blocksRead++;
  data = BF_Block_GetData(sht_block);
  shtRecord *shtRec;
  shtRecord shtRecord;
  Record *rec;
  Record record;
  shtRec = data;
  HT_block_info *b_info;
  SHT_block_info *shtb_info = (SHT_block_info*)(shtRec+(sht_info->maxRecords));
  for (int i=0;i<shtb_info->recordsNum;i++){ // iterate through all records in sht block
    memcpy(&shtRecord,shtRec+i,sizeof(shtRecord));
    blockId=shtRecord.blockid;
    if (strcmp(name,shtRecord.name)==0){
      CALL_BF(BF_GetBlock(fd,blockId,block));
      data = BF_Block_GetData(block);
      blocksRead++;
      rec = data;
      b_info = (HT_block_info*)(rec+(ht_info->maxRecords));
      for (int j=0;j<b_info->recordsNum;j++){ // for each record in sht block check the ht block for matches
        memcpy(&record,rec+j,sizeof(Record));
        if (strcmp(record.name,name)==0){
          printRecord(record);
        }
      }
      while (b_info->prevBlock !=0){ // check overflow blocks
      CALL_BF(BF_UnpinBlock(block));
      CALL_BF(BF_GetBlock(fd,b_info->prevBlock,block));
      blocksRead++;
      data = BF_Block_GetData(block);
      rec = data;
      b_info = (HT_block_info*)(rec+(ht_info->maxRecords));
        for (int j=0;j<b_info->recordsNum;j++){
          memcpy(&record,rec+j,sizeof(Record));
          if (strcmp(record.name,name)==0){
            printRecord(record);
          }
        }
      }
      CALL_BF(BF_UnpinBlock(block));
    }
  }
  while (shtb_info->prevBlock !=0){ // check overflow blocks of the Secondary Hash table
    CALL_BF(BF_UnpinBlock(sht_block));
    CALL_BF(BF_GetBlock(sht_fd,sht_blockId,sht_block));
    blocksRead++;
    data = BF_Block_GetData(sht_block);
    shtRec = data;
    shtb_info = (SHT_block_info*)(shtRec+(sht_info->maxRecords));
    for (int i=0;i<shtb_info->recordsNum;i++){ // iterate through all records in sht block
    memcpy(&shtRecord,shtRec+i,sizeof(shtRecord));
    blockId=shtRecord.blockid;
    if (strcmp(name,shtRecord.name)==0){
      CALL_BF(BF_GetBlock(fd,blockId,block));
      data = BF_Block_GetData(block);
      blocksRead++;
      rec = data;
      b_info = (HT_block_info*)(rec+(ht_info->maxRecords));
      for (int j=0;j<b_info->recordsNum;j++){ // for each record in sht block check the ht block for matches
        memcpy(&record,rec+j,sizeof(Record));
        if (strcmp(record.name,name)==0){
          printRecord(record);
        }
      }
      while (b_info->prevBlock !=0){ // check overflow blocks
      CALL_BF(BF_UnpinBlock(block));
      CALL_BF(BF_GetBlock(fd,b_info->prevBlock,block));
      blocksRead++;
      data = BF_Block_GetData(block);
      rec = data;
      b_info = (HT_block_info*)(rec+(ht_info->maxRecords));
        for (int j=0;j<b_info->recordsNum;j++){
          memcpy(&record,rec+j,sizeof(Record));
          if (strcmp(record.name,name)==0){
            printRecord(record);
          }
        }
      }
      CALL_BF(BF_UnpinBlock(block));
    }
    }  
  }
  CALL_BF(BF_UnpinBlock(sht_block));
}



