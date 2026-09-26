#ifndef MENU_H
#define MENU_H

#include "overlay.h"
#include "config.h"

typedef struct menu menu_t;

menu_t *menu_create(overlay_t *ov) __attribute__((warn_unused_result));
void menu_destroy(menu_t *m);

void menu_update_hotkey(menu_t *m, overlay_t *ov);

bool menu_is_open(const menu_t *m);

void menu_render(menu_t *m, const overlay_event_batch_t *events, esp_config_t *cfg);

#endif /* MENU_H */
