#include<iostream>
#include<semaphore.h>
#include"persistentstorage.h"



#define LRU 1
#define LFU 2

using namespace std;



struct cache_field{
    char key[256];
    char value[256];
    long timestamp;
    int frequency;
    bool modified;
    bool valid;
    struct cache_field* next;
    struct cache_field* prev;
};

string toString(char* arr){
    int i=0;
    string out = "";
    while(arr[i] && arr[i]!=' '){
        out+=arr[i++];
    }
    return out;
}

class Cache{
    private:
        int size;
        struct cache_field* head;
        int count;
        int timestamp;
        int replacement_policy;
        sem_t  mutex;

        void assign(char* arr,string str){
            int i;
            for(i=0;str[i];i++){
                arr[i] = str[i];
            }
            while(i<256){
                arr[i++] = 0;
            }
        }

        void insertIntoCache(string key, string value){
            struct cache_field* new_field;
            sem_wait(&mutex);
            if(count<size){
                new_field = (cache_field*)malloc(sizeof(cache_field));
                if(head==NULL){
                    head = new_field;
                    head->next = new_field;
                    head->prev = new_field;
                }else{
                    new_field->next = head;
                    new_field->prev = head->prev;
                    head->prev->next = new_field;
                    head->prev = new_field;
                    head = new_field;
                }
                count++;
                
            }else{
                new_field = head->prev;
            }
            
            
            if(new_field->modified){
                put_in_pers_store(&new_field->key[0],&new_field->value[0]);
            }
            //cout<<"Test2:"<<key<<" "<<value<<endl;
            assign(new_field->key,key);
            //cout<<"Test3:"<<key<<" "<<value<<endl;
            assign(new_field->value,value);
            new_field->frequency = 0;
            new_field->timestamp = timestamp++;
            new_field->valid = true;
            new_field->modified = false;
            
            head = new_field;
            sem_post(&mutex);
        }

        

    
    public:
        
        Cache(int cache_size, int r_policy){
            head = NULL;
            replacement_policy = r_policy;
            timestamp = 0;
            size = cache_size;
            sem_init(&mutex,0,1);
            count = 0;
            create_pers_store();
        }
        
        string get(string key){
            struct cache_field* current = head;
            sem_wait(&mutex);
            for(int i=0;i<count;i++){
                if(current->key == key){
                    //Removing from old position
                    
                    //Add to the start of list
                    if(current!=head){
                        current->prev->next = current->next;
                        current->next->prev = current->prev;
                        current->next = head;
                        current->prev = head->prev;
                        head->prev->next = current;
                        head->prev = current;
                        head = current;
                    }

                    current->timestamp = timestamp++;
                    current->frequency++;
                    
                    if(replacement_policy== LFU && current->frequency<current->next->frequency){
                        head = current->next;
                        head->prev = current->prev;
                        current->prev->next = head;

                        struct cache_field *tnode = head;

                        while(tnode->frequency>current->frequency && tnode->next!=head){
                            tnode = tnode->next;
                        }

                        current->prev = tnode->prev;
                        current->next = tnode;
                        current->prev->next = current;
                        tnode->prev = current;
                        
                    }
                    sem_post(&mutex);
                    return current->value;
                }
                current = current->next;
            }
            sem_post(&mutex);
            string value = get_from_pers_store(&key[0]);
            // cout<<toString(&value[0])<<" Not found in cache"<<endl;
            if(value!=""){
                insertIntoCache(key,value);
            }

            return value;
        }

        int insert(string key, string value){
            
            if(get(key)!=""){
                update(key,value);
                return 400;
            }
            put_in_pers_store(&key[0],&value[0]);
            
            insertIntoCache(key,value);
            return 200;
        }

        void update(string key, string value){
            
            sem_wait(&mutex);
            struct cache_field* current = head;
            for(int i=0;i<count;i++){
                if(current->key == key) break;
                current = current->next;
            }
            assign(current->value,value);
            current->modified = true;
            sem_post(&mutex);
        }

        void delete_key(string key){
            sem_wait(&mutex);
            struct  cache_field* current = head;
            for(int i=0;i<count;i++){
                if(current->key==key){
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                    free(current);
                }
            }
            sem_post(&mutex);
            del_from_pers_store(&key[0]);
        }

        void print(){
            struct cache_field* current=head;
            cout<<"key"<<"\t"<<"value"<<"\t"<<"TStamp"<<"\t"<<"Frequency"<<endl<<flush;
            for(int i=0;i<count;i++){
                cout<<current->key<<"\t"<<toString(current->value)<<"\t"<<current->timestamp<<"\t"<<current->frequency<<endl;
                current = current->next;
            }
            cout<<endl;
        }
        

};



/**/