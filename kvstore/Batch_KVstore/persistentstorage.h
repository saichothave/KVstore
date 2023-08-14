#include "headerfiles.h"
using namespace std;
#define MAXFILESREQ 64
// locks for multithreading
pthread_rwlock_t persistentstoragelocks[64];


// the first method which server will call is to create all stores for wwriting
char* create_store_number(int i) {
  string storeval = to_string(i);
  char* ptr = (char*)(malloc(10 * sizeof(int)));
  for (int j = 0; j < storeval.length(); j++) {
    ptr[j] = storeval[j];
  }
  ptr[storeval.length()] = '\0';
  return ptr;
}

void create_pers_store() {
  for (int i = 0; i < MAXFILESREQ; i++) {
    // create data stores

    char storenumber[10];  // this will store number 1 ,2,3,etc
    strcpy(storenumber, create_store_number(i));
    strcat(storenumber, ".txt");  // append store number with ".txt extension"
    // https://www.cplusplus.com/reference/cstdio/fopen/
    FILE* pers_store = fopen(storenumber, "w+");
    if (pers_store == NULL) printf("Out of storage");
    fclose(pers_store);
    // initialise locks for all stores
    persistentstoragelocks[i] = PTHREAD_RWLOCK_INITIALIZER;
  }
}

// we will define 3 utilities for cache to use
// 1) put_in_pers_store(char* key , char* value ) (to add key value in store)
// 2)del_from_pers_store(char* key)(to del from store )
// 3)get_from_pers_store(char* key)(to return value of corresponding key)

// ----------------------------------Put in persistent storage--------------
// http://www.cse.yorku.ca/~oz/hash.html
unsigned long getStoreNumber(char* str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++) hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash % 64;
}




int put_in_pers_store(char* key, char* value) {
  int storeno = getStoreNumber(key);
  char storenumber[10];  // this will store number 1 ,2,3,etc
  strcpy(storenumber, create_store_number(storeno));
  strcat(storenumber, ".txt");  // append store number with ".txt extension"
  // https://www.cplusplus.com/reference/cstdio/fopen/

  // lock that file number
  // cout<<"Test1:"<<key<<" "<<value<<endl;
  // cout << storeno;
  pthread_rwlock_wrlock(&persistentstoragelocks[storeno]);
  FILE* pers_store = fopen(storenumber, "r+");

  if (pers_store == NULL) {
    perror("file doesn't exist");
    pthread_rwlock_unlock(&persistentstoragelocks[storeno]);
  }

  char keyaluepair[517];
  for (int i = 0; i < 517; i++) keyaluepair[i] = ' ';
  keyaluepair[515] = '\n';
  keyaluepair[516] = '\0';
  keyaluepair[0] = 'v';//to denote valid entry
  strncpy(&keyaluepair[1], key, strlen(key));
  strncpy(&keyaluepair[257], value, strlen(value));

  //   cout << keyaluepair;

  bool isFreeEntryAvailable = false;
  int seek = 0;
  
  // check if entry is present then rewrite value
  // if there is any dirty entry if yes add this new key value pair there
  // https://www.cplusplus.com/reference/cstdio/fgets/
  char data[517];
  while (fgets(data, 517, pers_store) != NULL) {
    // cout << "key = " << keyaluepair << endl;
    // cout << "data = " << data << endl;
    // cout << "result = " << strncmp(&data[1], &keyaluepair[1], 256) << endl; 
    if (strncmp(&data[1], &keyaluepair[1], 256) == 0 && data[0] == 'v') {
      // cout << "duuplicate key " << endl;
      fseek(pers_store, -259, SEEK_CUR);
      fputs(&keyaluepair[257], pers_store);
      pthread_rwlock_unlock(&persistentstoragelocks[storeno]);
      fclose(pers_store);
      return 1;
    }
    // if data is dirty
    if (data[0] == 'd') {
      isFreeEntryAvailable = true;
    }
    // if free entry is not found then keep on moving seek ptr
    if (!isFreeEntryAvailable) seek += 516;
  }
  // https://www.cplusplus.com/reference/cstdio/fseek/
  if (isFreeEntryAvailable) {
    fseek(pers_store, seek, SEEK_SET);
  }
  

  fputs(keyaluepair, pers_store);
  

  // fputs(,pers_store);
  pthread_rwlock_unlock(&persistentstoragelocks[storeno]);
  
  fclose(pers_store);
  return 1;
}

