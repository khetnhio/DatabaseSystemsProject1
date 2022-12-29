#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define HT_ERROR -1

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}
// δεχεται το id και τον αριθμό των buckets
// και επιστρέφει αριθμό απο 0 εώς buckets-1
int hashFunction (int key, int buckets){  
  return key % buckets;                   
}
int HT_CreateFile(char *fileName,  int buckets){
  if (buckets>50) {
    printf("Maximum number of buckets exceeded for this implementation (50)\n");
    return HT_ERROR;
  }
  int fd;
  BF_Block *block0;
  BF_Block_Init(&block0);
  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fd));
  CALL_BF(BF_AllocateBlock(fd,block0));
  void *data = BF_Block_GetData(block0);
  HT_info *info=data;
  memcpy(info->filetype, "hash table", strlen("hash table")+1);
  info->fileDesc=fd;
  info->key=ID;
  info->bucketsNum=buckets;
  info->maxRecords=BF_BLOCK_SIZE/sizeof(Record);
  for (int i=0;i<buckets;i++){//αρχικοποίηση του hash table
    info->hashTable[i]=0; // οταν η τιμή είναι μηδέν, δεν υπάρχει κανένα block στο bucket
  }
  BF_Block_SetDirty(block0);
  CALL_BF(BF_UnpinBlock(block0));
  CALL_BF(BF_CloseFile(fd));
  return 0;
}

HT_info* HT_OpenFile(char *fileName){
  HT_info *info;
  int fd;
  BF_Block *block;
  BF_Block_Init(&block);
  BF_ErrorCode err;
  err =BF_OpenFile(fileName,&fd);
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL ;
  }
  err=BF_GetBlock(fd,0,block); //παίρνω το block 0 που περιέχει την δομή HT_info απο το αρχείο
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL ;
  }  
  void *data = BF_Block_GetData(block);
  info = data;
  if (strcmp(info->filetype,"hash table") != 0){
    fprintf(stderr,"File is not hash table file\n");
    return NULL ;    
  }
  return info;  
}


int HT_CloseFile( HT_info* HT_info ){
    int fd=HT_info->fileDesc;
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(fd,0,block));
    CALL_BF(BF_UnpinBlock(block));// to block 0 δεν γίνεται unpin μέχρι να κλείσει το αρχείο
    CALL_BF(BF_CloseFile(fd));
    return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record* record){
    int fd = ht_info->fileDesc; // αναγνωριστικό αρχείου
    int bkt = hashFunction(record->id,ht_info->bucketsNum); // το bucket που θα καταχωρηθέι το record
    int blockId = ht_info->hashTable[bkt]; // id του τελευταίου block στo bucket
    void *data;
    BF_Block *block;
    BF_Block_Init(&block); // το block που θα καταχωρηθουν τα δεδομένα
    HT_block_info *b_info;
    if (blockId == 0){ // περίπτωση που ο κάδος είναι άδειος
      CALL_BF(BF_AllocateBlock(fd,block));
      CALL_BF(BF_GetBlockCounter(fd, &blockId));
      blockId--;// το id είναι ο αριθμός των blocks-1
      ht_info->hashTable[bkt]=blockId;
      data = BF_Block_GetData(block);
      Record *rec = data;
      memcpy(rec,record,sizeof(Record));
      b_info = (HT_block_info*)(rec+(ht_info->maxRecords)); // δομή στο τέλος του block
      b_info->recordsNum=1;
      b_info->prevBlock = 0; // 0 = δεν υπάρχει προηγούμενο block στο bucket
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      return blockId;
    }
    CALL_BF(BF_GetBlock(fd,blockId,block));// παίρνω το τελευταίο block του bucket
    data = BF_Block_GetData(block);
    Record *rec = data;
    b_info =(HT_block_info*)(rec+(ht_info->maxRecords));
    if (b_info->recordsNum == ht_info->maxRecords){ // περίπτωση που το block είναι γεμάτο - Υπερχείλιση
      int newBlockId;
      CALL_BF(BF_UnpinBlock(block)); // αποδεσμέυω το παλιό block πριν φωρτώσω το καινούργιο
      CALL_BF(BF_AllocateBlock(fd,block));
      CALL_BF(BF_GetBlockCounter(fd, &newBlockId));
      newBlockId--;// το id είναι ο αριθμός των blocks-1
      ht_info->hashTable[bkt]=newBlockId; // ενημέρωση του hash table
      data = BF_Block_GetData(block);
      rec = data;
      memcpy(rec,record,sizeof(Record));
      b_info=(HT_block_info*)(rec+(ht_info->maxRecords));
      b_info->recordsNum=1;
      b_info->prevBlock = blockId;
      CALL_BF(BF_UnpinBlock(block));
      return newBlockId;
    }
    else if(b_info->recordsNum < ht_info->maxRecords){// περίπτωση που η νέα καταχώρηση χωράει στο block
      int recNum=b_info->recordsNum;
      //printf("Insterting: (%d,%s,%s,%s)\n",record.id,record.name,record.surname,record.city);
      memcpy(rec+recNum,record,sizeof(Record)); 
      b_info->recordsNum++;    
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      return blockId;      
    }
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
  int *val = value;
  int bkt = hashFunction(*val,ht_info->bucketsNum);
  int fd = ht_info->fileDesc;
  int blocksRead=0; // ο αρθμός των block που θα διαβαστεί
  BF_Block *block;
  BF_Block_Init(&block);
  int blockId=ht_info->hashTable[bkt];
  CALL_BF(BF_GetBlock(fd,blockId,block));// φορτώνεται το τελευταίο block του bucket
  blocksRead++;
  void *data = BF_Block_GetData(block);
  Record *rec = data;
  Record record;
  HT_block_info * b_info=(HT_block_info*)(rec+(ht_info->maxRecords));
  
  for(int i=0;i<b_info->recordsNum;i++){
    memcpy(&record,rec+i,sizeof(Record));
    if (record.id == *val){
    printRecord(record);
      }
  }

  while (b_info->prevBlock !=0){ // ελέγχονται και τα block υπερχείλισης για το id
    CALL_BF(BF_UnpinBlock(block));
    CALL_BF(BF_GetBlock(fd,b_info->prevBlock,block));
    blocksRead++;
    data = BF_Block_GetData(block);
    rec = data;
    b_info = (HT_block_info*)(rec+ht_info->maxRecords);
    for(int i=0;i<b_info->recordsNum;i++){
      memcpy(&record,rec+i,sizeof(Record));
      if (record.id == *val){
        printRecord(record);
      }
    }   
  }
  CALL_BF(BF_UnpinBlock(block));
  return blocksRead;
}




