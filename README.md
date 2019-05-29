# ProxyServerHTTP1.1

== Example ==

Setup proxy server:
~$ gcc main.c
~$ ./a.out 9090 //9090 is the server port

Client request:
~$ http_proxy=localhost:9090 curl google.com
