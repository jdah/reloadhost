#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>

typedef uint64_t hash_t;

typedef hash_t (*f_map_hash)(void*, void*);
typedef void *(*f_map_dup)(void*, void*);
typedef void (*f_map_free)(void*, void*);
typedef int (*f_map_cmp)(void*, void*, void*);
typedef void *(*f_map_alloc)(size_t, void*, void*);

typedef struct map_entry_t {
    void *key, *value;
} map_entry_t;

typedef struct map_internal_entry_t {
    map_entry_t entry;
    uint16_t dist;
    hash_t hash : 15;
    int used : 1;
} map_internal_entry_t;

typedef struct map_t {
    f_map_hash f_hash;
    f_map_dup f_keydup;
    f_map_cmp f_keycmp;
    f_map_free f_keyfree, f_valfree;

    // internal allocation functions, default to malloc/free, can be replaced
    f_map_alloc f_alloc;
    f_map_free f_free;

    size_t used, capacity, prime;
    map_internal_entry_t *entries;

    void *userdata;
} map_t;

enum {
    MAP_INSERT_ENTRY_IS_NEW = 1 << 0
};

// hash functions
hash_t map_hash_id(void *p, void*);
hash_t map_hash_str(void *p, void*);

// compare functions
int map_cmp_id(void *p, void *q, void*);
int map_cmp_str(void *p, void *q, void*);

// duplication functions
void *map_dup_str(void *s, void*);

// default map_alloc_fn implemented with stdlib's "realloc()"
void *map_default_alloc(size_t n, void *p, void*);

// default map_free_fn implemented with stdlib's "free()"
void map_default_free(void *p, void*);

// internal usage only
size_t _map_insert(
    map_t *self,
    hash_t hash,
    void *key,
    void *value,
    int flags,
    int *rehash_out);

// internal usage only
size_t _map_find(const map_t *self, hash_t hash, void *key);

// internal usage only
void _map_remove_at(map_t *self, size_t pos);

// internal usage only
void _map_remove(map_t *self, hash_t hash, void *key);

// create new map
void map_init(
    map_t *self,
    f_map_hash f_hash,
    f_map_alloc f_alloc,
    f_map_free f_free,
    f_map_dup f_keydup,
    f_map_cmp f_keycmp,
    f_map_free f_keyfree,
    f_map_free f_valfree,
    void *userdata);

// destroy map
void map_destroy(map_t *self);

// clear map of all keys and values
void map_clear(map_t *self);

// insert (k, v) into map, replacing value if it is already present, returns
// pointer to value
#define _map_insert3(_m, _k, _v) ({                                        \
        map_t *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k), *__v = (void*)(uintptr_t)(_v); \
        size_t __n =                                                        \
            _map_insert(                                                   \
                __m,                                                       \
                __m->f_hash(__k, __m->userdata),                           \
                __k,                                                       \
                __v,                                                       \
                MAP_INSERT_ENTRY_IS_NEW,                                   \
                NULL);                                                     \
        &__m->entries[__n].entry.value;                                    \
    })

// insert (k, v) into map, replacing value if it is already present, returns
// pointer T* to value
#define _map_insert4(T, _m, _k, _v) ({                                     \
        map_t *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k), *__v = (void*)(uintptr_t)(_v); \
        size_t __n =                                                        \
            _map_insert(                                                   \
                __m,                                                       \
                __m->f_hash(__k, __m->userdata),                           \
                __k,                                                       \
                __v,                                                       \
                MAP_INSERT_ENTRY_IS_NEW);                                  \
        (T*) (uintptr_t) &__m->entries[__n].entry.value;                   \
    })

// insert (k, v) into map, replacing value if it is already present, returns
// pointer to value
#define map_insert(...) VFUNC(_map_insert, __VA_ARGS__)

// returns true if map contians key
#define map_contains(_m, _k) ({                                            \
        const map_t *__m = (_m);                                      \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        _map_find(__m, __m->f_hash(__k, __m->userdata), __k) != SIZE_MAX; \
    })

// returns _T *value, NULL if not present
#define _map_find3(_T, _m, _k) ({                                          \
        const map_t *__m = (_m);                                      \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        size_t _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        _i == SIZE_MAX ? NULL : (_T*)(&__m->entries[_i].entry.value);     \
    })

// returns void **value, NULL if not present
#define _map_find2(_m, _k) ({                                              \
        const map_t *__m = (_m);                                      \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        size_t _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        _i == SIZE_MAX ? NULL : &__m->entries[_i].entry.value;            \
    })

// map_find(map, key) or map_find(TYPE, map, key) -> ptr to value, NULL if not
// found
#define map_find(...) VFUNC(_map_find, __VA_ARGS__)

