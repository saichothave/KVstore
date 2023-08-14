Members:
1) Subodh Shantaram Latkar (213050047)
2) Saurish Dilip Darodkar (213050046)
3) Sai Sunil Chothave (21Q05R005)


Commands to run the server and client:
    Steps for compilation:
	    mkdir -p cmake/build
	    pushd cmake/build
	    cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..
	    make -j

    To run server:
        ./server

    To run client:
        For Interactive KVstore :
            ./client
        For Batch KVstore provide input file like this if input file is present in Batch_KVstore folder :
            ./client ../../inputdata.txt




Server configurations (server_config.txt):
    - LISTENING_PORT
    - CACHE_REPLACEMENT_TYPE (1 indicates LRU Cache Replacement and 2 indicates LFU Cache Replacement)
    - CACHE_SIZE
    - THREAD_POOL_SIZE

Using the server_config.txt:
   - LISTENING_PORT : Port number which server will listen to
   - THREAD_POOL_SIZE: Number of worker threads the server will start.
   - CACHE_SIZE: The size of the cache the server will have.
   - CACHE_REPLACEMENT_TYPE: User can choose, if he wants to start the server based on LRU/LFU policy.

Key value store design:
    
    Server design:
        - The server uses the async api of grpc to export the async service.
        - The server starts the number of threads mentioned in the server_config.txt file
        - Reads server_config.txt file to set up the server port, cache replacement policy, cache size and number of worker threads.
        - These threads will be assigned incoming connections in a round robin fashion.
        - These threads use Completion queue to handle the requests.
    
    GET call design:
        - For any GET call, the request handler first searches the cache.
            - If found, it returns the value immediately and updates the frequency and Timestamp
            - If not, it will then go to the Persistent storage and search there. Either success(200) or error (400).

    PUT call design:
        - This function will first find if the key exists already. If not then it will insert it into the cache and main memory. 
        - If key exists, it will update the value to a new value in cache.


    DEL call design:
        - This function will delete the key value pair from cache as well as from main memory.

    
    Cache storage design:
        - The code uses a double ended circular queue for the implementation of both LRU and LFU cache.
        - It is maintained such that the front element contains the most recently used element and most frequently used element in LRU and LFU respectively. 
        - Insert into cache operation is done in O(1) time.
        - Search and delete operations take O(N) time.

    Persistent storage design:
	- Persistent storage is implemented using 64 text files.
	- Keys are hashed and on the basis of hash function they are inserted in particular storage.
	- Inserting takes O(n) and deletion also takes O(n) time, where n is the number of keys.


    Client design:
        - The client uses an asynchronous client to call remote methods.
        - The client will able to call GET, PUT and DEL remote methods.

        string GET(key):
            - string - key - The key we want to get.
        Return value: The response message from server to the client.
        
        string PUT(key,value):
            - string - key - The key we want to insert.
            - string value - The value we want to assign to key.
        Return value: The response message from server to the client.
        
        string DEL(key):
            - string - key - The key we want to delete.
        Return value: The response message from server to the client.

    Performance analysis:
	- We shared 2 graphs (inside the "Performance Analysis" folder) :
		1. Throughput vs Load.
		2. Response vs Load.
	- From the graph, we can infer that response time increases with increase in the number of the request on the server i.e. load on the server.
	- The relation looks like a linear relationship. Whereas throughput decreases with an increase in load on the server.
	- The decrease is not significant after some value.
     

References:
        1. https://www.cplusplus.com/reference/cstdio/fopen/
        2. http://www.cse.yorku.ca/~oz/hash.html
        3. https://www.cplusplus.com/reference/cstdio/fgets/
        4. https://www.cplusplus.com/reference/cstdio/fseek/
        5. https://stackoverflow.com/questions/41732884/grpc-multiple-services-in-cpp-async-server
        6. https://stackoverflow.com/questions/4024806/how-to-convert-from-const-char-to-unsigned-int-c/4024899