# ProxyServerHTTP1.1

## About
A simple SOCKSv5 http/https proxy implemented in C. Also listens on an additional port using a custom protocol to get metrics about the state of the proxy and to configure some options.

## Project content
The project contains the source files for the [proxy](bin) and a [client_example](client_example) for the communication with the proxy server using the custom protocol.

## Compiling the proxy server
Make sure you have gcc installed.  
Move to binaries folder:  
`cd bin`  
Compile all the sources:  
`gcc helpers.c main.c proxyClientActiveSocket.c proxyOriginActiveSocket.c proxyPassiveSocket.c proxyTransformation.c selector.c buffer.c configPassiveSocket.c configActiveSocket.c protocolV1.c -pthread -Wall`  
Now the executed program will be in the same directory by the name 'a.out'
If there are any problems with permissions run:  
`chmod +x ./a.out`  

## Running and using the proxy server
First compile the proxy as explained before. Then just execute:  
`./a.out`  
Now if everything is working the program will print into the stdout that the proxy is listening on port 8080 and manager on port 9090.  
Now for testing the proxy you may run for http:  
`http_proxy=localhost:8080 curl google.com`  
And with ssl:  
`https_proxy=localhost:8080 curl https://www.google.com`  
If everything worked fine then html will be shown in stdout and in the proxy server console will have logged the connection. Note that when ssl is used the connection will appear on port 443.

## Running the client for the manager
### Compilation
First move to the client source files located at 'client_example':  
`cd client_example`  
Now compile the source file:  
`gcc clientMain.c -pthread -lsctp -Wall`  
If there are any problems with permissions run:  
`chmod +x ./a.out`

### Running the client
First make sure the proxy server is running.  
Now run the client:  
`./a.out`  
If everything worked fine then a message will appear saying that the connection with the server has been made.  
Now you can run any of the following commands:  
* login \<username\> \<password\>
* setStatus \<new status\>
* getStatus
* setMediaTypes \<media types separated with a comma and without space\>
* getMediaTypes
* setTransformationCommand \<command to execute for transformations\>
* getTransformationCommand
* getCurrentConnections
* getTotalConnections
* getBytesTransferred  

Note: all commands require authentication. So first execute login command using one of the two registered users:
* username = "fastiz", password = "12345"
* username = "eze", password = "12345"  
