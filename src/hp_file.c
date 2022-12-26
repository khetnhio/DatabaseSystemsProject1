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
  if (code != BF_OK) {         \
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
  void *data = BF_Block_GetData(block);
  info = data;
  if (strcmp(info->filetype,"heap") != 0){
    fprintf(stderr,"File is not heap file\n");
    return NULL ;    
  }
  /*err=BF_UnpinBlock(block);
  if (err!=BF_OK){
    BF_PrintError(err);
    return NULL ;
  }*/
  return info;
}


int HP_CloseFile( HP_info* hp_info ){
    int fd=hp_info->fileDesc;
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_BF(BF_GetBlock(fd,0,block));
    CALL_BF(BF_UnpinBlock(block));// to block 0 δεν γίνεται unpin μέχρι να κλείσει το αρχείο
    CALL_BF(BF_CloseFile(fd));
    //BF_Block_Destroy(&block);
    return 0;
}

// void copy_record(Record *new_rec, Record *old_rec) { // Αντιγ´ρα
//   memcpy(new_rec->record, old_rec->record, 15);
//   new_rec->id = old_rec->id;
//   memcpy(new_rec->name, old_rec->name, 15);
//   memcpy(new_rec->surname, old_rec->surname, 20);
//   memcpy(new_rec->city, old_rec->city, 20);
// }

int HP_InsertEntry(HP_info* hp_info, Record record){
    int fd = hp_info->fileDesc; // αναγνωριστικό αρχείου
    int blockId = hp_info->lastBlock; // id του τελευταίου block στην μνήμη
    void *data;
    BF_Block *block;
    BF_Block_Init(&block); // το block που θα καταχωρηθουν τα δεδομένα
    HP_block_info* b_info;
    if (blockId == 0){ //περίπτωση που το αρχείο δέν έχει ακόμα blocks με δεδομένα
      hp_info->lastBlock++;
      blockId++;
      CALL_BF(BF_AllocateBlock(fd,block));
      data = BF_Block_GetData(block);
      Record *rec = data;
      memcpy(rec,&record,sizeof(Record));
      b_info = (HP_block_info*)rec+6;
      b_info->recordsNo=1;
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      return blockId;
    }
    CALL_BF(BF_GetBlock(fd,blockId,block));// φορτώνω το τελευταίο block στην μνήμη
    data = BF_Block_GetData(block);
    Record *rec = data;
    b_info =(HP_block_info*)rec+6;    
    if ( (b_info->recordsNo) == (hp_info->maxRecords) ){ // περίπτωση που το τελευταίο block του σωρού είναι γεμάτο
      hp_info->lastBlock++;
      blockId++;      
      CALL_BF(BF_UnpinBlock(block));// ξεκαρφιτσώνω το block πριν φορτώσω το καινούργιο       
      CALL_BF(BF_AllocateBlock(fd,block));//Δημιουργία καινούριου block
      data = BF_Block_GetData(block);
      Record *rec = data;
      memcpy(rec,&record,sizeof(Record));   
      b_info = (HP_block_info*)rec+6;
      b_info->recordsNo=1;  
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      return blockId;
    }      
    else if( (b_info->recordsNo) < (hp_info->maxRecords) ){// περίπτωση που η νέα καταχώριση χωράει στο τελευταίο block
      Record *rec = data;
      int recNom=b_info->recordsNo;
      //printf("Insterting: (%d,%s,%s,%s)\n",record.id,record.name,record.surname,record.city);
      memcpy(rec+recNom,&record,sizeof(Record)); 
      b_info->recordsNo++;    
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
      return blockId;
    }
}

int HP_GetAllEntries(HP_info* hp_info, int value){
  int blocksRead=0;
  int fd=hp_info->fileDesc;
  BF_Block *block;
  HP_block_info *b_info;
  Record *rec;
  Record record;
  void *data;
  BF_Block_Init(&block);
  for (int i=1;i<=hp_info->lastBlock;i++){// φορτώνω ένα ένα τα blocks του αρχείου
    CALL_BF(BF_GetBlock(fd,i,block));
    blocksRead++;
    data = BF_Block_GetData(block);
    rec = data;
    b_info = (HP_block_info*)rec+6;
    for(int j=0;j<b_info->recordsNo;j++){ //ελέγχω κάθε record του block για την τιμή value
      memcpy(&record,rec+j,sizeof(Record));
      if (record.id == value){
        printRecord(record);
      }
    }
    CALL_BF(BF_UnpinBlock(block));
  }
   return blocksRead;
}

