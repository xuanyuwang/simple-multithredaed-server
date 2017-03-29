#include <memory.h>
#include "RCB.h"
#include "sglib.h"

#define MAX_HTTP_SIZE 8192 /* size of buffer to allocate */

#define DLLIST_COMPARATOR(e1, e2) (e1->rcb.r_fsize - e2->rcb.r_fsize)

SGLIB_DEFINE_DL_LIST_PROTOTYPES(dllist, DLLIST_COMPARATOR, ptr_to_previous,
                                ptr_to_next);

SGLIB_DEFINE_DL_LIST_FUNCTIONS(dllist, DLLIST_COMPARATOR, ptr_to_previous,
                               ptr_to_next);

void insertRCB(RCB rcb) {
    dllist *new;
    struct sglib_dllist_iterator it;
    new = (dllist *) malloc(sizeof(dllist));
    new->rcb = rcb;
    sglib_dllist_add(&theRCBList, new);
}

/* parse the request and save it in a array */
RCB parseRequest(int fd, int sequence_number) {
    static char *buffer; /* request buffer */
    char *req = NULL;    /* ptr to req file */
    char *brk;           /* state used by strtok */
    char *tmp;           /* error checking ptr */
    FILE *fin = NULL;           /* input file handle */
    int len;             /* length of data read */
    RCB rcb;

    rcb.r_sequence_number = sequence_number;
    rcb.r_fd = fd;
    rcb.r_fhandle = fin;

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
        rcb.r_filename = (char*)malloc(sizeof(char) * strlen(req));
        strcpy(rcb.r_filename, req);
        fin = fopen(req, "r"); /* open file */
        rcb.r_fsize = get_file_size(fin);
        fclose(fin);
        rcb.r_quantum = rcb.r_fsize;
    }

    return rcb;
}

int get_file_size(FILE* fin) {
    fseek(fin, 0, SEEK_END);
    int sz = ftell(fin);
    return sz;
}

void displayRCB(RCB rcb) {
    printf("\n");
    printf("sequence number = %d\n", rcb.r_sequence_number);
    printf("file name = %s\n", rcb.r_filename);
    printf("file descriptor = %d\n", rcb.r_fd);
    printf("quantum = %d\n", rcb.r_quantum);
    printf("file size = %d\n", rcb.r_fsize);
}

void initRCBList() { theRCBList = NULL; }

void displayRCBList(){
    printf("*********************");
    printf("\ndisplay the list\n");
    dllist *l;
    for (l = sglib_dllist_get_first(theRCBList); l != NULL ; l = l->ptr_to_next) {
        displayRCB(l->rcb);
    }
}

void processRCB_SJF(){
    sglib_dllist_sort(&theRCBList);
//    displayRCBList();
    char *buffer;
    dllist *tmp;
    int len;

    buffer = (char*)malloc(MAX_HTTP_SIZE);
    if(!buffer){
        perror("Error while allocating memory");
        abort();
    }

    memset(buffer, 0, MAX_HTTP_SIZE);
    for(tmp = sglib_dllist_get_first(theRCBList); tmp != NULL; tmp = tmp->ptr_to_next){
        RCB rcb = tmp->rcb;

        printf("Start to process request for %s\n", rcb.r_filename);

        if(!rcb.r_filename){
            len = sprintf(buffer, "HTTP/1.1 400 Bad request\n\n");
            write(rcb.r_fd, buffer, len);
        }else{
            rcb.r_fhandle = fopen(rcb.r_filename, "r");
            if(!rcb.r_fhandle){
                len = sprintf(buffer, "HTTP/1.1 404 File not found\n\n");
                write(rcb.r_fd, buffer, len);
            }else{
                len = sprintf(buffer, "HTTP/1.1 200 OK\n\n");
                write(rcb.r_fd, buffer, len);

                do{
                    len = fread(buffer, 1, MAX_HTTP_SIZE, rcb.r_fhandle);
                    if (len < 0){
                        perror("Error while writing to client");
                    }else if(len > 0){
                        len = write(rcb.r_fd , buffer, len);
                        if (len < 1){
                            perror("Error while writing to client");
                        }
                    }
                }while (len == MAX_HTTP_SIZE);
                fclose(rcb.r_fhandle);
            }

        }
        close(rcb.r_fd);
        sglib_dllist_delete(&theRCBList, tmp);
        printf("Request %d\n completed", rcb.r_sequence_number);
    }
}

void processRCB_RR(){
    char *buffer;
    dllist *tmp;
    int len;
    int ALL_SENT = 0;

    buffer = (char*)malloc(MAX_HTTP_SIZE);
    if(!buffer){
        perror("Error while allocating memory");
        abort();
    }

    memset(buffer, 0, MAX_HTTP_SIZE);
    do{
        ALL_SENT = 1;
        for (tmp = sglib_dllist_get_first(theRCBList); tmp != NULL ; tmp = tmp->ptr_to_next){
            printf("Start to process request for %s\n", tmp->rcb.r_filename);
            if(!tmp->rcb.r_filename){
                len = sprintf(buffer, "HTTP/1.1 400 BAD request\n\n");
                write(tmp->rcb.r_fd, buffer, len);
            }else{
                tmp->rcb.r_fhandle = fopen(tmp->rcb.r_filename, "r");
                if(!tmp->rcb.r_fhandle){
                    len = sprintf(buffer, "HTTP/1.1 404 File not found\n\n");
                    write(tmp->rcb.r_fd, buffer, len);
                }else{
                    len = sprintf(buffer, "HTTP/1.1 200 OK\n\n");
                    write(tmp->rcb.r_fd, buffer, len);

                    fseek(tmp->rcb.r_fhandle, -tmp->rcb.r_fsize, SEEK_END);
                    len = fread(buffer, 1, MAX_HTTP_SIZE, tmp->rcb.r_fhandle);
                    if (len < 0){
                        perror("Error while writing to client");
                    }else if(len > 0){
                        len = write(tmp->rcb.r_fd, buffer, len);
                        if (len < 1) {
                            perror("Error while writing to client");
                        }else{
                            tmp->rcb.r_fsize -= len;
                            if(tmp->rcb.r_fsize != 0){
                                ALL_SENT = 0;
                            }else{
                                close(tmp->rcb.r_fd);
                                sglib_dllist_delete(&theRCBList, tmp);
                                printf("Request for %s completed\n", tmp->rcb.r_filename);
                            }
                        }
                    }else if(len == 0){
                        close(tmp->rcb.r_fd);
                        sglib_dllist_delete(&theRCBList, tmp);
                        printf("Request for %s completed\n", tmp->rcb.r_filename);
                    }
                }
            }
        }
    }while(ALL_SENT != 1);
}