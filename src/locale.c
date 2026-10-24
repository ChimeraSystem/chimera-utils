#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static const char *const s_categories[] = {
    "LC_COLLATE",
    "LC_CTYPE",
    "LC_MESSAGES",
    "LC_MONETARY",
    "LC_NUMERIC",
    "LC_TIME",
    NULL
};

static const char *get_cat_val(const char *cat) {
    const char *v = getenv("LC_ALL");
    if (v && v[0]) return v;
    v = getenv(cat);
    if (v && v[0]) return v;
    v = getenv("LANG");
    if (v && v[0]) return v;
    return "UTF-8";
}

#ifdef DEBUG
# define dprint(...) do { \
    extern long chimera_log(const char *msg); \
    char _dbuf[256]; \
    snprintf(_dbuf, sizeof(_dbuf), __VA_ARGS__); \
    chimera_log(_dbuf); \
} while (0)
#else
# define dprint(...) do { } while (0)
#endif

int main(int argc, char *argv[]) {
    dprint("[locale] start");
    bool opt_a = false;
    bool opt_m = false;
    bool opt_k = false;
    int arg_idx = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all-locales") == 0) {
            opt_a = true;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--charmaps") == 0) {
            opt_m = true;
        } else if (strcmp(argv[i], "-k") == 0) {
            opt_k = true;
        } else if (argv[i][0] != '-') {
            arg_idx = i;
            break;
        }
    }

    if (opt_a) {
        printf("C\nPOSIX\nUTF-8\nen_US.UTF-8\n");
        chimera_log("[locale] done (opt_a)");
        return 0;
    }

    if (opt_m) {
        printf("UTF-8\nUS-ASCII\n");
        chimera_log("[locale] done (opt_m)");
        return 0;
    }

    if (arg_idx > 0) {
        for (int i = arg_idx; i < argc; i++) {
            const char *name = argv[i];
            if (strcmp(name, "charmap") == 0) {
                if (opt_k) printf("charmap=\"UTF-8\"\n");
                else printf("UTF-8\n");
            } else {
                const char *val = get_cat_val(name);
                if (opt_k) printf("%s=\"%s\"\n", name, val);
                else printf("%s\n", val);
            }
        }
        chimera_log("[locale] done (arg_idx)");
        return 0;
    }

    /* Standard locale output */
    const char *lang = getenv("LANG");
    const char *lc_all = getenv("LC_ALL");

    if (lang && lang[0]) printf("LANG=\"%s\"\n", lang);
    else printf("LANG=\"en_US.UTF-8\"\n");

    for (int i = 0; s_categories[i]; i++) {
        printf("%s=\"%s\"\n", s_categories[i], get_cat_val(s_categories[i]));
    }

    if (lc_all && lc_all[0]) printf("LC_ALL=\"%s\"\n", lc_all);
    else printf("LC_ALL=\n");

    dprint("[locale] done");
    return 0;
}