// returns _T value, crashes if not present
#define _map_get3(_T, _m, _k) ({                                           \
        map_t *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        size_t _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        assert(_i != SIZE_MAX, "key not found");                          \
        *(_T*)(&__m->entries[_i].entry.value);                             \
    })

// returns void *value, crashes if not present
#define _map_get2(_m, _k) ({                                               \
        map_t *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        size_t _i = _map_find(__m, __m->f_hash(__k, __m->userdata), __k);   \
        assert(_i != SIZE_MAX, "key not found");                          \
        __m->entries[_i].entry.value;                                      \
    })

// map_get(map, key) or map_get(TYPE, map, key)
#define map_get(...) VFUNC(_map_get, __VA_ARGS__)

// remove key k from map
#define map_remove(_m, _k) ({                                              \
        map_t *__m = (_m);                                            \
        void *__k = (void*)(uintptr_t)(_k);                                           \
        _map_remove(__m, __m->f_hash(__k, __m->userdata), __k);            \
    })

// get next used entry from map index _i. returns _m->capacity at map end
#define map_next(_m, _i, _first) ({                                        \
        TYPEOF(_m) __m = (_m);                                             \
        size_t _j = _i;                                                     \
        if (!(_first)) { _j++; }                                           \
        while (_j < __m->capacity && !__m->entries[_j].used) { _j++; }     \
        assert(_j >= __m->capacity || __m->entries[_j].used);              \
        _j;                                                                \
    })

#define _map_each_impl(_KT, _VT, _m, _it, _itname)                         \
    typedef struct {                                                       \
        TYPEOF(_m) __m; size_t __i; _KT key; _VT value; } _itname;         \
    for (_itname _it = {                                                   \
            .__m = (_m),                                                   \
            .__i = map_next(_it.__m, 0, true),                             \
            .key = (_KT)(_it.__i < _it.__m->capacity ?                     \
                (uintptr_t)(_it.__m->entries[_it.__i].entry.key)           \
                : 0),                                                      \
            .value = (_VT)(_it.__i < _it.__m->capacity ?                   \
                (uintptr_t)(_it.__m->entries[_it.__i].entry.value)         \
                : 0)                                                       \
         }; _it.__i < _it.__m->capacity;                                   \
         _it.__i = map_next(_it.__m, _it.__i, false),                      \
         _it.key = (_KT)(_it.__i < _it.__m->capacity ?                     \
                    (uintptr_t)(_it.__m->entries[_it.__i].entry.key)       \
                    : 0),                                                  \
         _it.value = (_VT)(_it.__i < _it.__m->capacity ?                   \
                    (uintptr_t)(_it.__m->entries[_it.__i].entry.value)     \
                    : 0))

// map_each(KEY_TYPE, VALUE_TYPE, *map, it_name)
#define map_each(_KT, _VT, _m, _it)                                        \
    _map_each_impl(                                                        \
        _KT,                                                               \
        _VT,                                                               \
        _m,                                                                \
        _it,                                                               \
        CONCAT(_mei, __COUNTER__))

// true if map is empty
#define map_empty(_m) ((_m)->used == 0)

// number of elements in map
#define map_size(_m) ((_m)->used)

#ifdef UTIL_IMPL

#define _POSIX_C_SOURCE 200809L
#include <string.h>

#define ARRLEN(_arr) ((sizeof((_arr))) / ((sizeof((_arr)[0]))))

#define TYPEOF(_t) __typeof__((_t))
#define CONCAT_INNER(a, b) a ## b
#define CONCAT(a, b) CONCAT_INNER(a, b)

// see stackoverflow.com/questions/11761703
// get number of arguments with _NARG_
#define NARG(...) _NARG_I_(__VA_ARGS__,_RSEQ_N())
#define _NARG_I_(...) _ARG_N(__VA_ARGS__)
#define _ARG_N(                               \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,  \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define _RSEQ_N()                             \
     63,62,61,60,                             \
     59,58,57,56,55,54,53,52,51,50,           \
     49,48,47,46,45,44,43,42,41,40,           \
     39,38,37,36,35,34,33,32,31,30,           \
     29,28,27,26,25,24,23,22,21,20,           \
     19,18,17,16,15,14,13,12,11,10,           \
     9,8,7,6,5,4,3,2,1,0

// general definition for any function name
#define _VFUNC(name, n) CONCAT(name, n)
#define VFUNC(func, ...) _VFUNC(func, NARG(__VA_ARGS__))(__VA_ARGS__)

