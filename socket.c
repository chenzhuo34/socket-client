#include "socket.h"
#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

char connect_actions[][16] = {
		"Disconnect",
		"Quit",
		"List messages",
		"Get time",
		"Get name",
		"Get client list",
		"Send message"
};
const int connect_action_num = sizeof(connect_actions) / sizeof(connect_actions[0]);
char disconnect_actions[][16] = {
	"Connect",
	"Quit"
};
const int disconnect_action_num = sizeof(disconnect_actions) / sizeof(disconnect_actions[0]);

int is_connected;
int client_sockfd;

pthread_t recv_thread;
struct MsgList *recv_list;
sem_t *recv_list_sem;
char recv_list_sem_name[] = "recv_list";
struct Msg *recv_msg;
sem_t *recv_msg_sem;
char recv_msg_sem_name[] = "recv_msg";
sem_t *recv_msg_sem_waiting;
char recv_msg_sem_waiting_name[] = "recv_msg_waiting";


struct sock_package *create_sock_package(const char *data, int size)
{
	if (size > MAX_PACKAGE_SIZE_ONCE) {
		return NULL;
	}
	struct sock_package *ret = (struct sock_package *)malloc(sizeof(struct sock_package));
	ret->size = size;
	memset(ret->data, 0, MAX_PACKAGE_SIZE_ONCE);
	memcpy(ret->data, data, ret->size);
	return ret;
}

int do_connect()
{
	// input ip address, port
	char ip[16] = {0};
	// char ip[16] = "10.180.120.187";
	while (1) {
		printf("Server IP: ");
		int ip0, ip1, ip2, ip3;
		scanf("%d.%d.%d.%d", &ip0, &ip1, &ip2, &ip3);
		if ((ip0 < 0 || ip0 > 255) || (ip1 < 0 || ip1 > 255) || (ip2 < 0 || ip2 > 255) || (ip3 < 0 || ip3 > 255)) {
			printf("Invalid IP address!\n");
		} else {
			char buf[4];
			sprintf(buf, "%d", ip0);
			strcat(ip, buf);
			strcat(ip, ".");
			sprintf(buf, "%d", ip1);
			strcat(ip, buf);
			strcat(ip, ".");
			sprintf(buf, "%d", ip2);
			strcat(ip, buf);
			strcat(ip, ".");
			sprintf(buf, "%d", ip3);
			strcat(ip, buf);
			break;
		}
	}
	printf("Server Port: ");
	int port = 5392;
	scanf("%d", &port);

	struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;				// set family
    remote_addr.sin_addr.s_addr = inet_addr(ip);	// set ip address
    remote_addr.sin_port = htons(port);				// set port

	client_sockfd = socket(AF_INET, SOCK_STREAM, 0);// create socket
	int connect_res = connect(client_sockfd, (const struct sockaddr *) &remote_addr, sizeof(struct sockaddr));
	if (connect_res < 0) {
		printf("Connect to %s:%d failed.\n", ip, port);
		return 0;
	}

	// init semaphores
	recv_list_sem = sem_reset(recv_list_sem_name, 1);
	recv_msg_sem = sem_reset(recv_msg_sem_name, 0);
	recv_msg_sem_waiting = sem_reset(recv_msg_sem_waiting_name, 0);

	// create receive thread
	recv_list = create_msg_list();
	int res = pthread_create(&recv_thread, NULL, do_receive, NULL);
	if (res) {
		printf("Create receive thread failed.\n");
		close(client_sockfd);
		release_connect();
		return 0;
	}

	// set state
	is_connected = 1;
	printf("Connect succeed.\n");
	return 1;
}

void release_connect()
{
	// release resources
	sem_close(recv_list_sem);
	sem_unlink(recv_list_sem_name);
	free_msg_list(recv_list);
	
	sem_close(recv_msg_sem);
	sem_unlink(recv_msg_sem_name);

	sem_close(recv_msg_sem_waiting);
	sem_unlink(recv_msg_sem_waiting_name);
}

