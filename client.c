#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define PORT 8080
#define DATA_SIZE 1024

void* recv_msg(void* ptr) {
	char buf[DATA_SIZE];
	int rc;
	while (1) {
		rc = recv(*((int*)ptr), buf, DATA_SIZE, 0);

		if (rc == 0) {
			fprintf(stdout, "No data from server, exiting...\n");
			exit(EXIT_SUCCESS);
		}

		buf[rc] = '\0';
	}
}

int main(void) {
	int sockfd;
	struct sockaddr_in server;
	//int numbytes;
	//char buf[DATA_SIZE];
	pthread_t recv_msg_t;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	inet_aton("127.0.0.1", &server.sin_addr);

	int rc = connect(sockfd, (struct sockaddr*)&server, sizeof(struct sockaddr));
	if (rc < 0) {
		perror("connect() failed");
		exit(EXIT_FAILURE);
	}

	pthread_create(&recv_msg_t, NULL, recv_msg, (void*)&sockfd);
	//pthread_join(recv_msg_t, NULL);

	char str[DATA_SIZE];
	while (1) {
		fprintf(stdout, "Send data to the server: ");
		fgets(str, DATA_SIZE, stdin);
		rc = send(sockfd, str, strlen(str), 0);
		if (rc < 0) {
			perror("send() failed");
			exit(EXIT_FAILURE);
		}

		/*numbytes = recv(sockfd, buf, DATA_SIZE, 0);
		if (numbytes < 0) {
			perror("recv() failed");
			exit(EXIT_FAILURE);
		}
		buf[numbytes] = '\0';
		fprintf(stdout, "received %d bytes:\n%s", numbytes, buf);*/
		
		str[strcspn(str, "\n")] = '\0';
		if (strcmp(str, "quit") == 0) {
			fprintf(stdout, "Exiting...\n");
			break;
		}
	}

	close(sockfd);
}
