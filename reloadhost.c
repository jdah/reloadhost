#ifndef UTIL_IMPL
#define UTIL_IMPL
#include <stdio.h>
#endif // ifndef UTIL_IMPL

#include "util/map.h"
#include "reloadhost.h"

#define _POSIX_C_SOURCE 200809L
#include <string.h>
#undef _POSIX_C_SOURCE

#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/stat.h>
#include <time.h>

#define _STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) _STRINGIFY_IMPL(x)

typedef struct {
    char *name;
    void *storage;
} func_storage_t;

// persistent data exposed to client
static reload_host_t rh;

// currently loaded module
static void *module = NULL;

// map of storage address -> char* function name
static map_t funcs;

// see reload_host::reg_fn
static void reg_fn(void **p) {
    Dl_info info;
    dladdr(*p, &info);
    map_insert(&funcs, p, strdup(info.dli_sname));
}

// see reload_host::del_fn
static void del_fn(void **p) {
    map_remove(&funcs, p);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("%s", "usage: reloadhost <module> [args...]");
    }

    rh = (reload_host_t) {
        .reg_fn = reg_fn,
        .del_fn = del_fn,
        .userdata = NULL
    };

    map_init(
        &funcs,
        map_hash_id,
        NULL,
        NULL,
        NULL,
        map_cmp_id,
        NULL,
        map_default_free,
        NULL);

    struct timespec mod_time;
    rh_entry_f func = NULL;
    reload_host_op op = RH_INIT;

    while (true) {
        struct stat st;
        assert(stat(argv[1], &st) >= 0);

        struct timespec ts;
        timespec_get(&ts, TIME_UTC);

        // reload
        if (op == RH_INIT
            || (st.st_mtimespec.tv_sec > mod_time.tv_sec
                && ts.tv_sec > st.st_mtimespec.tv_sec + 1)) {
            if (module) {
                assert(!dlclose(module));
                module = NULL;
            }

            dlerror();
            module = dlopen(argv[1], RTLD_LOCAL | RTLD_LAZY);
            assert(module);

            func = (rh_entry_f) dlsym(module, STRINGIFY(RH_ENTRY_NAME));
            assert(func);
            mod_time = st.st_mtimespec;

            if (op != RH_INIT) {
                op = RH_RELOAD;
            }
        }

        if (op == RH_RELOAD) {
            // reload function pointers
            map_each(void**, char*, &funcs, it) {
                void *sym = dlsym(module, it.value);
                assert(sym);
                memcpy(it.key, &sym, sizeof(void*));
            }
        }

        const int res = func(argc - 1, &argv[1], op, &rh);
        if (res) {
            if (res == RH_CLOSE_REQUESTED) {
                printf("%s", "client requested close, exiting");
                return func(argc - 1, &argv[1], RH_DEINIT, &rh);
            } else {
                printf("client exited with code %d", res);
                return res;
            }
        }

        op = RH_STEP;
    }

    map_destroy(&funcs);

    return 0;
}
