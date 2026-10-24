
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fuse_manifest.h>

#define RECEIPTS_DIR "/Library/Receipts"

static void print_usage(const char *prog) {
    printf("Fuse Package Manager — Chimera Operating System\n");
    printf("Usage:\n");
    printf("  %s info <manifest.plist>          Display package metadata and payload\n", prog);
    printf("  %s check-deps <manifest.plist>    Verify all dependencies are installed\n", prog);
    printf("  %s verify <manifest.plist>        Verify payload files exist on disk\n", prog);
    printf("  %s list                           List installed packages in %s\n", prog, RECEIPTS_DIR);
    printf("  %s register <manifest.plist>      Register package receipt into %s\n", prog, RECEIPTS_DIR);
}

static int cmd_info(const char *path) {
    fuse_manifest_t *m = fuse_manifest_load(path);
    if (!m) {
        fprintf(stderr, "fuse: error: failed to load manifest at '%s'\n", path);
        return 1;
    }
    printf("=== Package Manifest: %s ===\n", path);
    fuse_manifest_dump(m);
    fuse_manifest_free(m);
    return 0;
}

static int cmd_check_deps(const char *path) {
    fuse_manifest_t *m = fuse_manifest_load(path);
    if (!m) {
        fprintf(stderr, "fuse: error: failed to load manifest at '%s'\n", path);
        return 1;
    }

    printf("Checking dependencies for package '%s' (%zu total):\n", m->name, m->dep_count);
    if (m->dep_count == 0) {
        printf("  [ OK ] No dependencies required.\n");
        fuse_manifest_free(m);
        return 0;
    }

    int missing = 0;
    for (size_t i = 0; i < m->dep_count; i++) {
        const char *dep = m->dependencies[i];
        if (fuse_manifest_check_dependency(dep, RECEIPTS_DIR)) {
            printf("  [  OK  ] %s (receipt found in %s)\n", dep, RECEIPTS_DIR);
        } else {
            printf("  [MISSING] %s (not found in %s)\n", dep, RECEIPTS_DIR);
            missing++;
        }
    }

    fuse_manifest_free(m);
    if (missing > 0) {
        printf("fuse: error: %d dependencies unsatisfied!\n", missing);
        return 1;
    }
    printf("All dependencies satisfied.\n");
    return 0;
}

static int cmd_verify(const char *path) {
    fuse_manifest_t *m = fuse_manifest_load(path);
    if (!m) {
        fprintf(stderr, "fuse: error: failed to load manifest at '%s'\n", path);
        return 1;
    }

    printf("Verifying payload files for package '%s' (prefix=%s):\n", m->name, m->install_prefix);
    bool ok = fuse_manifest_verify_files(m);
    fuse_manifest_free(m);
    if (!ok) {
        printf("fuse: error: payload verification failed.\n");
        return 1;
    }
    printf("All payload files verified successfully.\n");
    return 0;
}

static int cmd_list(void) {
    DIR *d = opendir(RECEIPTS_DIR);
    if (!d) {
        printf("No receipts directory at %s (no packages registered yet).\n", RECEIPTS_DIR);
        return 0;
    }

    printf("Installed Packages in %s:\n", RECEIPTS_DIR);
    struct dirent *de;
    int count = 0;
    char path[512];

    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        size_t len = strlen(de->d_name);
        if (len < 6 || strcmp(de->d_name + len - 6, ".plist") != 0) continue;

        snprintf(path, sizeof(path), "%s/%s", RECEIPTS_DIR, de->d_name);
        fuse_manifest_t *m = fuse_manifest_load(path);
        if (m) {
            printf("  * %-16s %-10s (Build %-3lld) - %s\n",
                   m->name, m->version, (long long)m->build,
                   m->description[0] ? m->description : m->bundle_id);
            fuse_manifest_free(m);
            count++;
        } else {
            printf("  * %s (unreadable receipt)\n", de->d_name);
            count++;
        }
    }
    closedir(d);

    printf("Total: %d package(s) installed.\n", count);
    return 0;
}

static int cmd_register(const char *path) {
    fuse_manifest_t *m = fuse_manifest_load(path);
    if (!m) {
        fprintf(stderr, "fuse: error: failed to load manifest at '%s'\n", path);
        return 1;
    }

    mkdir("/Library", 0755);
    mkdir(RECEIPTS_DIR, 0755);

    char dest[512];
    snprintf(dest, sizeof(dest), "%s/%s.plist", RECEIPTS_DIR, m->name);
    if (fuse_manifest_save(m, dest) != 0) {
        fprintf(stderr, "fuse: error: failed to write receipt to '%s'\n", dest);
        fuse_manifest_free(m);
        return 1;
    }

    printf("Successfully registered package '%s' v%s into %s\n", m->name, m->version, dest);
    fuse_manifest_free(m);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "info") == 0) {
        if (argc < 3) {
            fprintf(stderr, "fuse info: missing manifest path\n");
            return 1;
        }
        return cmd_info(argv[2]);
    } else if (strcmp(cmd, "check-deps") == 0) {
        if (argc < 3) {
            fprintf(stderr, "fuse check-deps: missing manifest path\n");
            return 1;
        }
        return cmd_check_deps(argv[2]);
    } else if (strcmp(cmd, "verify") == 0) {
        if (argc < 3) {
            fprintf(stderr, "fuse verify: missing manifest path\n");
            return 1;
        }
        return cmd_verify(argv[2]);
    } else if (strcmp(cmd, "list") == 0) {
        return cmd_list();
    } else if (strcmp(cmd, "register") == 0) {
        if (argc < 3) {
            fprintf(stderr, "fuse register: missing manifest path\n");
            return 1;
        }
        return cmd_register(argv[2]);
    } else if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "help") == 0) {
        print_usage(argv[0]);
        return 0;
    } else {
        fprintf(stderr, "fuse: unknown command '%s'\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
}

