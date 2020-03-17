#include "threadutils.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

struct MsgList *create_msg_list()
{
	struct MsgList *ret = (struct MsgList *)malloc(sizeof(struct MsgList));
	ret->head = ret->tail = NULL;
	ret->size = 0;
	return ret;
}

void free_msg_list(struct MsgList *list)
{
	if (!list) return;
	struct Msg *p = list->head;
	struct Msg *t;
	for (int i = 0; i < list->size; i++, p = t) {
		t = p->next;
		free_msg(p);
	}
	free(list);
}

void add_msg(struct MsgList *list, struct Msg *msg)
{
	if (!list || !msg) {
		return;
	}
	if (list->size == 0) {
		list->head = list->tail = msg;
	} else {
		list->tail->next = msg;
		list->tail = msg;
	}
	list->size ++;
}

struct Msg *pop_first(struct MsgList *list)
{
	struct Msg *ret = NULL;
	if (!list) {
		return ret;
	}

	ret = list->head;
	if (list->size <= 1) {
		list->head = list->tail = NULL;
	} else {
		list->head = list->head->next;
	}
	list->size --;
	return ret;
}

struct Msg *create_msg(char *data)
{
	struct Msg *msg = (struct Msg *)malloc(sizeof(struct Msg));
	msg->data = data;
	msg->next = NULL;
	return msg;
}

void free_msg(struct Msg *msg)
{
	if (!msg) return;
	free(msg->data);
	free(msg);
}

sem_t *sem_reset(const char *name, unsigned int value)
{
	sem_t *ret = sem_open(name, O_CREAT | O_EXCL, S_IRWXU, value);
	if (ret == SEM_FAILED) {
		sem_unlink(name);
		ret = sem_open(name, O_CREAT, S_IRWXU, value);
	}
	return ret;
}
