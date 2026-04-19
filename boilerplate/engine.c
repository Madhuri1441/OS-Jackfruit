#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

// ================== CHECK DUPLICATE ==================
int container_exists(char *id) {
    FILE *f = fopen("containers.txt", "r");
    if (!f) return 0;

    char cid[50], status[20];
    int pid;

    while (fscanf(f, "%s %d %s", cid, &pid, status) != EOF) {
        if (strcmp(cid, id) == 0 && strcmp(status, "running") == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// ================== SAVE ==================
void save_container(char *id, int pid, char *status) {
    FILE *f = fopen("containers.txt", "a");
    if (f) {
        fprintf(f, "%s %d %s\n", id, pid, status);
        fclose(f);
    }
}

// ================== RUN / START ==================
void create_container(char *id, char *rootfs, char *command, int background) {

    if (container_exists(id)) {
        printf("Container already running\n");
        return;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // CHILD

        chroot(rootfs);
        chdir("/");

        // LOG FILE
        char logname[100];
        sprintf(logname, "%s.log", id);

        int fd = open(logname, O_CREAT | O_WRONLY | O_TRUNC, 0644);

        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("exec failed");
        exit(1);
    }

    else if (pid > 0) {
        printf("Container %s started with PID %d\n", id, pid);
        save_container(id, pid, "running");

        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}

// ================== LIST ==================
void list_containers() {
    FILE *f = fopen("containers.txt", "r");

    if (!f) {
        printf("No containers\n");
        return;
    }

    char id[50], status[20];
    int pid;

    printf("Containers:\n");

    while (fscanf(f, "%s %d %s", id, &pid, status) != EOF) {
        printf("%s | PID: %d | %s\n", id, pid, status);
    }

    fclose(f);
}

// ================== STOP ==================
void stop_container(char *id) {
    FILE *f = fopen("containers.txt", "r");
    FILE *temp = fopen("temp.txt", "w");

    char cid[50], status[20];
    int pid, found = 0;

    while (fscanf(f, "%s %d %s", cid, &pid, status) != EOF) {
        if (strcmp(cid, id) == 0 && strcmp(status, "running") == 0) {
            kill(pid, SIGKILL);
            fprintf(temp, "%s %d stopped\n", cid, pid);
            printf("Stopped %s\n", id);
            found = 1;
        } else {
            fprintf(temp, "%s %d %s\n", cid, pid, status);
        }
    }

    fclose(f);
    fclose(temp);

    remove("containers.txt");
    rename("temp.txt", "containers.txt");

    if (!found) printf("Container not found\n");
}

// ================== LOGS ==================
void show_logs(char *id) {
    char file[100];
    sprintf(file, "%s.log", id);

    FILE *f = fopen(file, "r");
    if (!f) {
        printf("No logs found\n");
        return;
    }

    char line[200];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
}

// ================== MAIN ==================
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: run/start/ps/stop/logs\n");
        return 1;
    }

    if (strcmp(argv[1], "run") == 0) {
        create_container(argv[2], argv[3], argv[4], 0);
    }

    else if (strcmp(argv[1], "start") == 0) {
        create_container(argv[2], argv[3], argv[4], 1);
    }

    else if (strcmp(argv[1], "ps") == 0) {
        list_containers();
    }

    else if (strcmp(argv[1], "stop") == 0) {
        stop_container(argv[2]);
    }

    else if (strcmp(argv[1], "logs") == 0) {
        show_logs(argv[2]);
    }

    else {
        printf("Unknown command\n");
    }

    return 0;
}
