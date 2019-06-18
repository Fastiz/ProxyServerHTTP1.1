# ProxyServerHTTP1.1

== Report ==
The report is located in the root folder of the project and is named 'InformeProtocolos.pdf'.

== Proxy server ==
For proxy server installation run the following commands.
    Go to .c files folder:
    -   cd bin
    Compile files and generate ./a.out executable:
    -   gcc helpers.c main.c proxyClientActiveSocket.c proxyOriginActiveSocket.c proxyPassiveSocket.c proxyTransformation.c selector.c buffer.c configPassiveSocket.c configActiveSocket.c protocolV1.c -pthread -Wall
    Run the executable with port as first parameter:
    -   ./a.out 9090 //9090 is the server port

    NOTE: if there is a problem with permissions run:
    -   chmod +x ./a.out
    and then execute the program again.

== Proxy usage example ==
    -   http_proxy=localhost:9090 curl google.com
    -   https_proxy=localhost:9090 curl https://www.google.com

== Configuration client example ==
There is a client for connecting as proxy administrator that uses the protocol specified in the report file.
    Go to the folder with .c files.
    -   cd ./client_example
    Compile files and generate the executable:
    -   gcc clientMain.c -pthread -lsctp -Wall
    Run the executable:
    -   ./a.out

    NOTE: if there is a problem with permissions run:
    -   chmod +x ./a.out
    and then execute the program again.

== Using the client example ==
After executing the program use the following commands to communicate with the proxy.
    -   login <username> <password>
    -   setStatus <new status>
    -   getStatus
    -   setMediaTypes <media types separated with a comma and without space>
    -   getMediaTypes
    -   setTransformationCommand <command to execute for transformations>
    -   getTransformationCommand
    -   getCurrentConnections
    -   getTotalConnections
    -   getBytesTransferred
