#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define MAX_DATA_SIZE 1024

void remove_newline(char* str) {
	while (*str) {
		if (*str == '\n') {
			*str = '\0';
		}
		str++;
	}
}

void* recv_msg(void* ptr) {
	char buf[MAX_DATA_SIZE];
	int rc;
	while (1) {
		rc = recv(*((int*)ptr), buf, MAX_DATA_SIZE, 0);

		if (rc == 0) {
			fprintf(stdout, "Closing connection...\n");
			exit(EXIT_SUCCESS);
		}

		buf[rc] = '\0';
		fprintf(stdout, "%s\n", buf);
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <HOST> <PORT>\n", argv[0]);
		fprintf(stderr, "Write \'quit\' to exit the client.\n");
		exit(EXIT_FAILURE);
	}

	int sockfd;
	struct sockaddr_in server;
	pthread_t recv_msg_t;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &server.sin_addr);

	int rc = connect(sockfd, (struct sockaddr*)&server, sizeof(struct sockaddr));
	if (rc < 0) {
		perror("connect() failed");
		exit(EXIT_FAILURE);
	}

	pthread_create(&recv_msg_t, NULL, recv_msg, (void*)&sockfd);

	char str[MAX_DATA_SIZE];
	while (1) {
		fgets(str, MAX_DATA_SIZE, stdin);

		remove_newline(str);
		if (strcmp(str, "quit") == 0) {
			fprintf(stdout, "Exiting...\n");
			break;
		}

		rc = send(sockfd, str, strlen(str), 0);
		if (rc < 0) {
			perror("send() failed");
			exit(EXIT_FAILURE);
		}
	}

	close(sockfd);
}
