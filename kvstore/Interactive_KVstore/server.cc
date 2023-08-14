#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <bits/stdc++.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <grpc/support/log.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include"cache.cpp"
#include "keyvalue.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

using keyvaluestore::Functions;
using keyvaluestore::Reply;
using keyvaluestore::keyvaluepair;
using keyvaluestore::keyonly;
using namespace std;

#define LRU 1
#define LFU 2
#define DEFAULT_CS 4
#define DEFAULT_POLICY 1

// unordered_map<string,string> cache;
Cache c(DEFAULT_CS,DEFAULT_POLICY);
string LISTENING_PORT;
int CACHE_REPLACEMENT_TYPE;
int CACHE_SIZE;
int THREAD_POOL_SIZE;
int *thread_id;
int no_of_threads;
pthread_t *thread_workers;
vector<unique_ptr<ServerCompletionQueue>> cq_;
Functions::AsyncService service_;
unique_ptr<Server> server_;
int cur_thr;



void* HandleRpcs(void *);



void config_server() {
  
  fstream file;
  string line;
  int line_no=0;
  const char* param;
  const char *tokenize;
  string fname="../../server_config.txt";
  file.open(fname);
  if (file.is_open()){
    while(getline(file, line)){
      line_no++;
      tokenize=line.c_str();
      for(int i=0;i<strlen(tokenize);i++){
        if(tokenize[i] == '=')
          param=&tokenize[i+1];
      }
      if(line_no==1){
          stringstream s(param);
          s >> LISTENING_PORT;
      }
      if(line_no==2){
        stringstream s(param);
        s >> CACHE_REPLACEMENT_TYPE;
        // CACHE_REPLACEMENT_TYPE = param;
      }  
      if(line_no==3){
        stringstream s(param);
        s >> CACHE_SIZE;
      }
      if(line_no==4){
        stringstream s(param);
        s >> THREAD_POOL_SIZE;
      }
    }
  }
  else{
    cout<<"Error while opening config file of server";
  }
  file.close();
}

class ServerImpl final {
 public:
  ~ServerImpl() {
    // server_->Shutdown();
    // // Always shutdown the completion queue after the server.
    // cq_->Shutdown();
    for (int i = 0; i < no_of_threads; i++)
    {
      pthread_join(thread_workers[i], NULL);
      printf("Thread %d Joined\n", i);
    }
    server_->Shutdown();
    for (int i = 0; i < no_of_threads; i++){
      cq_[i]->Shutdown();
      printf("CQ of Thread %d Shutdown\n", i);
    }
  }