#define _NTH_ARG0(A0, ...) A0
#define _NTH_ARG1(A0, A1, ...) A1
#define _NTH_ARG2(A0, A1, A2, ...) A2
#define _NTH_ARG3(A0, A1, A2, A3, ...) A3
#define _NTH_ARG4(A0, A1, A2, A3, A4, ...) A4
#define _NTH_ARG5(A0, A1, A2, A3, A4, A5, ...) A5
#define _NTH_ARG6(A0, A1, A2, A3, A4, A5, A6, ...) A6
#define _NTH_ARG7(A0, A1, A2, A3, A4, A5, A6, A7, ...) A7
#define _NTH_ARG8(A0, A1, A2, A3, A4, A5, A6, A7, A8, ...) A8

// get NTH_ARG
#define _NTH_ARG(N) CONCAT(_NTH_ARG, N)
#define NTH_ARG(N, ...) _NTH_ARG(NARG(__VA_ARGS__))(__VA_ARGS__)

// planetmath.org/goodhashtableprimes
static const uint32_t PRIMES[] = {
    11, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

// load factors at which rehashing happens, expressed as percentages
#define MAP_LOAD_HIGH 80
#define MAP_LOAD_LOW 10

// distance is stored as a 16-bit unsigned integer
#define MAP_MAX_DISTANCE UINT16_MAX

// mask for map_internal_entry_t::hash
#define MAP_STORED_HASH_MASK ((1 << 15) - 1)

hash_t map_hash_id(void *p, void*) {
    return (hash_t) (p);
}

// fnv1a hash
hash_t map_hash_str(void *p, void*) {
    char *s = (char*) p;

    hash_t h = 0;
    while (*s != '\0') {
	    h = (h ^ ((char) (*s++))) * 1099511628211u;
    }

    return h;
}

int map_cmp_id(void *p, void *q, void*) {
    return (int) (((char*) p) - ((char*) (q)));
}

int map_cmp_str(void *p, void *q, void*) {
    return strcmp((char*) p, (char*) q);
}

void *map_dup_str(void *s, void*) {
    return strdup((char*) s);
}

void *map_default_alloc(size_t n, void *p, void*) {
    return realloc(p, n);
}

void map_default_free(void *p, void*) {
    free(p);
}

void map_init(
    map_t *map,
    f_map_hash f_hash,
    f_map_alloc f_alloc,
    f_map_free f_free,
    f_map_dup f_keydup,
    f_map_cmp f_keycmp,
    f_map_free f_keyfree,
    f_map_free f_valfree,
    void *userdata) {
    map->f_hash = f_hash;
    map->f_alloc = f_alloc ? f_alloc : map_default_alloc;
    map->f_free = f_free ? f_free : map_default_free;
    map->f_keydup = f_keydup;
    map->f_keycmp = f_keycmp;
    map->f_keyfree = f_keyfree;
    map->f_valfree = f_valfree;
    map->userdata = userdata;
    map->used = 0;
    map->capacity = 0;
    map->prime = 0;
    map->entries = NULL;
}

void map_destroy(map_t *map) {
    if (!map->entries) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++)
        if (map->entries[i].used) {
            if (map->f_keyfree) {
                map->f_keyfree(map->entries[i].entry.key, map->userdata);
            }

            if (map->f_valfree) {
                map->f_valfree(map->entries[i].entry.value, map->userdata);
            }
        }

    map->f_free(map->entries, map->userdata);

    map->used = 0;
    map->capacity = 0;
    map->prime = 0;
    map->entries = NULL;
}

// returns true on rehash
static int map_rehash(map_t *map, int force) {
    if (!map->entries) {
        return false;
    }

    size_t load = (map->used * 100) / (map->capacity);
    int
        low = load < MAP_LOAD_LOW,
        high = load > MAP_LOAD_HIGH;

    if (!force && ((!low && !high) ||
        (low && map->prime == 0) ||
        (high && map->prime == ARRLEN(PRIMES) - 1))) {
        return false;
    }

    size_t old_capacity = map->capacity;

    assert(
        force ||
        (high && map->prime != ARRLEN(PRIMES) - 1) ||
        (low && map->prime != 0));

    if (low && map->prime != 0) {
        map->prime--;
    } else if (force || (high && map->prime != (ARRLEN(PRIMES) - 1))) {
        map->prime++;
    }

    assert(map->prime < ARRLEN(PRIMES));
    map->capacity = PRIMES[map->prime];

    map_internal_entry_t *old = map->entries;
    map->entries =
        (map_internal_entry_t*)
            map->f_alloc(
                map->capacity * sizeof(map_internal_entry_t),
                NULL,
                map->userdata);
    memset(map->entries, 0, map->capacity * sizeof(map_internal_entry_t));

    // re-insert all entries
    for (size_t i = 0; i < old_capacity; i++) {
        if (old[i].used) {
            _map_insert(
                map,
                map->f_hash(old[i].entry.key, map->userdata),
                old[i].entry.key,
                old[i].entry.value,
                false,
                NULL);
        }
    }

    map->f_free(old, map->userdata);
    return true;
}