int do_disconnect()
{
	if (!is_connected) {
		return 0;
	}

	// close the socket
	close(client_sockfd);
	// reset state	
	is_connected = 0;
	// remove the receive thread
	pthread_join(recv_thread, NULL);
	
	return 1;
}

void * do_receive()
{
	char *data = NULL;
	int recv_length = 0;
	struct sock_package *p = (struct sock_package *)malloc(sizeof(struct sock_package));

	while (1) {
		if (!is_connected) break;
		int res = recv(client_sockfd, p, sizeof(struct sock_package), MSG_DONTWAIT);
		if (res > 0) {
			/**
			 * If this is the first segment of the package,
			 * allocate memory
			 */
			if (!data) {
				int total_length = get_package_size(p->data);
				data = (char *)malloc(sizeof(char) * total_length);
				recv_length = 0;
			}
			/**
			 * If this is the last segment of the package, (p->size > MAX_PACKAGE_SIZE_ONCE)
			 * handle this full package
			 */
			if (p->size > MAX_PACKAGE_SIZE_ONCE) {
				memcpy(data + recv_length, p->data, MAX_PACKAGE_SIZE_ONCE);
				recv_length += MAX_PACKAGE_SIZE_ONCE;
			} else {
				memcpy(data + recv_length, p->data, p->size);
				recv_length += p->size;
				struct Msg *msg = create_msg(data);
				/**
				 * If this is a response sock_package,
				 * main thread use it immediately,
				 * else enqueue it.
				 */
				if (get_package_type(data) == PACKAGE_TYPE_RESPOSE) {
					sem_wait(recv_msg_sem_waiting);
					recv_msg = msg;
					sem_post(recv_msg_sem);
				} else {
					sem_wait(recv_list_sem);
					add_msg(recv_list, msg);
					sem_post(recv_list_sem);
				}
				data = NULL;
			}
		} else if (res == 0) {
			printf("\n\nServer disconnected.\n");
			is_connected = 0;
			print_disconnected_actions();
			break;
		} else {
			if (errno == ETIMEDOUT) {
				/**
				 * No package
				 */
				continue;
			} else if (errno == ECONNREFUSED) {
				/**
				 * Server unreachable
				 */
				printf("\n\n跑路了\n> ");
				is_connected = 0;
				print_disconnected_actions();
				break;
			}
		}
	}
	release_connect();
	free(p);
	return NULL;
}

int do_list_msg()
{
	sem_wait(recv_list_sem);
	for (int i = 0; i < recv_list->size; i++) {
		printf(">>>>>>>>>>>>\n");
		struct Msg *msg = pop_first(recv_list);
		char *data = msg->data;
		data = get_package_data(data);
		printf("%d =>\n", *(int*)data);
		printf("%s\n", data + INST_NAME_SIZE);
		free_msg(msg);
		printf("<<<<<<<<<<<<\n");
	}
	sem_post(recv_list_sem);
	return 1;
}

int do_request(char type)
{
	// create packages
	char *data = create_request_package(type);
	// send
	send_package(client_sockfd, data, PACKAGE_SIZE_REQUEST);
	free(data);
	printf("Request sent. Waiting response ...\n\n");

	sem_post(recv_msg_sem_waiting);
	// receive response
	sem_wait(recv_msg_sem);
	data = recv_msg->data;
	char response_type = get_package_data(data)[RESPONSE_TYPE_OFFSET];
	char *content = get_package_data(data) + RESPONSE_CONTENT_OFFSET;
	/**
	 * print the package according to the response type
	 */
	if (response_type == REQUSET_TYPE_TIME) {
		printf("%s\n", content);
	} else if (response_type == REQUSET_TYPE_NAME) {
		printf("%s\n", content);
	} else if (response_type == REQUSET_TYPE_LIST) {
		int num = *(int *)content;
		content += sizeof(int);
		ClientData *cd;
		printf("\tname\taddress\t\tport\n");
		for (int i = 0; i < num; i++) {
			cd = (ClientData *)content;
			printf("%d", i);
			if (i == 0)
				printf(" *");
			printf("\t%d\t%s\t%d\n", cd->name, cd->addr, cd->port);
			content += sizeof(ClientData);
		}
	}
	free_msg(recv_msg);

	return 1;
}

