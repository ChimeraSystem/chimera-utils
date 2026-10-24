/* nano -- GNU nano compatible lightweight editor for Chimera OS
 * Based on Kilo by Salvatore Sanfilippo with GNU nano UI, DEC private
 * alternate screen buffers, and nano shortcut bindings.
 */

#define NANO_VERSION "2.0.6"

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>

#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2
#define HL_MLCOMMENT 3
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[2];
    char multiline_comment_start[3];
    char multiline_comment_end[3];
    int flags;
};

typedef struct erow {
    int idx;
    int size;
    int rsize;
    char *chars;
    char *render;
    unsigned char *hl;
    int hl_oc;
} erow;

struct editorConfig {
    int cx, cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int numrows;
    int rawmode;
    erow *row;
    int dirty;
    char *filename;
    char statusmsg[128];
    time_t statusmsg_time;
    struct editorSyntax *syntax;
    char *cut_buffer;
};

static struct editorConfig E;

enum KEY_ACTION {
    KEY_NULL = 0,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_G = 7,
    CTRL_H = 8,
    TAB = 9,
    CTRL_K = 11,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_O = 15,
    CTRL_Q = 17,
    CTRL_S = 19,
    CTRL_U = 21,
    CTRL_W = 23,
    CTRL_X = 24,
    ESC = 27,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

void editorSetStatusMessage(const char *fmt, ...);

/* C Syntax Highlighting */
static char *C_HL_extensions[] = { ".c", ".h", ".cpp", ".cc", ".m", ".mm", NULL };
static char *C_HL_keywords[] = {
    "switch","if","while","for","break","continue","return","else",
    "struct","union","typedef","static","enum","class",
    "int|","long|","double|","float|","char|","unsigned|","signed|",
    "void|","bool|","uint32_t|","uint64_t|","size_t|",NULL
};

static struct editorSyntax HLDB[] = {
    {
        C_HL_extensions,
        C_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
};
#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))

static struct termios orig_termios;
static volatile sig_atomic_t resize_pending;

static int editorWriteAll(int fd, const char *data, size_t size) {
    while (size) {
        ssize_t n = write(fd, data, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        data += n;
        size -= (size_t)n;
    }
    return 0;
}

void disableRawMode(int fd) {
    if (E.rawmode) {
        editorWriteAll(STDOUT_FILENO, "\033[?1049l", sizeof("\033[?1049l") - 1);
        editorWriteAll(STDOUT_FILENO, "\033[?25h", sizeof("\033[?25h") - 1);
        tcsetattr(fd, TCSAFLUSH, &orig_termios);
        E.rawmode = 0;
    }
}

void editorAtExit(void) {
    disableRawMode(STDIN_FILENO);
}

int enableRawMode(int fd) {
    struct termios raw;
    if (E.rawmode) return 0;
    if (!isatty(STDIN_FILENO)) return -1;
    atexit(editorAtExit);
    if (tcgetattr(fd, &orig_termios) == -1) return -1;

    raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) return -1;
    E.rawmode = 1;

    editorWriteAll(STDOUT_FILENO, "\033[?1049h", sizeof("\033[?1049h") - 1);
    editorWriteAll(STDOUT_FILENO, "\033[?25h", sizeof("\033[?25h") - 1);
    return 0;
}

int editorReadKey(int fd) {
    int nread;
    char c, seq[4];
    do {
        nread = read(fd, &c, 1);
        if (resize_pending) return KEY_NULL;
    } while (nread == 0 || (nread < 0 && errno == EINTR));
    if (nread == -1) exit(0);

    while (1) {
        switch (c) {
        case ESC:
            if (read(fd, seq, 1) != 1) return ESC;
            if (read(fd, seq + 1, 1) != 1) return ESC;

            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (read(fd, seq + 2, 1) != 1) return ESC;
                    if (seq[2] == '~') {
                        switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        }
                    }
                } else {
                    switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                }
            }
            break;
        default:
            return c;
        }
    }
}

int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
    (void)ifd; (void)ofd;
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
    const char *lines_env = getenv("LINES");
    const char *cols_env = getenv("COLUMNS");
    if (lines_env && cols_env) {
        *rows = atoi(lines_env);
        *cols = atoi(cols_env);
        if (*rows > 0 && *cols > 0) return 0;
    }
    *cols = 80;
    *rows = 24;
    return 0;
}

