#ifndef WIN_SPICE_OPTIONS_H
#define WIN_SPICE_OPTIONS_H

#include <stdbool.h>
#include <gmodule.h>

typedef struct Options {
    char *password;
    int port;
    bool ssl;
    int compression;
    const char *compression_text;

    GList *compression_name_list;
    GList *compression_list;
} Options;

Options *options_new();
char *options_get_string(Options *options, const char *key);
void options_set_string(Options *options, const char *key, const char *value);
int options_get_int(Options *options, const char *key);
void options_set_int(Options *options, const char *key, int value);
void options_destroy(Options *options);

#endif  /* WIN_SPICE_OPTIONS_H */
