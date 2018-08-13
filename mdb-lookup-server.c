// Stephanie Pan 
// sp3507
// Lab2 Part1
// mdb-lookup-server.c

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mylist.h"
#include "mdb.h"

#define KeyMax 5

static void die(const char *s) {
  perror(s);
  exit(1); 
}

int loadmdb(FILE *fp, struct List *dest) { // taken from mdb.c in lab4 solutions folder
  // read all records into memory 
  
  struct MdbRec r; 
  struct Node *node = NULL; 
  int count = 0; 

  while (fread(&r, sizeof(r), 1, fp) == 1) {
      // allocate memory for new record, copy it into the one
      // that was just read from the database 
      
      struct MdbRec *rec = (struct MdbRec *) malloc(sizeof(r));
      if (!rec) 
          return -1; 
      memcpy(rec, &r, sizeof(r)); 

      // add the record to linked list 
      node = addAfter(dest, node, rec); 
      if (node == NULL)
          return -1; 

      count++; 

  }

  // check for fread error
  if (ferror(fp)) 
      return -1; 

  return count; 

}

void freemdb(struct List *list) { // also taken from mdb.c in lab4 solutions folder
    // free all records 
    traverseList(list, &free); 
    removeAllNodes(list); 
}


int main(int argc, char **argv){

  // ignore SIGPIPE so that we don't terminate when we call
  // send() on a disconnected socket. 
  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) 
      die("signal() failed");

  // making sure you have all arguments you need  
  if (argc != 3) {
    fprintf(stderr, "usage: %s <database> <port num>\n", argv[0]);
    exit(1); 
  }

  unsigned short port = atoi(argv[2]); 

  // Creating listening socket 
  int servsock; 
  if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    die("socket failed"); 

  // Construct local address structure 
  struct sockaddr_in servaddr; 
  memset(&servaddr, 0, sizeof(servaddr)); 
  servaddr.sin_family = AF_INET; 
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
  servaddr.sin_port = htons(port); 

  // Bind to local address
  if(bind(servsock,(struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    die("bind failed"); 

  // Start listening 
  if(listen(servsock, 5) < 0)
    die("listen failed"); 

  int clntsock; 
  socklen_t clntlen; 
  struct sockaddr_in clntaddr; 

  while (1) {

    // Accept an incoming connection 
    clntlen = sizeof(clntaddr); // initialize the in-out parameter

    if ((clntsock = accept(servsock, (struct sockaddr *) &clntaddr, &clntlen)) < 0)
        die("accept failed"); 
    // accept() returned a connected socket and filled in the client's address into clntaddr

    // wrapping socket file descriptor with FILE *
    FILE *input = fdopen(clntsock, "r");   
    
    // creating linked list w/ all entries from database 
    char *filename = argv[1];
    FILE *fp = fopen(filename, "r"); 
    if (fp == NULL) {
      die(filename); 
    }

    // read all records from database file into memory, 
    // store in linked list 
    struct List list; 
    initList(&list); 

    int loaded = loadmdb(fp, &list);
    if (loaded < 0)
      die("loadmdb");

    fclose(fp); 

    // lookup loop 
    char line[1000];
    char key[KeyMax + 1]; 

    while(fgets(line, sizeof(line), input) != NULL) {
        // must null terminate string 
        strncpy(key, line, sizeof(key)-1); 
        key[sizeof(key) - 1] = '\0'; 

        // removing new line character 
        size_t last = strlen(key) - 1; 
        if (key[last] == '\n')
            key[last] = '\0'; 

        // traverse list, printing out matching records 
        struct Node *node = list.head; 
        int recNo = 1; 
        while (node) {
            struct MdbRec *rec = (struct MdbRec *)node->data; 
            if (strstr(rec->name, key) || strstr(rec->msg, key)) {
                char entry[60]; 
                int linelen = snprintf(entry, 1000, "%4d: {%s} said {%s}\n", recNo, rec->name, rec->msg); 
                if(send(clntsock, &entry, strlen(entry), 0) != linelen)
                    die("failed to get entry"); 
            }
            node = node->next; 
            recNo++; 
        }
        send(clntsock, "\n", strlen("\n"), 0); 
    }
    
    freemdb(&list); 

    fclose(input); 

    // close the client connection and go back to accept() 
    close(clntsock); 

  }
}
