#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#define MAX_LINE_LENGTH 1024

void process_read_command(int data_file, int results_file, off_t start_offset, off_t end_offset) {
    if (start_offset < 0 || end_offset < start_offset) {
        fprintf(stderr, "Invalid read range: start=%ld end=%ld\n", start_offset, end_offset);
        return;
    }

    off_t read_length = end_offset - start_offset + 1;
    off_t file_size = lseek(data_file, 0, SEEK_END);
    if (start_offset >= file_size) {
        fprintf(stderr, "Start offset beyond file size: start=%ld size=%ld\n", start_offset, file_size);
        return;
    }

    if (end_offset >= file_size) {
        fprintf(stderr, "End offset beyond file size: end=%ld size=%ld\n", end_offset, file_size);
        read_length = file_size - start_offset;
        return;
    }

    if (lseek(data_file, start_offset, SEEK_SET) == -1) {
        perror("lseek failed in read");
        return;
    }

    char *read_buffer = malloc(read_length);
    if (!read_buffer) {
        fprintf(stderr, "Memory allocation failed in read\n");
        return;
    }

    ssize_t bytes_read = read(data_file, read_buffer, read_length);
    if (bytes_read == -1) {
        perror("read failed");
        free(read_buffer);
        return;
    }

    write(results_file, read_buffer, bytes_read);
    write(results_file, "\n", 1);

    free(read_buffer);
}

void process_write_command(int data_file, off_t insert_offset, const char *insert_text) {
    size_t insert_len = strlen(insert_text);
    if (insert_len == 0) {
        fprintf(stderr, "Empty insert text\n");
        return;
    }

    off_t file_size = lseek(data_file, 0, SEEK_END);
    if (insert_offset < 0 || insert_offset > file_size) {
        fprintf(stderr, "Invalid insert offset: %ld (file size: %ld)\n", insert_offset, file_size);
        return;
    }

    if (insert_offset == file_size) {
        if (lseek(data_file, insert_offset, SEEK_SET) == -1) {
            perror("lseek failed at end write");
            return;
        }
        write(data_file, insert_text, insert_len);
        return;
    }

    size_t remaining_len = file_size - insert_offset;
    char *str_end = malloc(remaining_len);
    if (!str_end) {
        fprintf(stderr, "Memory allocation failed in write\n");
        return;
    }

    if (lseek(data_file, insert_offset, SEEK_SET) == -1 || read(data_file, str_end, remaining_len) == -1) {
        perror("lseek/read failed in write");
        free(str_end);
        return;
    }

    if (lseek(data_file, insert_offset, SEEK_SET) == -1 ||
        write(data_file, insert_text, insert_len) == -1 ||
        write(data_file, str_end, remaining_len) == -1) {
        perror("write failed");
        free(str_end);
        return;
    }

    free(str_end);
}

int rw_file_editor(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <data_file> <requests_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int data_file = open(argv[1], O_RDWR);
    if (data_file == -1) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    int requests_file = open(argv[2], O_RDONLY);
    if (requests_file == -1) {
        perror(argv[2]);
        close(data_file);
        return EXIT_FAILURE;
    }

    int results_file = open("read_results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (results_file == -1) {
        perror("read_results.txt");
        close(data_file);
        close(requests_file);
        return EXIT_FAILURE;
    }

    FILE *requests_stream = fdopen(requests_file, "r");
    if (!requests_stream) {
        perror("fdopen");
        close(data_file);
        close(requests_file);
        close(results_file);
        return EXIT_FAILURE;
    }

    char input_line[MAX_LINE_LENGTH];
    while (fgets(input_line, sizeof(input_line), requests_stream)) {
        size_t line_len = strlen(input_line);
        if (line_len > 0 && input_line[line_len - 1] == '\n') input_line[line_len - 1] = '\0';

        char command_type = input_line[0];

        switch (command_type) {
            case 'R': {
                off_t start, end;
                if (sscanf(input_line, "R %ld %ld", &start, &end) == 2) {
                    process_read_command(data_file, results_file, start, end);
                } else {
                    fprintf(stderr, "Malformed R command: %s\n", input_line);
                }
                break;
            }
            case 'W': {
                char *offset_str = input_line + 2;
                char *text_ptr = strchr(offset_str, ' ');
                if (text_ptr) {
                    *text_ptr = '\0';
                    off_t offset = atol(offset_str);
                    text_ptr++;
                    process_write_command(data_file, offset, text_ptr);
                } else {
                    fprintf(stderr, "Malformed W command: %s\n", input_line);
                }
                break;
            }
            case 'Q':
                fclose(requests_stream);
                close(data_file);
                close(results_file);
                return EXIT_SUCCESS;
            default:
                fprintf(stderr, "Unknown command: %s\n", input_line);
                break;
        }
    }

    fclose(requests_stream);
    close(data_file);
    close(results_file);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    return rw_file_editor(argc, argv);
}