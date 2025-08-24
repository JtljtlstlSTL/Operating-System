#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

// 打印进程信息
void print_pinfo(struct pstat *ps) {
    for (int i = 0; i < NPROC; i++) {
        if (ps->inuse[i]) {
            printf(1, "PID: %d, Tickets: %d, Ticks: %d\n", ps->pid[i], ps->tickets[i], ps->ticks[i]);
        }
    }
    printf(1, "\n");
}

int main(int argc, char* airgv[]) {
    int tickets[] = {30,40, 50, 60, 70, 80, 90}; // 进程的彩票数

    // 创建子进程
    for (int i = 0; i < 7; i++) {
        int pid = fork();
        if (pid < 0) {
            printf(1, "Error: fork failed\n");
            exit();
        } else if (pid == 0) {
            // 子进程，设置彩票数并进入死循环
            if (settickets(tickets[i]) < 0) {
                printf(1, "Error: settickets failed\n");
                exit();
            }
            while (1);
        }
    }

    // 主进程每隔一段时间获取并打印进程信息
    struct pstat ps;
    for (int i = 0; i < 50; i++) {
        sleep(50); // 等待100个时钟周期
        if (getpinfo(&ps) < 0) {
            printf(1, "Error: getpinfo failed\n");
            exit();
        }
        printf(1, "Process Info at time %d:\n", i * 50);
        print_pinfo(&ps);
    }

    // 等待所有子进程结束
    for (int i = 0; i < 7; i++) {
        wait();
    }

    exit();
}