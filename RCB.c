#include <memory.h>
#include "RCB.h"
#include "sglib.h"

#define MAX_HTTP_SIZE 8192 /* size of buffer to allocate */
#define MIDDLE_QUEUE_QUANTUM 65536

#define DLLIST_COMPARATOR(e1, e2) (e1->rcb.r_fsize - e2->rcb.r_fsize)

SGLIB_DEFINE_DL_LIST_PROTOTYPES(dllist, DLLIST_COMPARATOR, ptr_to_previous,
                                ptr_to_next);

SGLIB_DEFINE_DL_LIST_FUNCTIONS(dllist, DLLIST_COMPARATOR, ptr_to_previous,
                               ptr_to_next);

void insertRCB(RCB *rcb) {
//    dllist *new;
//    new = (dllist *) malloc(sizeof(dllist));
//    new->rcb = rcb;
//    sglib_dllist_add(&theRCBList, new);
    sglib_dllist_add(&theRCBList, rcb);
}

/* parse the request and save it in a array */
void parseRequest(RCB *rcb, int fd, int sequence_number) {
    static char *buffer; /* request buffer */
    char *req = NULL;    /* ptr to req file */
    char *brk;           /* state used by strtok */
    char *tmp;           /* error checking ptr */
    FILE *fin = NULL;           /* input file handle */
    int len;             /* length of data read */

    rcb->r_sequence_number = sequence_number;
    rcb->r_fd = fd;
    rcb->r_fhandle = fin;

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

    // build the rcb
    if (!req) { /* is req valid? */
        len = sprintf(buffer, "HTTP/1.1 400 Bad request\n\n");
        write(fd, buffer, len); /* if not, send err */
    } else {                  /* if so, open file */
        req++;                  /* skip leading / */
        rcb->r_filename = malloc(sizeof(char) * (1 + strlen(req)));
        strcpy(rcb->r_filename, req);
//        rcb.r_filename = req;
        fin = fopen(rcb->r_filename, "r"); /* open file */
        rcb->r_fsize = get_file_size(fin);
        fclose(fin);
        rcb->r_quantum = MAX_HTTP_SIZE;
    }

}

int get_file_size(FILE* fin) {
    fseek(fin, 0, SEEK_END);
    int sz = ftell(fin);
    return sz;
}

void displayRCB(RCB rcb) {
    printf("\n");
//    printf("sequence number = %d\n", rcb.r_sequence_number);
    printf("file name = %s\n", rcb.r_filename);
//    printf("file descriptor = %d\n", rcb.r_fd);
//    printf("quantum = %d\n", rcb.r_quantum);
    printf("file size = %d\n", rcb.r_fsize);
}

void initRCBList() { theRCBList = NULL; }

void displayRCBList(dllist *head) {
    printf("*********************");
    printf("\ndisplay the list\n");
    dllist *l;
    for (l = sglib_dllist_get_first(head); l != NULL; l = l->ptr_to_next) {
        displayRCB(l->rcb);
    }
}

void processRCB(RCB *rcb) {
    char *buffer;
    int len;
    int size;

    buffer = (char*)malloc(MAX_HTTP_SIZE);
    if(!buffer){
        perror("Error while allocating memory");
        abort();
    }

    memset(buffer, 0, MAX_HTTP_SIZE);

    if (!rcb->r_filename) {
        len = sprintf(buffer, "HTTP/1.1 400 Bad request\n\n");
        write(rcb->r_fd, buffer, len);
    } else {
        rcb->r_fhandle = fopen(rcb->r_filename, "r");
        if (!rcb->r_fhandle) {
            len = sprintf(buffer, "HTTP/1.1 404 File not found\n\n");
            write(rcb->r_fd, buffer, len);
        } else {
            len = sprintf(buffer, "HTTP/1.1 200 OK\n\n");
            write(rcb->r_fd, buffer, len);

            fseek(rcb->r_fhandle, -rcb->r_fsize, SEEK_END);
            if (rcb->r_quantum < rcb->r_fsize) {
                size = rcb->r_quantum;
            } else {
                size = rcb->r_fsize;
            }
            do {
                len = fread(buffer, 1, MAX_HTTP_SIZE, rcb->r_fhandle);
                if (len < 0) {
                    perror("Error while writing to client");
                } else if (len > 0) {
                    len = write(rcb->r_fd, buffer, len);
                    if (len < 1) {
                        perror("Error while writing to client");
                    } else {
                        size -= len;
                    }
                }
            } while (size != 0);
            rcb->r_fsize -= rcb->r_quantum;
            if (rcb->r_fsize < 0) {
                rcb->r_fsize = 0;
                fclose(rcb->r_fhandle);
            }
        }
    }
}

