# ProxyServerHTTP1.1

== Example ==

Setup proxy server:
~$ gcc helpers.c main.c proxyClientActiveSocket.c proxyOriginActiveSocket.c proxyPassiveSocket.c proxyTransformation.c selector.c buffer.c configPassiveSocket.c configActiveSocket.c protocolV1.c -pthread -Wall
~$ ./a.out 9090 //9090 is the server port

Setup del cliente:
~$ gcc clientMain.c -pthread -lsctp -Wall

Client request:
~$ http_proxy=localhost:9090 curl google.com
~$ https_proxy=localhost:9090 curl https://www.google.com
