/*
 * File: sws.c
 * Author: Alex Brodsky
 * Purpose: This file contains the implementation of a simple web server.
 *          It consists of two functions: main() which contains the main
 *          loop accept client connections, and serve_client(), which
 *          processes each client request.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#include "RCB.h"
#include "network.h"

#define MAX_HTTP_SIZE 8192 /* size of buffer to allocate */
#define MAX_REQ 100

/********* Global ***************/
static char SJF[4] = "SJF";
static char RR[3] = "RR";
static char MLFB[5] = "MLFB";
int sequence_counter;
sem_t sem_full;
sem_t sem_empty;
sem_t sem_mutex;

/*********** Function ************/

void *work(void *);

/* This function takes a file handle to a client, reads in the request,
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters:
 *             fd : the file descriptor to the client connection
 * Returns: None
 */
static void serve_client(int fd) {
    static char *buffer; /* request buffer */
    char *req = NULL;    /* ptr to req file */
    char *brk;           /* state used by strtok */
    char *tmp;           /* error checking ptr */
    FILE *fin;           /* input file handle */
    int len;             /* length of data read */

    if (!buffer) { /* 1st time, alloc buffer */
        buffer = malloc(MAX_HTTP_SIZE);
        if (!buffer) { /* error check */
            perror("Error while allocating memory");
            abort();
        }
    }

    memset(buffer, 0, MAX_HTTP_SIZE);
    if (read(fd, buffer, MAX_HTTP_SIZE) <= 0) { /* read req from client */
        perror("Error while reading request");
        abort();
    }

    /* standard requests are of the form
     *   GET /foo/bar/qux.html HTTP/1.1
     * We want the second token (the file path).
     */
    tmp = strtok_r(buffer, " ", &brk); /* parse request */
    if (tmp && !strcmp("GET", tmp)) {
        req = strtok_r(NULL, " ", &brk);
    }
    printf("req is: %s\n", req);

    if (!req) { /* is req valid? */
        len = sprintf(buffer, "HTTP/1.1 400 Bad request\n\n");
        write(fd, buffer, len); /* if not, send err */
    } else {                  /* if so, open file */
        req++;                  /* skip leading / */
        fin = fopen(req, "r");  /* open file */
        if (!fin) {             /* check if successful */
            len = sprintf(buffer, "HTTP/1.1 404 File not found\n\n");
            write(fd, buffer, len);                       /* if not, send err */
        } else {                                        /* if so, send file */
            len = sprintf(buffer, "HTTP/1.1 200 OK\n\n"); /* send success code */
            write(fd, buffer, len);

            do {                                          /* loop, read & send file */
                len = fread(buffer, 1, MAX_HTTP_SIZE, fin); /* read file chunk */
                if (len < 0) {                              /* check for errors */
                    perror("Error while writing to client");
                } else if (len > 0) { /* if none, send chunk */
                    len = write(fd, buffer, len);
                    if (len < 1) { /* check for errors */
                        perror("Error while writing to client");
                    }
                }
            } while (len == MAX_HTTP_SIZE); /* the last chunk < 8192 */
            fclose(fin);
        }
    }
    close(fd); /* close client connectuin*/
}

void initialize() {
    sequence_counter = 1;
    initDLList(workQ);
    initDLList(readyQ);
    sem_init(&sem_empty, 0, 100);
    sem_init(&sem_full, 0, 0);
    sem_init(&sem_mutex, 0, 1);
}

/* This function is where the program starts running.
 *    The function first parses its command line parameters to determine port #
 *    Then, it initializes, the network and enters the main loop.
 *    The main loop waits for a client (1 or more to connect, and then processes
 *    all clients by calling the seve_client() function for each one.
 * Parameters:
 *             argc : number of command line parameters (including program name
 *             argv : array of pointers to command line parameters
 * Returns: an integer status code, 0 for success, something else for error.
 */
int main(int argc, char **argv) {
    int port = -1; /* server port # */
    int fd;        /* client file descriptor */
    char *algo;
    int numOfWorkers;
    pthread_t *workers;


    /* check for and process parameters
     */
    if ((argc < 2) || (sscanf(argv[1], "%d", &port) < 1) || sscanf(argv[3], "%d", &numOfWorkers) < 1) {
        printf("usage: sms <port>\n");
        return 0;
    } else {
        sscanf(argv[1], "%d", &port);
        algo = argv[2];
        sscanf(argv[3], "%d", &numOfWorkers);
        sequence_counter = 1;
    }

    /********************************************/
    network_init(port); /* init network module */
    initialize();
    workers = (pthread_t *) malloc(sizeof(pthread_t) * numOfWorkers);
    for (int i = 0; i < numOfWorkers; ++i) {
        pthread_create(&workers[i], NULL, work, (void *) algo);
    }

    /********************** Infinite loop ****************/
    for (;;) {        /* main loop */
        network_wait(); /* wait for clients */

        printf("\nGot connections\n");
        for (fd = network_open(); fd >= 0; fd = network_open()) { /* get clients */
            sem_wait(&sem_empty);
            sem_wait(&sem_mutex);
            /*******************/
//             serve_client(fd); /* process each client */
            dllist *node = malloc(sizeof(dllist));
            parseRequest(&node->rcb, fd, sequence_counter);

            workQ = insertRCB(node, workQ);
            sequence_counter++;
//            displayRCBList(workQ);
            /*******************/
            sem_post(&sem_mutex);
            sem_post(&sem_full);
        }
//        if(strcmp(algo, SJF) == 0){
//            processRCB_SJF();
//        }else if(strcmp(algo, RR) == 0){
//            processRCB_RR(readyQ);
//        } else if (strcmp(algo, MLFB) == 0) {
//            processRCB_MLFB(readyQ);
//        }
//        initialize();
    }
}

void *work(void *para) {
    char *algo = (char *) para;
    pthread_t dealRCB;
    while (1) {
        sem_wait(&sem_full);
        sem_wait(&sem_mutex);
        /*******************/
        if (dllistLen(workQ) > 0) {
            dllist *node = getFirstRCB(workQ);
            workQ = deleteRCB(workQ, node);
            readyQ = insertRCB(node, readyQ);
            printf("The ready Queue:\n");
            displayRCBList(readyQ);
        }
//        else if (dllistLen(workQ) == 0 && dllistLen(readyQ) != 0) {
        if (dllistLen(readyQ) != 0) {
            dllist *node;
            if (strcmp(algo, SJF) == 0) {
                processRCB_SJF();
            } else if (strcmp(algo, RR) == 0) {

            } else if (strcmp(algo, MLFB) == 0) {

            }
        }

        /*******************/
        sem_post(&sem_mutex);
        sem_post(&sem_empty);


        sem_wait(&sem_mutex);
        /*******************/
        /*******************/
        sem_post(&sem_mutex);
    }

}
