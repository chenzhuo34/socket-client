#include "app.h"
#include "socket.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	printf("Welcome to CY disco!\n");
	is_connected = 0;
	while (1) {
		if (is_connected) {
			print_connected_actions();
		} else {
			print_disconnected_actions();
		}
		int action;
		scanf("%d", &action);
		printf("\n");
		if (is_connected) {
			if (action == 0) {
				do_disconnect();
			} else if (action == 1) {
				do_disconnect();
				break;
			} else if (action == 2) {
				do_list_msg();
			} else if (action == 3) {
				do_request(REQUSET_TYPE_TIME);
			} else if (action == 4) {
				do_request(REQUSET_TYPE_NAME);
			} else if (action == 5) {
				do_request(REQUSET_TYPE_LIST);
			} else if (action == 6) {
				do_send_message();
			} else {
				printf("Undefined action, please choose again.\n");
			}
		} else {
			if (action == 0) {
				do_connect();
			} else if (action == 1) {
				break;
			} else {
				printf("Undefined action, please choose again.\n");
			}
		}
	}
	
	return 0;
}
