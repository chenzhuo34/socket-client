#include "app.h"

#include <stdlib.h>
#include <string.h>

void set_package_size(char *package, int size)
{
	*(int*)package = size;
}

int get_package_size(const char *package)
{
	return *(int*)package;
}

int get_package_data_size(const char *package)
{
	return get_package_size(package) - PACKAGE_HEADER_SIZE;
}

void set_package_type(char *package, char type)
{
	package[PACKAGE_TYPE_OFFSET] = type;
}

int get_package_type(const char *package)
{
	return package[PACKAGE_TYPE_OFFSET];
}

char *get_package_data(char *package)
{
	return package + PACKAGE_HEADER_SIZE;
}

void set_package_data(char *package, const char *data, int size)
{
	set_package_part_data(package, data, size, 0);
}

void set_package_part_data(char *package, const char *data, int size, int offset)
{
	if (size <= get_package_data_size(package) - offset) {
		memcpy(get_package_data(package) + offset, data, size);
	}
}

char *create_request_package(char type)
{
	char *data = (char *)malloc(sizeof(char) * PACKAGE_SIZE_REQUEST);
	set_package_size(data, PACKAGE_SIZE_REQUEST);
	set_package_type(data, PACKAGE_TYPE_REQUEST);
	get_package_data(data)[REQUSET_TYPE_OFFSET] = type;
	return data;
}