int is_separator(int c) {
    return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];", c) != NULL;
}

int editorRowHasOpenComment(erow *row) {
    if (row->hl && row->rsize && row->hl[row->rsize - 1] == HL_MLCOMMENT &&
        (row->rsize < 2 || (row->render[row->rsize - 2] != '*' ||
                            row->render[row->rsize - 1] != '/'))) return 1;
    return 0;
}

void editorUpdateSyntax(erow *row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (E.syntax == NULL) return;

    int i, prev_sep, in_string, in_comment;
    char **keywords = E.syntax->keywords;
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    /* Point to the previous row */
    int prev_hl_oc = (row->idx > 0) ? editorRowHasOpenComment(&E.row[row->idx - 1]) : 0;

    prev_sep = 1;
    in_string = 0;
    in_comment = prev_hl_oc;

    i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        if (scs[0] && !in_string && !in_comment) {
            if (!strncmp(row->render + i, scs, strlen(scs))) {
                memset(row->hl + i, HL_COMMENT, row->rsize - i);
                break;
            }
        }

        if (mcs[0] && mce[0] && !in_string) {
            if (in_comment) {
                row->hl[i] = HL_MLCOMMENT;
                if (!strncmp(row->render + i, mce, strlen(mce))) {
                    memset(row->hl + i, HL_MLCOMMENT, strlen(mce));
                    i += strlen(mce);
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(row->render + i, mcs, strlen(mcs))) {
                memset(row->hl + i, HL_MLCOMMENT, strlen(mcs));
                i += strlen(mcs);
                in_comment = 1;
                continue;
            }
        }

        if (in_string) {
            row->hl[i] = HL_STRING;
            if (c == '\\') {
                row->hl[i + 1] = HL_STRING;
                i += 2;
                continue;
            }
            if (c == in_string) in_string = 0;
            i++;
            prev_sep = 1;
            continue;
        } else {
            if (c == '"' || c == '\'') {
                in_string = c;
                row->hl[i] = HL_STRING;
                i++;
                continue;
            }
        }

        if (!isprint(c)) {
            row->hl[i] = HL_NONPRINT;
            i++;
            prev_sep = 0;
            continue;
        }

        if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
            (c == '.' && prev_hl == HL_NUMBER)) {
            row->hl[i] = HL_NUMBER;
            i++;
            prev_sep = 0;
            continue;
        }

        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;

                if (!strncmp(row->render + i, keywords[j], klen) &&
                    is_separator(row->render[i + klen])) {
                    memset(row->hl + i, kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }

        prev_sep = is_separator(c);
        i++;
    }

    int oc = editorRowHasOpenComment(row);
    if (row->hl_oc != oc && row->idx + 1 < E.numrows)
        editorUpdateSyntax(&E.row[row->idx + 1]);
    row->hl_oc = oc;
}

int editorSyntaxToColor(int hl) {
    switch (hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 36;
    case HL_KEYWORD1:  return 33;
    case HL_KEYWORD2:  return 32;
    case HL_STRING:    return 35;
    case HL_NUMBER:    return 31;
    case HL_MATCH:     return 34;
    default:           return 37;
    }
}

void editorSelectSyntaxHighlight(char *filename) {
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i]) {
            char *p;
            int patlen = strlen(s->filematch[i]);
            if ((p = strstr(filename, s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    E.syntax = s;
                    return;
                }
            }
            i++;
        }
    }
}

void editorUpdateRow(erow *row) {
    unsigned int tabs = 0, nonprint = 0;
    int j, idx;

    free(row->render);
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == TAB) tabs++;

    unsigned long alloc_size = (unsigned long)row->size + tabs * 8 + nonprint * 9 + 1;
    row->render = malloc(alloc_size);
    idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == TAB) {
            row->render[idx++] = ' ';
            while ((idx + 1) % 8 != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    if (at != E.numrows) {
        memmove(E.row + at + 1, E.row + at, sizeof(E.row[0]) * (E.numrows - at));
        for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
    }
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].hl = NULL;
    E.row[at].hl_oc = 0;
    E.row[at].render = NULL;
    E.row[at].rsize = 0;
    E.row[at].idx = at;
    editorUpdateRow(&E.row[at]);
    E.numrows++;
    E.dirty++;
}

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(E.row + at, E.row + at + 1, sizeof(E.row[0]) * (E.numrows - at - 1));
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
    E.numrows--;
    E.dirty++;
}

