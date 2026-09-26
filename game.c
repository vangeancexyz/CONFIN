#include "game.h"

#include <math.h>
#include <unistd.h>

// engine.so
static const uintptr_t DW_LOCAL_PLAYER  = 0xf946b0;
static const uintptr_t DW_ENTITY_LIST   = 0xfa8508;
static const uintptr_t DW_VIEW_MATRIX   = 0xB92D44;
static const uintptr_t M_I_HEALTH       = 0xc0;
static const uintptr_t M_VEC_ORIGIN     = 0x2f8;
static const uintptr_t ENT_ENTRY_SIZE   = 0x20;
static const uintptr_t DW_ENGINE_CLIENT = 0xA12838;

// client.so
static const uintptr_t M_BONE_ARRAY_PTR = 0xB40;
static const uintptr_t M_BONE_COUNT     = 0xB50;
static const uintptr_t M_B_DORMANT      = 0x1EA;
static const uintptr_t M_I_TEAM_NUM     = 0xC8;


// FOR INTERNAL VERSION
// a VTable do ClientModeShared = CreateMove
// static const uintptr_t M_B_HOP =

bool game_get_player_name(game_ctx_t *game, int slot, char *out_name, size_t size) {
    uintptr_t iface = MEM_READ(game->mem,
                               game->engine.base + DW_ENGINE_CLIENT,
                               uintptr_t);
    if (iface == 0) return false;

    // player_info_t starts at slot * 0x84 within the array
    uintptr_t info_addr = iface + (uintptr_t)(slot * 0x84);

    char name[32];
    if (mem_read_raw(game->mem, info_addr, name, sizeof name) != MEM_OK) return false;
    name[31] = '\0';

    snprintf(out_name, size, "%s", name);
    return out_name[0] != '\0';
}

static float vec3_distance(vec3_t a, vec3_t b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz) / 50.0f;
}

static bool world_to_screen(vec3_t pos, view_matrix_t vm,
                             float screen_width, float screen_height, vec2_t *out) {
    float w = pos.x * vm.m[12] + pos.y * vm.m[13] + pos.z * vm.m[14] + vm.m[15];
    if (w < 0.1f) return false;

    float x = pos.x * vm.m[0] + pos.y * vm.m[1] + pos.z * vm.m[2] + vm.m[3];
    float y = pos.x * vm.m[4] + pos.y * vm.m[5] + pos.z * vm.m[6] + vm.m[7];

    float ndc_x = x / w;
    float ndc_y = y / w;

    out->x = (screen_width / 2.0f) * (ndc_x + 1.0f);
    out->y = (screen_height / 2.0f) * (1.0f - ndc_y);
    return true;
}

void game_wait_attach(game_ctx_t *game, mem_ctx_t *mem) {
    game->mem = mem;

    while (mem_get_module(mem, "client.so", &game->client) != MEM_OK) sleep(1);
    while (mem_get_module(mem, "engine.so", &game->engine) != MEM_OK) sleep(1);
}

bool game_get_local_player(game_ctx_t *game, vec3_t *out_pos, int *out_health, uintptr_t *out_addr) {
    uintptr_t local_player = MEM_READ(game->mem, game->client.base + DW_LOCAL_PLAYER, uintptr_t);
    if (local_player == 0) return false;

    *out_addr = local_player;
    *out_health = MEM_READ(game->mem, local_player + M_I_HEALTH, int);
    *out_pos = MEM_READ(game->mem, local_player + M_VEC_ORIGIN, vec3_t);
    return true;
}

