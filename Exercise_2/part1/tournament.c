// tournament.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_GLADIATORS 4

int main() {
    char* gladiator_names[NUM_GLADIATORS] = {"Maximus", "Lucius", "Commodus", "Spartacus"};
    char* gladiator_files[NUM_GLADIATORS] = {"G1.txt", "G2.txt", "G3.txt", "G4.txt"};
    pid_t pids[NUM_GLADIATORS];
    int status;
    pid_t last_pid = -1;

    for (int i = 0; i < NUM_GLADIATORS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execl("./gladiator", "gladiator", gladiator_files[i], NULL);
            perror("exec failed");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            pids[i] = pid;
        } else {
            perror("fork failed");
            exit(1);
        }
    }

    // Wait for all gladiators to finish
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        pid_t finished_pid = wait(&status);
        last_pid = finished_pid;
    }

    // Find which gladiator had the last PID
    for (int i = 0; i < NUM_GLADIATORS; i++) {
        if (pids[i] == last_pid) {
            printf("The gods have spoken, the winner of the tournament is %s!\n", gladiator_names[i]);
            break;
        }
    }

    return 0;
}
