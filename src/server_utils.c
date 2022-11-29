#include "server.h"

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

int server_client_check_username(Server* s, char* username) {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (strcmp(s->usernames[i], username) == 0) {
			return 1;
		}
	}

	return -1;
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

	// TODO register the username

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

void server_send_error_message(int fd, char* message) {
	Message msg;
	strcpy(msg.username, "Server");
	strcpy(msg.message, message);
	send(fd, (void*)&msg, sizeof msg, 0);
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
	// server_client_check_username()
	rc = server_register_fd(s, new_fd);
	if (rc < 0) {
		fprintf(stderr, "Server is full! Rejected %d\n", new_fd);
		server_send_error_message(new_fd, "No space available for new clients!");
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

void setnonblocking(int fd) {
    int rc = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (rc < 0) {
		perror("fcntl() failed");
		close(fd);
		exit(EXIT_FAILURE);
	}
}