void processRCB_SJF() {
    sglib_dllist_sort(&theRCBList);
    dllist *tmp;

    for (tmp = sglib_dllist_get_first(theRCBList); tmp != NULL; tmp = tmp->ptr_to_next) {
        printf("Start to process request for %s\n", tmp->rcb.r_filename);

        do {
            processRCB(&(tmp->rcb));
        } while (tmp->rcb.r_fsize > 0);

        close(tmp->rcb.r_fd);
        sglib_dllist_delete(&theRCBList, tmp);
        printf("Request %d completed\n", tmp->rcb.r_sequence_number);
        free(tmp);
    }
}

void processRCB_RR(dllist *rcbdllist) {
    displayRCBList(rcbdllist);
    dllist *tmp;
    int ALL_SENT = 0;
    int turn = 1;

    do{
        printf("\n******* Turn: %d ******\n", turn);
        turn++;
        ALL_SENT = 1;
        for (tmp = sglib_dllist_get_first(rcbdllist); tmp != NULL; tmp = tmp->ptr_to_next) {
            printf("Start to process request for %s\n", tmp->rcb.r_filename);
            processRCB(tmp);
            if (tmp->rcb.r_fsize != 0) {
                ALL_SENT = 0;
            } else {
                close(tmp->rcb.r_fd);
                sglib_dllist_delete(&rcbdllist, tmp);
                printf("Request %d completed: %s\n", tmp->rcb.r_sequence_number, tmp->rcb.r_filename);
//                free(tmp);
            }
        }
    }while(ALL_SENT != 1);
    printf("After RR\n");
    displayRCBList(rcbdllist);
}

void processRCB_MLFB(dllist *originalRCBList) {
    dllist *highest = NULL;
    dllist *middle = NULL;
    dllist *tmp;

    // process the high priority queue
    highest = originalRCBList;
    if (highest == NULL) {
        return;
    }
    printf("Begin to process the highest priority queue\n");
//    displayRCBList(highest);

    for (tmp = sglib_dllist_get_first(highest); tmp != NULL; tmp = tmp->ptr_to_next) {
        processRCB(&tmp->rcb);
        if (tmp->rcb.r_fsize == 0) {
            close(tmp->rcb.r_fd);
            sglib_dllist_delete(&highest, tmp);
            printf("Request %d completed: %s\n", tmp->rcb.r_sequence_number, tmp->rcb.r_filename);
        } else {
            tmp->rcb.r_quantum = MIDDLE_QUEUE_QUANTUM;
            dllist *toMiddle = (dllist *) malloc(sizeof(dllist));
            toMiddle->rcb = tmp->rcb;
            sglib_dllist_add(&middle, toMiddle);
            sglib_dllist_delete(&highest, tmp);
//            free(tmp);
        }
    }

    processRCB_middle(middle);
}

void processRCB_middle(dllist *middle) {
    dllist *low = NULL;
    dllist *tmp;
    // process the
    if (middle == NULL) {
        return;
    }
    printf("Begin to process the middle priority queue\n");
    displayRCBList(middle);
    for (tmp = sglib_dllist_get_first(middle); tmp != NULL; tmp = tmp->ptr_to_next) {
        processRCB(&tmp->rcb);
        if (tmp->rcb.r_fsize == 0) {
            close(tmp->rcb.r_fd);
            sglib_dllist_delete(&middle, tmp);
            printf("Request %d completed: %s\n", tmp->rcb.r_sequence_number, tmp->rcb.r_filename);
        } else {
            dllist *toLow = (dllist *) malloc(sizeof(dllist));
            toLow->rcb = tmp->rcb;
            sglib_dllist_add(&low, toLow);
            sglib_dllist_delete(&middle, tmp);
            free(tmp);
        }
    }
    processRCB_low(low);
}

void processRCB_low(dllist *low) {
    // process the low priority queue
    if (low == NULL) {
        return;
    }
    printf("Begin to process the low priority queue\n");
    processRCB_RR(low);
}

void freeRCBList(dllist *node) {

}