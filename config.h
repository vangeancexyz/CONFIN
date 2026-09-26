#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct {
    bool show_box;
    bool show_health_bar;
    bool show_line;
    bool hide_teammates;
	bool show_skeleton;
} esp_config_t;

static inline esp_config_t config_default(void) {
    esp_config_t cfg;
    cfg.show_box = true;
    cfg.show_health_bar = true;
    cfg.show_line = false;
    cfg.hide_teammates = true;
	cfg.show_skeleton = true;
    return cfg;
}

#endif /* CONFIG_H */
