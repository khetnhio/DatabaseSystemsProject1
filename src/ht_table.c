#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define HT_ERROR -1

#define CALL_BF(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return HT_ERROR;             \
    }                         \
  }

int hashFunction (int key, int buckets){
  return key % buckets;
}
int HT_CreateFile(char *fileName,  int buckets){
  if (buckets>59) {
    printf("Maximum number of buckets exceeded for this implementation (59)\n");
    return HT_ERROR;
  }

  int fd;
  BF_Block *block0;
  BF_Block_Init(&block0);
  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fd));
  void *data;
  CALL_BF(BF_AllocateBlock(fd,block0));
  data = BF_Block_GetData(block0);
  HT_info *info=data;
  memcpy(info->filetype, "hash table", strlen("hash table")+1);
  info->fileDesc=fd;
  info->key=ID;
  info->bucketsNum=buckets;
  info->maxRecords=6;
  for (int i=0;i<buckets;i++){
    info->hashTable[i][0]=i;
  }
  
  BF_Block_SetDirty(block0);
  CALL_BF(BF_UnpinBlock(block0));
  CALL_BF(BF_CloseFile(fd));
  //CALL_BF(BF_Close());
  return 0;
}

HT_info* HT_OpenFile(char *fileName){
    return NULL;
}


int HT_CloseFile( HT_info* HT_info ){
    return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
    return 0;
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
    return 0;
}




