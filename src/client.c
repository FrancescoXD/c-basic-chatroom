#include "client.h"

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

void send_username(int fd, char* username) {
	int rc = send(fd, username, sizeof username, 0);
	if (rc < 0) {
		perror("send() failed");
		exit(EXIT_FAILURE);
	}

	char* rcvmsg[MAX_DATA_SIZE];
	rc = recv(fd, rcvmsg, MAX_DATA_SIZE, 0);
	if (rc == 0) {
		fprintf(stderr, "Username already taken!");
		exit(EXIT_FAILURE);
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
	Message msg;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &server.sin_addr);

	char username[MAX_USER_LEN];
	fprintf(stdout, "Username: ");
	fgets(username, MAX_USER_LEN, stdin);
	remove_newline(username);
	strcpy(msg.username, username);

	int rc = connect(sockfd, (struct sockaddr*)&server, sizeof(struct sockaddr));
	if (rc < 0) {
		perror("connect() failed");
		exit(EXIT_FAILURE);
	}

	//send_username(sockfd, msg.message);

	pthread_create(&recv_msg_t, NULL, recv_msg, (void*)&sockfd);

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
