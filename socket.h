#pragma once

#include "threadutils.h"

#define MAX_PACKAGE_SIZE_ONCE 64

int do_connect();
int do_disconnect();
int do_list_msg();
int do_request(char type);
int do_send_message();
void * do_receive();
void release_connect();

struct sock_package {
	int size;
	char data[MAX_PACKAGE_SIZE_ONCE];
};

struct sock_package *create_sock_package(const char *data, int size);

void send_package(int socket, char *data, int size);

extern char connect_actions[][16];
extern const int connect_action_num;
extern char disconnect_actions[][16];
extern const int disconnect_action_num;

extern int is_connected;
extern int client_sockfd;

extern pthread_t recv_thread;
extern struct MsgList *recv_list;
extern sem_t *recv_list_sem;
extern char recv_list_sem_name[];
extern struct Msg *recv_msg;
extern sem_t *recv_msg_sem;
extern char recv_msg_sem_name[];

void print_connected_actions();
void print_disconnected_actions();