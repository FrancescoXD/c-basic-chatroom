#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0
#define MAX_DATA_SIZE 1024
#define MAX_USER_LEN 30

typedef struct {
	char username[MAX_USER_LEN];
	char message[MAX_DATA_SIZE];
} Message;

void compress_array(int* compress_array_c, int* nfds, struct pollfd* fds) {
	*compress_array_c = FALSE;
	for (int i = 0; i < *nfds; ++i) {
		if (fds[i].fd == -1) {
			for (int j = i; j < *nfds; ++j) {
				fds[j].fd = fds[j+1].fd;
			}
			i--;
			(*nfds)--;
		}
	}
}

void close_fds(struct pollfd* fds, int* nfds) {
	for (int i = 0; i < *nfds; ++i) {
		close(fds[i].fd);
	}
}

void main_loop(int* nfds, int timeout, struct pollfd* fds, int listen_fd) {
	int len, rc;
	int new_sd; // new server descriptor (incoming connection)
	int end_server = FALSE;
	int close_conn;
	int compress_array_c;

	Message msg;

	do {
		fprintf(stdout, "Waiting on poll()...\n");
		rc = poll(fds, *nfds, timeout);
		if (rc < 0) {
			perror("poll() failed");
			break;
		}

		// check if 3 minutes are elapsed
		if (rc == 0) {
			fprintf(stderr, "poll() timed out!\n");
			break;
		}

		int current_size = *nfds;
		for (int i = 0; i < current_size; ++i) {
			if (fds[i].revents == 0)
				continue;

			if (fds[i].revents != POLLIN) {
				fprintf(stderr, "Error revents: %d\n", fds[i].revents);
				end_server = TRUE;
				break;
			}

			if (fds[i].fd == listen_fd) {
				fprintf(stdout, "Listening socket is available\n");
				do {
					new_sd = accept(listen_fd, NULL, NULL);
					if (new_sd < 0) {
						if (errno != EWOULDBLOCK) {
							perror("accept() failed");
							end_server = TRUE;
						}
						break;
					}

					rc = fcntl(new_sd, F_SETFL, O_NONBLOCK);
					if (rc < 0) {
						perror("fcntl() failed");
						end_server = TRUE;
					}

					// add the incoming connection to pollfd structure
					fprintf(stdout, "New incoming connection: %d\n", new_sd);
					fds[*nfds].fd = new_sd;
					fds[*nfds].events = POLLIN;
					(*nfds)++;
				} while (new_sd != -1);
			} else {
				fprintf(stdout, "Descriptor is readable: %d\n", fds[i].fd);
				close_conn = FALSE;

				do {
					rc = recv(fds[i].fd, (void*)&msg, sizeof msg, 0);
					if (rc < 0) {
						if (errno != EWOULDBLOCK) {
							perror("recv() failed");
							close_conn = TRUE;
						}
						break;
					}

					if (rc == 0) {
						fprintf(stdout, "Connection closed\n");
						close_conn = TRUE;
						break;
					}
			
					// data was received
					len = rc;
					fprintf(stdout, "Bytes receved: %d\n", len);
					fprintf(stdout, "Sending to all clients...\n");

					int sending_socket = fds[i].fd;
					for (int k = 1; k < current_size; ++k) {
						if (fds[k].fd != sending_socket) {
							//fprintf(stdout, "sockfd: %d\n", fds[k].fd);
							//fprintf(stdout, "Buffer (%d bytes):\nusername: %s\nmessage: %s\n", len, msg.username, msg.message);
							rc = send(fds[k].fd, (void*)&msg, len, 0);
							fprintf(stdout, "Bytes sent: %d\n", rc);
							if (rc < 0) {
								perror("send() failed");
								close_conn = TRUE;
								break;
							}
						}
					}
				} while (TRUE);

				if (close_conn) {
					close(fds[i].fd);
					fds[i].fd = -1;
					compress_array_c = TRUE;
				}
			}
		}

		if (compress_array_c) {
			compress_array(&compress_array_c, nfds, fds);
		}
	} while (end_server == FALSE);
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <HOST> <PORT>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int rc, on = 1;
	int listen_fd; // listen server descriptor

	struct sockaddr_in addr;
	struct pollfd fds[200];
	int nfds = 1;

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

	// initialize the pollfd structure
	memset(fds, 0, sizeof(fds));

	// set up the initial socket
	fds[0].fd = listen_fd;
	fds[0].events = POLLIN;
	// set up timeout (if no activiy the program will stop)
	int timeout = (3 * 60 * 1000);

	main_loop(&nfds, timeout, fds, listen_fd);

	close_fds(fds, &nfds);
}
