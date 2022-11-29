#ifndef __SERVER_H__
#define __SERVER_H__

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

/**
 * @brief Struct containing all infos about the server
 * @param epollfd File descriptor returned by epoll_create1()
 * @param listenfd File descriptor returned by socket()
 * @param clientfd Clients' file descriptor array
 * @param usernames Clients' usernames array
*/
typedef struct {
	int epollfd;
	int listenfd;
	int clientfd[MAX_CLIENTS];
	char usernames[MAX_CLIENTS][MAX_USER_LEN];
} Server;

/**
 * @brief Broadcast message to all the clients
 * @param s Server struct
 * @param buf Data to send
 * @param len Size of the data to send
 * @param sender File descriptor of the sender
*/
void broadcast_message(Server* s, void* buf, int len, int sender);

/**
 * @brief Monitor a new fd
 * @param s Server struct
 * @param fd File descriptor to monitor
*/
void server_register_epoll(Server* s, int fd);

/**
 * @brief Check if the username is already taken [TODO]
 * @param s Server struct
 * @param username Username to check
*/
int server_client_check_username(Server* s, char* username);

/**
 * @brief Find the client fd in the fds list
 * @param s Server struct
 * @param fd File descriptor to find
 * @return Returns the file descriptor if found, otherwise -1
*/
int server_client_find(Server* s, int fd);

/**
 * @brief Find an available space for a new client
 * @param s Server struct
 * @return Returns the index of an empty space for a new client, othwerise -1
*/
int server_client_available(Server* s);

/**
 * @brief Register a new incoming connection
 * @param s Server Struct
 * @param fd File descriptor to register
 * @return Returns 0 on success, -1 on error
*/
int server_register_fd(Server* s, int fd);

/**
 * @brief Remove the client from epoll monitoring
 * @param s Server struct
 * @param fd File descriptor to remove
*/
void server_unregister_fd(Server* s, int fd);

/**
 * @brief Remove the client from the file descriptors array
 * @param s Server struct
 * @param fd File descriptor to remove
*/
void server_close_fd(Server* s, int fd);

/**
 * @brief Send an error message with "Server" username
 * @param fd File descriptor where to send the message
 * @param message Message to send
*/
void server_send_error_message(int fd, char* message);

/**
 * @brief Handler to a new connection
 * @param s Server struct
*/
void server_event_connection_new(Server* s);

/**
 * @brief Handler to received messages
 * @param s Server struct
 * @param event Event to handle
*/
void server_event_connection_receive(Server* s, struct epoll_event event);

/**
 * @brief Set the file descriptor to non blocking
 * @param fd File descripto to modify
*/
void setnonblocking(int fd);

void main_loop(struct epoll_event* events, int timeout, Server* s);

#endif
