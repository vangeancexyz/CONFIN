#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "mem.h"

#define GAME_MAX_ENEMIES 64
#define GAME_MAX_BONES   128

typedef struct { float x, y, z; } vec3_t;
typedef struct { float x, y; } vec2_t;
typedef struct { float m[16]; } view_matrix_t;
typedef struct { float m[3][4]; } bone_matrix_t; // matrix3x4_t source engine

typedef struct {
    mem_ctx_t *mem;
    mem_module_t client;
    mem_module_t engine;
} game_ctx_t;

typedef struct {
    uintptr_t entity_addr; // required for game_get_bones() after the scan
    int health;
    float distance;
    vec3_t world_pos;
    vec2_t screen_bottom;
    vec2_t screen_top;
} enemy_t;

void game_wait_attach(game_ctx_t *game, mem_ctx_t *mem);

// reads the local player. false = could not read
bool game_get_local_player(game_ctx_t *game, vec3_t *out_pos, int *out_health, uintptr_t *out_addr);

int game_scan_enemies(game_ctx_t *game, uintptr_t local_player_addr, vec3_t local_pos,
                      float screen_width, float screen_height, bool hide_teammates,
                      enemy_t *out, int max_enemies);


// int game_get_bones(game_ctx_t *game, uintptr_t entity_addr,
//                    float screen_width, float screen_height,
//                    vec2_t *out_points, int max_points);

void game_get_bones(game_ctx_t *game, uintptr_t entity_addr,
                   float screen_width, float screen_height,
                   vec2_t *out_points);

bool game_get_player_name(game_ctx_t *game, int slot, char *out_name, size_t size);

#endif /* GAME_H */