int do_send_message()
{
	int number = 0;
	char mes[INST_MAX_MESSAGE_SIZE + 1] = {0};
	char tmp[INST_MAX_MESSAGE_SIZE + 1];
	// get client number
	printf("send to: ");
	scanf("%d", &number);
	// get message
	printf("message:\n=> ");
	scanf("%s", tmp);
	int mes_size = 0;
	while (tmp[strlen(tmp) - 1] == '\\') {
		printf(" > ");
		tmp[strlen(tmp) - 1] = '\0';
		char *t = tmp;
		char *p = tmp;
		char *q;
		while ((q = strstr(p, "\\"))) {
			if (q[1] == '\\') {
				q[1] = '\0';
				int t_size = strlen(t);
				if (mes_size + t_size > INST_MAX_MESSAGE_SIZE)
					goto mes_input_done;
				strcat(mes, t);
				mes_size += t_size;
				t = p = q + 2;
			} else {
				q[0] = '\n';
				p = q + 1;
			}
		}
		int t_size = strlen(t);
		if (mes_size + t_size > INST_MAX_MESSAGE_SIZE)
			goto mes_input_done;
		strcat(mes, t);
		mes_size += t_size;
		scanf("%s", tmp);
	}
	int tmp_size = strlen(tmp);
	if (mes_size + tmp_size > INST_MAX_MESSAGE_SIZE)
		goto mes_input_done;
	strcat(mes, tmp);
	mes_size += tmp_size;

mes_input_done:
	printf("message size: %d\n", mes_size);
	printf("%s\n", mes);
	int number_size = sizeof(int);
	int total_size = PACKAGE_HEADER_SIZE + number_size + mes_size + 1;
	// create send package
	char *data = (char *)malloc(sizeof(char) * total_size);
	set_package_size(data, total_size);
	set_package_type(data, PACKAGE_TYPE_INST);
	set_package_part_data(data, (char *)&number, number_size, 0);
	set_package_part_data(data, mes, mes_size, number_size);

	// send the package
	send_package(client_sockfd, data, total_size);
	free(data);

	// wait response
	sem_wait(recv_msg_sem);
	data = recv_msg->data;
	char response_type = get_package_data(data)[RESPONSE_TYPE_OFFSET];
	if (response_type == RESPONSE_TYPE_SUCC) {
		printf("Message sent.\n");
	} else {
		printf("Message sent failed.\n");
	}
	free_msg(recv_msg);

	return 1;
}

void send_package(int socket, char *data, int size)
{
	int sent_size = 0;
	int full = 0;
	while (size) {
		int this_size;
		full = 0;
		if (size > MAX_PACKAGE_SIZE_ONCE) {
			this_size = MAX_PACKAGE_SIZE_ONCE;
			full = 1;
		} else {
			this_size = size;
		}
		struct sock_package *p = create_sock_package(data + sent_size, this_size);
		if (full) p->size++;
		int res = send(socket, (char *)p, sizeof(struct sock_package), 0);
		// printf("sent %d\n", res);
		free(p);
		size -= this_size;
		sent_size += this_size;
	}
}

void print_connected_actions()
{
	printf("\nPlease select an action. Choose with numbers.\n");
	for (int i = 0; i < connect_action_num; i++) {
		printf("\t%d) ", i);
		printf("%s", connect_actions[i]);
		printf("\n");
	}
	printf("\n> ");
	fflush(stdout);
}

void print_disconnected_actions()
{
	printf("\nPlease select an action. Choose with numbers.\n");
	for (int i = 0; i < disconnect_action_num; i++) {
		printf("\t%d) ", i);
		printf("%s", disconnect_actions[i]);
		printf("\n");
	}
	printf("\n> ");
	fflush(stdout);
}