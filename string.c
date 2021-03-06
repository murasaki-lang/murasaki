#include <stdio.h>
#include <string.h>
#include "MEM.h"
#include "murasaki.h"

#define STRING_ALLOC_SIZE (256)

static char *st_string_literal_buffer = NULL;
static int st_string_literal_buffer_size = 0;
static int st_string_literal_buffer_alloc_size = 0;

void mrsk_open_string_literal(void)
{
    st_string_literal_buffer_size = 0;
}

void mrsk_add_string_literal(int letter)
{
    if (st_string_literal_buffer_size == st_string_literal_buffer_alloc_size) {
        st_string_literal_buffer_alloc_size += STRING_ALLOC_SIZE;
        st_string_literal_buffer = MEM_realloc(st_string_literal_buffer,
                                               st_string_literal_buffer_alloc_size);
    }

    st_string_literal_buffer[st_string_literal_buffer_size] = letter;
    st_string_literal_buffer_size++;
}

void mrsk_reset_string_literal_buffer(void)
{
    MEM_free(st_string_literal_buffer);
    st_string_literal_buffer = NULL;
    st_string_literal_buffer_size = 0;
    st_string_literal_buffer_alloc_size = 0;
}

char *mrsk_close_string_literal(void)
{
    char *new_str;

    new_str = mrsk_malloc(st_string_literal_buffer_size + 1);

    memcpy(new_str, st_string_literal_buffer, st_string_literal_buffer_size);
    new_str[st_string_literal_buffer_size] = '\0';

    return new_str;
}

char * mrsk_create_identifier(char *str)
{
    char *new_str;

    new_str = mrsk_malloc(strlen(str) + 1);

    strcpy(new_str, str);

    return new_str;
}
