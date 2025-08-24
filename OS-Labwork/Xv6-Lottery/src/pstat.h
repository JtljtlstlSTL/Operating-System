#pragma once
#include "param.h"

struct pstat{
    int inuse[NPROC];//是否正在使用
    int tickets[NPROC];//票数
    int pid[NPROC];//process id
    int ticks[NPROC];//每个进程当累计CPU时间
};