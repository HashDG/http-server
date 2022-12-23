#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "htable.h"
#include "session.h"

#define THREAD_LIMIT 100
#define HTTP_FOLDER "http_ressources"

void ht_init(ht* ressources) {
	load_files(ressources, HTTP_FOLDER, 0);
	load_handlers(ressources);
}

int main() {
	int c_sock, opt = 1, idx;
	struct sockaddr_in s_addr = {AF_INET, htons(1080), {inet_addr("127.0.0.1")}, 0}, c_addr;
	socklen_t c_size = sizeof(c_addr);
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	char ip[INET_ADDRSTRLEN];
	ht * ressources = ht_create(); // k: path v: http_ressource
	pthread_t thread;
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
		printf("Setting up impossible\n");
		return 1;
	}
	
	if ( bind(sockfd, (const struct sockaddr *) &s_addr, sizeof(s_addr)) < 0 ) {
		printf("Binding impossible\n");
		close(sockfd);
		return 1;
	} else {
		printf("Binding done\n");
	}
	
	if ( listen(sockfd, 5) < 0) {
		printf("Listening impossible\n");
		close(sockfd);
		return 1;
	} else {
		printf("Listening\n");
	}
	
	if ( ressources == NULL ) {
		printf("Can't load html ressources");
	} else {
		ht_init(ressources);
	}
	
	while ((c_sock = accept(sockfd, (struct sockaddr *) &c_addr, &c_size))) {
		inet_ntop(AF_INET, &(c_addr.sin_addr), ip, INET_ADDRSTRLEN);
		printf("Connection %s:%i\n", ip, c_addr.sin_port);
	
		// TODO implémenter correctement limite de threads
		if (0 < THREAD_LIMIT) {
			arguments_t args;
			args.arg1 = &c_sock;
			args.arg2 = &ressources;
		
			pthread_create(&thread, NULL, session, &args);
		} else {
			printf("Thread limit exceed\n");
		}
	}
	
	printf("Server closed\n");
	close(sockfd);
	return 0;
}
