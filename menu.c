#include "menu.h"

#include <stdlib.h>
#include <X11/keysym.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#define NK_SDL_RENDERER_SDL_H <SDL2/SDL.h>
#include "vendor/nuklear.h"
#include "vendor/nuklear_sdl_renderer.h"

#define MENU_HOTKEY XK_Insert

struct menu {
    struct nk_context *ctx;
    bool open;
    bool hotkey_was_down;
};

menu_t *menu_create(overlay_t *ov) {
    menu_t *m = calloc(1, sizeof *m);
    if (!m) return NULL;

    //m->ctx = nk_sdl_init(NULL, overlay_renderer(ov));
    m->ctx = nk_sdl_init(overlay_window(ov), overlay_renderer(ov));
    if (!m->ctx) { free(m); return NULL; }

    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    m->open = false;
    m->hotkey_was_down = false;
    return m;
}

void menu_destroy(menu_t *m) {
    if (!m) return;
    nk_sdl_shutdown();
    free(m);
}

bool menu_is_open(const menu_t *m) {
    return m->open;
}

void menu_update_hotkey(menu_t *m, overlay_t *ov) {
    bool down = overlay_is_key_down(ov, MENU_HOTKEY);

    if (down && !m->hotkey_was_down) {
        m->open = !m->open;
        overlay_set_clickable(ov, m->open);
    }
    m->hotkey_was_down = down;
}

void menu_render(menu_t *m, const overlay_event_batch_t *events, esp_config_t *cfg) {
    if (!m->open) return;

    //FIX TODO: This menu has some kind of bug where it's not clickable, it's not functional....

    nk_input_begin(m->ctx);
    for (int i = 0; i < events->count; i++) {
        SDL_Event evt = events->items[i];
        nk_sdl_handle_event(&evt);
    }
    nk_input_end(m->ctx);

    if (nk_begin(m->ctx, "CONFIN ESP", nk_rect(50, 50, 260, 220),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE)) {

        nk_layout_row_dynamic(m->ctx, 25, 1);
        nk_checkbox_label(m->ctx, "BOX ESP", &cfg->show_box);

        nk_layout_row_dynamic(m->ctx, 25, 1);
        nk_checkbox_label(m->ctx, "LIFE BAR", &cfg->show_health_bar);

        nk_layout_row_dynamic(m->ctx, 25, 1);
        nk_checkbox_label(m->ctx, "TRACER", &cfg->show_line);

        nk_layout_row_dynamic(m->ctx, 25, 1);
        nk_checkbox_label(m->ctx, "HIDDEN ALLY", &cfg->hide_teammates);
    }
    nk_end(m->ctx);

    nk_sdl_render(NK_ANTI_ALIASING_ON);
}