int game_scan_enemies(game_ctx_t *game, uintptr_t local_player_addr, vec3_t local_pos,
                      float screen_width, float screen_height, bool hide_teammates,
                      enemy_t *out, int max_enemies) {


    view_matrix_t vm = MEM_READ(game->mem, game->engine.base + DW_VIEW_MATRIX, view_matrix_t);
    int count = 0;

    int local_team = MEM_READ(game->mem, local_player_addr + M_I_TEAM_NUM, int);

    for (int i = 1; i < 64 && count < max_enemies; i++) {
        uintptr_t list_entry = game->client.base + DW_ENTITY_LIST + ((uintptr_t)i * ENT_ENTRY_SIZE);
        uintptr_t entity_addr = MEM_READ(game->mem, list_entry, uintptr_t);

        if (entity_addr == 0 || entity_addr == local_player_addr) continue;

        int entity_team = MEM_READ(game->mem, entity_addr + M_I_TEAM_NUM, int);

        if (hide_teammates && (entity_team == local_team)) {
            continue;
        }

        int hp = MEM_READ(game->mem, entity_addr + M_I_HEALTH, int);
        if (hp <= 1 || hp > 200) continue;

        bool is_dormant = MEM_READ(game->mem, entity_addr + M_B_DORMANT, bool);
        if (is_dormant) continue;

        vec3_t pos = MEM_READ(game->mem, entity_addr + M_VEC_ORIGIN, vec3_t);
        if (fabsf(pos.x) < 1.0f && fabsf(pos.y) < 1.0f) continue; /* lixo de memoria */

        vec3_t top = pos;
        top.z += 72.0f;

        vec2_t screen_bottom, screen_top;
        if (!world_to_screen(pos, vm, screen_width, screen_height, &screen_bottom)) continue;
        if (!world_to_screen(top, vm, screen_width, screen_height, &screen_top)) continue;

        // printf("Slot %d: entity_addr = 0x%lx, HP = %d\n", i, entity_addr, hp);


        out[count].entity_addr = entity_addr;
        out[count].health = hp;
        out[count].distance = vec3_distance(local_pos, pos);
        out[count].world_pos = pos;
        out[count].screen_bottom = screen_bottom;
        out[count].screen_top = screen_top;
        count++;
    }

    return count;
}

// int game_get_bones(game_ctx_t *game, uintptr_t entity_addr,
//                    float screen_width, float screen_height,
//                    vec2_t *out_points, int max_points) {
//     int bone_count = MEM_READ(game->mem, entity_addr + M_BONE_COUNT, int);
//     if (bone_count <= 0 || bone_count > GAME_MAX_BONES) return 0; /* checagem de sanidade */
//
//     uintptr_t bone_array = MEM_READ(game->mem, entity_addr + M_BONE_ARRAY_PTR, uintptr_t);
//     if (bone_array == 0) return 0;
//
//     view_matrix_t vm = MEM_READ(game->mem, game->engine.base + DW_VIEW_MATRIX, view_matrix_t);
//
//     int limit = (bone_count < max_points) ? bone_count : max_points;
//     int count = 0;
//
//     for (int i = 0; i < limit; i++) {
//         bone_matrix_t bm = MEM_READ(game->mem,
//                                      bone_array + (uintptr_t)i * sizeof(bone_matrix_t),
//                                      bone_matrix_t);
//
//         vec3_t bone_pos = { bm.m[0][3], bm.m[1][3], bm.m[2][3] };
//
//         vec2_t screen_pos;
//         if (world_to_screen(bone_pos, vm, screen_width, screen_height, &screen_pos)) {
//             out_points[count++] = screen_pos;
//         }
//     }
//
//     return count;
// }

void game_get_bones(game_ctx_t *game, uintptr_t entity_addr,
                   float screen_width, float screen_height,
                   vec2_t *out_points) {

    // Initialize all bones as "invalid/off-screen"
    for (int i = 0; i < GAME_MAX_BONES; i++) {
        out_points[i].x = -1.0f;
        out_points[i].y = -1.0f;
    }

    int bone_count = MEM_READ(game->mem, entity_addr + M_BONE_COUNT, int);
    if (bone_count <= 0 || bone_count > GAME_MAX_BONES) return;

    uintptr_t bone_array = MEM_READ(game->mem, entity_addr + M_BONE_ARRAY_PTR, uintptr_t);
    if (bone_array == 0) return;

    view_matrix_t vm = MEM_READ(game->mem, game->engine.base + DW_VIEW_MATRIX, view_matrix_t);

    for (int i = 0; i < bone_count; i++) {
        bone_matrix_t bm = MEM_READ(game->mem,
                                     bone_array + (uintptr_t)i * sizeof(bone_matrix_t),
                                     bone_matrix_t);

        vec3_t bone_pos = { bm.m[0][3], bm.m[1][3], bm.m[2][3] };

        vec2_t screen_pos;
        if (world_to_screen(bone_pos, vm, screen_width, screen_height, &screen_pos)) {
            // The index 'i' is preserved! Bone 8 will always be at out_points[8]
            out_points[i] = screen_pos;
        }
    }
}
