#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 30 // you can change it if you want
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"
#define BUCKETS_NUM 12
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
    srand(12569874);
    BF_Init(LRU);
    // Αρχικοποιήσεις
    HT_CreateFile(FILE_NAME,BUCKETS_NUM);
    SHT_CreateSecondaryIndex(INDEX_NAME,BUCKETS_NUM,FILE_NAME);
    HT_info* info = HT_OpenFile(FILE_NAME);
    SHT_info* index_info = SHT_OpenSecondaryIndex(INDEX_NAME);

    // Θα ψάξουμε στην συνέχεια το όνομα searchName
    Record record=randomRecord();
    char searchName[15];
    strcpy(searchName, record.name);

    // Κάνουμε εισαγωγή τυχαίων εγγραφών τόσο στο αρχείο κατακερματισμού τις οποίες προσθέτουμε και στο δευτερεύον ευρετήριο
    printf("Insert Entries\n");
    for (int id = 0; id < RECORDS_NUM; ++id) {
        record = randomRecord();
        int block_id = HT_InsertEntry(info, &record);
        //printf("insert entries worked on HT\n");
        SHT_SecondaryInsertEntry(index_info, &record, id);
        printf("%d, ",id);
    }
    // // Τυπώνουμε όλες τις εγγραφές με όνομα searchName
    // printf("RUN PrintAllEntries for name %s\n",searchName);
    // SHT_SecondaryGetAllEntries(info,index_info,searchName);

    // Κλείνουμε το αρχείο κατακερματισμού και το δευτερεύον ευρετήριο
    SHT_CloseSecondaryIndex(index_info);
    HT_CloseFile(info);
    
    BF_Close();

}
