#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

void monitor_cpu(int pipe_fd) {
    char buffer[256];
    FILE *fp = popen("top -bn1 | grep 'Cpu(s)' | awk '{print $2 + $4 \"%\"}'", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        write(pipe_fd, "CPU: ", 5);
        write(pipe_fd, buffer, strlen(buffer));
        fclose(fp);
    }
}

void monitor_memory(int pipe_fd) {
    char buffer[256];
    FILE *fp = popen("free -m | awk '/Mem:/ {print $3 \"MB used of \" $2 \"MB\"}'", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        write(pipe_fd, "Memory: ", 8);
        write(pipe_fd, buffer, strlen(buffer));
        fclose(fp);
    }
}

void monitor_network(int pipe_fd) {
    char buffer[256];
    FILE *fp = popen("cat /sys/class/net/enp0s3/statistics/rx_bytes", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        write(pipe_fd, "Network RX: ", 12);
        write(pipe_fd, buffer, strlen(buffer));
        fclose(fp);
    }

    fp = popen("cat /sys/class/net/enp0s3/statistics/tx_bytes", "r");
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        write(pipe_fd, "Network TX: ", 12);
        write(pipe_fd, buffer, strlen(buffer));
        fclose(fp);
    }
}

int main() {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    int log_fd = open("system_monitor.log", O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    if (log_fd == -1) {
        perror("Failed to open log file");
        return 1;
    }

    pid_t pid;

    pid = fork();
    if (pid == 0) {
        close(pipe_fd[PIPE_READ]);
        monitor_cpu(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_WRITE]);
        exit(0);
    }

    pid = fork();
    if (pid == 0) {
        close(pipe_fd[PIPE_READ]);
        monitor_memory(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_WRITE]);
        exit(0);
    }

    pid = fork();
    if (pid == 0) {
        close(pipe_fd[PIPE_READ]);
        monitor_network(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_WRITE]);
        exit(0);
    }

    close(pipe_fd[PIPE_WRITE]);

    char buffer[256];
    ssize_t nbytes;
    while ((nbytes = read(pipe_fd[PIPE_READ], buffer, sizeof(buffer))) > 0) {
        write(log_fd, buffer, nbytes);
    }

    close(pipe_fd[PIPE_READ]);
    close(log_fd);

    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    return 0;
}
