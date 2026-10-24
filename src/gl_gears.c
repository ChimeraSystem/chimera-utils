
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include <GL/gl.h>
#include <gui.h>
#include <font8x16.h>

#define WIN_W 480
#define WIN_H 360

static inline uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static void draw_char(gui_window_t *win, int px, int py, char c, uint32_t fg) {
    if (c < 0 || c > 127) c = '?';
    const uint8_t *glyph = &g_font8x16[(uint8_t)c * 16];
    for (int y = 0; y < 16; y++) {
        uint8_t bits = glyph[y];
        int ty = py + y;
        if (ty < 0 || ty >= (int)win->height) continue;
        for (int x = 0; x < 8; x++) {
            int tx = px + x;
            if (tx < 0 || tx >= (int)win->width) continue;
            if (bits & (1 << x)) {
                win->surface[ty * win->stride + tx] = fg;
            }
        }
    }
}

static void draw_string(gui_window_t *win, int px, int py, const char *str, uint32_t fg) {
    while (*str) {
        draw_char(win, px, py, *str++, fg);
        px += 8;
    }
}

static void draw_fps_hud(gui_window_t *win, float fps, float ms) {
    int bx = 12, by = 12, bw = 170, bh = 50;

    for (int y = by; y < by + bh; y++) {
        if (y < 0 || y >= (int)win->height) continue;
        for (int x = bx; x < bx + bw; x++) {
            if (x < 0 || x >= (int)win->width) continue;
            bool is_border = (x == bx || x == bx + bw - 1 || y == by || y == by + bh - 1);
            if (is_border) {
                win->surface[y * win->stride + x] = 0xFF4A5568;
            } else {
                uint32_t bg = win->surface[y * win->stride + x];
                uint8_t r = ((bg >> 16) & 0xFF) / 4 + 12;
                uint8_t g = ((bg >> 8) & 0xFF) / 4 + 14;
                uint8_t b = (bg & 0xFF) / 4 + 20;
                win->surface[y * win->stride + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }

    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "FPS: %.1f", (double)fps);
    char ms_buf[32];
    snprintf(ms_buf, sizeof(ms_buf), "%.1f ms (CPU)", (double)ms);

    draw_string(win, bx + 10, by + 6, fps_buf, 0xFF00FF88);
    draw_string(win, bx + 10, by + 26, ms_buf, 0xFFA0AEC0);
}

static void draw_gear(float inner_radius, float outer_radius, float width,
                      int teeth, float tooth_depth,
                      float r, float g, float b) {
    int i;
    float r0 = inner_radius;
    float r1 = outer_radius - tooth_depth * 0.5f;
    float r2 = outer_radius + tooth_depth * 0.5f;
    float da = 3.14159265f / (float)teeth / 2.0f;
    float half_w = width * 0.5f;

    const float lx = 0.57735f, ly = 0.57735f, lz = 0.57735f;

    float front_lit = 0.40f + 0.60f * fmaxf(0.0f, lz);
    glColor3f(r * front_lit, g * front_lit, b * front_lit);

    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++) {
        float angle = (float)i * 2.0f * 3.14159265f / (float)teeth;

        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), half_w);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), half_w);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), half_w);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), half_w);

        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), half_w);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), half_w);
        glVertex3f(r1 * cosf(angle + 4.0f * da), r1 * sinf(angle + 4.0f * da), half_w);
        glVertex3f(r0 * cosf(angle + 4.0f * da), r0 * sinf(angle + 4.0f * da), half_w);
    }
    glEnd();

    float back_lit = 0.35f + 0.65f * fmaxf(0.0f, -lz);
    glColor3f(r * back_lit, g * back_lit, b * back_lit);

    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++) {
        float angle = (float)i * 2.0f * 3.14159265f / (float)teeth;

        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -half_w);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -half_w);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -half_w);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -half_w);

        glVertex3f(r0 * cosf(angle + 4.0f * da), r0 * sinf(angle + 4.0f * da), -half_w);
        glVertex3f(r1 * cosf(angle + 4.0f * da), r1 * sinf(angle + 4.0f * da), -half_w);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -half_w);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -half_w);
    }
    glEnd();

    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++) {
        float angle = (float)i * 2.0f * 3.14159265f / (float)teeth;

        float u1 = r2 * cosf(angle + da) - r1 * cosf(angle);
        float v1 = r2 * sinf(angle + da) - r1 * sinf(angle);
        float len1 = sqrtf(u1 * u1 + v1 * v1);
        if (len1 > 0.0001f) { u1 /= len1; v1 /= len1; }
        float nx1 = v1, ny1 = -u1;
        float lit1 = 0.35f + 0.65f * fmaxf(0.0f, nx1 * lx + ny1 * ly);
        glColor3f(r * lit1, g * lit1, b * lit1);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), half_w);
        glVertex3f(r1 * cosf(angle), r1 * sinf(angle), -half_w);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -half_w);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), half_w);

        float nx2 = cosf(angle + 1.5f * da), ny2 = sinf(angle + 1.5f * da);
        float lit2 = 0.35f + 0.65f * fmaxf(0.0f, nx2 * lx + ny2 * ly);
        glColor3f(r * lit2, g * lit2, b * lit2);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), half_w);
        glVertex3f(r2 * cosf(angle + da), r2 * sinf(angle + da), -half_w);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -half_w);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), half_w);

        float u2 = r1 * cosf(angle + 3.0f * da) - r2 * cosf(angle + 2.0f * da);
        float v2 = r1 * sinf(angle + 3.0f * da) - r2 * sinf(angle + 2.0f * da);
        float len2 = sqrtf(u2 * u2 + v2 * v2);
        if (len2 > 0.0001f) { u2 /= len2; v2 /= len2; }
        float nx3 = v2, ny3 = -u2;
        float lit3 = 0.35f + 0.65f * fmaxf(0.0f, nx3 * lx + ny3 * ly);
        glColor3f(r * lit3, g * lit3, b * lit3);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), half_w);
        glVertex3f(r2 * cosf(angle + 2.0f * da), r2 * sinf(angle + 2.0f * da), -half_w);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -half_w);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), half_w);

        float nx4 = cosf(angle + 3.5f * da), ny4 = sinf(angle + 3.5f * da);
        float lit4 = 0.35f + 0.65f * fmaxf(0.0f, nx4 * lx + ny4 * ly);
        glColor3f(r * lit4, g * lit4, b * lit4);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), half_w);
        glVertex3f(r1 * cosf(angle + 3.0f * da), r1 * sinf(angle + 3.0f * da), -half_w);
        glVertex3f(r1 * cosf(angle + 4.0f * da), r1 * sinf(angle + 4.0f * da), -half_w);
        glVertex3f(r1 * cosf(angle + 4.0f * da), r1 * sinf(angle + 4.0f * da), half_w);
    }
    glEnd();

    glBegin(GL_QUADS);
    for (i = 0; i < teeth; i++) {
        float angle = (float)i * 2.0f * 3.14159265f / (float)teeth;
        float nx = -cosf(angle + 2.0f * da), ny = -sinf(angle + 2.0f * da);
        float lit = 0.25f + 0.50f * fmaxf(0.0f, nx * lx + ny * ly);
        glColor3f(r * lit, g * lit, b * lit);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), -half_w);
        glVertex3f(r0 * cosf(angle), r0 * sinf(angle), half_w);
        glVertex3f(r0 * cosf(angle + 4.0f * da), r0 * sinf(angle + 4.0f * da), half_w);
        glVertex3f(r0 * cosf(angle + 4.0f * da), r0 * sinf(angle + 4.0f * da), -half_w);
    }
    glEnd();
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("[gl_gears] Starting Chimera legacy CPU 3D demo...\n");

    gui_window_t *win = gui_create_window("Chimera CPU 3D Demo", WIN_W, WIN_H);
    if (!win) {
        fprintf(stderr, "[gl_gears] Error: failed to connect to window compositor\n");
        return 1;
    }

    gl_context_t *ctx = glCreateContext(win, WIN_W, WIN_H);
    if (!ctx) {
        fprintf(stderr, "[gl_gears] Error: failed to create OpenGL context\n");
        gui_destroy_window(win);
        return 1;
    }

    printf("[gl_gears] Context initialized: %s | %s\n",
           glGetString(GL_RENDERER), glGetString(GL_VERSION));

    glViewport(0, 0, WIN_W, WIN_H);
    glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    float view_rot_x = 20.0f;
    float view_rot_y = 30.0f;
    float gear_angle = 0.0f;
    bool auto_orbit = true;
    bool mouse_dragging = false;
    int32_t last_mx = 0, last_my = 0;
    bool running = true;

    uint64_t last_fps_time = get_time_us();
    uint32_t frame_count = 0;
    float cur_fps = 60.0f;
    float cur_ms = 16.6f;

    while (running) {
        uint64_t frame_start = get_time_us();

        gui_event_t ev;
        while (gui_poll_event(win, &ev)) {
            if (ev.type == GUI_EVENT_CLOSE) {
                running = false;
                break;
            } else if (ev.type == GUI_EVENT_MOUSE_DOWN) {
                mouse_dragging = true;
                last_mx = ev.mouse_x;
                last_my = ev.mouse_y;
                auto_orbit = false;
            } else if (ev.type == GUI_EVENT_MOUSE_UP) {
                mouse_dragging = false;
            } else if (ev.type == GUI_EVENT_MOUSE_MOVE) {
                if (mouse_dragging) {
                    float dx = (float)(ev.mouse_x - last_mx);
                    float dy = (float)(ev.mouse_y - last_my);
                    view_rot_y += dx * 0.6f;
                    view_rot_x += dy * 0.6f;
                    last_mx = ev.mouse_x;
                    last_my = ev.mouse_y;
                }
            } else if (ev.type == GUI_EVENT_KEY_DOWN) {
                if (ev.ascii == ' ' || ev.keycode == 57) {
                    auto_orbit = !auto_orbit;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glScalef(0.08f, 0.10667f, 0.08f);
        if (auto_orbit) {
            float orbit_t = (float)gear_angle * 0.0174532925f;
            glRotatef(20.0f + 6.0f * sinf(orbit_t * 0.8f), 1.0f, 0.0f, 0.0f);
            glRotatef(30.0f + 10.0f * sinf(orbit_t * 0.5f), 0.0f, 1.0f, 0.0f);
        } else {
            glRotatef(view_rot_x, 1.0f, 0.0f, 0.0f);
            glRotatef(view_rot_y, 0.0f, 1.0f, 0.0f);
        }

        glTranslatef(0.8f, -0.6f, 0.0f);

        glPushMatrix();
        glTranslatef(-3.0f, -2.0f, 0.0f);
        glRotatef(gear_angle, 0.0f, 0.0f, 1.0f);
        draw_gear(1.0f, 4.0f, 1.0f, 20, 0.7f, 0.85f, 0.15f, 0.10f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(3.1f, -2.0f, 0.0f);
        glRotatef(-2.0f * gear_angle - 9.0f, 0.0f, 0.0f, 1.0f);
        draw_gear(0.5f, 2.0f, 2.0f, 10, 0.7f, 0.10f, 0.80f, 0.25f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-3.1f, 4.2f, 0.0f);
        glRotatef(-2.0f * gear_angle - 25.0f, 0.0f, 0.0f, 1.0f);
        draw_gear(1.3f, 2.0f, 0.5f, 10, 0.7f, 0.20f, 0.35f, 0.95f);
        glPopMatrix();

        glSwapBuffers(ctx);

        draw_fps_hud(win, cur_fps, cur_ms);
        /* The HUD is drawn after glSwapBuffers published the gear image. */
        gui_flush_window(win, 12, 12, 170, 50);

        frame_count++;
        uint64_t now = get_time_us();
        if (now - last_fps_time >= 1000000ULL) {
            uint64_t elapsed = now - last_fps_time;
            cur_fps = (float)frame_count * 1000000.0f / (float)elapsed;
            cur_ms = (float)elapsed / ((float)frame_count * 1000.0f);
            printf("[gl_gears] %.1f FPS (%.2f ms/frame) | Chimera legacy CPU rasterizer (3-Gear Train)\n",
                   (double)cur_fps, (double)cur_ms);
            frame_count = 0;
            last_fps_time = now;
        }

        gear_angle += 1.5f;
        if (gear_angle >= 3600.0f) gear_angle -= 3600.0f;

        uint64_t frame_duration = get_time_us() - frame_start;
        uint32_t target_hz = 280;
        const char *env_hz = getenv("CHIMERA_HZ");
        if (!env_hz) env_hz = getenv("HZ");
        if (env_hz) {
            uint32_t h = (uint32_t)atoi(env_hz);
            if (h >= 30 && h <= 1000) target_hz = h;
        }
        uint64_t target_frame_us = 1000000ULL / target_hz;
        if (frame_duration < target_frame_us) {
            usleep((useconds_t)(target_frame_us - frame_duration));
        }
    }

    glDestroyContext(ctx);
    gui_destroy_window(win);
    printf("[gl_gears] Exiting.\n");
    return 0;
}
