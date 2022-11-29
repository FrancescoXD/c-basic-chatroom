#include "server.h"

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

	// set non blocking socket
	setnonblocking(listen_fd);

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
	close(listen_fd);
}