  // There is no shutdown handling in this code.
  void Run() {
    config_server();
    c = Cache(CACHE_SIZE,CACHE_REPLACEMENT_TYPE);
    no_of_threads=THREAD_POOL_SIZE;
    string server_address="0.0.0.0:"+LISTENING_PORT;
    // cout << "PORT:"<<LISTENING_PORT;
  
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    // cq_ = builder.AddCompletionQueue();
    
    thread_id = (int *)malloc(sizeof(int) * no_of_threads);
    thread_workers = (pthread_t *)malloc(sizeof(pthread_t) * no_of_threads);

    for (int i = 0; i < no_of_threads; i++)
      {
        thread_id[i] = i;
        cq_.push_back(builder.AddCompletionQueue());
      }
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    // HandleRpcs();
      for (int i = 0; i < no_of_threads; i++)
        pthread_create(&thread_workers[i], NULL, HandleRpcs, (void *)&thread_id[i]);
  }
};

  
  // Class encompasing the state and logic needed to serve a request.
  class KVServer {
   public:
      // Invoke the serving logic right away.
     virtual void Proceed() = 0;
    };

  class GetKeyValue final : public KVServer{
    public:
    explicit GetKeyValue(Functions::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // Invoke the serving logic right away.
      Proceed();
      }

    void Proceed() {
      if (status_ == CREATE) {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

        // As part of the initial CREATE state, we *request* that the system
        // start processing GET requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different KVServer
        // instances can serve different requests concurrently), in this case
        // the memory address of this KVServer instance.
        service_->RequestGET(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);

      } else if (status_ == PROCESS) {
        // cout<<"Server Processing Get Request";
        // Spawn a new KVServer instance to serve new clients while we process
        // the one for this KVServer. The instance will deallocate itself as
        // part of its FINISH state.
        new GetKeyValue(service_, cq_);
        cout<<"Processing Get Request\n";
        // cout<<"\n thread no. :"<<cur_thr;
        sleep(1);
        // The actual processing.
        string response = "";
        string key = request_.key();
        string value = "";
        reply_.set_key(key);
        string temp=c.get(key);
        if(key.size() > 256){
          response = "Status 400 -- Key-Value pair size exceeded!\n";
          reply_.set_message(response);
          reply_.set_code(400);
          // return Status::OK;
        }
        else if(temp == ""){
          response = "Status 400 -- KEY NOT EXIST!\n";
          reply_.set_message(response);
          reply_.set_code(400);
        }
        else{
          string val=temp;
          response = "Status 200 -- Retrieved value is :"+val;
          reply_.set_value(val);
          reply_.set_message(response);
          reply_.set_code(200);  
        }
        reply_.set_message(response);
        c.print();

        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (KeyValue).
        delete this;
      }
    }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    Functions::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    keyonly request_;
    keyvaluepair reply_;
    ServerAsyncResponseWriter<keyvaluepair> responder_;
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  class PutKeyValue final : public KVServer{
    public:
    explicit PutKeyValue(Functions::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // Invoke the serving logic right away.
      Proceed();
      }

    void Proceed() {
      if (status_ == CREATE) {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;
        service_->RequestPUT(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
        

      } else if (status_ == PROCESS) {
        // Spawn a new KVServer instance to serve new clients while we process
        // the one for this KVServer. The instance will deallocate itself as
        // part of its FINISH state.
        new PutKeyValue(service_, cq_);
        cout<<"\nProcessing PUT request\n";
        sleep(1);

        string response = "";
        string key = request_.key();
        string value = request_.value();
        reply_.set_message(response);

        if(key.size() > 256 || value.size() > 256)
         {
            response = "Status 400 -- Key-Value pair SIZE EXCEEDED!\n";
            reply_.set_message(response);
            reply_.set_code(400);
          }
    
        else if(c.insert(key,value) == 400){
          c.update(key,value);
          response = "Status 200 -- Key-Value pair is OVERWRITTEN!\n";
          c.print();
          reply_.set_message(response);
          reply_.set_code(200);
        }
    
        else{
              
              response = "Status 200 -- Key-Value pair INSERTED SUCCESSFULLY!\n";
              c.print();
              reply_.set_message(response);
              reply_.set_code(200);
              // return Status::OK;
          }
        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
        
      } else {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (KeyValue).
        delete this;
      }
    }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    Functions::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    keyvaluepair request_;
    Reply reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<Reply> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  class DelKeyValue final : public KVServer{
    public:
    explicit DelKeyValue(Functions::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      // Invoke the serving logic right away.
      Proceed();
      }

    void Proceed() {
      if (status_ == CREATE) {
        // Make this instance progress to the PROCESS state.
        status_ = PROCESS;

        // As part of the initial CREATE state, we *request* that the system
        // start processing SayHello requests. In this request, "this" acts are
        // the tag uniquely identifying the request (so that different KeyValue
        // instances can serve different requests concurrently), in this case
        // the memory address of this KeyValue instance.
        service_->RequestDEL(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);

      } else if (status_ == PROCESS) {
        // Spawn a new KeyValue instance to serve new clients while we process
        // the one for this KeyValue. The instance will deallocate itself as
        // part of its FINISH state.
        new DelKeyValue(service_, cq_);
        cout<<"\nProcessing DEL request\n";
        sleep(1);

        // The actual processing.
        string response = "";
        string key = request_.key();

            if(key.size() > 256){
              response = "Status 400 -- Key-Value pair SIZE EXCEEDED!\n";
              reply_.set_message(response);
              reply_.set_code(400);
              // return Status::OK;
            }
    
            else if(c.get(key) == ""){
                response = "Status 400 --KEY NOT EXIST\n";
                reply_.set_message(response);
                reply_.set_code(400);
                // return Status::OK;
          }
      
        else{
            c.delete_key(key);
            response = "Status 200 -- Key-Value pair DELETED SUCCESSFULLY!";
            c.print();
            reply_.set_message(response);
            reply_.set_code(200);
            // return Status::OK;
        }
        // And we are done! Let the gRPC runtime know we've finished, using the
        // memory address of this instance as the uniquely identifying tag for
        // the event.
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        GPR_ASSERT(status_ == FINISH);
        // Once in the FINISH state, deallocate ourselves (KeyValue).
        delete this;
      }
    }

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    Functions::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    ServerContext ctx_;

    // What we get from the client.
    
    // What we send back to the client.
    keyonly request_;
    Reply reply_;

    // The means to get back to the client.
    ServerAsyncResponseWriter<Reply> responder_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  // This can be run in multiple threads if needed.
  void* HandleRpcs(void* tid) {
    // Spawn a new KeyValue instance to serve new clients.
    int th_id = *(int *)tid;
    cout<<"Thread "<<th_id<<" is running\n";
    
    new GetKeyValue(&service_, cq_[th_id].get());
    new PutKeyValue(&service_, cq_[th_id].get());
    new DelKeyValue(&service_, cq_[th_id].get());
    
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a KeyValue instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down. 
      GPR_ASSERT(cq_[th_id]->Next(&tag, &ok));
      // cout<<"\nThread "<<th_id<<" handling request "<<endl;
      GPR_ASSERT(ok);
      static_cast<KVServer*>(tag)->Proceed();
    }
  }
  


int main(int argc, char** argv) {
  ServerImpl server;

  server.Run();
  // server.~ServerImpl();
  return 0;
}