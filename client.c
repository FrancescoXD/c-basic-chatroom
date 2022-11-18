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
#define MAX_USER_LEN 30

typedef struct {
	char username[MAX_USER_LEN];
	char message[MAX_DATA_SIZE];
} Message;

void remove_newline(char* str) {
	while (*str) {
		if (*str == '\n') {
			*str = '\0';
		}
		str++;
	}
}

void* recv_msg(void* ptr) {
	Message msg;
	int rc;
	while (1) {
		rc = recv(*((int*)ptr), (void*)&msg, sizeof msg, 0);

		if (rc == 0) {
			fprintf(stdout, "Closing connection...\n");
			exit(EXIT_SUCCESS);
		}

		fprintf(stdout, "%s: %s\n", msg.username, msg.message);
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

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
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

	Message msg;

	char username[MAX_USER_LEN];
	fprintf(stdout, "Username: ");
	fgets(username, MAX_USER_LEN, stdin);
	remove_newline(username);
	strcpy(msg.username, username);

	char str[MAX_DATA_SIZE];
	while (1) {
		fgets(str, MAX_DATA_SIZE, stdin);

		remove_newline(str);
		if (strcmp(str, "quit") == 0) {
			fprintf(stdout, "Exiting...\n");
			break;
		}
		strcpy(msg.message, str);

		rc = send(sockfd, (void*)&msg, sizeof msg, 0);
		if (rc < 0) {
			perror("send() failed");
			exit(EXIT_FAILURE);
		}
	}

	close(sockfd);
}