char *editorRowsToString(int *buflen) {
    char *buf = NULL, *p;
    int totlen = 0;
    int j;

    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size + 1;
    *buflen = totlen;
    totlen++;

    p = buf = malloc(totlen);
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    *p = '\0';
    return buf;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(row->chars + at + 1, row->chars + at, row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(row->chars + row->size, s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(row->chars + at, row->chars + at + 1, row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

void editorInsertChar(int c) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row) {
        while (E.numrows <= filerow)
            editorInsertRow(E.numrows, "", 0);
    }
    row = &E.row[filerow];
    editorRowInsertChar(row, filecol, c);
    if (E.cx == E.screencols - 1)
        E.coloff++;
    else
        E.cx++;
}

void editorInsertNewline(void) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row) {
        if (filerow == E.numrows) {
            editorInsertRow(filerow, "", 0);
            goto fixcursor;
        }
        return;
    }
    if (filecol >= row->size)
        editorInsertRow(filerow + 1, "", 0);
    else {
        editorInsertRow(filerow + 1, row->chars + filecol, row->size - filecol);
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        editorUpdateRow(row);
    }
fixcursor:
    if (E.cy == E.screenrows - 1) {
        E.rowoff++;
    } else {
        E.cy++;
    }
    E.cx = 0;
    E.coloff = 0;
}

void editorDelChar(void) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row || (filecol == 0 && filerow == 0)) return;
    if (filecol == 0) {
        filecol = E.row[filerow - 1].size;
        editorRowAppendString(&E.row[filerow - 1], row->chars, row->size);
        editorDelRow(filerow);
        row = NULL;
        if (E.cy == 0)
            E.rowoff--;
        else
            E.cy--;
        E.cx = filecol;
        if (E.cx >= E.screencols) {
            int shift = (E.screencols - E.cx) + 1;
            E.cx -= shift;
            E.coloff += shift;
        }
    } else {
        editorRowDelChar(row, filecol - 1);
        if (E.cx == 0 && E.coloff)
            E.coloff--;
        else
            E.cx--;
    }
    if (row) editorUpdateRow(row);
}

