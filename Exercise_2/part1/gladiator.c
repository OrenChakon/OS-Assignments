// gladiator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_OPPONENTS 3
#define NUM_GLADIATORS 4

int get_opponent_attack(int id) {
    // Open G{id}.txt and read its attack value
    char filename[10];
    snprintf(filename, sizeof(filename), "G%d.txt", id);
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("Could not open opponent file");
        exit(1);
    }
    int health, attack;
    fscanf(f, "%d, %d,", &health, &attack);
    fclose(f);
    return attack;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <gladiator_file>\n", argv[0]);
        exit(1);
    }

    // Parse gladiator file
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror("Could not open gladiator file");
        exit(1);
    }

    int health, attack, opponents[MAX_OPPONENTS];
    fscanf(f, "%d, %d, %d, %d, %d", &health, &attack, &opponents[0], &opponents[1], &opponents[2]);
    fclose(f);

    // Prepare log file
    char log_filename[20];
    snprintf(log_filename, sizeof(log_filename), "%.*s_log.txt", 2, argv[1]);
    FILE* log = fopen(log_filename, "w");
    if (!log) {
        perror("Could not open log file");
        exit(1);
    }

    fprintf(log, "Gladiator process started. PID: %d\n", getpid());

    while (health > 0) {
        for (int i = 0; i < MAX_OPPONENTS && health > 0; i++) {
            int damage = get_opponent_attack(opponents[i]);
            fprintf(log, "Facing opponent %d... Taking %d damage\n", opponents[i], damage);
            health -= damage;
            if (health > 0) {
                fprintf(log, "Are you not entertained? Remaining health: %d\n", health);
            } else {
                fprintf(log, "The gladiator has fallen... Final health: %d\n", health);
                break;
            }
        }
    }

    fclose(log);
    return 0;
}
