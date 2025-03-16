#include "uthash.h"
#include <winsock2.h>

typedef struct {
    u_int tag;  // 作为 key
    u_int rod_type;
    u_int rod_shortcut;
    UT_hash_handle hh; // uthash 需要的句柄
} RodEntry;

RodEntry *rod_table = NULL;

void set_rod_type(u_int tag, u_int rod_type) {
    RodEntry *s;
    HASH_FIND_INT(rod_table, &tag, s);
    if (!s) {
        s = malloc(sizeof(RodEntry));
        s->tag = tag;
        s->rod_shortcut = 0;
    }
    s->rod_type = rod_type;
    HASH_ADD_INT(rod_table, tag, s);
}

void set_rod_shortcut(u_int tag, u_int rod_shortcut) {
    RodEntry *s;
    HASH_FIND_INT(rod_table, &tag, s);
    if (!s) {
        s = malloc(sizeof(RodEntry));
        s->tag = tag;
        s->rod_type = 0;
    }
    s->rod_shortcut = rod_shortcut;
    HASH_ADD_INT(rod_table, tag, s);
}

u_int get_rod_type(u_int tag) {
    RodEntry *s;
    HASH_FIND_INT(rod_table, &tag, s);
    return s ? s->rod_type : 0;
}

u_int get_rod_shortcut(u_int tag) {
    RodEntry *s;
    HASH_FIND_INT(rod_table, &tag, s);
    return s ? s->rod_shortcut : 0;
}