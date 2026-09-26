#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>
#include "game.h"
#include "config.h"
#include <SDL2/SDL_ttf.h>

// quad via SDL_RenderGeometry
void draw_thick_line(SDL_Renderer *r, float x1, float y1, float x2, float y2,
                      float thickness, SDL_Color color);

void draw_enemy_esp(SDL_Renderer *r, float screen_width, float screen_height,
                     const enemy_t *enemy, SDL_Color color, float line_thickness,
                     TTF_Font *font,
                     const char *player_name,
                     const esp_config_t *cfg);

void draw_skeleton_points(SDL_Renderer *r, const vec2_t *points, int count, SDL_Color color);

void draw_skeleton(SDL_Renderer *r, const vec2_t *bones, SDL_Color color, float thickness);

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
               int x, int y, SDL_Color color);

void draw_health_text(SDL_Renderer *r, TTF_Font *font, SDL_Rect box, int health);

typedef struct {
    TTF_Font *font_small;  // 12px — HP
    TTF_Font *font_name;   // 14px — name
} draw_fonts_t;


#endif /* DRAW_H */