size_t _map_insert(
    map_t *map,
    hash_t hash,
    void *key,
    void *value,
    int flags,
    int *rehash_out) {
    if (!map->entries) {
        assert(map->used == 0);
        assert(map->capacity == 0);
        assert(map->prime == 0);
        map->capacity = PRIMES[map->prime];
        map->entries =
            (map_internal_entry_t*)
                map->f_alloc(
                    map->capacity * sizeof(map_internal_entry_t),
                    NULL,
                    map->userdata);
        memset(
            map->entries, 0, map->capacity * sizeof(map_internal_entry_t));
    }

    int did_rehash = false;
    size_t pos = hash % map->capacity, dist = 0;
    hash_t hashbits = hash & MAP_STORED_HASH_MASK;

    while (true) {
        map_internal_entry_t *entry = &map->entries[pos];
        if (!entry->used || entry->dist < dist) {
            map_internal_entry_t new_entry = {
                .entry = (map_entry_t) { .key = key, .value = value },
                .dist = (uint16_t) dist,
                .hash = hashbits,
                .used = true,
            };

            if (flags & MAP_INSERT_ENTRY_IS_NEW) {
                map->used++;
            }

            if ((flags & MAP_INSERT_ENTRY_IS_NEW) && map->f_keydup) {
                new_entry.entry.key = map->f_keydup(key, map->userdata);
            }

            if (!entry->used) {
                *entry = new_entry;
            } else {
                // distance is less than the one that we're trying to insert,
                // displace the element to reduce lookup times
                map_internal_entry_t e = *entry;
                *entry = new_entry;
                _map_insert(
                    map,
                    map->f_hash(e.entry.key, map->userdata),
                    e.entry.key,
                    e.entry.value,
                    flags & ~MAP_INSERT_ENTRY_IS_NEW,
                    &did_rehash);
            }

            break;
        } else if (
            entry->dist == dist &&
            entry->hash == hashbits &&
            !map->f_keycmp(entry->entry.key, key, map->userdata)) {
            // entry already exists, replace value
            if (map->f_valfree) {
                map->f_valfree(entry->entry.value, map->userdata);
            }

            entry->entry.value = value;
            break;
        }

        // rehash if we have a greater distance than storage allows
        dist++;
        if (dist >= MAP_MAX_DISTANCE) {
            did_rehash = true;
            if (rehash_out) { *rehash_out = true; }
            map_rehash(map, true);
            return _map_insert(map, hash, key, value, flags, NULL);
        }

        // check next location
        pos = (pos + 1) % map->capacity;
    }

    // rehash if necessary
    if (map_rehash(map, false) || did_rehash) {
        // recompute pos as it may have moved on rehash
        if (rehash_out) { *rehash_out = true; }
        pos = _map_find(map, hash, key);
    }

    return pos;
}

size_t _map_find(const map_t *map, hash_t hash, void *key) {
    if (!map->entries) { return SIZE_MAX; }

    size_t pos = hash % map->capacity, res = SIZE_MAX;
    hash_t hashbits = hash & MAP_STORED_HASH_MASK;

    while (true) {
        if (!map->entries[pos].used) {
            break;
        }

        if (map->entries[pos].hash == hashbits
            && !map->f_keycmp(map->entries[pos].entry.key, key, map->userdata)) {
            res = pos;
            break;
        }

        pos = (pos + 1) % map->capacity;
    }

    return res;
}

void _map_remove_at(map_t *map, size_t pos) {
    assert(map->entries);

    size_t next_pos = 0;
    assert(pos < map->capacity);

    map_internal_entry_t *entry = &map->entries[pos];
    if (entry->entry.key && map->f_keyfree) {
        map->f_keyfree(entry->entry.key, map->userdata);
    }

    if (entry->entry.value && map->f_valfree) {
        map->f_valfree(entry->entry.value, map->userdata);
    }

    map->used--;

    // mark empty
    entry->used = false;

    // shift further entries back by one
    while (true) {
        next_pos = (pos + 1) % map->capacity;

        // stop when unused entry is found or dist is 0
        if (!map->entries[next_pos].used
            || map->entries[next_pos].dist == 0) {
            break;
        }

        // copy next pos into current
        map->entries[pos] = map->entries[next_pos];
        map->entries[pos].dist--;

        // mark next entry as empty
        map->entries[next_pos].used = false;

        pos = next_pos;
    }

    map_rehash(map, false);
}

void _map_remove(map_t *map, hash_t hash, void *key) {
    const size_t pos = _map_find(map, hash, key);
    assert(pos != SIZE_MAX);
    _map_remove_at(map, pos);
    assert(!map_find(map, key));
}

void map_clear(map_t *map) {
    map_destroy(map);
}
#endif // ifdef UTIL_IMPL
