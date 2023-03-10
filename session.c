#include "session.h"

#define REQUEST_SIZE 4294967296
#define BODY_SIZE 2147483648
#define HEADER_SIZE 65536
#define CONTENT_TYPE_SIZE 255
#define CONTENT_LENGTH_SIZE 255
#define MIME_SIZE 128
#define FILENAME_SIZE 255

#define HTTP_FOLDER "http_ressources"
#define OK "HTTP/1.1 200 OK\r\n"
#define NOT_FOUND "HTTP/1.1 404	 Not Found\r\n"
#define METHOD_NOT_ALLOWED "HTTP/1.1 405 Method Not Allowed\r\n"

static char * parse_request(char * key, char * request) {
	// r_str est de la taille de ma soustraction des deux pointeurs
	char* str = strstr(request, "HTTP/"), r_str[str-request];
	strncpy(r_str, request, (--str)-request);
	r_str[str-request] = '\0';
	
	if (r_str[str-request-1]=='/') {
		strcat(r_str, "index.html");
	}
	strcpy(key, r_str+5);
}

static size_t file_size(FILE * file) {
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
    return size;
}

static bool ends_with(const char* txt, const char* suffix) {
	size_t txt_len = strlen(txt), suf_len = strlen(suffix);
	if (suf_len > txt_len) {
        return 0;
    }
    return strcmp(txt + txt_len - suf_len, suffix) == 0;
}

static bool starts_with(const char* pre, const char* txt) {
	return (strstr(txt, pre) == txt);
}

static char * parse_mime(const char * uri) {
	if (ends_with(uri, ".css")) {
		return "text/css";
	}
	return "text/plain"; 
}

void http_ressource_init(http_ressource* http_r, char* fullpath, magic_t cookie) {
	if (ends_with(fullpath, ".cgi")) {
        http_r->type = CGI;
        http_r->cgi_filename = fullpath; 
    } else {
		http_r->type = HTML_FILE;
		http_r->ressource = malloc(sizeof(html_ressource));
		http_r->ressource->mime_type = malloc(sizeof(char) * MIME_SIZE);
		const char * type = magic_file(cookie, fullpath);
        if (strcmp(type, "text/plain") == 0) {
        	strcpy(http_r->ressource->mime_type, parse_mime(fullpath));
        } else {
        	strcpy(http_r->ressource->mime_type, type);
        }
        FILE * html_file = fopen(fullpath, (starts_with("text", type) ? "r" : "rb") );
   		http_r->ressource->file = html_file;
	}
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
        	http_ressource * http_r = malloc(sizeof(http_ressource));
        	http_ressource_init(http_r, &full_path[0], cookie);
        	printf("[%s]\n", &full_path[strlen(HTTP_FOLDER)+1   ]);
        	ht_put(ressources, &full_path[strlen(HTTP_FOLDER)+1], http_r);
        }
    }
    magic_close(cookie);
    closedir(dir);
}

void simple_handler(char * body, char * path, char ** args) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char s[64];
	size_t ret = strftime(s, sizeof(s), "%c", tm);
    
	strcat(body, "<!DOCTYPE html><html><h1>Salut utilisateur, il est ");
	strcat(body, s);
	strcat(body, "</h1></html>");
}

void load_handlers(ht* ressources) {
	http_ressource * ressource = malloc(sizeof(http_ressource));
	ressource->type = HANDLER;
	ressource->handler = malloc(sizeof(http_handler));
	ressource->handler->method = "GET";
	ressource->handler->handle = &simple_handler;
	
	ht_put(ressources, "time", ressource);
}

static void manage_cgi(char* request, char* reponse, char* cgi_filename) {
	pid_t pid = fork();
	if (pid==0) {
		execl(cgi_filename, request, NULL);
	} else {
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			FILE* fp = popen(cgi_filename, "r");

      		char buffer[1024];
     		while (fgets(buffer, sizeof(buffer), fp) != NULL) {
     			strcat(reponse, buffer);
      		}
      		pclose(fp);
		}
	}
}

