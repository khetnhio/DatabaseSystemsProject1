#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 2000 // you can change it if you want
#define FILE_NAME "datahash.db"
#define BUCKETS_NUM 40
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {

  BF_Init(LRU);
  
  HT_CreateFile(FILE_NAME,BUCKETS_NUM);
  HT_info* info = HT_OpenFile(FILE_NAME);

  Record record;
  srand(12569874);
  int r;
  printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    HT_InsertEntry(info, &record);
  }


  // printf("this is the final hash table:\n");
  // printf("[%d ",info->hashTable[0]);
  // for (int i=1;i<BUCKETS_NUM;i++){
  //   printf(",%d ",info->hashTable[i]);
  // }
  // printf("]\n");

  // printf("RUN PrintAllEntries\n");
  // int id = rand() % RECORDS_NUM;
  // HT_GetAllEntries(info, &id);

  printf("RUN PrintAllEntries\n");
  for (int id=0;id<RECORDS_NUM;id++){
    HT_GetAllEntries(info, &id);
  }



  HT_CloseFile(info);
  BF_Close();
}
