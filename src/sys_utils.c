
#include <asm-generic/errno.h>
#include <sys_utils.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

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
    int32_t res = rmdir(path);
    if (errno == ENOTEMPTY) {
        fprintf(stderr, "Directory wasnt empty deleting recursive\n");
        DIR* dir = opendir(path);
        if (!dir) {
            fprintf(stderr, "failed reading dir: %s\n", path);
            return 1;
        }

        struct dirent* entry = NULL;
        while (entry = readdir(dir)) {
            /* only files in this dir */
            fprintf(stderr, "Entry Name: %s\n", entry->d_name);
            res = unlink(entry->d_name);
            if (!res) {
                fprintf(stderr, "Unlink Error\n");
                return 1;
            }
        }
        res = rmdir(path);
    }
}

