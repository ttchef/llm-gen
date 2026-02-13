
#include <asm-generic/errno.h>
#include <linux/limits.h>
#include <sys_utils.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <containers/darray.h>

int32_t create_dir_if_not_exists(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            fprintf(stderr, "failed creating directory %s\n", path);
            return 1;
        }
        fprintf(stderr, "Created directory\n");
    }
    fprintf(stderr, "Dirrectory already exists\n");
}

int32_t remove_dir(const char *path) {
    DIR* dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", path);
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            if (remove_dir(full_path) != 0) {
                closedir(dir);
                return -1; 
            }
        } else {
            if (unlink(full_path) != 0) {
                fprintf(stderr, "Failed to unlink file: %s\n", full_path);
                closedir(dir);
                return -1;
            }
        }
    }
    closedir(dir);

    if (rmdir(path) != 0) {
        fprintf(stderr, "Failed to remove directory: %s\n", path);
        return -1; 
    }

    return 0; 
}

int32_t read_files_in_dir(const char *path, char ***fonts) {
    DIR* dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Failed to open direcotry: %s\n", path);
        return -1;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char* full_path = malloc(PATH_MAX);
        snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);
        darrayPush(*fonts, full_path);
    }
    closedir(dir);

    return 0;
}

