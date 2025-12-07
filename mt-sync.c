/* Взаимодействие двух процессов с помощью file IO без использования pipes */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define SYNC_FILE "sync.txt"
#define MAX_ITERATIONS 10

void wait_for_flag(int fd, char expected_flag, const char* process_name) {
    printf("[%s] Переход в состояние SLEEP (ожидание сигнала '%c')\n", process_name, expected_flag);
    fflush(stdout);

    char buf;
    while (1) {
        // Читаем текущий флаг
        lseek(fd, 0, SEEK_SET);
        ssize_t bytes = read(fd, &buf, 1);
        if (bytes == -1) {
            perror("read");
            exit(1);
        }
        if (bytes > 0 && buf == expected_flag) {
            // Получен ожидаемый сигнал
            break;
        }
        // Не тот флаг или данных пока нет – немного спим и повторяем попытку
        usleep(100000); // 0.1 секунды
    }

    printf("[%s] Переход в состояние READY (получен сигнал '%c')\n", process_name, expected_flag);
    fflush(stdout);
}

void send_flag(int fd, char flag_value, const char* process_name) {
    printf("[%s] Отправка сигнала '%c' другому процессу\n", process_name, flag_value);
    fflush(stdout);

    lseek(fd, 0, SEEK_SET);
    char buf = flag_value;
    if (write(fd, &buf, 1) == -1) {
        perror("write");
        exit(1);
    }
    fsync(fd);
}

void process_a() {
    int fd = open(SYNC_FILE, O_RDWR);
    if (fd == -1) {
        perror("open (Process A)");
        exit(1);
    }

    printf("[Process A] Начальное состояние: READY\n");
    fflush(stdout);

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        printf("\n*** Итерация %d (Process A) ***\n", i + 1);
        fflush(stdout);

        // Имитация работы процесса A в состоянии READY
        sleep(1);

        // Отправляем сигнал процессу B
        send_flag(fd, '1', "Process A");

        // Процесс A переходит в состояние SLEEP и ждет сигнал '0'
        wait_for_flag(fd, '0', "Process A");
    }

    close(fd);
}

void process_b() {
    int fd = open(SYNC_FILE, O_RDWR);
    if (fd == -1) {
        perror("open (Process B)");
        exit(1);
    }

    printf("[Process B] Начальное состояние: SLEEP\n");
    fflush(stdout);

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        // B ожидает сигнал '1' от процесса A
        wait_for_flag(fd, '1', "Process B");

        printf("\n*** Итерация %d (Process B) ***\n", i + 1);
        fflush(stdout);

        // Имитация работы процесса B в состоянии READY
        sleep(1);

        // Отправляем сигнал процессу A
        send_flag(fd, '0', "Process B");
    }

    close(fd);
}

int main() {
    // Создаем/очищаем файл-флаг и устанавливаем начальное значение '0'
    int fd = open(SYNC_FILE, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open sync file");
        exit(1);
    }
    // Записываем '0' как начальный флаг
    char initial_flag = '0';
    if (write(fd, &initial_flag, 1) == -1) {
        perror("write initial flag");
        exit(1);
    }
    close(fd);

    printf("=== Запуск механизма пинг-понг ===\n\n");
    fflush(stdout);

    // Создаем дочерний процесс B
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        process_b();
        exit(0);
    } else {
        process_a();
        // Ждем завершения процесса B
        wait(NULL);
        unlink(SYNC_FILE);
        printf("\n=== Механизм пинг-понг завершен ===\n");
    }

    return 0;
}
