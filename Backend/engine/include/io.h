#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define LINE_CAP 4096
#define WORD_CAP 256

/* 
 * STRINGS
 */

typedef struct {
    const char *data;
    size_t len;
} String;


String string_make(const char *data, size_t len);
String string_from_cstr(const char *cstr);
/*
    Copies the string to a cstring, dynamically allocating the returned pointer.
    FREE AFTER USE
*/
char* string_to_cstr(String s);

void string_chop_left(String *s);
void string_chop_right(String *s);
void string_trim_left(String *s);
void string_trim_right(String *s);
void string_trim(String *s);

void string_print(String s);
size_t string_len(String s);
bool string_parse_int(String s, int *out);
bool string_eq(String a, String b);
bool string_starts_with(String s, String prefix);
int string_split(const char *line, String argv[], int max_split);

/* 
 * IO
 */

int read_line(char *buf, size_t cap);
bool read_word(char *buf, size_t cap);
bool read_int(int *out);
bool join_args(int argc, char **argv, int start, char *out_buf, size_t buf_cap);


#endif
