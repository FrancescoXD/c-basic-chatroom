#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 20
#define MAX_CLIENTS 20
#define MAX_DATA_SIZE 1024
#define MAX_USER_LEN 30

typedef struct {
	char username[MAX_USER_LEN];
	char message[MAX_DATA_SIZE];
} Message;

typedef struct {
	int epollfd;
	int listenfd;
	int clientfd[MAX_CLIENTS];
} Server;

void broadcast_message(Server* s, void* buf, int len, int sender) {
	int rc;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (s->clientfd[i] != -1 && s->clientfd[i] != sender) {
			rc = send(s->clientfd[i], buf, len, 0);
			fprintf(stdout, "Bytes sent: %d\n", rc);
			if (rc < 0) {
				perror("send() failed");
			}
		}
	}
}

void server_register_epoll(Server* s, int fd) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	int rc = epoll_ctl(s->epollfd, EPOLL_CTL_ADD, fd, &event);
	if (rc < 0) {
		perror("epoll_ctl() failed");
		exit(EXIT_FAILURE);
	}
}

int server_client_find(Server* s, int fd) {
	for (unsigned i = 0; i < MAX_CLIENTS; ++i) {
		if (s->clientfd[i] == fd) return i;
	}

	return -1;
}

int server_client_available(Server* s) {
	for (unsigned i = 0; i < MAX_CLIENTS; ++i) {
		if (s->clientfd[i] == -1) return i;
	}

	return -1;
}

int server_register_fd(Server* s, int fd) {
	server_register_epoll(s, fd);
	int i = server_client_available(s);
	if (i < 0) {
		return -1;
	}
	s->clientfd[i] = fd;

	return 0;
}

void server_unregister_fd(Server* s, int fd) {
	int rc = epoll_ctl(s->epollfd, EPOLL_CTL_DEL, fd, 0);
	if (rc < 0) {
		perror("epoll_ctl() failed");
		exit(EXIT_FAILURE);
	}
}

void server_close_fd(Server* s, int fd) {
	server_unregister_fd(s, fd);
	int i = server_client_find(s, fd);
	if (i < 0) {
		perror("Cannot find the client");
		return;
	}
	s->clientfd[i] = -1;
	close(fd);
}

void server_event_connection_new(Server* s) {
	int new_fd = accept(s->listenfd, NULL, NULL);
	if (new_fd < 0) {
		perror("accept() failed");
		return;
	}

	int rc = fcntl(new_fd, F_SETFL, O_NONBLOCK);
	if (rc < 0) {
		perror("fcntl() failed");
		return;
	}

	// monitor the incoming connection
	fprintf(stdout, "New incoming connection: %d\n", new_fd);
	rc = server_register_fd(s, new_fd);
	if (rc < 0) {
		fprintf(stderr, "Server is full! Rejected %d\n", new_fd);
		Message msg;
		strcpy(msg.username, "Server");
		strcpy(msg.message, "No space available for new clients");
		send(new_fd, (void*)&msg, sizeof msg, 0);
		close(new_fd);
	}
}

void server_event_connection_receive(Server* s, struct epoll_event event) {
	int close_conn = false;
	Message msg;

	int rc = recv(event.data.fd, (void*)&msg, sizeof msg, 0);
	if (rc < 0) {
		perror("recv() failed");
		close_conn = true;
	}

	if (rc == 0) {
		fprintf(stdout, "Connection closed\n");
		close_conn = true;
	}

	if (close_conn) {
		server_close_fd(s, event.data.fd);
		return;
	}
			
	// data was received
	fprintf(stdout, "Bytes receved: %d\n", rc);
	fprintf(stdout, "Sending to all clients...\n");
	broadcast_message(s, (void*)&msg, rc, event.data.fd);
}

void main_loop(struct epoll_event* events, int timeout, Server* s) {
	int end_server = false;
	int event_count;

	do {
		fprintf(stdout, "Polling...\n");
		event_count = epoll_wait(s->epollfd, events, MAX_EVENTS, timeout);
		if (event_count < 0) {
			perror("poll() failed");
			break;
		}

		// check if 3 minutes are elapsed
		if (event_count == 0) {
			fprintf(stderr, "poll() timed out!\n");
			break;
		}

		fprintf(stdout, "%d ready events\n", event_count);
		for (int i = 0; i < event_count; ++i) {
			if (events[i].data.fd == s->listenfd) {
				fprintf(stdout, "Listening socket is available\n");
				server_event_connection_new(s);
			} else {
				fprintf(stdout, "Descriptor is readable: %d\n", events[i].data.fd);
				server_event_connection_receive(s, events[i]);
			}
		}
	} while (end_server == false);
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <HOST> <PORT>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int rc, on = 1;
	int listen_fd;
	struct sockaddr_in addr;
	struct epoll_event event, events[MAX_EVENTS];
	int epoll_fd;
	Server server;
	memset(server.clientfd, -1, sizeof server.clientfd);

	listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	// allow socket to be reusable
	rc = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	if (rc < 0) {
		perror("setsockopt() failed");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	// set non blocking socket (POSIX way)
	rc = fcntl(listen_fd, F_SETFL, O_NONBLOCK);
	if (rc < 0) {
		perror("fcntl() failed");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	// bind the socket
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &addr.sin_addr);
	rc = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
	if (rc < 0) {
		perror("bind() failed");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	// listen
	rc = listen(listen_fd, 32);
	if (rc < 0) {
		perror("listen() failed");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}
	fprintf(stdout, "Server is listening...\n");

	// epoll
	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		perror("epoll_create1() failed");
		exit(EXIT_FAILURE);
	}
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = listen_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
	int timeout = 3 * 60 * 1000;

	server.listenfd = listen_fd;
	server.epollfd = epoll_fd;

	main_loop(events, timeout, &server);

	if (close(epoll_fd)) {
		perror("close(epoll_fd) failed");
		exit(EXIT_FAILURE);
	}
}
