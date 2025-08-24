#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <search.h>  // 用于二分查找
#include "mapreduce.h"

#define INITIAL_PARTITION_CAPACITY 1000

typedef struct {
    char* key;
    char* value;
} Pair;

typedef struct {
    Pair* pairs;
    int count;
    int capacity;
    pthread_mutex_t lock;
} Partition;

// 全局变量
static Partition* partitions;
static int num_partitions;
static Mapper map_func;
static Reducer reduce_func;
static Partitioner partition_func;
static char** input_files;
static int num_files;
static int next_file_index;
static pthread_mutex_t file_index_lock;
static pthread_barrier_t map_barrier;
static pthread_barrier_t reduce_barrier;

// 初始化分区
static void init_partitions(int num_reducers) {
    num_partitions = num_reducers;
    partitions = malloc(num_partitions * sizeof(Partition));
    if (!partitions) {
        perror("Failed to allocate partitions");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_partitions; i++) {
        partitions[i].capacity = INITIAL_PARTITION_CAPACITY;
        partitions[i].count = 0;
        partitions[i].pairs = malloc(partitions[i].capacity * sizeof(Pair));
        if (!partitions[i].pairs) {
            perror("Failed to allocate partition pairs");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_init(&partitions[i].lock, NULL);
    }
}

// 释放分区资源
static void free_partitions() {
    for (int i = 0; i < num_partitions; i++) {
        for (int j = 0; j < partitions[i].count; j++) {
            free(partitions[i].pairs[j].key);
            free(partitions[i].pairs[j].value);
        }
        free(partitions[i].pairs);
        pthread_mutex_destroy(&partitions[i].lock);
    }
    free(partitions);
}

// 比较函数用于排序
static int compare_keys(const void* a, const void* b) {
    const Pair* kv1 = (const Pair*)a;
    const Pair* kv2 = (const Pair*)b;
    return strcmp(kv1->key, kv2->key);
}

// 分区排序
static void sort_partitions() {
    for (int i = 0; i < num_partitions; i++) {
        qsort(partitions[i].pairs, partitions[i].count, sizeof(Pair), compare_keys);
    }
}

// 默认哈希分区函数
unsigned long MR_DefaultHashPartition(char* key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

// 发射键值对（线程安全且高效）
void MR_Emit(char* key, char* value) {
    unsigned long partition = partition_func(key, num_partitions);
    Partition* part = &partitions[partition];
    
    pthread_mutex_lock(&part->lock);
    
    // 检查并扩容
    if (part->count >= part->capacity) {
        part->capacity *= 2;
        Pair* new_pairs = realloc(part->pairs, part->capacity * sizeof(Pair));
        if (!new_pairs) {
            pthread_mutex_unlock(&part->lock);
            perror("Failed to expand partition");
            exit(EXIT_FAILURE);
        }
        part->pairs = new_pairs;
    }
    
    // 复制键值
    part->pairs[part->count].key = strdup(key);
    part->pairs[part->count].value = strdup(value);
    if (!part->pairs[part->count].key || !part->pairs[part->count].value) {
        pthread_mutex_unlock(&part->lock);
        perror("Failed to duplicate key/value");
        exit(EXIT_FAILURE);
    }
    part->count++;
    
    pthread_mutex_unlock(&part->lock);
}

// 获取键值（线程安全且高效）
static char* GetVal(char* key, int partition_number) {
    static __thread int index = 0;       // 线程局部存储
    static __thread char* current_key = NULL;
    static __thread int start_pos = 0;   // 当前键的起始位置
    
    if (current_key == NULL || strcmp(current_key, key) != 0) {
        current_key = key;
        start_pos = 0;
        index = 0;
        
        // 二分查找定位键的起始位置
        Pair* arr = partitions[partition_number].pairs;
        int low = 0, high = partitions[partition_number].count - 1;
        while (low <= high) {
            int mid = low + (high - low) / 2;
            int cmp = strcmp(arr[mid].key, key);
            if (cmp == 0) {
                // 找到第一个出现的位置
                while (mid > 0 && strcmp(arr[mid-1].key, key) == 0) {
                    mid--;
                }
                start_pos = mid;
                index = mid;
                break;
            } else if (cmp < 0) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }
    }
    
    if (index < partitions[partition_number].count && 
        strcmp(partitions[partition_number].pairs[index].key, key) == 0) {
        return partitions[partition_number].pairs[index++].value;
    }
    return NULL;
}

// Mapper线程函数
static void* mapper_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&file_index_lock);
        if (next_file_index >= num_files) {
            pthread_mutex_unlock(&file_index_lock);
            break;
        }
        char* file_name = input_files[next_file_index++];
        pthread_mutex_unlock(&file_index_lock);
        
        map_func(file_name);
    }
    
    // 同步等待所有Mapper完成
    pthread_barrier_wait(&map_barrier);
    return NULL;
}

// Reducer线程函数
static void* reducer_thread(void* arg) {
    int partition = *(int*)arg;
    int i = 0;
    
    while (i < partitions[partition].count) {
        char* current_key = partitions[partition].pairs[i].key;
        reduce_func(current_key, GetVal, partition);
        
        // 跳过相同键的所有值
        while (i < partitions[partition].count && 
               strcmp(partitions[partition].pairs[i].key, current_key) == 0) {
            i++;
        }
    }
    
    // 同步等待所有Reducer完成
    pthread_barrier_wait(&reduce_barrier);
    return NULL;
}

// 主MapReduce函数
void MR_Run(int argc, char* argv[], 
            Mapper map, int num_mappers, 
            Reducer reduce, int num_reducers, 
            Partitioner partition) {
    // 参数检查
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // 初始化全局状态
    map_func = map;
    reduce_func = reduce;
    partition_func = partition;
    input_files = &argv[1];
    num_files = argc - 1;
    next_file_index = 0;
    
    pthread_mutex_init(&file_index_lock, NULL);
    pthread_barrier_init(&map_barrier, NULL, num_mappers + 1);  // +1 for main thread
    pthread_barrier_init(&reduce_barrier, NULL, num_reducers + 1);
    
    init_partitions(num_reducers);
    
    // 创建Mapper线程
    pthread_t mappers[num_mappers];
    for (int i = 0; i < num_mappers; i++) {
        if (pthread_create(&mappers[i], NULL, mapper_thread, NULL) != 0) {
            perror("Failed to create mapper thread");
            exit(EXIT_FAILURE);
        }
    }
    
    // 等待所有Mapper完成
    pthread_barrier_wait(&map_barrier);
    
    // 排序分区
    sort_partitions();
    
    // 创建Reducer线程
    pthread_t reducers[num_reducers];
    int partitions_ids[num_reducers];
    for (int i = 0; i < num_reducers; i++) {
        partitions_ids[i] = i;
        if (pthread_create(&reducers[i], NULL, reducer_thread, &partitions_ids[i]) != 0) {
            perror("Failed to create reducer thread");
            exit(EXIT_FAILURE);
        }
    }
    
    // 等待所有Reducer完成
    pthread_barrier_wait(&reduce_barrier);
    
    // 清理资源
    for (int i = 0; i < num_mappers; i++) pthread_join(mappers[i], NULL);
    for (int i = 0; i < num_reducers; i++) pthread_join(reducers[i], NULL);
    
    free_partitions();
    pthread_mutex_destroy(&file_index_lock);
    pthread_barrier_destroy(&map_barrier);
    pthread_barrier_destroy(&reduce_barrier);
}