static void manage_asset(char * header, unsigned char * body, size_t * body_size, char * path, http_ressource * asset) {
	char * content_type = malloc(sizeof(char) * CONTENT_TYPE_SIZE), * content_length = malloc(sizeof(char) * CONTENT_LENGTH_SIZE);
	if (asset->type == HTML_FILE) {
		*body_size = file_size(asset->ressource->file);
		fread(body, sizeof(unsigned char), *body_size, asset->ressource->file);
	} else if (asset->type == HANDLER) {
		http_handler * handler = asset->handler;
		if (strstr(handler->method, "GET")) {
			(*handler->handle)(body, path, NULL);
			*body_size = strlen(body);
		} else {
			strcat(header, METHOD_NOT_ALLOWED);
			strcpy(body, "<html><h1>Error 405</h1></html>");
		}
	}
	
	sprintf(content_type, "Content-Type: %s", asset->ressource->mime_type);
		if (starts_with("text", asset->ressource->mime_type)) {
			strcat(content_type, ";charset=utf-8");
		}				
	sprintf(content_length, "Content-Length: %li", *body_size);
	
	strcat(header, content_type);
	strcat(header, "\r\n");
	strcat(header, content_length);	
	strcat(header, "\r\n");
	
	free(content_type);
	free(content_length);
}

static void reply(char * reponse, size_t * reponse_size, ht * ressources, bool keep_alive, char * request) {
	unsigned char * body = malloc(sizeof(unsigned char) * BODY_SIZE), * key = malloc(sizeof(char) * FILENAME_SIZE), * header = malloc(sizeof(char) * HEADER_SIZE);
	size_t body_size;
	http_ressource * asset;

	if (starts_with("GET", request)) {
		parse_request(key, request);
		printf("request: [%s]\n", key);
		if (ht_contains_key(ressources, key)) {
			printf("OK\n");
			strcpy(header, OK);
			asset = (http_ressource*) ht_get(ressources, key);
			
			if (asset->type == CGI) {
				strcpy(reponse, header);
				manage_cgi(request, reponse, asset->cgi_filename);
				*reponse_size = strlen(reponse);
				return;
			} else {
				manage_asset(header, body, &body_size, key, asset);				
			}
			
			if (keep_alive) {
				strcat(header, "Connection: keep-alive\r\n");	
			}
		} else {
			printf("NOT FOUND\n");
			strcpy(header, NOT_FOUND);
			strcpy(body, "<html><h1>Error 404: Not Found/h1></html>");
		}
	} else {
		printf("METHOD NOT ALLOWED\n");
		strcpy(header, METHOD_NOT_ALLOWED);
		strcpy(body, "<html><h1>Error 405: Method Not Allowed</h1></html>");
	}
	
	strcat(header, "\r\n");
	
	strcpy(reponse, header);
	memcpy(reponse+(strlen(header)), body, sizeof(unsigned char) * body_size );
	*reponse_size = strlen(header) + body_size;
	
	free(header);
	free(body);
	free(key);
}

void * session (void* arguments) {
	arguments_t* args = (arguments_t*) arguments;
	
	int c_sock = *( (int*) args->arg1);
	ht * ressources = *( (ht **) args->arg2);
	
	bool keep_alive = true;
	size_t reponse_size;
	char * request = malloc(sizeof(char)*REQUEST_SIZE), * reponse = malloc(sizeof(char)*REQUEST_SIZE);
	
	while ( read(c_sock, request, REQUEST_SIZE) > 0 && keep_alive) {
		printf("=======BEGIN-READ=======\n%s\n=======END-READ=======\n", request);
		reply(reponse, &reponse_size, ressources, keep_alive, request);
		printf("#######BEGIN-RESPONSE#######\n%s\n#######END-RESPONSE#######\n", reponse);

		write(c_sock, reponse, reponse_size);
		memset(reponse, 0, sizeof(char)*REQUEST_SIZE);
	}
	printf("Session closed\n");
	close(c_sock);

	free(request);
	free(reponse);
}

