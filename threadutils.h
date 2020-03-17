#pragma once

#include <semaphore.h>

struct Msg {
	char *data;
	struct Msg *next;
};

struct MsgList {
	int size;
	struct Msg *head;
	struct Msg *tail;
};

struct MsgList *create_msg_list();
void free_msg_list(struct MsgList *list);
void add_msg(struct MsgList *list, struct Msg *msg);
struct Msg *pop_first(struct MsgList *list);
struct Msg *create_msg(char *data);
void free_msg(struct Msg *msg);

sem_t *sem_reset(const char *name, unsigned int value);