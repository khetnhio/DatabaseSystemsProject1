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
#define BUCKETS 12
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int secondHashFunction (char *name,int buckets){
  char *namePtr = name;
  char letter;
  int sum = 0;
  memcpy(&letter,namePtr,sizeof(char));
  while (letter != '\0') {
    printf("letter =%d\n",letter);
    sum += (letter)*(letter)*0.938181;
    namePtr++;
    memcpy(&letter,namePtr,sizeof(char));
  }
  printf("The sum is: %d\n",sum);
  return sum % buckets;
}

int main() {
    srand(12569874);
    BF_Init(LRU);
    // Αρχικοποιήσεις
    HT_CreateFile(FILE_NAME,BUCKETS);
    SHT_CreateSecondaryIndex(INDEX_NAME,BUCKETS,FILE_NAME);
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
        SHT_SecondaryInsertEntry(index_info, &record, block_id);
    }
    // Τυπώνουμε όλες τις εγγραφές με όνομα searchName
    printf("RUN PrintAllEntries for name %s\n",searchName);
    SHT_SecondaryGetAllEntries(info,index_info,searchName);

    // Κλείνουμε το αρχείο κατακερματισμού και το δευτερεύον ευρετήριο
    SHT_CloseSecondaryIndex(index_info);
    HT_CloseFile(info);
    //
    BF_Close();

//   // hash function testing  
//   char names[][15] ={  "Yannis",
//   "Christofos",
//   "Sofia",
//   "Marianna",
//   "Vagelis",
//   "Maria",
//   "Iosif",
//   "Dionisis",
//   "Konstantina",
//   "Theofilos",
//   "Giorgos",
//   "Dimitris"};
//   int bktcap[BUCKETS];
//   for (int i=0;i<BUCKETS;i++){
//     bktcap[i]=0;
//   }
//   char name[15];
//   int bkt;
//   for (int i=0;i<BUCKETS;i++){
//     //name = &names[0][i];
//     memcpy(name,&names[i][0],15);
//     printf("checking name: %s\n",name);
//     bkt = secondHashFunction(name,BUCKETS);
//     bktcap[bkt]++;
//   }
//   for(int i=0;i<BUCKETS;i++){
//     printf("The bucket No: %d, has %d names.\n",i,bktcap[i]);
//   }
//   int eval=0;
//   for (int i=0;i<BUCKETS;i++){
//     if (bktcap[i]==0){
//       eval--;
//     }
//   }
//   printf("Hash function eval:%d\n",eval);
}