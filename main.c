#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "mem.h"
#include "overlay.h"
#include "game.h"
#include "draw.h"
#include "menu.h"
#include "config.h"

#define PROCESS_NAME  "cstrike_linux64"
// https://wiki.hypr.land/Configuring/Basics/Monitors/
#define MONITOR_X     0
#define MONITOR_Y     0
#define SCREEN_WIDTH  1920.0f
#define SCREEN_HEIGHT 1080.0f
#define ESP_THICKNESS 2.0f

int main(void) {
    printf("CONFIN ESP\n[*] Waiting for CS:Source...\n");
    fflush(stdout);

    overlay_config_t ov_cfg = { MONITOR_X, MONITOR_Y, (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT };
    overlay_t *ov = overlay_create(&ov_cfg);
    if (!ov) {
        fprintf(stderr, "[-] Failed to create overlay (X11/SDL)\n");
        return 1;
    }

    menu_t *menu = menu_create(ov);
    if (!menu) {
        fprintf(stderr, "[-] Failed to create menu nuklear\n");
        overlay_destroy(ov);
        return 1;
    }

    mem_ctx_t *mem = mem_create(PROCESS_NAME);
    if (!mem) {
        fprintf(stderr, "[-] Out of memory for reading context\n");
        menu_destroy(menu);
        overlay_destroy(ov);
        return 1;
    }

    while (mem_attach(mem) != MEM_OK) sleep(1);

    game_ctx_t game;
    game_wait_attach(&game, mem);

    printf("[+] Game found. Activating...\n");
    fflush(stdout);

    SDL_Renderer *renderer = overlay_renderer(ov);
    esp_config_t config = config_default();

    TTF_Init();
    TTF_Font *font = TTF_OpenFont("resources/font/Segoe UI.ttf", 13);



    bool running = true;
    while (running) {
        overlay_event_batch_t events = overlay_poll_events(ov);
        if (events.quit_requested) running = false;

        menu_update_hotkey(menu, ov);

        overlay_frame_begin(ov);

        if (menu_is_open(menu)) {
            menu_render(menu, &events, &config);
        } else {
            vec3_t my_pos;
            int my_health;
            uintptr_t local_player;

            if (game_get_local_player(&game, &my_pos, &my_health, &local_player)) {
                enemy_t enemies[GAME_MAX_ENEMIES];
                int count = game_scan_enemies(&game, local_player, my_pos,
                                               SCREEN_WIDTH, SCREEN_HEIGHT,
                                               config.hide_teammates,
                                               enemies, GAME_MAX_ENEMIES);

                for (int i = 0; i < count; i++) {
                    //SDL_Color color = (enemies[i].distance < 10.0f)
                                           //? (SDL_Color){ 255, 0, 0, 255 }
                                           //: (SDL_Color){ 0, 255, 0, 255 };
                    // PURPLE
                    SDL_Color color = { 255, 0, 255, 255 };

                    char player_name[32] = "?";
                    game_get_player_name(&game, i, player_name, sizeof player_name);
                    //draw_enemy_esp(renderer, SCREEN_WIDTH, SCREEN_HEIGHT,
                                   //&enemies[i], color, ESP_THICKNESS, font, player_name, &config);

                    vec2_t bones[GAME_MAX_BONES];
					if (config.show_skeleton)
					{
						game_get_bones(&game, enemies[i].entity_addr,
										SCREEN_WIDTH, SCREEN_HEIGHT,
									    bones);

						draw_skeleton(renderer, bones, color, 1.5f);
					}

					// int bone_count = game_get_bones(&game, enemies[i].entity_addr,
					// 								SCREEN_WIDTH, SCREEN_HEIGHT,
					// 				 				bones);

					// int bone_count = game_get_bones(&game, enemies[i].entity_addr,
					// 								SCREEN_WIDTH, SCREEN_HEIGHT,
					// 								bones, GAME_MAX_BONES);

                    //draw_skeleton_points(renderer, bones, bone_count, color);
                    //draw_skeleton_points(renderer, bones, GAME_MAX_BONES, color);
                }
            }
        }

        overlay_frame_end(ov);
        usleep(5 * 1000);
    }

    mem_destroy(mem);
    menu_destroy(menu);
    overlay_destroy(ov);
    return 0;
}
