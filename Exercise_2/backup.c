#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void create_hard_link(const char *src, const char *dst) {
    if (link(src, dst) == -1) {
        perror("link");
    }
}

void copy_symlink(const char *src, const char *dst) {
    char target[PATH_MAX];
    ssize_t len = readlink(src, target, sizeof(target) - 1);
    if (len == -1) {
        perror("readlink");
        return;
    }
    target[len] = '\0';

    if (symlink(target, dst) == -1) {
        perror("symlink");
    }
}

void copy_directory(const char *src, const char *dst) {
    DIR *dir = opendir(src);
    if (!dir) {
        perror("opendir");
        return;
    }
    struct stat st;
    if (lstat(src, &st) == -1) {
        perror("lstat");
        return;
    }

    if (mkdir(dst, st.st_mode) == -1) {
        perror("mkdir");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        if (lstat(src_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            copy_directory(src_path, dst_path);
        } else if (S_ISLNK(st.st_mode)) {
            copy_symlink(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            create_hard_link(src_path, dst_path);
        }

        // Apply permissions
        if (!S_ISLNK(st.st_mode)) {
            chmod(dst_path, st.st_mode);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <src_dir> <backup_dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct stat st;

    if (stat(argv[1], &st) == -1 || !S_ISDIR(st.st_mode)) {
        perror("src dir");
        exit(EXIT_FAILURE);
    }

    if (access(argv[2], F_OK) == 0) {
        fprintf(stderr, "backup dir already exists\n");
        perror("backup dir");
        exit(EXIT_FAILURE);
    }

    copy_directory(argv[1], argv[2]);

    return 0;
}