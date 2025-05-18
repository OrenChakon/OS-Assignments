#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#define MAX_PATH 1024
#define MAX_FILENAME 256
#define MAX_FILES 100

// Function prototypes
void print_usage(const char *program_name);
int check_directory(const char *path);
void create_directory(const char *path);
void sync_files(const char *source_dir, const char *dest_dir);
int compare_files(const char *file1, const char *file2);
void copy_file(const char *source, const char *dest);
int is_regular_file(const char *path);

int main(int argc, char *argv[]) {
    char current_dir[MAX_PATH];
    
    // Get and print current working directory
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("getcwd failed");
        return 1;
    }
    printf("Current working directory: %s\n", current_dir);

    // Check command line arguments
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char *source_dir = argv[1];
    const char *dest_dir = argv[2];

    // Check if source directory exists
    if (!check_directory(source_dir)) {
        printf("Error: Source directory '%s' does not exist.\n", source_dir);
        return 1;
    }

    // Check/create destination directory
    if (!check_directory(dest_dir)) {
        create_directory(dest_dir);
        printf("Created destination directory '%s'.\n", dest_dir);
    }

    printf("Synchronizing from %s to %s\n", source_dir, dest_dir);
    sync_files(source_dir, dest_dir);
    printf("Synchronization complete.\n");

    return 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s <source_directory> <destination_directory>\n", program_name);
}

int check_directory(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

void create_directory(const char *path) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("/bin/mkdir", "mkdir", path, NULL);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    }
}

int is_regular_file(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

int compare_files(const char *file1, const char *file2) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd, STDOUT_FILENO);
        close(null_fd);
        execl("/usr/bin/diff", "diff", "-q", file1, file2, NULL);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        int status;
        wait(&status);
        return WEXITSTATUS(status);
    }
    return -1;
}

void copy_file(const char *source, const char *dest) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("/bin/cp", "cp", source, dest, NULL);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    }
}

void sync_files(const char *source_dir, const char *dest_dir) {
    DIR *dir;
    struct dirent *entry;
    char source_path[MAX_PATH];
    char dest_path[MAX_PATH];
    struct stat source_stat, dest_stat;

    dir = opendir(source_dir);
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct full paths
        snprintf(source_path, MAX_PATH, "%s/%s", source_dir, entry->d_name);
        snprintf(dest_path, MAX_PATH, "%s/%s", dest_dir, entry->d_name);

        // Only process regular files
        if (!is_regular_file(source_path)) {
            continue;
        }

        // Check if file exists in destination
        if (stat(dest_path, &dest_stat) != 0) {
            // File doesn't exist in destination
            printf("New file found: %s\n", entry->d_name);
            copy_file(source_path, dest_path);
            printf("Copied: %s -> %s\n", source_path, dest_path);
        } else {
            // File exists in both directories
            if (stat(source_path, &source_stat) != 0) {
                perror("stat failed");
                continue;
            }

            int diff_result = compare_files(source_path, dest_path);
            if (diff_result == 0) {
                printf("File %s is identical. Skipping...\n", entry->d_name);
            } else if (diff_result == 1) {
                if (source_stat.st_mtime > dest_stat.st_mtime) {
                    printf("File %s is newer in source. Updating...\n", entry->d_name);
                    copy_file(source_path, dest_path);
                    printf("Copied: %s -> %s\n", source_path, dest_path);
                } else {
                    printf("File %s is newer in destination. Skipping...\n", entry->d_name);
                }
            }
        }
    }

    closedir(dir);
}