// -----------------------------------Del from persistent storage------------
int del_from_pers_store(char* key) {
  int storeno = getStoreNumber(key);
  char storenumber[10];  // this will store number 1 ,2,3,etc
  strcpy(storenumber, create_store_number(storeno));
  strcat(storenumber, ".txt");  // append store number with ".txt extension"
  // https://www.cplusplus.com/reference/cstdio/fopen/
  // cout << "store no = \n" << storeno;
  pthread_rwlock_wrlock(&persistentstoragelocks[storeno]);
  FILE* pers_store = fopen(storenumber, "r+");

  if (pers_store == NULL) {
    perror("file doesn't exist");
    pthread_rwlock_unlock(&persistentstoragelocks[storeno]);
    return 0;
  }

  char keyonly[257];
  for(int i = 0;i < 257 ;i++)
    keyonly[i] = ' ';
    keyonly[256] = '\0';
    // cout << "len before " << strlen(keyonly) << endl;
    strncpy(keyonly,key , strlen(key));
   
    char data[517];

    while(fgets(data , 517,pers_store)!= NULL){
        if(strncmp(&data[1],keyonly , 256) == 0){
            if(data[0] == 'v'){
                // cout << "found key " << endl;
                fseek(pers_store , -516,SEEK_CUR);
                fputs("d" , pers_store);
                fclose(pers_store);
                pthread_rwlock_unlock(&persistentstoragelocks[storeno]);
                return 1;
            }
        }
    }
    
    pthread_rwlock_unlock(&persistentstoragelocks[storeno]);

    fclose(pers_store);
   
    return 0;


}

// ------------------------------------get from persistent storage------------
string get_from_pers_store(char* key) {
  
  int storeno = getStoreNumber(key);
  char storenumber[10];  // this will store number 1 ,2,3,etc
  strcpy(storenumber, create_store_number(storeno));
  strcat(storenumber, ".txt");  // append store number with ".txt extension"
  // https://www.cplusplus.com/reference/cstdio/fopen/
  // cout << "store no = \n" << storeno;
  
  FILE* pers_store = fopen(storenumber, "r+");

  if (pers_store == NULL) {
    perror("file doesn't exist");
    return "";
  }


  // now we need to check if the key exists in a given file
  char keyonly[257];
  for (int i = 0; i < 257; i++) keyonly[i] = ' ';
  keyonly[256] = '\0';
  // cout << "len before " << strlen(keyonly) << endl;
  
  strncpy(keyonly, key, strlen(key));
  
  char data[517];
  
  while (fgets(data, 517, pers_store) != NULL) {
    if (strncmp(&data[1], keyonly, 256) == 0) {
      if (data[0] == 'v') {
        char* valueToReturn = (char*)malloc(257);
        strncpy(valueToReturn, &data[257], 257);
        fclose(pers_store);
        string out;
        
        return out.append(valueToReturn);
      }
    }
  }

  fclose(pers_store);
  return "";
}

// testing purposes------------------------

// int main() {
//   create_pers_store();

//   for (int i = 0; i < 10; i++) {
//     cout << "1 for put \n 2 for get \n 3 del \n" << endl;

//     while (1) {
//       int choice;
//       cout << "enter choice \n";
//       cin >> choice;
//       char key[256], value[256];
//       switch (choice) {
//         case 1:
//           cout << "enter key \n";
//           cin >> key;
//           cout << "enter value \n";

//           cin >> value;
//           put_in_pers_store(key, value);
//           break;
//         case 2:
//           cout << "Enter key whose value to search \n";
//           cin >> key;
//           cout << get_from_pers_store(key);
//           break;
//         case 3:
//             cout << "enter key to delete ";
//             cin >> key;
//             cout << del_from_pers_store(key);
//             break;
//       }
//     }
//   }
//   return 0;
// }
