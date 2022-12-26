#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 200 // you can change it if you want
#define FILE_NAME "data.db"

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
  
  HT_CreateFile(FILE_NAME,10);
  HT_info* info = HT_OpenFile(FILE_NAME);
  
  
  // HT_info *info;
  // for (int i=0;i<59;i++){
  //   info->hashTable[i][0]=i;
  // }
  //   for (int i=0;i<59;i++){
  //   info->hashTable[i][1]=-1;
  // }
  // for (int i=0;i<59;i++){
  //   printf("[%d, %d]\n",info->hashTable[i][0],info->hashTable[i][1]);
  // }

  Record record;
  srand(12569874);
  int r;
  printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    HT_InsertEntry(info, record);
  }

  printf("RUN PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  HT_GetAllEntries(info, &id);

  HT_CloseFile(info);
  BF_Close();
}
