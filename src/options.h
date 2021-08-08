#ifndef WIN_SPICE_OPTIONS_H
#define WIN_SPICE_OPTIONS_H

#include <stdbool.h>
#include <gmodule.h>

typedef struct WinSpiceOption{
    char password[128];
    int port;
    bool ssl;

    GList *compression_name_list;
    GList *compression_list;
} WinSpiceOption;

WinSpiceOption *winspice_options_new();
char *winspice_options_get_string(WinSpiceOption *options, const char *key);
int winspice_options_get_int(WinSpiceOption *options, const char *key);
void winspice_options_free(WinSpiceOption *options);

#endif  /* WIN_SPICE_OPTIONS_H */
