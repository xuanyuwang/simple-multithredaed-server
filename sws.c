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
sem_t sem_full; // Count the number of threads which can work.
sem_t sem_empty; // How many request can be processed in the future.
sem_t sem_mutex; // Binary semophor, a mutex

/*********** Function Declaration ************/

void *work(void *);


/*********** Function Definition ************/
void initialize() {
    sequence_counter = 1;
    initDLList(workQ);
    initDLList(readyQ);
    initDLList(middle);
    initDLList(low);
    sem_init(&sem_empty, 0, MAX_REQ);
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
        fflush(stdout);
        return 0;
    } else {
        sscanf(argv[1], "%d", &port);
        algo = argv[2];
        sscanf(argv[3], "%d", &numOfWorkers);
        sequence_counter = 1;
    }

    /**************** Initialization **********************/
    network_init(port); /* init network module */
    initialize();
    // Before this line, there is only one thread in this process.
    // Create many threads in the next line.
    workers = (pthread_t *) malloc(sizeof(pthread_t) * numOfWorkers);
    // For each thread, let them start with function *work*. All threads will be blocked at the beginning
    // of functiion *work*. Then, until there is a request, a thread will be woke up at the end of the following
    // for-loop.
    for (int i = 0; i < numOfWorkers; ++i) {
        pthread_create(&workers[i], NULL, work, (void *) algo);
    }

    /********************** Infinite main loop ****************/
    for (;;) {        /* main loop */
        network_wait(); /* wait for clients */

        printf("\nGot connections\n");
        fflush(stdout);
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
    }
}

void *work(void *para) {
    char *algo = (char *) para;
    pthread_t dealRCB;
    while (1) {
        if (strcmp(algo, MLFB) == 0) {
            sem_wait(&sem_full);
            sem_wait(&sem_mutex);

            /*******************/
            if (dllistLen(workQ) > 0) {
                dllist *node = getFirstRCB(workQ);
                workQ = deleteRCB(workQ, node);
                readyQ = insertRCB(node, readyQ);
                printf("Reuqest for file %s admitted\n", node->rcb.r_filename);
                fflush(stdout);
            }
            if (dllistLen(readyQ) != 0) {
                dllist *node;
                if (strcmp(algo, SJF) == 0) {
                    processRCB_SJF();
                } else if (strcmp(algo, RR) == 0) {
                    processRCB_RR(&readyQ);
                } else if (strcmp(algo, MLFB) == 0) {
                    processRCB_MLFB(&readyQ);
                }
            }
            /*******************/
            sem_post(&sem_mutex);
            sem_post(&sem_empty);
        } else {
            sem_wait(&sem_mutex);
            /*******************/
            if (dllistLen(workQ) > 0) {
                dllist *node = getFirstRCB(workQ);
                workQ = deleteRCB(workQ, node);
                readyQ = insertRCB(node, readyQ);
                printf("Reuqest for file %s admitted\n", node->rcb.r_filename);
                fflush(stdout);
            } else if (dllistLen(readyQ) != 0) {
                dllist *node;
                if (strcmp(algo, SJF) == 0) {
                    processRCB_SJF();
                } else if (strcmp(algo, RR) == 0) {
                    processRCB_RR(&readyQ);
                } else if (strcmp(algo, MLFB) == 0) {
                    processRCB_MLFB(&readyQ);
                }
            }
            /*******************/
            sem_post(&sem_mutex);
            sem_post(&sem_empty);
        }

    }

}
