#include "../include/io.h"
#include "../include/log.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>


/* 
* STRING WRAPPER IMPLEMENTATION
*/

static const String string_whitespaces = {"\t\n\v\f\r ", 6};

static bool string_has_char(String s, char ch) {
    for (size_t i = 0; i < s.len; i++) {
        if (s.data[i] == ch) return true;
    }
    return false;
}

String string_make(const char *data, size_t len) {
    return (String){ .data = data, .len = len };
}

String string_from_cstr(const char *cstr) {
    return string_make(cstr, strlen(cstr));
}

char* string_to_cstr(String s) {
    if (s.len == 0) return NULL;
    char *out = (char*)malloc(s.len + 1);
    memcpy(out, s.data, s.len);
    out[s.len] = '\0';

    return out;
}

void string_chop_left(String *s) {
    if (s->len == 0) return;
    s->len  -= 1;
    s->data += 1;
}

void string_chop_right(String *s) {
    if (s->len == 0) return;
    s->len -= 1;
}

void string_trim_left(String *s) {
    while (s->len > 0 && string_has_char(string_whitespaces, s->data[0])) {
        s->data += 1;
        s->len  -= 1;
    }
}

void string_trim_right(String *s) {
    while (s->len > 0 && string_has_char(string_whitespaces, s->data[s->len - 1])) {
        s->len -= 1;
    }
}

void string_trim(String *s) {
    string_trim_left(s);
    string_trim_right(s);
}

void string_print(String s) {
    printf("%.*s\n", (int)s.len, s.data);
}

size_t string_len(String s) {
    return s.len;
}

bool string_eq(String a, String b) {
    if (a.len != b.len) return false;
    return memcmp(a.data, b.data, a.len) == 0;
}

bool string_starts_with(String s, String prefix) {
    if (prefix.len > s.len) return false;
    return memcmp(s.data, prefix.data, prefix.len) == 0;
}

bool string_parse_int(String s, int64_t *out) {
    char *cstr = string_to_cstr(s);
    if (cstr == NULL) return false;

    char *endptr;
    *out = (int64_t)strtol(cstr, &endptr, 10);
    bool ok = (*endptr == '\0');

    free(cstr);
    return ok;
}

/* 
 * IO FUNCTIONS
 */

int read_line(char *buf, size_t cap)
{
    if (!fgets(buf, (int)cap, stdin)) return 0;
 
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') {
        buf[--n] = '\0';
    } else if (n + 1 == cap) {   /* buffer cheio e sem '\n': cabe ou foi truncada? */
        int c = getchar();
        if (c != '\n' && c != EOF) {
            while (c != '\n' && c != EOF) c = getchar();   /* descarta o resto */
            return -1;
        }
    }                            /* senao: ultima linha do arquivo, sem '\n' */
    if (n > 0 && buf[n - 1] == '\r') buf[n - 1] = '\0';   /* Windows */
    return 1;
}



int string_split(const char *line, String argv[], int max_split) {
    if (max_split <= 0) return -1;
    int argc = 0;
    char *p = line;
    char *start;
    size_t len;

    for (;;) {
        while (isspace((unsigned char)*p)) p++;
        if (!(*p)) return argc;
        start = p;
        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p || *p == '\0') {
            len = p - start;
            String temp = {.data = start, .len = len};
            argv[argc++] = temp;
            if (argc == max_split) return argc;
        }
    }
}

/*
    Concatena os `argc` números de argumentos, armazenados em `*argv[]`, a partir de uma posição `start`
    do vetor de argumentos.

    Devolve false se não couber, se buf_cap for 0, ou se algum argv[i] for NULL.
 */
bool string_join_args(int argc, char **argv, int start, char *out_buf, size_t buf_cap) {
    if (buf_cap == 0) return false;
    out_buf[0] = '\0';

    size_t len = 0;
    for (int i = start; i < argc; i++) {
        if (argv[i] == NULL) {
            LOG_ERROR("argv[%d] is NULL", i);
            return false;
        }

        size_t n = strlen(argv[i]);
        if (len + n + 2 > buf_cap) {
            LOG_ERROR("Buffer out of capacity");
            return false;
        }
        memcpy(out_buf + len, argv[i], n);
        len += n;
        out_buf[len] = '\0';
    }

    return true;
}