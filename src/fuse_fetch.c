#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct download {
    int fd;
    uint64_t size, limit;
};

static size_t receive(char *data, size_t size, size_t count, void *opaque) {
    struct download *output = opaque;
    if (size && count > SIZE_MAX / size) return 0;
    size_t length = size * count, offset = 0;
    if (length > output->limit - output->size) return 0;
    while (offset < length) {
        ssize_t written = write(output->fd, data + offset, length - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return 0;
        offset += (size_t)written;
    }
    output->size += length;
    return length;
}

int main(int argc, char **argv) {
    if (argc != 5 && (argc != 6 || strcmp(argv[5], "--allow-http"))) {
        fprintf(stderr, "usage: fuse-fetch URL OUTPUT MAX_BYTES ACCEPT [--allow-http]\n");
        return 1;
    }
    char *end;
    errno = 0;
    unsigned long long limit = strtoull(argv[3], &end, 10);
    if (errno || *end || !limit || limit > 512ULL * 1024 * 1024) return 1;
    if (strchr(argv[4], '\r') || strchr(argv[4], '\n') || strlen(argv[4]) > 128) return 1;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 1;
    CURL *curl = curl_easy_init();
    CURLU *url = curl_url();
    struct curl_slist *headers = NULL;
    struct download output = {.fd = -1, .limit = limit};
    char *host = NULL, *scheme = NULL;
    int result = 1;
    char errors[CURL_ERROR_SIZE] = {0}, accept[160], authorization[1200];
    if (!curl || !url || curl_url_set(url, CURLUPART_URL, argv[1], 0) ||
        curl_url_get(url, CURLUPART_HOST, &host, 0) || curl_url_get(url, CURLUPART_SCHEME, &scheme, 0)) goto done;
    if (strcmp(scheme, "https") && (argc != 6 || strcmp(scheme, "http"))) {
        fprintf(stderr, "fuse-fetch: HTTPS is required\n");
        goto done;
    }
    snprintf(accept, sizeof(accept), "Accept: %s", argv[4]);
    headers = curl_slist_append(NULL, accept);
    if (!headers) goto done;
    const char *token = getenv("FUSE_GITHUB_TOKEN");
    if (token && *token && !strcmp(scheme, "https") &&
        (!strcmp(host, "api.github.com") || !strcmp(host, "raw.githubusercontent.com"))) {
        if (strlen(token) > 1024) goto done;
        for (const char *p = token; *p; p++) if ((unsigned char)*p < 33 || (unsigned char)*p > 126) goto done;
        snprintf(authorization, sizeof(authorization), "Authorization: Bearer %s", token);
        struct curl_slist *next = curl_slist_append(headers, authorization);
        if (!next) goto done;
        headers = next;
    }
    output.fd = open(argv[2], O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (output.fd < 0) {
        fprintf(stderr, "fuse-fetch: cannot create output: %s\n", strerror(errno));
        goto done;
    }
#define SET(option, value) do { if (curl_easy_setopt(curl, option, value) != CURLE_OK) goto done; } while (0)
    SET(CURLOPT_URL, argv[1]);
    SET(CURLOPT_HTTPHEADER, headers);
    SET(CURLOPT_USERAGENT, "Chimera-Fuse/1");
    SET(CURLOPT_ERRORBUFFER, errors);
    SET(CURLOPT_PROTOCOLS_STR, argc == 6 ? "https,http" : "https");
    SET(CURLOPT_REDIR_PROTOCOLS_STR, argc == 6 ? "https,http" : "https");
    SET(CURLOPT_FOLLOWLOCATION, 1L);
    SET(CURLOPT_MAXREDIRS, 8L);
    SET(CURLOPT_UNRESTRICTED_AUTH, 0L);
    SET(CURLOPT_SSL_VERIFYPEER, 1L);
    SET(CURLOPT_SSL_VERIFYHOST, 2L);
    SET(CURLOPT_FAILONERROR, 1L);
    SET(CURLOPT_CONNECTTIMEOUT, 20L);
    SET(CURLOPT_TIMEOUT, 1800L);
    SET(CURLOPT_LOW_SPEED_LIMIT, 256L);
    SET(CURLOPT_LOW_SPEED_TIME, 60L);
    SET(CURLOPT_NOSIGNAL, 1L);
    SET(CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    SET(CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)limit);
    SET(CURLOPT_WRITEFUNCTION, receive);
    SET(CURLOPT_WRITEDATA, &output);
    CURLcode status = curl_easy_perform(curl);
    if (status != CURLE_OK) {
        fprintf(stderr, "fuse-fetch: %s\n", errors[0] ? errors : curl_easy_strerror(status));
        goto done;
    }
    if (fsync(output.fd)) goto done;
    result = 0;
done:
    if (output.fd >= 0) {
        if (close(output.fd)) result = 1;
        if (result) unlink(argv[2]);
    }
    curl_free(host);
    curl_free(scheme);
    curl_slist_free_all(headers);
    if (url) curl_url_cleanup(url);
    if (curl) curl_easy_cleanup(curl);
    curl_global_cleanup();
    return result;
}
