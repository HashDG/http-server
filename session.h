#ifndef _SESSION_H_
#define _SESSION_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <magic.h>

#include "htable.h"

typedef enum http_ressource_type {
	HANDLER,
	HTML_FILE
} http_ressource_type;

typedef struct http_handler {
	char * method;
	char * (*handle)(char*,char**); // path, args 
} http_handler;

// Manage various html ressources like : CSS, HTML... 
typedef struct html_ressource {
	char * mime_type;
	FILE * file;
} html_ressource;

typedef struct http_ressource {
	http_ressource_type type;
	union {
		html_ressource * ressource;
		http_handler * handler;
	};
} http_ressource;

typedef struct arguments_t {
	void * arg1,* arg2;
} arguments_t;

char* readfile(FILE * file);
void load_files(ht* ressources, char* path, int level);
void load_handlers(ht* ressources);
void* session(void* arguments);


#endif // _SESSION_H_
