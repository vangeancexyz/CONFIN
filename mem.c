#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/uio.h>

struct mem_ctx {
    pid_t pid;
    char  name[256];
};

static int is_all_digits(const char *s) {
    if (*s == '\0') return 0;
    for (; *s != '\0'; ++s) {
        if (!isdigit((unsigned char)*s)) return 0;
    }
    return 1;
}

static int parse_maps_range(const char *line, uintptr_t *start, uintptr_t *end) {
    const char *dash = strchr(line, '-');
    const char *space = strchr(line, ' ');
    if (!dash || !space || space <= dash) return MEM_ERR_IO;

    char start_buf[32], end_buf[32];
    size_t start_len = (size_t)(dash - line);
    size_t end_len = (size_t)(space - (dash + 1));
    if (start_len >= sizeof(start_buf) || end_len >= sizeof(end_buf)) return MEM_ERR_IO;

    memcpy(start_buf, line, start_len);
    start_buf[start_len] = '\0';
    memcpy(end_buf, dash + 1, end_len);
    end_buf[end_len] = '\0';

    char *e1, *e2;
    errno = 0;
    uintptr_t s = (uintptr_t)strtoull(start_buf, &e1, 16);
    uintptr_t e = (uintptr_t)strtoull(end_buf, &e2, 16);
    if (errno != 0 || e1 == start_buf || e2 == end_buf) return MEM_ERR_IO;

    *start = s;
    *end = e;
    return MEM_OK;
}

mem_ctx_t *mem_create(const char *process_name) {
    mem_ctx_t *ctx = malloc(sizeof *ctx);
    if (!ctx) return NULL;

    ctx->pid = 0;
    snprintf(ctx->name, sizeof ctx->name, "%s", process_name);
    return ctx;
}

void mem_destroy(mem_ctx_t *ctx) {
    free(ctx);
}

pid_t mem_pid(const mem_ctx_t *ctx) {
    return ctx->pid;
}

int mem_attach(mem_ctx_t *ctx) {
    DIR *dir = opendir("/proc");
    if (!dir) return MEM_ERR_IO;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;
        if (!is_all_digits(entry->d_name)) continue;

        char comm_path[300];
        snprintf(comm_path, sizeof comm_path, "/proc/%s/comm", entry->d_name);

        FILE *f = fopen(comm_path, "r");
        if (!f) continue;

        char comm_name[256];
        int ok = (fscanf(f, "%255s", comm_name) == 1);
        fclose(f);

        if (ok && strcmp(comm_name, ctx->name) == 0) {
            ctx->pid = (pid_t)atoi(entry->d_name);
            closedir(dir);
            return MEM_OK;
        }
    }

    closedir(dir);
    return MEM_ERR_NOT_FOUND;
}

int mem_get_module(mem_ctx_t *ctx, const char *module_name, mem_module_t *out) {
    if (ctx->pid == 0) return MEM_ERR_NOT_FOUND;

    char maps_path[64];
    snprintf(maps_path, sizeof maps_path, "/proc/%d/maps", ctx->pid);

    FILE *f = fopen(maps_path, "r");
    if (!f) return MEM_ERR_IO;

    char line[1024];
    uintptr_t min_addr = (uintptr_t)-1;
    uintptr_t max_addr = 0;
    int found = 0;

    while (fgets(line, sizeof line, f) != NULL) {
        if (strstr(line, module_name) == NULL) continue;

        uintptr_t start, end;
        if (parse_maps_range(line, &start, &end) != MEM_OK) continue;

        if (start < min_addr) min_addr = start;
        if (end > max_addr) max_addr = end;
        found = 1;
    }
    fclose(f);

    if (!found) return MEM_ERR_NOT_FOUND;

    out->base = min_addr;
    out->size = max_addr - min_addr;
    return MEM_OK;
}

int mem_read_raw(mem_ctx_t *ctx, uintptr_t address, void *buf, size_t size) {
    struct iovec local = { buf, size };
    struct iovec remote = { (void *)address, size };

    ssize_t r = process_vm_readv(ctx->pid, &local, 1, &remote, 1, 0);
    return (r == (ssize_t)size) ? MEM_OK : MEM_ERR_IO;
}

int mem_write_raw(mem_ctx_t *ctx, uintptr_t address, const void *buf, size_t size) {
    struct iovec local = { (void *)buf, size };
    struct iovec remote = { (void *)address, size };

    ssize_t w = process_vm_writev(ctx->pid, &local, 1, &remote, 1, 0);
    return (w == (ssize_t)size) ? MEM_OK : MEM_ERR_IO;
}

uintptr_t mem_find_signature(mem_ctx_t *ctx, uintptr_t module_base,
                              size_t module_size, const char *signature) {
    int *pattern = NULL;
    size_t count = 0, cap = 0;

    char *sig_copy = strdup(signature);
    if (!sig_copy) return 0;

    char *tok = strtok(sig_copy, " \t");
    while (tok) {
        if (count == cap) {
            cap = cap ? cap * 2 : 16;
            int *grown = realloc(pattern, cap * sizeof *pattern);
            if (!grown) { free(pattern); free(sig_copy); return 0; }
            pattern = grown;
        }
        pattern[count++] = (strcmp(tok, "?") == 0 || strcmp(tok, "??") == 0)
                                ? -1
                                : (int)strtoul(tok, NULL, 16);
        tok = strtok(NULL, " \t");
    }
    free(sig_copy);

    uint8_t *buf = malloc(module_size);
    if (!buf || mem_read_raw(ctx, module_base, buf, module_size) != MEM_OK) {
        free(buf);
        free(pattern);
        return 0;
    }

    uintptr_t result = 0;
    for (size_t i = 0; i + count <= module_size; ++i) {
        int matched = 1;
        for (size_t j = 0; j < count; ++j) {
            if (pattern[j] != -1 && buf[i + j] != (uint8_t)pattern[j]) {
                matched = 0;
                break;
            }
        }
        if (matched) { result = module_base + i; break; }
    }

    free(buf);
    free(pattern);
    return result;
}
