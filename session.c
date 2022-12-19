#include "session.h"

#define REQUEST_SIZE 4294967296
#define HEADER_SIZE 65536
#define CONTENT_TYPE_SIZE 256
#define MIME_SIZE 128
#define HTTP_FOLDER "http_resources"
#define OK "HTTP/1.1 200 OK\n"
#define NOT_FOUND "HTTP/1.1 400 Not Found\n"
#define METHOD_NOT_ALLOWED "HTTP/1.1 405 Method Not Allowed\n"

static char * parse_request(char * request) {
	// r_str est de la taille de ma soustraction des deux pointeurs
	char* str = strstr(request, "HTTP/"), r_str[str-request];
	strncpy(r_str, request, (--str)-request);
	r_str[str-request] = '\0';
	
	if (r_str[str-request-1]=='/') {
		strcat(r_str, "index.html");
	}
	return strdup(r_str+5);
}

static char * parse_mime(char * uri) {
	uri = strrchr(uri, '.');
	if (uri != NULL) {
		if (strcmp(uri, ".css")==0) {
			return "text/css";
		}
	}
	return "text/plain"; 
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
    
    if (strstr(name, ".css")) {
    	printf("css: <%s>\n", readfile(file));
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
	magic_t cookie;

    if ( (dir = opendir(path)) == NULL) {
        printf("Cannot open directory %s\n", path);
        return;
    }
    
    cookie = magic_open(MAGIC_MIME_TYPE);
    magic_load(cookie, NULL);

	while ((entry = readdir(dir)) != NULL) {
		char full_path[1024];
        snprintf(full_path, sizeof(full_path), (path[strlen(path)] == '/') ? "%s%s": "%s/%s", path, entry->d_name);
        if (entry->d_type == DT_DIR) {
        	if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            	load_files(ressources, full_path, level + 1);
            }
        } else {

        	FILE * html_file = fopen(full_path, "r");
        	http_ressource * http_r = malloc(sizeof(http_ressource));
        	
        	http_r->type = HTML_FILE;
        	http_r->ressource = malloc(sizeof(html_ressource));
        	http_r->ressource->mime_type = malloc(sizeof(char) * MIME_SIZE);
        	
        	const char * type = magic_file(cookie, &full_path[0]);
        	if (strcmp(type, "text/plain") == 0) {
        		strcpy(http_r->ressource->mime_type, parse_mime(&full_path[strlen(HTTP_FOLDER)+1]));
        	} else {
        		strcpy(http_r->ressource->mime_type, type);
        	}
        	
			http_r->ressource->file = html_file;
			
        	ht_put(ressources, &full_path[strlen(HTTP_FOLDER)+1], http_r);
        }
    }
    magic_close(cookie);
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
	char * request = malloc(sizeof(char)*REQUEST_SIZE), * reponse, * body, * header = malloc(sizeof(char)*HEADER_SIZE), * content_type = malloc(sizeof(char)*CONTENT_TYPE_SIZE);
	FILE * file;
	
	read(c_sock, request, REQUEST_SIZE);
	
	printf("=======BEGIN-READ=======\n%s\n=======END-READ=======\n", request);

	if (strstr(request, "GET")) {
		char * parsed_request = parse_request(request);
		printf("requête:\t[%s]\n", parsed_request);
		if (ht_contains_key(ressources, parsed_request)) {
			http_ressource* http_r = (http_ressource*) ht_get(ressources, parsed_request);
			strcat(header, OK);
						
			if (http_r->type == HTML_FILE) {
				sprintf(content_type, "Content-type: %s;charset=utf-8", http_r->ressource->mime_type);
				strcat(header, content_type);				
				body = readfile(http_r->ressource->file);				
			} else if (http_r->type == HANDLER) {
				http_handler * handler = http_r->handler;
				if (strstr(handler->method, "GET")) {
					body = (*handler->handle)(parsed_request, NULL);
				} else {
					strcat(header, METHOD_NOT_ALLOWED);
					body = "<html><h1>Error 405</h1></html>";
				}
			}
		} else {
			printf("NOT FOUND\n");
			strcat(header, NOT_FOUND);
			body = "<html><h1>Error 404</h1></html>";
		}
	} else {
		close(c_sock);
		return NULL;
	}
	strcat(header, "\n\r\n\r");
	
	reponse=malloc(sizeof(char)*(strlen(header)+strlen(body)));
	
	strcat(reponse, header);
	strcat(reponse, body);
	
	write(c_sock, reponse, strlen(reponse));

	close(c_sock);
}

