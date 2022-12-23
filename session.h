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

typedef struct html_ressource html_ressource;
typedef struct http_handler http_handler;

typedef struct http_ressource {
	enum http_ressource_type { HANDLER, HTML_FILE } type;
	union {
		struct html_ressource {	
			char * mime_type;
			FILE * file;
		} * ressource;
		struct http_handler {
			char * method;
			void (*handle)(char*, char*,char**); // body, path, args 
		} * handler;
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