int editorOpen(char *filename) {
    FILE *fp;
    E.dirty = 0;
    free(E.filename);
    E.filename = strdup(filename);

    fp = fopen(filename, "r");
    if (!fp) {
        if (errno != ENOENT) {
            perror("Loading file");
            return -1;
        }
        return 0;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        if (linelen && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            line[--linelen] = '\0';
        editorInsertRow(E.numrows, line, linelen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
    return 0;
}

int editorSave(void) {
    int len;
    char *buf = editorRowsToString(&len);
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) goto writeerr;

    if (ftruncate(fd, len) == -1) goto writeerr;
    if (editorWriteAll(fd, buf, len)) goto writeerr;

    close(fd);
    free(buf);
    E.dirty = 0;
    editorSetStatusMessage("[ Wrote %d lines ]", E.numrows);
    return 0;

writeerr:
    free(buf);
    if (fd != -1) close(fd);
    editorSetStatusMessage("[ Error writing file: %s ]", strerror(errno));
    return 1;
}

struct abuf {
    char *b;
    int len;
};
#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;
    memcpy(new + ab->len, s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

#define abLiteral(ab, text) abAppend((ab), (text), sizeof(text) - 1)

static void editorShortcutRow(struct abuf *ab, const char *const *items) {
    int remaining = E.screencols;
    for (int i = 0; items[i] && remaining > 0; i++) {
        int tag = remaining < 2 ? remaining : 2;
        abLiteral(ab, "\033[7m");
        abAppend(ab, items[i], tag);
        abLiteral(ab, "\033[0m");
        remaining -= tag;
        int rest = (int)strlen(items[i]) - 2;
        if (rest > remaining) rest = remaining;
        abAppend(ab, items[i] + 2, rest);
        remaining -= rest;
    }
}

void editorRefreshScreen(void) {
    int y;
    erow *r;
    char buf[64];
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\033[?25l", 6); /* Hide cursor */
    abAppend(&ab, "\033[H", 3);    /* Home */

    /* 1. Header Row (GNU nano style, reverse video SGR 7) */
    abAppend(&ab, "\033[7m", 4);
    char title[128];
    char ver[] = "GNU nano " NANO_VERSION;
    char fname_buf[64];
    snprintf(fname_buf, sizeof(fname_buf), "File: %s", E.filename ? E.filename : "New Buffer");
    char mod_buf[32] = "";
    if (E.dirty) snprintf(mod_buf, sizeof(mod_buf), "Modified");

    int left_len = snprintf(title, sizeof(title), "  %-16s %-32s", ver, fname_buf);
    int mod_len = (int)strlen(mod_buf);
    if (mod_len > E.screencols) mod_len = E.screencols;
    int left_limit = E.screencols - mod_len;
    if (left_len > left_limit) left_len = left_limit;
    abAppend(&ab, title, left_len);
    int pad = E.screencols - left_len - mod_len;
    while (pad-- > 0) abAppend(&ab, " ", 1);
    abAppend(&ab, mod_buf, mod_len);
    abAppend(&ab, "\033[0m\r\n", 6);

    /* 2. Text editing lines */
    for (y = 0; y < E.screenrows; y++) {
        int filerow = E.rowoff + y;
        if (filerow >= E.numrows) {
            abAppend(&ab, "\033[0K\r\n", 6);
            continue;
        }

        r = &E.row[filerow];
        int len = r->rsize - E.coloff;
        int current_color = -1;
        if (len > 0) {
            if (len > E.screencols) len = E.screencols;
            char *c = r->render + E.coloff;
            unsigned char *hl = r->hl + E.coloff;
            for (int j = 0; j < len; j++) {
                if (hl[j] == HL_NONPRINT) {
                    char sym = (c[j] <= 26) ? ('@' + c[j]) : '?';
                    abAppend(&ab, "\033[7m", 4);
                    abAppend(&ab, &sym, 1);
                    abAppend(&ab, "\033[0m", 4);
                } else if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        abAppend(&ab, "\033[39m", 5);
                        current_color = -1;
                    }
                    abAppend(&ab, c + j, 1);
                } else {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color) {
                        char cbuf[16];
                        int clen = snprintf(cbuf, sizeof(cbuf), "\033[%dm", color);
                        current_color = color;
                        abAppend(&ab, cbuf, clen);
                    }
                    abAppend(&ab, c + j, 1);
                }
            }
        }
        abLiteral(&ab, "\033[39m");
        if (len < E.screencols) abLiteral(&ab, "\033[0K");
        abLiteral(&ab, "\r\n");
    }

    /* 3. Status message bar */
    abAppend(&ab, "\033[0K", 4);
    int msglen = strlen(E.statusmsg);
    if (msglen && time(NULL) - E.statusmsg_time < 5) {
        if (msglen > E.screencols) msglen = E.screencols;
        abAppend(&ab, E.statusmsg, msglen);
    }
    abAppend(&ab, "\r\n", 2);

    /* 4. Bottom Two Shortcut Rows (GNU nano style with reverse tags) */
    /* Row 1 */
    abAppend(&ab, "\033[0K", 4);
    static const char *const first[] = {"^G Get Help  ", "^O WriteOut  ", "^R Read File  ",
        "^Y Prev Page  ", "^K Cut Text    ", "^C Cur Pos", NULL};
    editorShortcutRow(&ab, first);
    abLiteral(&ab, "\r\n");

    /* Row 2 */
    abAppend(&ab, "\033[0K", 4);
    static const char *const second[] = {"^X Exit      ", "^J Justify   ", "^W Where Is   ",
        "^V Next Page  ", "^U UnCut Text  ", "^T To Spell", NULL};
    editorShortcutRow(&ab, second);

    /* 5. Position cursor */
    int cx = 1;
    int filerow = E.rowoff + E.cy;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    if (row) {
        for (int j = E.coloff; j < (E.cx + E.coloff); j++) {
            if (j < row->size && row->chars[j] == TAB) cx += 7 - ((cx) % 8);
            cx++;
        }
    }
    /* +2 because row 1 is 1-based, and row 1 is the header */
    snprintf(buf, sizeof(buf), "\033[%d;%dH", E.cy + 2, cx);
    abAppend(&ab, buf, strlen(buf));
    abAppend(&ab, "\033[?25h", 6); /* Show cursor */

    editorWriteAll(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

void editorFind(int fd) {
    char query[128] = {0};
    int qlen = 0;
    int last_match = -1;
    int find_next = 0;

    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    while (1) {
        editorSetStatusMessage("Search: %s (ESC to cancel)", query);
        editorRefreshScreen();

        int c = editorReadKey(fd);
        if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
            if (qlen != 0) query[--qlen] = '\0';
            last_match = -1;
        } else if (c == ESC || c == ENTER) {
            if (c == ESC) {
                E.cx = saved_cx; E.cy = saved_cy;
                E.coloff = saved_coloff; E.rowoff = saved_rowoff;
            }
            editorSetStatusMessage("");
            return;
        } else if (c == ARROW_RIGHT || c == ARROW_DOWN) {
            find_next = 1;
        } else if (c == ARROW_LEFT || c == ARROW_UP) {
            find_next = -1;
        } else if (isprint(c)) {
            if (qlen < (int)sizeof(query) - 1) {
                query[qlen++] = c;
                query[qlen] = '\0';
                last_match = -1;
            }
        }

        if (last_match == -1) find_next = 1;
        if (find_next && qlen > 0) {
            char *match = NULL;
            int current = last_match;
            for (int i = 0; i < E.numrows; i++) {
                current += find_next;
                if (current == -1) current = E.numrows - 1;
                else if (current == E.numrows) current = 0;
                match = strstr(E.row[current].render, query);
                if (match) {
                    last_match = current;
                    E.cy = 0;
                    E.cx = match - E.row[current].render;
                    E.rowoff = current;
                    E.coloff = 0;
                    break;
                }
            }
            find_next = 0;
        }
    }
}

void editorMoveCursor(int key) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch (key) {
    case ARROW_LEFT:
        if (E.cx == 0) {
            if (E.coloff) E.coloff--;
            else if (filerow > 0) {
                E.cy--;
                E.cx = E.row[filerow - 1].size;
                if (E.cx > E.screencols - 1) {
                    E.coloff = E.cx - E.screencols + 1;
                    E.cx = E.screencols - 1;
                }
            }
        } else {
            E.cx--;
        }
        break;
    case ARROW_RIGHT:
        if (row && filecol < row->size) {
            if (E.cx == E.screencols - 1) E.coloff++;
            else E.cx++;
        } else if (row && filecol == row->size) {
            E.cx = 0;
            E.coloff = 0;
            if (E.cy == E.screenrows - 1) E.rowoff++;
            else E.cy++;
        }
        break;
    case ARROW_UP:
        if (E.cy == 0) {
            if (E.rowoff) E.rowoff--;
        } else {
            E.cy--;
        }
        break;
    case ARROW_DOWN:
        if (filerow < E.numrows) {
            if (E.cy == E.screenrows - 1) E.rowoff++;
            else E.cy++;
        }
        break;
    }

    filerow = E.rowoff + E.cy;
    filecol = E.coloff + E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    int rowlen = row ? row->size : 0;
    if (filecol > rowlen) {
        E.cx -= filecol - rowlen;
        if (E.cx < 0) {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

void editorProcessKeypress(int fd) {
    int c = editorReadKey(fd);
    switch (c) {
    case KEY_NULL:
        break;
    case ENTER:
        editorInsertNewline();
        break;
    case CTRL_X: /* nano Exit */
    case CTRL_Q:
        if (E.dirty) {
            editorSetStatusMessage("Save modified buffer (ANSWERING \"No\" WILL DESTROY CHANGES) ? (Y/N)");
            editorRefreshScreen();
            int conf = editorReadKey(fd);
            if (conf == 'y' || conf == 'Y') {
                if (editorSave() == 0) exit(0);
                return;
            } else if (conf == 'n' || conf == 'N') {
                exit(0);
            } else {
                editorSetStatusMessage("[ Cancelled ]");
                return;
            }
        }
        exit(0);
        break;
    case CTRL_O: /* nano WriteOut (Save) */
    case CTRL_S:
        editorSave();
        break;
    case CTRL_W: /* nano Where Is (Find) */
        editorFind(fd);
        break;
    case CTRL_G: /* nano Get Help */
        editorSetStatusMessage("^G Help  ^O WriteOut  ^W WhereIs  ^K Cut  ^U Uncut  ^C CurPos  ^X Exit");
        break;
    case CTRL_C: /* nano Cur Pos */ {
        int filerow = E.rowoff + E.cy + 1;
        int filecol = E.coloff + E.cx + 1;
        float pct = E.numrows ? ((float)filerow / (float)E.numrows * 100.0f) : 100.0f;
        editorSetStatusMessage("line %d/%d (%.0f%%), col %d", filerow, E.numrows, pct, filecol);
        break;
    }
    case CTRL_K: /* nano Cut line */ {
        int filerow = E.rowoff + E.cy;
        if (filerow < E.numrows) {
            free(E.cut_buffer);
            E.cut_buffer = strdup(E.row[filerow].chars);
            editorDelRow(filerow);
            editorSetStatusMessage("[ Cut line to buffer ]");
        }
        break;
    }
    case CTRL_U: /* nano Uncut line */ {
        if (E.cut_buffer) {
            int filerow = E.rowoff + E.cy;
            editorInsertRow(filerow, E.cut_buffer, strlen(E.cut_buffer));
            editorSetStatusMessage("[ Uncut line ]");
        }
        break;
    }
    case BACKSPACE:
    case CTRL_H:
    case DEL_KEY:
        editorDelChar();
        break;
    case PAGE_UP:
    case PAGE_DOWN:
        if (c == PAGE_UP && E.cy != 0) E.cy = 0;
        else if (c == PAGE_DOWN && E.cy != E.screenrows - 1) E.cy = E.screenrows - 1;
        {
            int times = E.screenrows;
            while (times--) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
        break;
    case HOME_KEY:
        E.cx = 0;
        E.coloff = 0;
        break;
    case END_KEY:
        if (E.rowoff + E.cy < E.numrows) {
            E.cx = E.row[E.rowoff + E.cy].size;
            if (E.cx >= E.screencols) {
                E.coloff = E.cx - E.screencols + 1;
                E.cx = E.screencols - 1;
            }
        }
        break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;
    case CTRL_L:
        editorRefreshScreen();
        break;
    case ESC:
        break;
    default:
        editorInsertChar(c);
        break;
    }
}

void updateWindowSize(void) {
    if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &E.screenrows, &E.screencols) == -1) {
        perror("Unable to query screen size");
        exit(1);
    }
    /* Room for header (1), status msg (1), and 2-line nano shortcut bar (2) */
    E.screenrows -= 4;
    if (E.screenrows < 1) E.screenrows = 1;
}

void handleSigWinCh(int sig) {
    (void)sig;
    resize_pending = 1;
}

void initEditor(void) {
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;
    E.cut_buffer = NULL;
    updateWindowSize();
    signal(SIGWINCH, handleSigWinCh);
}

int main(int argc, char **argv) {
    initEditor();
    if (argc >= 2) {
        editorSelectSyntaxHighlight(argv[1]);
        editorOpen(argv[1]);
    } else {
        E.filename = strdup("New Buffer");
    }

    enableRawMode(STDIN_FILENO);
    editorSetStatusMessage("  GNU nano " NANO_VERSION "             ^G Get Help  ^O WriteOut  ^X Exit");

    while (1) {
        if (resize_pending) {
            resize_pending = 0;
            updateWindowSize();
            if (E.cy >= E.screenrows) E.cy = E.screenrows - 1;
            if (E.cx >= E.screencols) E.cx = E.screencols - 1;
        }
        editorRefreshScreen();
        editorProcessKeypress(STDIN_FILENO);
    }
    return 0;
}
