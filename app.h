#pragma once

#define PACKAGE_HEADER_SIZE 5
#define PACKAGE_TYPE_OFFSET 4
#define PACKAGE_TYPE_REQUEST 0
#define PACKAGE_TYPE_RESPOSE 1
#define PACKAGE_TYPE_INST 2

#define PACKAGE_SIZE_REQUEST 6
#define REQUSET_TYPE_OFFSET 0
#define REQUSET_TYPE_TIME 0
#define REQUSET_TYPE_NAME 1
#define REQUSET_TYPE_LIST 2

#define RESPONSE_TYPE_OFFSET 0
#define RESPONSE_CONTENT_OFFSET 1
#define RESPONSE_TYPE_TIME 0
#define RESPONSE_TYPE_NAME 1
#define RESPONSE_TYPE_LIST 2
#define RESPONSE_TYPE_SUCC 3
#define RESPONSE_TYPE_FAIL 4

#define INST_NAME_SIZE 4
#define INST_MAX_MESSAGE_SIZE 256

void set_package_size(char *package, int size);
int get_package_size(const char *package);
int get_package_data_size(const char *package);
void set_package_type(char *package, char type);
int get_package_type(const char *package);
char *get_package_data(char *package);
void set_package_data(char *package, const char *data, int size);
void set_package_part_data(char *package, const char *data, int size, int offset);

char *create_request_package(char type);

typedef struct _C_DATA_ {
	int name;
	int port;
	char addr[16];
} ClientData;
