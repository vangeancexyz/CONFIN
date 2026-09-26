#include "draw.h"

#include <math.h>

void draw_thick_line(SDL_Renderer *r, float x1, float y1, float x2, float y2,
                      float thickness, SDL_Color color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 0.0001f) return;

    float nx = -dy / length * (thickness / 2.0f);
    float ny =  dx / length * (thickness / 2.0f);

    SDL_Vertex verts[4] = {
        { { x1 + nx, y1 + ny }, color, { 0.0f, 0.0f } },
        { { x1 - nx, y1 - ny }, color, { 0.0f, 0.0f } },
        { { x2 - nx, y2 - ny }, color, { 0.0f, 0.0f } },
        { { x2 + nx, y2 + ny }, color, { 0.0f, 0.0f } },
    };
    int indices[6] = { 0, 1, 2, 0, 2, 3 };

    SDL_RenderGeometry(r, NULL, verts, 4, indices, 6);
}

static SDL_Rect compute_box_rect(vec2_t top, vec2_t bottom) {
    float height = bottom.y - top.y;
    float width = height / 2.2f;

    SDL_Rect rect;
    rect.x = (int)(top.x - width / 2.0f);
    rect.y = (int)top.y;
    rect.w = (int)width;
    rect.h = (int)height;
    return rect;
}

static void draw_box_outline(SDL_Renderer *r, SDL_Rect box, SDL_Color color) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_Rect outer = { box.x - 1, box.y - 1, box.w + 2, box.h + 2 };
    SDL_Rect inner = { box.x + 1, box.y + 1, box.w - 2, box.h - 2 };
    SDL_RenderDrawRect(r, &outer);
    SDL_RenderDrawRect(r, &inner);

    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(r, &box);
}

static void draw_health_bar(SDL_Renderer *r, SDL_Rect box, int health) {
    const int bar_thickness = 3;
    int bar_x = box.x - bar_thickness - 3;
    int bar_y = box.y;
    int bar_h = box.h;

    float hp_percent = (health > 100) ? 1.0f : (health / 100.0f);
    int fill_h = (int)(bar_h * hp_percent);
    int fill_y = bar_y + (bar_h - fill_h);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_Rect bg = { bar_x - 1, bar_y - 1, bar_thickness + 2, bar_h + 2 };
    SDL_RenderFillRect(r, &bg);

    if (health > 50)      SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
    else if (health > 20) SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
    else                   SDL_SetRenderDrawColor(r, 255, 0, 0, 255);

    SDL_Rect fill = { bar_x, fill_y, bar_thickness, fill_h };
    SDL_RenderFillRect(r, &fill);
}

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
               int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(r, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
               }

void draw_health_text(SDL_Renderer *r, TTF_Font *font, SDL_Rect box, int health) {
    char buf[16];
    snprintf(buf, sizeof buf, "%d HP", health);
    SDL_Color color = {255, 255, 255, 255};
    draw_text(r, font, buf, box.x, box.y + box.h + 4, color);
}

// void draw_player_name(SDL_Renderer *r, TTF_Font *font, SDL_Rect box, const char *name);

void draw_enemy_esp(SDL_Renderer *r, float screen_width, float screen_height,
                     const enemy_t *enemy, SDL_Color color, float line_thickness,
                     TTF_Font *font,
                     const char *player_name,
                     const esp_config_t *cfg) {
    SDL_Rect box = compute_box_rect(enemy->screen_top, enemy->screen_bottom);

    draw_health_text(r, font, box, enemy->health);

    if (cfg->show_box) draw_box_outline(r, box, color);
    if (cfg->show_health_bar) draw_health_bar(r, box, enemy->health);

    if (cfg->show_line) {
        draw_thick_line(r, screen_width / 2.0f, screen_height,
                         enemy->screen_bottom.x, enemy->screen_bottom.y,
                         line_thickness, color);
    }
}

void draw_skeleton_points(SDL_Renderer *r, const vec2_t *points, int count, SDL_Color color) {
    const float radius = 3.0f;
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

	// for (int i = 0; i < count; i++) {
	// 	if (points[i].x != 0.0f || points[i].y != 0.0f) {
	// 		printf("bone[%d] = (%.0f, %.0f)\n", i, points[i].x, points[i].y);
	// 	}
	// }

    for (int i = 0; i < count; i++) {
        SDL_Rect dot;
        dot.x = (int)(points[i].x - radius);
        dot.y = (int)(points[i].y - radius);
        dot.w = (int)(radius * 2.0f);
        dot.h = (int)(radius * 2.0f);
        SDL_RenderFillRect(r, &dot);
    }
}

void draw_skeleton(SDL_Renderer *r, const vec2_t *bones, SDL_Color color, float thickness) {

    static const int pairs[][2] = {
        {14, 13}, {13, 12}, {12, 11}, {11, 0},              // spine - head

        {14, 15}, {15, 16}, {16, 17}, {17, 43},             // left arm
        {14, 28}, {28, 30}, {30, 44}, {44, 42},             // right arm

        {0, 6},   {6, 7},   {7, 8},                         // left leg
        {0, 2},   {2, 3},   {3, 4},                         // right leg
    };

    int num_pairs = sizeof(pairs) / sizeof(pairs[0]);

    for (int i = 0; i < num_pairs; i++) {
        int idA = pairs[i][0];
        int idB = pairs[i][1];


        if (bones[idA].x != 0.0f && bones[idB].x != 0.0f ) {
		//if (bones[idA].x != -1.0f && bones[idB].x != -1.0f) {
            draw_thick_line(r, bones[idA].x, bones[idA].y,
                               bones[idB].x, bones[idB].y,
                               thickness, color);
        }
    }
}
