// runtime.c

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef struct {
    void *addr;
    long last_tid;
    int shared;
} AddrInfo;

#define MAX_ADDRS 4096  // 简单版本，够你这些 demo 用了

static AddrInfo g_table[MAX_ADDRS];
static int g_used = 0;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *g_log_fp  = NULL;
static FILE *g_json_fp = NULL;

static void ensure_files_opened(void) {
    if (!g_log_fp) {
        g_log_fp = fopen("memtrace.log", "a");
        if (!g_log_fp) {
            perror("fopen memtrace.log");
        }
    }
    if (!g_json_fp) {
        g_json_fp = fopen("memtrace.json", "a");
        if (!g_json_fp) {
            perror("fopen memtrace.json");
        }
    }
}

static AddrInfo *find_or_insert(void *addr, long tid) {
    for (int i = 0; i < g_used; ++i) {
        if (g_table[i].addr == addr)
            return &g_table[i];
    }
    if (g_used >= MAX_ADDRS)
        return NULL;

    g_table[g_used].addr = addr;
    g_table[g_used].last_tid = tid;
    g_table[g_used].shared = 0;
    return &g_table[g_used++];
}

void recordAccess(void *addr, int isWrite, int line, const char *file) {
    long tid = (long)pthread_self();

    pthread_mutex_lock(&g_lock);
    ensure_files_opened();

    int shared = 0;

    AddrInfo *info = find_or_insert(addr, tid);
    if (info) {
        // 首次出现：last_tid 就是当前 tid
        if (info->last_tid == 0) {
            info->last_tid = tid;
        } else if (info->last_tid != tid) {
            // 不同线程访问同一地址 → 共享
            info->shared = 1;
        }
        shared = info->shared;
    }

    const char *fname = file ? file : "unknown";

    if (g_log_fp) {
        fprintf(g_log_fp,
                "[TID %ld] %c %p file=%s line=%d shared=%d\n",
                tid,
                isWrite ? 'W' : 'R',
                addr,
                fname,
                line,
                shared);
        fflush(g_log_fp);
    }

    if (g_json_fp) {
        fprintf(g_json_fp,
                "{\"tid\": %ld, \"addr\": \"%p\", \"write\": %d, "
                "\"file\": \"%s\", \"line\": %d, \"shared\": %d}\n",
                tid,
                addr,
                isWrite ? 1 : 0,
                fname,
                line,
                shared);
        fflush(g_json_fp);
    }

    pthread_mutex_unlock(&g_lock);
}
