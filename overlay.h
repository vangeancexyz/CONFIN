#ifndef OVERLAY_H
#define OVERLAY_H

#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct overlay overlay_t;

typedef struct {
    int x, y;
    int width, height;
} overlay_config_t;

#define OVERLAY_MAX_EVENTS 64

typedef struct {
    SDL_Event items[OVERLAY_MAX_EVENTS];
    int count;
    bool quit_requested;
} overlay_event_batch_t;

SDL_Window *overlay_window(overlay_t *ov);
overlay_t *overlay_create(const overlay_config_t *cfg) __attribute__((warn_unused_result));
void overlay_destroy(overlay_t *ov);

SDL_Renderer *overlay_renderer(overlay_t *ov);

overlay_event_batch_t overlay_poll_events(overlay_t *ov);

void overlay_frame_begin(overlay_t *ov);
void overlay_frame_end(overlay_t *ov);

void overlay_set_clickable(overlay_t *ov, bool clickable);
bool overlay_is_key_down(overlay_t *ov, long x11_keysym);

#endif /* OVERLAY_H */
