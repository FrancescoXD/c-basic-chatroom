#ifndef __CLIENT_H__
#define __CLIENT_H__

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

/**
 * @brief Remove the newline character from a string
 * @param str
*/
void remove_newline(char* str);
void* recv_msg(void* ptr);
void send_username(int fd, char* username);

#endif
