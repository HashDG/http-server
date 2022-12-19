#include "session.h"

#define REQUEST_SIZE 4294967296
#define HTTP_FOLDER "/home/hashdg/dev/Projets/http-server/http_resources"
#define OK "HTTP/1.1 200 OK\nContent-type: text/html; charset=utf-8\n\r\n\r\n"
#define NOT_FOUND "HTTP/1.1 400 Not Found\nContent-type: text/html; charset=utf-8\n\r\n\r\n"
#define METHOD_NOT_ALLOWED "HTTP/1.1 405 Method Not Allowed\nContent-type: text/html;charset=utf-8\n\r\n\r\n"

static char* parse(char * request) {
	// r_str est de la taille de ma soustraction des deux pointeurs
	char* str = strstr(request, "HTTP/"), r_str[str-request];
	strncpy(r_str, request, (--str)-request);
	r_str[str-request] = '\0';
	
	if (r_str[str-request-1]=='/') {
		strcat(r_str, "index.html");
	}
	return strdup(r_str+5);
}

char* readfile(FILE * file) {
	int length;
	char* str;
	fseek(file, 0, SEEK_END);
	length=ftell(file);
	rewind(file);
	str=malloc(sizeof(char) * length);
	fread(str, sizeof(char), length, file);
	return strdup(str);
}

static bool is_html(FILE * file, char * name) {
	char line[1024];
	    
    if (strstr(name, "favicon.ico")) {
    	return true;
    }

	while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, "<html") != NULL) {
        	rewind(file);
            return true;
        }
    }
	return false;
}

void load_files(ht* ressources, char* path, int level) {
	DIR * dir;
	struct dirent *entry;

    if ( (dir = opendir(path)) == NULL) {
        printf("Cannot open directory %s\n", path);
        return;
    }

	while ((entry = readdir(dir)) != NULL) {
		char full_path[1024];
        snprintf(full_path, sizeof(full_path), (path[strlen(path)] == '/') ? "%s%s": "%s/%s", path, entry->d_name);
        if (entry->d_type == DT_DIR) {
        	if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            	load_files(ressources, full_path, level + 1);
            }
        } else {
        	FILE * html_file = fopen(full_path, "r");
        	http_ressource * ressource = malloc(sizeof(http_ressource));
        	if ( is_html(html_file, &full_path[strlen(HTTP_FOLDER)+1]) || strstr(entry->d_name, "favicon.ico")) {
        		ressource->type = HTML_FILE;
        		ressource->html_file = html_file;
        		ht_put(ressources, &full_path[strlen(HTTP_FOLDER)+1], ressource);
        	}
        }
    }
    closedir(dir);
}

char * simple_handler(char * path, char ** args) {
	char * r_str = malloc(sizeof(char) * 256);
	
	time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    size_t ret = strftime(s, sizeof(s), "%c", tm);
    
    strcat(r_str, "<!DOCTYPE html><html><h1>Salut utilisateur, il est ");
    strcat(r_str, s);
    strcat(r_str, "</h1></html>");
    return r_str;
}

void load_handlers(ht* ressources) {
	http_ressource * ressource = malloc(sizeof(http_ressource));
	ressource->type = HANDLER;
	ressource->handler = malloc(sizeof(http_handler));
	ressource->handler->method = "GET";
	ressource->handler->handle = &simple_handler;
	
	ht_put(ressources, "time", ressource);
}

void * session (void* arguments) {
	arguments_t* args = (arguments_t*) arguments;
	
	int c_sock = *( (int*) args->arg1);
	ht * ressources = *( (ht **) args->arg2);
	
	int idx = 0, read_size;
	char * request = malloc(sizeof(char)*REQUEST_SIZE), * reponse, * body, * header;
	FILE * file;
	
	read(c_sock, request, REQUEST_SIZE);
	
	printf("=======BEGIN-READ=======\n%s\n=======END-READ=======\n", request);

	if (strstr(request, "GET")) {
		char * parsed_request = parse(request);
		printf("requête:\t[%s]\n", parsed_request);
		if (ht_contains_key(ressources, parsed_request)) {
			http_ressource* ressource = (http_ressource*) ht_get(ressources, parsed_request);
			header = OK;
			if (ressource->type == HTML_FILE) {
				body = readfile(ressource->html_file);
			} else if (ressource->type == HANDLER) {
				http_handler * handler = ressource->handler;
				if (strstr(handler->method, "GET")) {
					body = (*handler->handle)(parsed_request, NULL);
				} else {
					header = METHOD_NOT_ALLOWED;
					body = "<html><h1>Error 405</h1></html>";
				}
			}
		} else {
			printf("NOT FOUND\n");
			header = NOT_FOUND;
			body = "<html><h1>Error 404</h1></html>";
		}
	} else {
		close(c_sock);
		return NULL;
	}
	
	reponse=malloc(sizeof(char)*(strlen(header)+strlen(body)));
	
	strcat(reponse, header);
	strcat(reponse, body);
	
	write(c_sock, reponse, strlen(reponse));

	close(c_sock);
}

