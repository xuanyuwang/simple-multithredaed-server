#ifndef RCB_H
#define RCB_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "sglib.h"

struct _request_control_block {
    int r_sequence_number;
    int r_fd;
    FILE *r_fhandle;
    int r_quantum;
    int r_fsize;
    char *r_filename;
};
typedef struct _request_control_block RCB;
typedef struct dllist {
    RCB rcb;
    struct dllist *ptr_to_next;
    struct dllist *ptr_to_previous;
} dllist;


dllist *readyQ;
dllist *workQ;

void parseRequest(RCB *rcb, int fd, int sequence_number);

void initDLList(dllist*);
void displayRCB(RCB);
void displayRCBList(dllist *head);
dllist* insertRCB(dllist *node, dllist* list);

void processRCB_SJF();

int dllistLen(dllist* head);

dllist* getFirstRCB(dllist* list);
dllist* deleteRCB(dllist* list, dllist* node);
void processRCB_RR(dllist *);

void processRCB_MLFB(dllist *);
#endif
