simple POSIX platform live code reloading

=== USAGE ===
$ reloadhost <your_shared_library> [argv...]

=== CLIENT ===
Your target application will need to support the reloadhost. A simple example:

#include "reloadhost.h"

// all of your heap allocated application state
state_t *state;

// called on RH_RELOAD
static void reload() {
    // ...
}

int RH_ENTRY_NAME(
    int argc,
    char *argv[],
    reload_host_op op,
    reload_host_t *reload_host);

// used when built with RELOADABLE but linked into executable instead of shared
int main(int argc, char *argv[]) {
    int res;
    if ((res = RH_ENTRY_NAME(argc, argv, RH_INIT, NULL))) { return res; }
    while (true) {
        if ((res = RH_ENTRY_NAME(argc, argv, RH_STEP, NULL))) {
            if (res == RH_CLOSE_REQUESTED) {
                break;
            } else {
                return res;
            }
        }
    }
    return RH_ENTRY_NAME(argc, argv, RH_DEINIT, NULL);
}

// reload_host entry point
int RH_ENTRY_NAME(
    int argc,
    char *argv[],
    reload_host_op op,
    reload_host_t *reload_host) {
    if (!state) {
        state = calloc(1, sizeof(*state));
    }

    switch (op) {
    case RH_INIT:
        state = calloc(1, sizeof(*state));
        if (reload_host) {
            reload_host->userdata = state;
            state->reload_host = reload_host;
        }
        init();
        return 0;
    case RH_DEINIT:
        deinit();
        return 0;
    case RH_RELOAD:
        assert(reload_host);
        state = reload_host->userdata;
        reload();
        return 0;
    case RH_STEP:
        if (reload_host) {
            state = reload_host->userdata;
        }
        if (state->quit) { return RH_CLOSE_REQUESTED; }
    }

    if (reload_host) {
        state->reload_host = reload_host;
    }

    // whatever your loop function is
    loop();
    return 0;
}
