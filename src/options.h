#ifndef WIN_SPICE_OPTIONS_H
#define WIN_SPICE_OPTIONS_H

#include <stdbool.h>
#include <gmodule.h>

typedef struct Options {
    char password[128];
    int port;
    bool ssl;

    GList *compression_name_list;
    GList *compression_list;
} Options;

Options *options_new();
char *options_get_string(Options *options, const char *key);
int options_get_int(Options *options, const char *key);
void options_destroy(Options *options);

#endif  /* WIN_SPICE_OPTIONS_H */
