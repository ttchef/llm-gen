
#ifndef SYS_UTILS_H
#define SYS_UTILS_H

#include <stdio.h>
#include <stdint.h>

int32_t create_dir_if_not_exists(const char* path);
int32_t remove_dir(const char* path);
int32_t read_files_in_dir(const char* path, char*** fonts);

#endif // SYS_UTILS_H


