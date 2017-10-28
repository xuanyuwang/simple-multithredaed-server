# Simple Multi-Threaded Server

## ABSTRACT

This project aims to build a Caching Multi-Threaded Schedulding Web Server. There are mainly three features in the server:

+ **Schedulding**. There will be three different scheduling algorithms which are **Shortest Job First, Round Robin,** and **Multileve Queues with Feedback**.
+ **Multi-Threading**.This feature is on top of the **Scheduling** feature.
+ **Cacheing**

As you can see, the point of this project is to play threads. 

## BACKGROUND

### THE WEB SERVER

A web server is indeed a program which can receive the requests sent by clients and then response the requests.

The crucial problem here is not how a server serve a request. However, it is how to handle many requests at the same time. It seem natural that a server should deal with many requests simultineously today. Otherwise, we will think a serve is clumsy even stupid.

Basically, a server should have two modules. One of them should handle the networking part, while another process requests. Here, I use a provided network module which contains three functions:

+ **void network init(int port)**. This function will be called once when the web server starts up. It will bind the web server to the given port number and do other initialization works.
+ **void network wait()**. This function will put the web server to sleep until there is a available.
+ **int network open()**. This function opens a connection to the next waiting client and returns a file descriptor. Otherwaise, it returns -1.

As I will build the web server based on the provided simple web server (sws), I want to make a simple description about sws. The `main` function in sws.c takes one argument from the command line which represent the port number the server will be binded to. After the `main` function is called, either it will wait for clients or connect to clients and process their request, infinitely.

For each request, the server will receive it, parse it, send back an appropriate response, and then close the connection.

## PROJECT PLAN

The project is divided into three parts:

+ Implement the scheduling algorithms for one thread.
+ Add multi-thread mechanism.
+ Add caching mechanism.



