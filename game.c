#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#ifdef WASM
#include <emscripten/fetch.h>
#else
#include <pthread.h>
#include <curl/curl.h>
#endif

#include "game.h"

// global state
// most funcs assume state pointer is valid when accessed
// init properly before use
GameState *state = NULL;

// MARK: :init :malloc

void gamestate_create() {
    state = malloc(sizeof(GameState));
    if (!state) {
        printe("Error malloc game state");
        return;
    }

    Vec2 world_center = (Vec2) { 
        (WORLD_W * TILE_SIZE * DEFAULT_SPRITE_SCALE) / 2.0f,
        (WORLD_H * TILE_SIZE * DEFAULT_SPRITE_SCALE) / 2.0f 
    };
    
    // Pixel camera setup
    float virtual_ratio = (float) SCREEN_WIDTH / (float) VIRT_WIDTH;

    *state = (GameState) {
        .camera = {
            .target = (Vec2) { VIRT_WIDTH/2.0f, VIRT_HEIGHT/2.0f },
            .offset = (Vec2) { VIRT_WIDTH/2.0f, VIRT_HEIGHT/2.0f },
            .rotation = 0.0f,
            .zoom = 1.0f
        },
        .sprite_sheet = LoadTexture(SPRITE_SHEET_PATH),
        .custom_font = LoadFontEx(CUSTOM_FONT_PATH, 64 * 2, 0, 250),
        .render_texture = LoadRenderTexture(VIRT_WIDTH, VIRT_HEIGHT),
        .source_rect = (Rect) {
            0.0f, 0.0f, 
            (float) VIRT_WIDTH, -(float) VIRT_HEIGHT
        },
        .dest_rect = (Rect) {
            -virtual_ratio, -virtual_ratio, 
            SCREEN_WIDTH + (virtual_ratio*2), SCREEN_HEIGHT + (virtual_ratio*2)
        },

        // Sound
        .sound_intensity = 1.0,

        // General
        .screen = MAIN_MENU,
        .timer = (Timers) {
            .game_start_ts = get_current_time_millis(),
            .pause_ts = 0,
            .pickups_cleanup_ts = 0,
            .last_hurt_sound_ts = 0,
            .last_heart_spawn_ts = 0,

            .bullet_ts = 0,
            .splinter_ts = 0,
            .spike_ts = 0,
            .flame_ts = 0,
            .frost_wave_ts = 0,
            .orbs_ts = 0,

            .qtree_update_ts = 0,
            .enemy_spawn_ts = 0,
            .enemy_spawn_interval = DEFAULT_ENEMY_SPAWN_INTERVAL_MS,
        },
        .stats = (Stats) {
            .bullet_interval = get_base_stat_value(BULLET_INTERVAL),
            .bullet_count = get_base_stat_value(BULLET_COUNT),
            .bullet_spread = get_base_stat_value(BULLET_SPREAD),
            .bullet_range = get_base_stat_value(BULLET_RANGE),
            .bullet_damage = get_base_stat_value(BULLET_DAMAGE),
            .bullet_penetration = get_base_stat_value(BULLET_PENETRATION),

            .splinter_interval = get_base_stat_value(SPLINTER_INTERVAL),
            .splinter_range = get_base_stat_value(SPLINTER_RANGE),
            .splinter_count = get_base_stat_value(SPLINTER_COUNT),
            .splinter_damage = get_base_stat_value(SPLINTER_DAMAGE),
            .splinter_penetration = get_base_stat_value(SPLINTER_PENETRATION),

            .spike_interval = get_base_stat_value(SPIKE_INTERVAL),
            .spike_speed = get_base_stat_value(SPIKE_SPEED),
            .spike_count = get_base_stat_value(SPIKE_COUNT),
            .spike_damage = get_base_stat_value(SPIKE_DAMAGE),

            .flame_interval = get_base_stat_value(FLAME_INTERVAL),
            .flame_lifetime = get_base_stat_value(FLAME_LIFETIME),
            .flame_spread = get_base_stat_value(FLAME_SPREAD),
            .flame_distance = get_base_stat_value(FLAME_DISTANCE),
            .flame_damage = get_base_stat_value(FLAME_DAMAGE),

            .frost_wave_interval = get_base_stat_value(FROST_WAVE_INTERVAL),
            .frost_wave_lifetime = get_base_stat_value(FROST_WAVE_LIFETIME),
            .frost_wave_damage = get_base_stat_value(FROST_WAVE_DAMAGE),

            .orbs_interval = get_base_stat_value(ORBS_INTERVAL),
            .orbs_damage = get_base_stat_value(ORBS_DAMAGE),
            .orbs_count = get_base_stat_value(ORBS_COUNT),
            .orbs_size = get_base_stat_value(ORBS_SIZE),

            .player_speed = get_base_stat_value(PLAYER_SPEED),
            .shiny_chance = get_base_stat_value(SHINY_CHANCE),
        },
        .upgrades = (Upgrades) {
            .bullet_level = 1,
            .splinter_level = 0,
            .spike_level = GOD ? 1 : 0,
            .flame_level = 0,
            .frost_wave_level = 0,
            .orbs_level = 0,
            .speed_level = 0,
            .shiny_level = 0,
        },
        .available_upgrades = (Upgrades) {
            .bullet_level = 0,
            .splinter_level = 0,
            .spike_level = 0,
            .flame_level = 0,
            .frost_wave_level = 0,
            .orbs_level = 0,
            .speed_level = 0,
            .shiny_level = 0,
        },
        .toasts = (Toast*) malloc(MAX_NUM_TOASTS * sizeof(Toast)),
        .toasts_count = 0,
        .wave_index = 0,
        .player_level = 1,
        .kill_count = 0,
        .is_fullscreen = false,

        // Player
        .player_pos = world_center,
        .player_health = MAX_PLAYER_HEALTH,
        .player_heading_dir = (Vec2) { 1, 0 },
        .is_joystick_enabled = false,
        .touch_down_pos = Vector2Zero(),
        .joystick_direction = Vector2Zero(),
        .mana_count = 0,
        .mana_particles = (Vec2*) malloc(MAX_MANA_PARTICLES * sizeof(Vec2)),
        .mana_particles_count = 0,

        // Weapon
        .bullets = (Bullet*) malloc(MAX_BULLETS * sizeof(Bullet)),
        .bullet_count = 0,
        .enemy_bullets = (Bullet*) malloc(MAX_ENEMY_BULLETS * sizeof(Bullet)),
        .enemy_bullet_count = 0,
        .flame_particles = (Particle*) malloc(MAX_FLAME_PARTICLES * sizeof(Particle)),
        .flame_particles_count = 0,
        .frost_wave_particles = (Particle*) malloc(MAX_FROST_WAVE_PARTICLES * sizeof(Particle)),

        // Enemy
        .enemies = (Enemy*) malloc(MAX_ENEMIES * sizeof(Enemy)),
        .enemy_count = 0,
        .enemy_qtree = qtree_create(get_visible_rect(world_center, 1.0)),
        .num_enemies_per_tick = 1,

        // World
        .world_dims = (Rect) { 
            0, 0, 
            WORLD_W * TILE_SIZE * DEFAULT_SPRITE_SCALE, 
            WORLD_H * TILE_SIZE * DEFAULT_SPRITE_SCALE 
        },
        .decorations = (Decoration*) malloc(NUM_DECORATIONS * sizeof(Decoration)),
        .pickups = (Pickup*) malloc(MAX_PICKUPS * sizeof(Pickup)),
        .pickups_count = 0,
        .pickups_qtree = qtree_create(
            (QRect) {
                world_center.x, world_center.y,
                32000, 32000
            }
        ),

        // UI
        .master_volume = 1.0f,
        .is_master_volume_dragging = false,
        .main_menu_enemies = (Enemy*) malloc(MAX_MAIN_MENU_ENEMIES * sizeof(Enemy)),
        .main_menu_enemies_count = 0,

        // Others
        .query_points = malloc(MAX_ENEMIES * sizeof(QPoint)),
        .num_query_points = 0,

        .is_show_debug_gui = false,
        .temp = (Temp) {
            .ct = 0,
            .num_enemies_drawn = 0
        },
    };

    if (!state->bullets || !state->enemies || !state->query_points || !state->enemy_qtree) {
        free(state->bullets);
        free(state->query_points);
        free(state->enemies);
        qtree_destroy(state->enemy_qtree);
        free(state);

        printe("Error malloc game state components");
        return;
    }

    // idk if I need this
    SetTextureFilter(state->render_texture.texture, TEXTURE_FILTER_POINT);

    reset_main_menu_enemies();
    handle_window_resize();
    sounds_init();
    decorations_init();
    post_analytics_async(GAME_OPEN);
}

// :deinit :destroy
void gamestate_destroy() {
    if (state == NULL) {
        printe("state empty");
        return;
    }

    free(state->toasts);
    state->toasts_count = 0;

    free(state->mana_particles);
    state->mana_particles_count = 0;

    free(state->bullets);
    state->bullet_count = 0;
    free(state->enemy_bullets);
    state->enemy_bullet_count = 0;
    free(state->flame_particles);
    state->flame_particles_count = 0;
    free(state->frost_wave_particles);
    state->frost_wave_particles_count = 0;

    free(state->enemies);
    state->enemy_count = 0;
    qtree_destroy(state->enemy_qtree);

    free(state->decorations);
    free(state->pickups);
    state->pickups_count = 0;
    qtree_destroy(state->pickups_qtree);

    free(state->main_menu_enemies);
    state->main_menu_enemies_count = 0;

    free(state->query_points);
    state->num_query_points = 0;

    UnloadTexture(state->sprite_sheet);
    UnloadRenderTexture(state->render_texture);
    UnloadFont(state->custom_font);

    sounds_destroy();

    free(state);
    state = NULL;
}

// MARK: :pause :resume :timer

void pause_timers() {
    state->timer.pause_ts = get_current_time_millis();
}

void resume_timers() {
    int delta = get_current_time_millis() - state->timer.pause_ts;
    state->timer.pause_ts = 0;
    state->timer.game_start_ts += delta;
    state->timer.pickups_cleanup_ts += delta;
    state->timer.last_hurt_sound_ts += delta;

    state->timer.bullet_ts += delta;
    state->timer.splinter_ts += delta;
    state->timer.spike_ts += delta;
    state->timer.flame_ts += delta;
    state->timer.frost_wave_ts += delta;
    state->timer.orbs_ts += delta;

    state->timer.qtree_update_ts += delta;
    state->timer.enemy_spawn_ts += delta;
}

// MARK: :render :draw

void draw_game() {
    ClearBackground(BLACK);

    // game draw
    BeginTextureMode(state->render_texture);
    {
        ClearBackground(COLOR_DARK_BLUE);
        BeginMode2D(state->camera);
        {
            draw_decorations();
            draw_pickups();
            draw_enemies();
            draw_particles();
            draw_bullets();
            draw_player();
        }
        EndMode2D();
    }
    EndTextureMode();

    DrawTexturePro(
        state->render_texture.texture, 
        state->source_rect, 
        state->dest_rect, 
        Vector2Zero(), 0.0f, WHITE
    );

    draw_ui();
    draw_toasts();
    draw_upgrade_menu();

    // Debug
    {
        float xpos = 10;
        float ypos = 200;
        float ypadding = 30;
        Color color = GRAY;
        int font_size = 20;
        if (state->is_show_debug_gui) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.2));
            DrawTextEx(
                state->custom_font,
                TextFormat("Fps: %i", GetFPS()),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#level: %i", state->player_level),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#Enemies: %i", state->enemy_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#Bullets: %i", state->bullet_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("ppos: %f, %f", state->player_pos.x, state->player_pos.y),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("Wave: %d", state->wave_index),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#Enemies on screen: %d", state->temp.num_enemies_drawn),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("Spawn rate: %d", state->num_enemies_per_tick),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#Pickups: %d", state->pickups_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("mana: %d", state->mana_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("kills: %ld", state->kill_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#mana-particles: %d", state->mana_particles_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("#flame-particles: %d", state->flame_particles_count),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
            DrawTextEx(
                state->custom_font,
                TextFormat("health: %f", state->player_health),
                (Vec2){ xpos, ypos }, font_size, 2, color
            );
            ypos += ypadding;
        }

        // show fps
        if (DEBUG) {
            DrawTextEx(
                state->custom_font, 
                TextFormat("%d", GetFPS()), 
                (Vec2) { GetScreenWidth() - 30, 4 },
                15, 2, ColorAlpha(COLOR_BROWN, 0.5)
            );
        }
    }
}

void draw_player() {
    draw_sprite(&state->sprite_sheet, (Vec2) {0, 9}, state->player_pos, DEFAULT_SPRITE_SCALE, 0, WHITE);
    DrawCircleLinesV(state->player_pos, 80, ColorAlpha(COLOR_BROWN, 0.1));

    // :healthbar
    Vec2 size = { 16, 1 };
    DrawRectangleV(
        (Vec2) { state->player_pos.x - size.x/2, state->player_pos.y + 10 },
        size, COLOR_BROWN
    );
    DrawRectangleV(
        (Vec2) { state->player_pos.x - size.x/2, state->player_pos.y + 10 },
        (Vec2) { size.x * (state->player_health / MAX_PLAYER_HEALTH), size.y },
        COLOR_WHITE
    );
}

void draw_enemies() {
    {
        // Draw culling for enemies
        state->num_query_points = 0;
        qtree_query(
            state->enemy_qtree, 
            state->enemy_qtree->boundary,
            state->query_points,
            &state->num_query_points
        );

        state->temp.num_enemies_drawn = state->num_query_points;
        for (int i = 0; i < state->num_query_points; i++) {
            QPoint pt = state->query_points[i];
            Enemy *enemy = &state->enemies[pt.id];
            Vec2 loc = get_enemy_sprite_pos(enemy->type, enemy->is_shiny);
            bool is_flip_x = enemy->pos.x < state->player_pos.x;

            Color flash = WHITE;
            if (enemy->is_frozen) {
                flash = COLOR_FLASH_FROST;
            }
            if (enemy->is_taking_damage) {
                flash = COLOR_FLASH_HURT;
            }

            draw_spritev(
                &state->sprite_sheet,
                (Vec2) { loc.x, loc.y },
                enemy->pos,
                get_enemy_scale(enemy->type),
                0,
                is_flip_x,
                true,
                flash,
                pt.id
            );

            // this chokes when you draw for too many enemies
            // draw_health_bar(enemy->pos, enemy->health, get_enemy_health(enemy->type, enemy->is_shiny));
        }
    }
}

void draw_particles() {
    // Mana pickups
    for (int i = 0; i < state->mana_particles_count; i ++) {
        draw_sprite(
            &state->sprite_sheet,
            (Vec2) {1, 9},
            state->mana_particles[i],
            0.4, 0, WHITE
        );
    }

    // Flame
    for (int i = 0; i < state->flame_particles_count; i++) {
        draw_sprite(
            &state->sprite_sheet,
            get_attack_sprite(FLAME),
            state->flame_particles[i].pos,
            0.6, 0, COLOR_RED
        );
    }

    // Frost wave
    for (int i = 0; i < state->frost_wave_particles_count; i++) {
        draw_sprite(
            &state->sprite_sheet,
            get_attack_sprite(FROST_WAVE),
            state->frost_wave_particles[i].pos,
            0.6, 0, COLOR_FLASH_FROST
        );
    }
}

void draw_bullets() {
    // Player bullets
    for (int i = 0; i < state->bullet_count; i++) {
        float angle = atan2f(
            state->bullets[i].direction.y, 
            state->bullets[i].direction.x) * (180.0f / PI);

        bool is_spin = state->bullets[i].type == SPIKE || state->bullets[i].type == ORBS;
        if (is_spin)
            angle = GetRandomValue(0, 360);

        float button_scale = 1.2f;
        if (state->bullets[i].type == ORBS)
            button_scale = 1.2f + (state->upgrades.orbs_level * 0.1f);

        draw_sprite(
            &state->sprite_sheet,
            get_attack_sprite(state->bullets[i].type),
            state->bullets[i].pos,
            button_scale, angle, WHITE
        );
    }

    // Enemy bullets
    for (int i = 0; i < state->enemy_bullet_count; i++) {
        draw_sprite(
            &state->sprite_sheet,
            get_attack_sprite(state->enemy_bullets[i].type),
            state->enemy_bullets[i].pos,
            0.8, 0, COLOR_RED
        );
    }
}

void draw_decorations() {
    for (int i = 0; i < NUM_DECORATIONS; i++) {
        Vec2 sprite_pos = (Vec2) { 
            ((state->decorations[i].decoration_idx) % 10) + 1,
            (state->decorations[i].decoration_idx + 1 > 9) ? 1 : 0
        };
        draw_sprite(
            &state->sprite_sheet,
            sprite_pos,
            state->decorations[i].pos,
            0.8, 0, WHITE
        );
    }
}

void draw_pickups() {
    state->num_query_points = 0;
    qtree_query(
        state->pickups_qtree,
        // this will cull the off screen pickups
        state->enemy_qtree->boundary,
        state->query_points,
        &state->num_query_points
    );

    for (int i = 0; i < state->num_query_points; i++) {
        QPoint pt = state->query_points[i];
        Vec2 sprite_pos = { 1, 9 };
        float sprite_scale = 0.4;
        if (state->pickups[pt.id].type == MANA_SHINY) {
            sprite_pos = (Vec2) { 2, 9 };
        }
        if (state->pickups[pt.id].type == HEALTH) {
            sprite_pos = (Vec2) { 5, 9 };
            sprite_scale = 0.5;
        }

        draw_sprite(
            &state->sprite_sheet, sprite_pos,
            (Vec2) { pt.x, pt.y }, sprite_scale, 0, WHITE
        );
    }
}

// :toast
void draw_toasts() {
    float ww = (float) GetScreenWidth();
    float wh = (float) GetScreenHeight();
    int font_size = 24;
    int spacing = 45;
    int padding = 10;

    for (int i = 0; i < state->toasts_count; i++) {
        Toast t = state->toasts[i];
        Vec2 size = MeasureTextEx(state->custom_font, t.message, font_size, 2);
        Vec2 pos = {
            .x = ww/2 - size.x/2,
            // .x = 20,
            .y = (wh * 0.90) + (i * spacing)
        };
        Vec2 pos_offset = {
            pos.x - padding,
            pos.y - padding
        };

        DrawRectangleV(
            pos_offset, 
            Vector2AddValue(size, padding * 2), 
            ColorAlpha(COLOR_BROWN, 0.3)
        );
        DrawTextEx(
            state->custom_font,
            t.message,
            pos,
            font_size, 2, WHITE
        );
    }
}

void draw_health_bar(Vec2 pos, float health, float max_health) {
    DrawRectangleV((Vec2) {pos.x - 8, pos.y + 8}, (Vec2) { 10, 2 }, COLOR_BROWN);
    DrawRectangleV((Vec2) {pos.x - 8, pos.y + 8}, (Vec2) { 10 * (health / max_health), 2 }, COLOR_WHITE);
}

void reset_main_menu_enemies() {
    state->main_menu_enemies_count = 0;
    for (int i = 0; i < MAX_MAIN_MENU_ENEMIES; i ++) {
        state->main_menu_enemies[i] = (Enemy) {
            .pos = (Vec2) {
                .x = GetRandomValue(0, SCREEN_WIDTH),
                .y = GetRandomValue(0, SCREEN_HEIGHT)
            },
            .health = 1,
            // .type = GetRandomValue(0, 100) > 70 ? RAM : BAT,
            .type = BAT,
            .speed = get_enemy_speed(DEMON_PUP),
            .spawn_ts = 0,
            .is_shiny = false,
            .is_player_found = false,
            .player_found_ts = 0,
            // charge dir is used for main menu 
            // to make enemies move
            .charge_dir = get_rand_unit_vec2(),
            .last_spawn_ts = 0,
            .is_frozen = false,
            .is_taking_damage = false,
        };
        state->main_menu_enemies_count += 1;
    }
}

// this updates and draws the main menu enemies
void update_main_menu_enemies() {
    float ww = (float) GetScreenWidth();
    float wh = (float) GetScreenHeight();

    for (int i = 0; i < state->main_menu_enemies_count; i++) {
        Enemy *enemy = &state->main_menu_enemies[i];
        bool is_flip_x = enemy->pos.x > ww / 2.0f;
        Vec2 velocity = Vector2Scale(enemy->charge_dir, 0.3);
        enemy->pos = Vector2Add(enemy->pos, velocity);

        if (enemy->pos.x <= 0 || enemy->pos.x >= ww) {
            enemy->charge_dir.x *= -1;
        }
        if (enemy->pos.y <= 0 || enemy->pos.y >= wh) {
            enemy->charge_dir.y *= -1;
        }

        Vec2 loc = get_enemy_sprite_pos(enemy->type, false);
        draw_spritev(
            &state->sprite_sheet,
            (Vec2) { loc.x, loc.y },
            enemy->pos,
            get_enemy_scale(enemy->type) * 2.5,
            0,
            is_flip_x,
            true,
            ColorAlpha(WHITE, 0.2),
            i
        );
    }
}

// MARK: :menu
void draw_main_menu() {
    if (state->screen != MAIN_MENU) return;
    ClearBackground(COLOR_DARK_BLUE);

    float ww = (float) GetScreenWidth();
    float wh = (float) GetScreenHeight();
    float time = GetTime();
    float amplitude = 10;
    float speed = 2;

    // version
    {
        DrawTextEx(
            state->custom_font, 
            TextFormat("%s", VERSION), 
            (Vec2) { 20, wh - 35 }, 20, 2, WHITE
        );
    }

    // title
    {
        {
            const char *title = "Blob";
            int font_size = 80;
            Vec2 title_size = MeasureTextEx(state->custom_font, title, font_size, 2);
            Vec2 title_pos = (Vec2) {
                ww / 2 - title_size.x / 2,
                wh * 0.2 + sinf(time * speed) * amplitude
            };

            DrawTextEx(
                state->custom_font,
                title,
                Vector2AddValue(title_pos, 4),
                font_size, 3, COLOR_RED
            );
            DrawTextEx(
                state->custom_font,
                title,
                title_pos,
                font_size, 3, COLOR_WHITE
            );
        }
        {
            const char *title = "Survival";
            int font_size = 80;
            Vec2 title_size = MeasureTextEx(state->custom_font, title, font_size, 2);
            Vec2 title_pos = (Vec2) {
                ww/2 - title_size.x/2,
                (wh * 0.2 + 80) + sinf(time * speed) * amplitude
            };
            DrawTextEx(
                state->custom_font,
                title,
                Vector2AddValue(title_pos, 4),
                font_size, 3, COLOR_RED
            );
            DrawTextEx(
                state->custom_font,
                title,
                title_pos,
                font_size, 3, COLOR_WHITE
            );
        }
    }

    float y_start = wh * 0.7;
    int font_size = 35;
    // play btn
    {
        const char *text = "Play";
        Vec2 btn_size = (Vec2) { 250, 70 };
        Vec2 pos = (Vec2) { ww/2 - btn_size.x/2, y_start };
        bool is_play = main_menu_button((Button) {
            .label = text,
            .pos = pos,
            .size = btn_size,
            .bg = COLOR_BROWN,
            .fg = COLOR_WHITE,
            .shadow = COLOR_CEMENT,
            .font_size = font_size
        });
        y_start += 100;

        // :start :play
        if (is_play || IsKeyPressed(KEY_ENTER)) {
            state->screen = IN_GAME;
            state->timer.game_start_ts = get_current_time_millis();
            post_analytics_async(GAME_START);
        }
    }
}

// MARK: :ui

void draw_ui() {
    float ww = (float) GetScreenWidth();
    float wh = (float) GetScreenHeight();
    GameScreen *screen = &state->screen;

    // Pause
    {
        if (*screen == PAUSE_MENU) {
            
            // bg
            DrawRectangleV(
                (Vec2) { 0, 0 }, 
                (Vec2) { ww, wh }, 
                ColorAlpha(COLOR_DARK_BLUE, 0.7)
            );

            // Title
            {
                const char *text = "Game Paused";
                int font_size = 35;
                Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
                Vec2 pos = { (ww/2) - (text_size.x/2), wh * 0.1 };
                draw_text_with_shadow(text, pos, font_size, COLOR_WHITE, COLOR_RED);
            }

            // Volume controls
            {
                const char *text = "Master Volume";
                slider(
                    0.4f, text,
                    &state->master_volume,
                    &state->is_master_volume_dragging
                );
                SetMasterVolume(state->master_volume);
            }

            // Unpause button
            {
                const char *text = "Back";
                float y_start = wh * 0.8;
                int font_size = 35;
                Vec2 btn_size = (Vec2) { 250, 70 };
                Vec2 pos = (Vec2) { ww/2 - btn_size.x/2, y_start };
                bool is_back = upgrade_menu_button((Button) {
                    .label = text,
                    .pos = pos,
                    .size = btn_size,
                    .bg = COLOR_BROWN,
                    .fg = COLOR_WHITE,
                    .shadow = COLOR_BROWN,
                    .font_size = font_size
                });

                if (is_back || IsKeyPressed(KEY_ESCAPE)) {
                    resume_timers();
                    state->screen = IN_GAME;
                }
            }

            return;
        }
    }

    // In game
    {
        if (*screen == IN_GAME) {
            // Mana bar
            {
                int max_current_level_mana = get_level_mana_threshold(state->player_level);
                int ww = GetScreenWidth();
                float percentage_mana = (float) state->mana_count / max_current_level_mana;
                DrawRectangleV(Vector2Zero(), (Vec2) { percentage_mana * ww, 10 }, COLOR_LIGHT_PINK);
            }

            // Kill count
            {
                int font_size = 25;
                const char *text = TextFormat("%ld", state->kill_count);
                draw_sprite(&state->sprite_sheet, (Vec2) {0, 10}, (Vec2) {20, 40}, 5, 0, WHITE);
                DrawTextEx(
                    state->custom_font, text,
                    (Vec2) { 50, 40 - 7 }, font_size, 2, COLOR_WHITE
                );
            }

            // Timer
            {
                // can't draw this other states are the time needs to be reset
                // at the end of the pause/resume cycle
                int now = get_current_time_millis();
                int elapsed = now - state->timer.game_start_ts;
                int minutes = elapsed / 60000;
                int seconds = (elapsed / 1000) % 60;

                int font_size = 35;
                const char *text = TextFormat("%d:%02d", minutes, seconds);
                Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
                Vec2 pos = (Vec2) { (ww/2) - (text_size.x/2), 30 };

                draw_text_with_shadow(text, pos, font_size, COLOR_WHITE, COLOR_RED);
            }

            // Pause button
            {
                Vec2 pos = (Vec2) { ww - 40, 40 };
                float scale = 2.0f;
                float sprite_size = TILE_SIZE * scale;
                draw_sprite(
                    &state->sprite_sheet, 
                    (Vec2) {6, 10}, Vector2AddValue(pos, 1.5),
                    DEFAULT_SPRITE_SCALE * scale, 0, WHITE
                );
                draw_sprite(
                    &state->sprite_sheet, 
                    (Vec2) {5, 10}, pos,
                    DEFAULT_SPRITE_SCALE * scale, 0, WHITE
                );

                bool is_mouse_hovered = CheckCollisionPointRec(
                    GetMousePosition(), (Rect) {
                        pos.x - sprite_size / 2, pos.y - sprite_size / 2, 
                        sprite_size, sprite_size
                    }
                );
                if (is_mouse_hovered && IsMouseButtonPressed(0)) {
                    pause_timers();
                    state->screen = PAUSE_MENU;
                    state->is_joystick_enabled = false;
                }
            }
        }
    }

    // :death
    // Death
    {
        if (*screen == DEATH) {
            // bg
            DrawRectangleV(
                (Vec2) { 0, 0 }, 
                (Vec2) { ww, wh }, 
                ColorAlpha(COLOR_DARK_BLUE, 0.7)
            );

            // text
            int font_size = 35;

            {
                const char *text = "You Died";
                Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
                Vec2 pos = (Vec2) { (ww/2) - (text_size.x/2), wh * 0.1 };
                draw_text_with_shadow(text, pos, font_size, COLOR_WHITE, COLOR_RED);
            }
            {
                const char *text = "You survived for";
                Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
                Vec2 pos = (Vec2) { (ww/2) - (text_size.x/2), wh * 0.3 };
                draw_text_with_shadow(text, pos, font_size, COLOR_WHITE, COLOR_CEMENT);
            }
            {
                int elapsed = state->death_ts - state->timer.game_start_ts;
                int minutes = elapsed / 60000;
                int seconds = (elapsed / 1000) % 60;
                const char *text = TextFormat("%d minutes and %d seconds", minutes, seconds);
                Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
                Vec2 pos = (Vec2) { (ww/2) - (text_size.x/2), wh * 0.35 };
                draw_text_with_shadow(text, pos, font_size, COLOR_WHITE, COLOR_CEMENT);
            }
            {
                const char *text = TextFormat("You Killed %d enemies", state->kill_count);
                Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
                Vec2 pos = (Vec2) { (ww/2) - (text_size.x/2), wh * 0.5 };
                draw_text_with_shadow(text, pos, font_size, COLOR_WHITE, COLOR_CEMENT);
            }

            // btn
            {
                const char *text = "Retry";
                float y_start = wh * 0.8;
                Vec2 btn_size = (Vec2) { 250, 70 };
                Vec2 pos = (Vec2) { ww/2 - btn_size.x/2, y_start };
                bool is_play = upgrade_menu_button((Button) {
                    .label = text,
                    .pos = pos,
                    .size = btn_size,
                    .bg = COLOR_BROWN,
                    .fg = COLOR_WHITE,
                    .shadow = COLOR_BROWN,
                    .font_size = font_size
                });

                // :retry :restart
                if (is_play || IsKeyPressed(KEY_ENTER)) {
                    gamestate_destroy();
                    gamestate_create();
                    state->screen = IN_GAME;
                    post_analytics_async(GAME_START);
                }
            }
        }
    }

    // Joystick
    {
        if (state->is_joystick_enabled) {
            DrawCircleV(state->touch_down_pos, 30, ColorAlpha(COLOR_BROWN, 0.5));
            DrawCircleV(
                Vector2Add(
                    Vector2Scale(state->joystick_direction, 15),
                    state->touch_down_pos
                ),
                10, ColorAlpha(COLOR_WHITE, 0.5)
            );
        }
    }

}

// MARK: :upgrade

void draw_upgrade_menu() {
    if (state->screen != UPGRADE_MENU) return;

    float ww = (float) GetScreenWidth();
    float wh = (float) GetScreenHeight();

    // auto upgrade
    if (state->player_level > 18) {
        int min = BULLET_UPGRADE;
        int max = SHINY_UPGRADE;
        UpgradeType rand_type = (UpgradeType) GetRandomValue(min, max);

        upgrade_stat(rand_type);
        state->screen = IN_GAME;
        resume_timers();
        play_sound_modulated(&state->sound_upgrade, 0.3);
        return;
    }

    // title
    {
        const char *text = "Pick an Upgrade";
        int font_size = 35;
        Vec2 text_size = MeasureTextEx(state->custom_font, text, font_size, 2);
        Vec2 pos = { (GetScreenWidth()/2) - (text_size.x/2), wh * 0.15 };
        Vec2 pos_offset = Vector2AddValue(pos, 2);

        DrawTextEx(state->custom_font, text, pos_offset, font_size, 2, COLOR_RED);
        DrawTextEx(state->custom_font, text, pos, font_size, 2, COLOR_WHITE);
    }

    // options
    {
        // size values are repeated in the draw func
        Vec2 size = (Vec2) { 400, 50 };
        Vec2 pos = (Vec2) {
            ww/2 - size.x/2,
            wh * 0.3
        };

        Upgrades *level = &state->available_upgrades;
        draw_upgrade_option(BULLET_UPGRADE, level->bullet_level, &pos);
        draw_upgrade_option(SPLINTER_UPGRADE, level->splinter_level, &pos);
        draw_upgrade_option(SPIKE_UPGRADE, level->spike_level, &pos);
        draw_upgrade_option(FLAME_UPGRADE, level->flame_level, &pos);
        draw_upgrade_option(FROST_UPGRADE, level->frost_wave_level, &pos);
        draw_upgrade_option(ORBS_UPGRADE, level->orbs_level, &pos);
        draw_upgrade_option(SPEED_UPGRADE, level->speed_level, &pos);
        draw_upgrade_option(SHINY_UPGRADE, level->shiny_level, &pos);
    }
}

void draw_upgrade_option(UpgradeType type, int level, Vec2 *pos) {
    // This is a draw func but it also updates the stats and levels of the upgrades

    int font_size = 30;
    Vec2 size = (Vec2) { 400, 60 };
    float gap = 80.0f;

    char *label = "Err";
    switch (type) {
        case BULLET_UPGRADE:
            label = "Bullet";
            break;
        case SPLINTER_UPGRADE:
            label = "Splinter";
            break;
        case SPIKE_UPGRADE:
            label = "Spike";
            break;
        case FLAME_UPGRADE:
            label = "Flame";
            break;
        case FROST_UPGRADE:
            label = "Frost Wave";
            break;
        case ORBS_UPGRADE:
            label = "Orbs";
            break;
        case SPEED_UPGRADE:
            label = "Player Speed";
            break;
        case SHINY_UPGRADE:
            label = "Shiny Probability";
            break;
    }

    bool is_enabled = false;
    if (level > 0) {
        is_enabled = upgrade_menu_button((Button) {
            .label = label,
            .pos = *pos, .size = size,
            .bg = COLOR_BROWN, .fg = COLOR_WHITE, .shadow = COLOR_BROWN,
            .font_size = font_size
        });
        pos->y += gap;
    }

    if (is_enabled) {
        state->screen = IN_GAME;
        resume_timers();
        upgrade_stat(type);
        play_sound_modulated(&state->sound_upgrade, 0.5);
    }
}

// :sprite
void draw_sprite(Texture2D *sprite_sheet, Vec2 tile, Vec2 pos, float scale, float rotation, Color tint) {
    draw_spritev(sprite_sheet, tile, pos, scale, rotation, false, false, tint, 0);
}

void draw_spritev(Texture2D *sprite_sheet, Vec2 tile, Vec2 pos, float scale, float rotation, bool is_flip_x, bool is_bob, Color tint, int bob_id) {
    float bob_amplitude = 7;
    float bob_speed = 3;
    int time = GetTime() - bob_id;
    float width = TILE_SIZE * (is_flip_x ? -1 : 1);
    rotation = is_bob ? sinf(time * bob_speed) * bob_amplitude : rotation;

    DrawTexturePro(
        *sprite_sheet,
        (Rect) { tile.x * TILE_SIZE, tile.y * TILE_SIZE, width, TILE_SIZE },
        (Rect) { pos.x, pos.y, TILE_SIZE * scale, TILE_SIZE * scale },
        (Vec2) { (TILE_SIZE/2.0f) * scale, (TILE_SIZE/2.0f) * scale },
        rotation,
        tint
    );
}

// MARK: :update

void update_game() {
    update_toasts();
    sounds_update();

    if (state->screen != IN_GAME) {
        return;
    }

    handle_player_input();
    handle_virtual_joystick_input();

    update_bullets();
    update_enemies();
    update_particles();
    update_player();
    update_pickups();
    update_decorations();

    if (state->player_health <= 0) {
        state->screen = DEATH;
        state->death_ts = get_current_time_millis();
        pause_timers();
        post_analytics_async(GAME_OVER);
    }

    // :wave
    // Update wave
    {
        int now = get_current_time_millis();
        int time_elapsed = now - state->timer.game_start_ts;
        int next_state_ts = (WAVE_DURATION_MILLIS * state->wave_index) + 
            (WAVE_DURATION_INCREMENT_MILLIS * state->wave_index);
        int wave_increment = GOD ? 100 : get_num_enemies_per_tick();
        if (time_elapsed > next_state_ts) {
            state->wave_index += 1;
            state->num_enemies_per_tick = state->wave_index * wave_increment;

            if (time_elapsed > 5 * 60 * 1000) {
                state->timer.enemy_spawn_interval = DEFAULT_ENEMY_SPAWN_INTERVAL_MS - (state->wave_index * 10);
            }
        }

    }
}

// :player
void update_player() {
    // Camera follow
    state->camera.target = (Vec2) { state->player_pos.x, state->player_pos.y };

    // Player hurt
    // :hurt
    {
        state->num_query_points = 0;
        qtree_query(
            state->enemy_qtree,
            (QRect) {
                state->player_pos.x,
                state->player_pos.y,
                10, 10
            },
            state->query_points,
            &state->num_query_points
        );
        for (int i = 0; i < state->num_query_points; i++) {
            QPoint pt = state->query_points[i];
            float damage = get_enemy_damage(state->enemies[pt.id].type);
            if (Vector2DistanceSqr(state->player_pos, state->enemies[pt.id].pos) <= 50) {
                state->player_health -= damage;
                if (get_current_time_millis() - state->timer.last_hurt_sound_ts > 400) {
                    play_sound_modulated(&state->sound_hurt, 0.5);
                    state->timer.last_hurt_sound_ts = get_current_time_millis();
                }
            }
        }
    }

    // Player enemy bullet collisions
    {
        for (int i = 0; i < state->enemy_bullet_count; i++) {
            Bullet *b = &state->enemy_bullets[i];
            float dist = Vector2DistanceSqr(state->player_pos, b->pos);
            if (dist < 50) {
                state->player_health -= b->strength;
                b->penetration = 0;
                b->strength = 0;
                if (get_current_time_millis() - state->timer.last_hurt_sound_ts > 400) {
                    play_sound_modulated(&state->sound_hurt, 0.5);
                    state->timer.last_hurt_sound_ts = get_current_time_millis();
                }
            }
        }
    }

    // Slow auto-heal
    {
        if (state->player_health < MAX_PLAYER_HEALTH) {
            if (GetRandomValue(0, 100) > 90) {
                state->player_health += 0.05;
            }
        }
    }
}

// :enemy
void update_enemies() {
    int ww = get_window_width();
    int wh = get_window_height();

    Vec2 player_pos = state->player_pos;
    Bullet *bullets = state->bullets;
    Enemy *enemies = state->enemies;
    int bullet_count = state->bullet_count;
    int *enemy_count = &state->enemy_count;

    // :reset
    // Reset the Quadtree
    {
        int now = get_current_time_millis();
        if (now - state->timer.qtree_update_ts > QTREE_UPDATE_INTERVAL_MILLIS) {
            state->timer.qtree_update_ts = now;
            qtree_reset_boundary(
                state->enemy_qtree, 
                get_visible_rect(state->player_pos, state->camera.zoom)
            );
            qtree_clear(state->enemy_qtree);

            /**
             * Cleanup dead enemies
             * 
             * Dead enemies should only be cleaned up during qtree reset,
             * This is so that the unordered enemy array shifting while deleting them
             * doesn't cause the ids in the qtree to be invalid 
             */
            for (int i = 0; i < *enemy_count; i++) {
                // :mana :health :heart
                bool should_drop_mana = (GetRandomValue(0, 100) > 50) || enemies[i].is_shiny;
                bool is_kill_enemy = true;

                if (enemies[i].health > 0) {
                    is_kill_enemy = false;
                    int lifetime = get_current_time_millis() - enemies[i].spawn_ts;
                    // if the enemy is far away cull it
                    if (lifetime > 10 * 1000) {
                        int dist = Vector2DistanceSqr(enemies[i].pos, state->player_pos);
                        if (dist > 200 * 200) {
                            should_drop_mana = false;
                            is_kill_enemy = true;
                        }
                    }
                }

                if (is_kill_enemy && should_drop_mana) {
                    // valid kill, leave mana behind
                    if (state->pickups_count < MAX_PICKUPS) {
                        state->pickups[state->pickups_count] = (Pickup) {
                            .pos = (Vec2) { enemies[i].pos.x, enemies[i].pos.y },
                            .type = get_pickup_spawn_type(enemies[i].is_shiny),
                            .spawn_ts = get_current_time_millis(),
                            .is_collected = false
                        };

                        qtree_insert(state->pickups_qtree, (QPoint) {
                            .x = enemies[i].pos.x,
                            .y = enemies[i].pos.y,
                            .id = state->pickups_count
                        });
                        state->pickups_count ++;
                    }
                }

                if (is_kill_enemy) {
                    enemies[i] = enemies[*enemy_count - 1];
                    *enemy_count -= 1;
                }
            }

            for (int i = 0; i < *enemy_count; i++) {
                qtree_insert(state->enemy_qtree, (QPoint) {
                    state->enemies[i].pos.x,
                    state->enemies[i].pos.y,
                    i
                });
            }
        }
    }

    // Bullet collision
    // :collision
    for (int i = 0; i < bullet_count; i++) {
        if (bullets[i].strength <= 0 || bullets[i].penetration <= 0) {
            continue;
        }
        int bullet_size = 5;

        if (bullets[i].type == ORBS) {
            bullet_size = state->stats.orbs_size;
        }

        state->num_query_points = 0;
        qtree_query(
            state->enemy_qtree,
            (QRect) {
                bullets[i].pos.x - 2.5f,
                bullets[i].pos.y - 2.5f,
                bullet_size, bullet_size
            },
            state->query_points,
            &state->num_query_points
        );

        for (int j = 0; j < state->num_query_points; j++) {
            QPoint pt = state->query_points[j];
            float damage = fmin(bullets[i].strength, enemies[pt.id].health);
            if (enemies[pt.id].health <= 0) {
                continue;
            }

            enemies[pt.id].health -= damage;
            enemies[pt.id].is_taking_damage = true;
            enemies[pt.id].damage_ts = get_current_time_millis();
            // bullets[i].strength -= damage;
            bullets[i].penetration -= 1;

            // idk how else to do this in a better way
            if (enemies[pt.id].health <= 0) {
                state->kill_count += 1;
            }

            // let one bullet hurt only one enemy
            if (bullets[i].penetration <= 0) break;
        }
    }

    // Area collisions
    // :area
    {
        // Flame area damage
        if (get_current_time_millis() - state->timer.flame_ts < state->stats.flame_lifetime) {
            Vec2 flame_dir = state->player_heading_dir;
            // range buffer to account for tringle and cone differences
            float flame_range = (state->stats.flame_distance * (PARTICLE_LIFETIME / 1000.0f)) + 10;
            Vec2 flame_dest = point_at_dist(state->player_pos, state->player_heading_dir, flame_range);

            // flame triangle
            Vec2 left_bound = Vector2Add(
                player_pos, 
                Vector2Scale(
                    rotate_vector(flame_dir, -state->stats.flame_spread), 
                    flame_range
                )
            );
            Vec2 right_bound = Vector2Add(
                player_pos, 
                Vector2Scale(rotate_vector(flame_dir, state->stats.flame_spread), flame_range)
            );

            // query rect
            Vec2 rect_center = get_line_center(flame_dest, state->player_pos);
            // 20 buffer
            float dist = Vector2Distance(flame_dest, state->player_pos) + 20;
            QRect rect = { rect_center.x, rect_center.y, dist/2, dist/2 };

            state->num_query_points = 0;
            qtree_query(
                state->enemy_qtree, rect,
                state->query_points, &state->num_query_points
            );

            for (int i = 0; i < state->num_query_points; i++) {
                QPoint pt = state->query_points[i];
                if (enemies[pt.id].health <= 0) {
                    continue;
                }

                // this is a cone approximation
                // there's 2 triangles, as the spread angle grows, 
                // we need the 2nd minor triangle to handle the other section of the cone
                bool is_in_major_triangle = is_point_in_triangle(
                    enemies[pt.id].pos, state->player_pos, left_bound, right_bound);
                bool is_in_minor_triangle = is_point_in_triangle(
                    enemies[pt.id].pos, flame_dest, left_bound, right_bound);

                if (is_in_major_triangle || is_in_minor_triangle) {
                    enemies[pt.id].health -= state->stats.flame_damage;
                    enemies[pt.id].is_taking_damage = true;
                    enemies[pt.id].damage_ts = get_current_time_millis();
                    if (enemies[pt.id].health <= 0) {
                        state->kill_count += 1;
                    }
                }
            }
        }

        // Frost area slowdown
        {
            if (get_current_time_millis() - state->timer.frost_wave_ts < state->stats.frost_wave_lifetime) {
                float dist = 100;
                QRect rect = { player_pos.x, player_pos.y, dist/2, dist/2 };
                state->num_query_points = 0;
                qtree_query(
                    state->enemy_qtree, rect,
                    state->query_points, &state->num_query_points
                );

                for (int i = 0; i < state->num_query_points; i++) {
                    QPoint pt = state->query_points[i];
                    bool can_frost = !enemies[pt.id].is_frozen;
                    if (can_frost) {
                        float cur_dist = Vector2DistanceSqr(enemies[pt.id].pos, state->player_pos);
                        if (cur_dist <= dist/2 * dist/2) {
                            enemies[pt.id].is_frozen = true;
                            enemies[pt.id].speed /= 2.0f;
                            enemies[pt.id].health -= state->stats.frost_wave_damage;
                            enemies[pt.id].frozen_ts = get_current_time_millis();
                        }
                    }
                }
            }
        }
    }

    // Enemies update
    {
        for (int i = 0; i < *enemy_count; i++) {
            Vec2 player_dir = Vector2Subtract(player_pos, enemies[i].pos);
            player_dir = Vector2Normalize(player_dir);
            
            if (enemies[i].health <= 0) {
                continue;
            }

            // :seperation :separation :boid
            Vec2 separation = Vector2Zero();
            {
                float perception_radius = 10.0f;
                
                // adjust how often we want to do separation
                int separation_threshold = 10;
                if (state->enemy_count < 100) {
                    separation_threshold = 50;
                } else if (state->enemy_count < 200) {
                    separation_threshold = 70;
                } else if (state->enemy_count < 500) {
                    separation_threshold = 90;
                } else {
                    separation_threshold = 99;
                }

                // TODO do i not do separation at some point?
                // maybe when the fps drops too low?
                // this may result in the qtree crashing due to many items in a single tile
                // if (state->enemy_count > 5000) {
                //     // don't do any separation
                //     separation_threshold = 101;
                // }

                // separation is applied randomly
                if (GetRandomValue(0, 100) > separation_threshold) {
                    state->num_query_points = 0;
                    qtree_query(
                        state->enemy_qtree,
                        (QRect) {
                            enemies[i].pos.x,
                            enemies[i].pos.y,
                            perception_radius * 2,
                            perception_radius * 2
                        },
                        state->query_points,
                        &state->num_query_points
                    );

                    for (int j = 0; j < state->num_query_points; j++) {
                        QPoint pt = state->query_points[j];
                        if (pt.id != i) {
                            Vec2 neighbor = Vector2Subtract(enemies[pt.id].pos, enemies[i].pos);
                            float dist = Vector2Length(neighbor);
                            
                            if (dist < perception_radius && dist > 0) {
                                // repulsion force, stronger at closer distances
                                float repulsion_strength = (1.0f - (dist / perception_radius)) * 0.5f;
                                Vec2 repulsion = Vector2Normalize(neighbor);
                                repulsion = Vector2Scale(repulsion, -repulsion_strength * ENEMY_SPEED);
                                separation = Vector2Add(separation, repulsion);
                            }
                        }
                    }
                }
            }

            // Enemy pos update
            // :behaviour

            // Unique enemy updates
            switch (enemies[i].type) {

                /**
                 * Default enemy behaviour is in the defualt case
                 * Jump to the defualt case for all enemies
                 */

                case RAM:
                    {
                        // Gets close to the player and pauses
                        // Then charges with a lot of speed for sometime

                        float dist_to_player = Vector2DistanceSqr(state->player_pos, enemies[i].pos);
                        if (dist_to_player > 50 * 50) {
                            enemies[i].is_player_found = false;
                            enemies[i].player_found_ts = 0;
                        } else if (!enemies[i].is_player_found) {
                            enemies[i].is_player_found = true;
                            enemies[i].player_found_ts = get_current_time_millis();
                        }

                        if (enemies[i].is_player_found) {
                            int time_elapsed = get_current_time_millis() - enemies[i].player_found_ts;
                            if (time_elapsed < 500) {
                                // wait for some time
                                enemies[i].charge_dir = player_dir;
                                break;
                            } else if (time_elapsed < 1500) {
                                // charge in last player dir for some time
                                Vec2 velocity = Vector2Scale(enemies[i].charge_dir, enemies[i].speed * 2.5f);
                                velocity = Vector2Add(velocity, separation);
                                velocity = Vector2Scale(velocity, GetFrameTime());
                                enemies[i].pos = Vector2Add(enemies[i].pos, velocity);
                                break;
                            } else {
                                // reset
                                enemies[i].is_player_found = false;
                                enemies[i].player_found_ts = 0;
                            }
                        }
                        goto default_behaviour;
                    }
                case MAGE:
                    {
                        // Gets close to the player
                        // Then shoots bullets until the player is in vision

                        float dist_to_player = Vector2DistanceSqr(state->player_pos, enemies[i].pos);
                        if (dist_to_player > 75 * 75) {
                            enemies[i].is_player_found = false;
                            enemies[i].player_found_ts = 0;
                        } else if (!enemies[i].is_player_found) {
                            enemies[i].is_player_found = true;
                            enemies[i].player_found_ts = get_current_time_millis();
                        }

                        if (enemies[i].is_player_found) {
                            int time_elapsed = get_current_time_millis() - enemies[i].player_found_ts;
                            if (time_elapsed > 1000 && state->enemy_bullet_count < MAX_ENEMY_BULLETS) {
                                // fire a bullet
                                state->enemy_bullets[state->enemy_bullet_count] = (Bullet) {
                                    .pos = enemies[i].pos,
                                    .direction = player_dir,
                                    .spawnTs = get_current_time_millis(),
                                    .strength = 10,
                                    .penetration = 1,
                                    .type = MAGE_BULLET,
                                    .speed = get_attack_speed(MAGE_BULLET),
                                    .angle = 0
                                };
                                state->enemy_bullet_count += 1;

                                // reset time acts as a bullet interval
                                enemies[i].player_found_ts = get_current_time_millis();
                            }

                            // don't approach the player once found
                            break;
                        }
                        goto default_behaviour;
                    }
                case DEMON:
                    {
                        // Gets close to the player
                        // Spawns pup enemies and spawns many bullets

                        float dist_to_player = Vector2DistanceSqr(state->player_pos, enemies[i].pos);
                        if (dist_to_player > 90 * 90) {
                            enemies[i].is_player_found = false;
                            enemies[i].player_found_ts = 0;
                        } else if (!enemies[i].is_player_found) {
                            enemies[i].is_player_found = true;
                            enemies[i].player_found_ts = get_current_time_millis();
                        }

                        if (enemies[i].is_player_found) {
                            int time_elapsed = get_current_time_millis() - enemies[i].player_found_ts;
                            if (time_elapsed > 2000) {

                                // fire bullets
                                int num_bullets = 5;
                                float spread_angle = 60.0f;
                                float half_angle = spread_angle / 2.0f;
                                float angle_increment = spread_angle / (num_bullets - 1);

                                for (int j = 0; j < num_bullets; j++) {
                                    float angle = -half_angle + j * angle_increment;
                                    Vec2 bullet_dir = rotate_vector(player_dir, angle);

                                    if (state->enemy_bullet_count > MAX_ENEMY_BULLETS) break;

                                    state->enemy_bullets[state->enemy_bullet_count] = (Bullet) {
                                        .pos = enemies[i].pos,
                                        .direction = bullet_dir,
                                        .spawnTs = get_current_time_millis(),
                                        .strength = 10,
                                        .penetration = 1,
                                        .type = DEMON_BULLET,
                                        .speed = get_attack_speed(DEMON_BULLET),
                                        .angle = 0
                                    };
                                    state->enemy_bullet_count += 1;
                                }

                                // reset time acts as a bullet interval
                                enemies[i].player_found_ts = get_current_time_millis();
                            }

                            // don't approach the player once found
                            break;
                        }

                        // Spawn pups
                        // :pups
                        {
                            int num_pups_to_spawn = 5;
                            int time_elapsed = (get_current_time_millis() - enemies[i].last_spawn_ts);
                            float spawn_distance = 20.0f;
                            bool can_spawn = GetRandomValue(0, 100) > 90 && state->enemy_count < MAX_ENEMIES;
                            // bool can_spawn = state->enemy_count < MAX_ENEMIES;
                            if (can_spawn && time_elapsed > 10000 && dist_to_player < 100 * 100) {
                                enemies[i].last_spawn_ts = get_current_time_millis();
                                for (int j = 0; j < num_pups_to_spawn; j++) {
                                    float angle = (2.0f * PI / num_pups_to_spawn) * j;
                                    Vector2 spawn_offset = (Vec2) {
                                        .x = cosf(angle) * spawn_distance,
                                        .y = sinf(angle) * spawn_distance
                                    };

                                    // Spawn the pup
                                    enemies[*enemy_count] = (Enemy) {
                                        .pos = (Vec2) {
                                            .x = enemies[i].pos.x + spawn_offset.x,
                                            .y = enemies[i].pos.y + spawn_offset.y
                                        },
                                        .health = get_enemy_health(DEMON_PUP, false),
                                        .type = DEMON_PUP,
                                        .speed = get_enemy_speed(DEMON_PUP),
                                        .spawn_ts = get_current_time_millis(),
                                        .is_shiny = false,
                                        .is_player_found = false,
                                        .player_found_ts = 0,
                                        .charge_dir = Vector2Zero(),
                                        .last_spawn_ts = 0,
                                        .is_frozen = false,
                                        .is_taking_damage = false,
                                    };

                                    *enemy_count += 1;
                                }
                            }
                        }

                        goto default_behaviour;
                    }
                default:
                default_behaviour:
                    {
                        // default behaviour, approach player
                        Vec2 velocity = Vector2Scale(player_dir, enemies[i].speed);
                        velocity = Vector2Add(velocity, separation);
                        velocity = Vector2Scale(velocity, GetFrameTime());
                        enemies[i].pos = Vector2Add(enemies[i].pos, velocity);
                        break;
                    }
            }

            // Extra stuff
            if (enemies[i].is_frozen) {
                if (get_current_time_millis() - enemies[i].frozen_ts > 2000) {
                    enemies[i].is_frozen = false;
                    enemies[i].frozen_ts = 0;
                    enemies[i].speed = get_enemy_speed(enemies[i].type);
                }
            }

            if (enemies[i].is_taking_damage) {
                if (get_current_time_millis() - enemies[i].damage_ts > 100) {
                    enemies[i].is_taking_damage = false;
                    enemies[i].damage_ts = 0;
                }
            }
        }
    }

    // Auto spawn enemies
    // :spawn
    {
        int elapsed = get_current_time_millis() - state->timer.enemy_spawn_ts;
        if (elapsed > state->timer.enemy_spawn_interval) {
            for (int i = 0; i < state->num_enemies_per_tick; i++) {
                if (*enemy_count >= MAX_ENEMIES) {
                    break;
                }

                Vec2 randPos = Vector2Zero();
                float diagonal_length = sqrt(ww * ww + wh * wh) / 2;
                randPos = get_rand_pos_around_point(player_pos, diagonal_length, diagonal_length + 30);

                int rand_enemy = get_next_enemy_spawn_type();
                bool is_shiny = GetRandomValue(1, 100) > (100 - state->stats.shiny_chance);
                state->timer.enemy_spawn_ts = get_current_time_millis();
                enemies[*enemy_count] = (Enemy) {
                    .pos = randPos,
                    .health = get_enemy_health(rand_enemy, is_shiny),
                    .type = rand_enemy,
                    .speed = get_enemy_speed(rand_enemy),
                    .spawn_ts = get_current_time_millis(),
                    .is_shiny = is_shiny,
                    .is_player_found = false,
                    .player_found_ts = 0,
                    .charge_dir = Vector2Zero(),
                    .last_spawn_ts = 0,
                    .is_frozen = false,
                    .is_taking_damage = false
                };

                *enemy_count += 1;
            }
        }
    }
}

// :particle
void update_particles() {
    Vec2 player_pos = state->player_pos;

    // Mana pickup particles
    {
        int i = 0;
        while (i < state->mana_particles_count) {

            // reached player
            float dist = Vector2DistanceSqr(player_pos, state->mana_particles[i]);
            if (dist < 10) {
                state->mana_particles[i] = state->mana_particles[state->mana_particles_count - 1];
                state->mana_particles_count -= 1;
                i++;
                continue;
            }

            // move towards player
            Vec2 dir = Vector2Subtract(player_pos, state->mana_particles[i]);
            dir = Vector2Normalize(dir);
            state->mana_particles[i].x += dir.x * GetFrameTime() * 200;
            state->mana_particles[i].y += dir.y * GetFrameTime() * 200;

            i++;
        }
    }

    // Flame particles
    {
        int i = 0;
        while (i < state->flame_particles_count) {
            Particle *p = &state->flame_particles[i];

            int time_alive = get_current_time_millis() - p->spawn_ts;
            if (time_alive > p->lifetime) {
                state->flame_particles[i] = state->flame_particles[state->flame_particles_count - 1];
                state->flame_particles_count -= 1;
                i ++;
                continue;
            }

            Vec2 dir = state->flame_particles[i].dir;
            state->flame_particles[i].pos.x += dir.x * GetFrameTime() * state->stats.flame_distance;
            state->flame_particles[i].pos.y += dir.y * GetFrameTime() * state->stats.flame_distance;

            i ++;
        }
    }

    // Frost wave particles
    {
        int i = 0;
        while (i < state->frost_wave_particles_count) {
            Particle *p = &state->frost_wave_particles[i];

            int time_alive = get_current_time_millis() - p->spawn_ts;
            if (time_alive > p->lifetime) {
                state->frost_wave_particles[i] = state->frost_wave_particles[state->frost_wave_particles_count - 1];
                state->frost_wave_particles_count -= 1;
                i ++;
                continue;
            }

            Vec2 dir = state->frost_wave_particles[i].dir;
            state->frost_wave_particles[i].pos.x += dir.x * GetFrameTime() * 120;
            state->frost_wave_particles[i].pos.y += dir.y * GetFrameTime() * 120;

            i ++;
        }
    }
}

// :bullet :gun :weapon
void update_bullets() {
    Bullet *bullets = state->bullets;
    Bullet *enemy_bullets = state->enemy_bullets;
    Enemy *enemies = state->enemies;

    Vec2 player_pos = state->player_pos;
    int *bullet_count = &state->bullet_count;
    int *enemy_bullet_count = &state->enemy_bullet_count;

    // Auto shoot weapon
    // :fire :shoot
    {
        // Simple Bullet
        bool is_simple_shoot = get_current_time_millis() - state->timer.bullet_ts > state->stats.bullet_interval;
        if (is_simple_shoot) {
            float min_dist = FLT_MAX;
            bool enemy_found = false;
            Vec2 enemy_pos = Vector2Zero();

            play_sound_modulated(&state->sound_bullet_fire, 0.1);

            state->num_query_points = 0;
            qtree_query(
                state->enemy_qtree,
                (QRect) { player_pos.x, player_pos.y, GUN_VISION, GUN_VISION },
                state->query_points,
                &state->num_query_points
            );

            for (int i = 0; i < state->num_query_points; i++) {
                QPoint pt = state->query_points[i];
                float dist = Vector2DistanceSqr(player_pos, enemies[pt.id].pos);
                if (dist < min_dist) {
                    min_dist = dist;
                    enemy_pos = enemies[pt.id].pos;
                    enemy_found = true;
                }
            }

            Vec2 dir = Vector2Subtract(enemy_pos, player_pos);
            dir = Vector2Normalize(dir);

            if (!enemy_found) {
                dir = get_rand_unit_vec2();
            }

            for (int i = 0; i < state->stats.bullet_count; i++) {
                state->timer.bullet_ts = get_current_time_millis();
                if (*bullet_count >= MAX_BULLETS) {
                    break;
                }

                // first bullet always hits the target
                float angle_spread = 0;
                if (i != 0) {
                    int spread = (int) state->stats.bullet_spread;
                    // TODO probably have to clamp the spread angle here
                    angle_spread = GetRandomValue(-spread, spread);
                }

                dir = rotate_vector(dir, angle_spread);
                bullets[*bullet_count] = (Bullet) {
                    .pos = player_pos,
                    .direction = dir,
                    .spawnTs = get_current_time_millis(),
                    .strength = state->stats.bullet_damage,
                    .penetration = state->stats.bullet_penetration,
                    .type = BULLET,
                    .speed = get_attack_speed(BULLET),
                    .angle = 0
                };
                *bullet_count += 1;
            }
        }

        // :splinter
        // Revolving bullets are fired when the old ones die

        bool is_shoot_splinter = get_current_time_millis() - state->timer.splinter_ts > state->stats.splinter_interval;
        if (is_shoot_splinter && state->upgrades.splinter_level > 0) {
            play_sound_modulated(&state->sound_splinter, 0.3);
            for (int i = 0; i < state->stats.splinter_count; i++) {
                Vec2 dir = get_rand_unit_vec2();
                if (*bullet_count < MAX_BULLETS) {
                    bullets[*bullet_count] = (Bullet){
                        .pos = player_pos,
                        .direction = dir,
                        .spawnTs = get_current_time_millis(),
                        .strength = state->stats.splinter_damage,
                        .penetration = state->stats.splinter_penetration,
                        .type = SPLINTER,
                        .speed = get_attack_speed(SPLINTER),
                        .angle = 0
                    };
                    *bullet_count += 1;
                }
                state->timer.splinter_ts = get_current_time_millis();
            }
        }

        // :orbs
        bool is_shoot_orbs = get_current_time_millis() - state->timer.orbs_ts > state->stats.orbs_interval;
        if (is_shoot_orbs && state->upgrades.orbs_level > 0) {
            for (int i = 0; i < state->stats.orbs_count; i ++) {
                Vec2 dir = get_rand_unit_vec2();
                if (*bullet_count < MAX_BULLETS) {
                    bullets[*bullet_count] = (Bullet) {
                        .pos = player_pos,
                        .direction = dir,
                        .spawnTs = get_current_time_millis(),
                        .strength = state->stats.orbs_damage,
                        .penetration = INT_MAX,
                        .type = ORBS,
                        .speed = get_attack_speed(ORBS),
                        .angle = 0
                    };
                    *bullet_count += 1;
                }
            }
            state->timer.orbs_ts = get_current_time_millis();
            play_sound_modulated(&state->sound_orb, 0.3);
        }
    }

    // Bullet update
    int num_spikes = 0;
    {
        int i = 0;
        while (i < *bullet_count) {
            Bullet *b = &bullets[i];
            int now = get_current_time_millis();

            bool is_kill_bullet = b->penetration <= 0 || b->strength <= 0 || 
                    (now - b->spawnTs) > get_attack_range(b->type);
            // this kills slow orb bullets when reversing
            bool is_slow_bullet = b->speed < -20;
            if (is_kill_bullet || is_slow_bullet) {
                // Unordered remove
                // Swap the current bullet with the last one and then pop the last bullet
                bullets[i] = bullets[*bullet_count - 1];
                *bullet_count -= 1;
                i++;
                continue;
            }

            if (b->type == SPIKE) {
                b->angle += state->stats.spike_speed * GetFrameTime();
                if (b->angle > 2 * PI) {
                    b->angle -= 2 * PI;
                }

                b->pos.x = state->player_pos.x + SPIKE_RADIUS * cosf(b->angle);
                b->pos.y = state->player_pos.y + SPIKE_RADIUS * sinf(b->angle);

                i++;
                num_spikes ++;
                continue;
            }
            if (b->type == ORBS) {
                b->speed -= 1;
            }

            b->pos.x += b->direction.x * GetFrameTime() * b->speed;
            b->pos.y += b->direction.y * GetFrameTime() * b->speed;
            i++;
        }
    }

    // Launch revolving bullets
    // :spike
    {
        bool is_shoot_spike = get_current_time_millis() - state->timer.spike_ts > state->stats.spike_interval;
        if (state->upgrades.spike_level > 0 && is_shoot_spike && num_spikes < state->stats.spike_count) {
            Vec2 dir = get_rand_unit_vec2();
            if (*bullet_count < MAX_BULLETS) {
                bullets[*bullet_count] = (Bullet){
                    .pos = player_pos,
                    .direction = dir,
                    .spawnTs = get_current_time_millis(),
                    .strength = state->stats.spike_damage,
                    .penetration = 10,
                    .type = SPIKE,
                    .speed = get_attack_speed(SPIKE),
                    .angle = GetRandomValue(0, 360)
                };
                *bullet_count += 1;
            }
            state->timer.spike_ts = get_current_time_millis();
        }
    }

    // Flamethrower
    // :flamethrower :flame
    {
        bool is_shoot_flame = get_current_time_millis() - state->timer.flame_ts > state->stats.flame_interval;
        bool is_post_shoot = get_current_time_millis() - state->timer.flame_ts < state->stats.flame_lifetime;

        if (state->upgrades.flame_level > 0 && (is_shoot_flame || is_post_shoot)) {
            if (state->flame_particles_count <= MAX_FLAME_PARTICLES) {
                for (int i = 0; i < 2; i ++) {
                    state->flame_particles[state->flame_particles_count] = (Particle) {
                        .pos = state->player_pos,
                        .dir = rotate_vector(
                            state->player_heading_dir, 
                            GetRandomValue(-state->stats.flame_spread, state->stats.flame_spread)
                        ),
                        .lifetime = PARTICLE_LIFETIME,
                        .spawn_ts = get_current_time_millis()
                    };
                    state->flame_particles_count += 1;
                }
            }

            if (is_shoot_flame) {
                // TODO playing this on a low interval breaks my speakers
                if (!GOD) 
                    play_sound_modulated(&state->sound_flamethrower, 0.9);

                state->timer.flame_ts = get_current_time_millis();
            }
        }
    }

    // Frost wave
    // :frost
    {
        int num_frost_particles = 3;
        bool is_shoot_frost_wave = get_current_time_millis() - state->timer.frost_wave_ts > state->stats.frost_wave_interval;
        bool is_post_shoot = get_current_time_millis() - state->timer.frost_wave_ts < state->stats.frost_wave_lifetime;

        if (state->upgrades.frost_wave_level > 0 && (is_shoot_frost_wave || is_post_shoot)) {
            for (int i = 0; i < num_frost_particles; i++) {
                if (state->frost_wave_particles_count <= MAX_FROST_WAVE_PARTICLES) {
                    state->frost_wave_particles[state->frost_wave_particles_count] = (Particle) {
                        .pos = state->player_pos,
                        .dir = get_rand_unit_vec2(),
                        .lifetime = state->stats.frost_wave_lifetime,
                        .spawn_ts = get_current_time_millis()
                    };
                    state->frost_wave_particles_count += 1;
                }
            }

            if (is_shoot_frost_wave) {
                state->timer.frost_wave_ts = get_current_time_millis();
                play_sound_modulated(&state->sound_frost_wave, 0.4);
            }
        }
    }

    // Enemy bullets update
    {
        int i = 0;
        while (i < *enemy_bullet_count) {
            Bullet *b = &enemy_bullets[i];
            int now = get_current_time_millis();

            if ((now - b->spawnTs) > get_attack_range(b->type) || b->penetration <= 0) {
                // Unordered remove
                // Swap the current bullet with the last one and then pop the last bullet
                enemy_bullets[i] = enemy_bullets[*enemy_bullet_count - 1];
                *enemy_bullet_count -= 1;
                i++;
                continue;
            }

            b->pos.x += b->direction.x * GetFrameTime() * get_attack_speed(b->type);
            b->pos.y += b->direction.y * GetFrameTime() * get_attack_speed(b->type);
            i++;
        }
    }
}

// :pickups
void update_pickups() {
    // Cleanup collectd pickups
    {
        int now = get_current_time_millis();
        if (now - state->timer.pickups_cleanup_ts > PICKUPS_CLEANUP_INTERVAL) {
            state->timer.pickups_cleanup_ts = now;

            for (int i = 0; i < state->pickups_count; i++) {
                bool too_old = get_current_time_millis() - state->pickups[i].spawn_ts >= PICKUPS_LIFETIME;
                if (too_old || state->pickups[i].is_collected) {
                    state->pickups[i] = state->pickups[state->pickups_count - 1];
                    state->pickups_count -= 1;
                }
            }
        }
    }

    // Grab pickups
    {
        float perception_radius = 25.0f;
        state->num_query_points = 0;
        qtree_query(
            state->pickups_qtree,
            (QRect) {
                state->player_pos.x,
                state->player_pos.y,
                perception_radius,
                perception_radius
            },
            state->query_points,
            &state->num_query_points
        );

        for (int j = 0; j < state->num_query_points; j++) {
            QPoint pt = state->query_points[j];
            float dist = Vector2DistanceSqr(state->player_pos, (Vec2) { pt.x, pt.y });
            PickupType pickup_type = state->pickups[pt.id].type;
            if (dist > perception_radius * perception_radius) {
                continue;
            }

            // remove the pickup item
            state->pickups[pt.id].is_collected = true;
            qtree_remove(state->pickups_qtree, pt);

            if (pickup_type == MANA || pickup_type == MANA_SHINY) {
                int mana_value = get_mana_value();
                state->mana_count += pickup_type == MANA_SHINY ? 7 * mana_value : mana_value;

                // mana pickup particle
                // TODO there's a bug here, sometimes particles that are far away are being attracted to the player
                // I tried doing a (&& state->pickups[pt.id].is_collected) but it doesn't help
                // Hence doing a distance check for now, 
                // It doesn't fully work properly and a few pickups don't have pickup particle effect
                float perception_range = perception_radius * perception_radius;
                if (state->mana_particles_count < MAX_MANA_PARTICLES && dist <= perception_range) {
                    state->mana_particles[state->mana_particles_count] = (Vec2) { pt.x, pt.y };
                    state->mana_particles_count += 1;
                }

                // :levelup
                int level_mana_count = get_level_mana_threshold(state->player_level);
                if (state->mana_count >= level_mana_count) {
                    state->mana_count -= level_mana_count;
                    state->player_level += 1;

                    update_available_upgrades();
                    state->screen = UPGRADE_MENU;
                    pause_timers();
                }
            } else if (pickup_type == HEALTH) {
                play_sound_modulated(&state->sound_health_pickup, 0.9);
                state->player_health = Clamp(state->player_health + 100, 0, MAX_PLAYER_HEALTH);
            } else {
                printe("Picked up an unhandled pickup");
            }
        }
    }
}

void update_toasts() {
    if (state->toasts_count <= 0) {
        return;
    }

    int now = get_current_time_millis();
    int elapsed = now - state->toasts[0].spawn_ts;
    if (elapsed <= TOAST_LIEFTIME_MS) {
        return;
    }

    for (int i = 1; i < state->toasts_count; i++) {
        state->toasts[i - 1] = state->toasts[i];
        state->toasts[i - 1].spawn_ts = now;
    }
    state->toasts_count -= 1;
}

// MARK: :sound :music

void sounds_init() {
    InitAudioDevice();

    // bg
    state->sound_bg = LoadMusicStream(SOUND_BG_1);
    state->sound_bg.looping = true;

    // heartbeat
    state->sound_heartbeat = LoadMusicStream(SOUND_HEARTBEAT);
    state->sound_heartbeat.looping = true;

    // impact
    state->sound_upgrade = LoadSound(SOUND_UPGRADE);
    state->sound_bullet_fire = LoadSound(SOUND_BULLET_FIRE);
    state->sound_splinter = LoadSound(SOUND_SPLINTER);
    state->sound_flamethrower = LoadSound(SOUND_FLAME_THROWER);
    state->sound_frost_wave = LoadSound(SOUND_FROST_WAVE);
    state->sound_orb = LoadSound(SOUND_ORB);
    state->sound_hurt= LoadSound(SOUND_HURT);
    state->sound_health_pickup= LoadSound(SOUND_HEALTH_PICKUP);

    PlayMusicStream(state->sound_bg);
    PlayMusicStream(state->sound_heartbeat);
    SetMusicVolume(state->sound_heartbeat, 0.0f);
}

void sounds_destroy() {
    StopMusicStream(state->sound_bg);
    UnloadMusicStream(state->sound_bg);

    StopMusicStream(state->sound_heartbeat);
    UnloadMusicStream(state->sound_heartbeat);

    UnloadSound(state->sound_upgrade);
    UnloadSound(state->sound_bullet_fire);
    UnloadSound(state->sound_splinter);
    UnloadSound(state->sound_flamethrower);
    UnloadSound(state->sound_frost_wave);
    UnloadSound(state->sound_orb);
    UnloadSound(state->sound_hurt);
    UnloadSound(state->sound_health_pickup);

    // Closing audio before unloading sound crashes the game on android
    CloseAudioDevice();
}

void sounds_update() {
    UpdateMusicStream(state->sound_bg);
    UpdateMusicStream(state->sound_heartbeat);

    // :heartbeat
    {
        float health_ratio = state->player_health / MAX_PLAYER_HEALTH;
        if (health_ratio < 0.7) {
            state->sound_intensity = health_ratio + 0.3;
            float tempo = 1.0f + (1.0f - health_ratio * 2.0);
            tempo = fmax(1.0f, fmin(2.0f, tempo));

            SetMusicVolume(state->sound_heartbeat, 0.3f + (1.0f - health_ratio));
            SetMusicPitch(state->sound_heartbeat, tempo);
            SetMusicVolume(state->sound_bg, health_ratio + 0.3);
        } else {
            SetMusicVolume(state->sound_heartbeat, 0.0f);
        }

        if (state->player_health <= 0) {
            SetMusicVolume(state->sound_heartbeat, 0.0f);
            SetMusicPitch(state->sound_heartbeat, 1.0f);
            SetMusicVolume(state->sound_bg, 1.0);
        }
    }
}

void play_sound_modulated(Sound *sound, float volume) {
    float pitch = 0.8f + (GetRandomValue(0, 1000) / 1000.0f) * 0.4f;
    // float pan = -1.0f + (GetRandomValue(0, 1000) / 1000.0f) * 2.0f;

    // TODO do we need pan at all?
    SetSoundPitch(*sound, pitch);
    SetSoundVolume(*sound, volume * state->sound_intensity);
    // SetSoundPan(*sound, pan);
    PlaySound(*sound);
}

// MARK: :decorations

void decorations_init() {
    int num_decorations_in_view = NUM_DECORATIONS / 3;
    for (int i = 0; i < num_decorations_in_view; i ++) {
        int rand_decoration = GetRandomValue(0, TOTAL_NUM_DECORATIONS);
        state->decorations[i] = (Decoration) {
            .pos = (Vec2) {
                GetRandomValue(0, state->world_dims.width),
                GetRandomValue(0, state->world_dims.height)
            },
            .decoration_idx = rand_decoration
        };
    }

    for (int i = num_decorations_in_view; i < (NUM_DECORATIONS - num_decorations_in_view); i ++) {
        int rand_decoration = GetRandomValue(0, TOTAL_NUM_DECORATIONS);
        state->decorations[i] = (Decoration) {
            .pos = (Vec2) {
                GetRandomValue(0, state->world_dims.width),
                GetRandomValue(0, state->world_dims.height)
            },
            .decoration_idx = rand_decoration
        };
    }
}

void update_decorations() {
    for (int i = 0; i < NUM_DECORATIONS; i ++) {
        Decoration *d = &state->decorations[i];
        float dist = Vector2DistanceSqr(state->player_pos, d->pos);
        float dist_threshold = 300.0f;
        if (dist > dist_threshold * dist_threshold) {
            int rand_decoration = GetRandomValue(0, TOTAL_NUM_DECORATIONS);
            Vec2 rand_pos = get_rand_pos_around_point(state->player_pos, 200.0f, 300.0f);
            state->decorations[i] = (Decoration) {
                .pos = rand_pos,
                .decoration_idx = rand_decoration
            };
        }
    }
}

// MARK: :input

void handle_player_input() {
    Vec2 *player_pos = &state->player_pos;
    bool is_dir_invalid = true;
    Vec2 old_pos = *player_pos;

    // wasd movement
    {
        Vec2 dir = Vector2Zero();
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { dir.y -= 1; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { dir.y += 1; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) { dir.x -= 1; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { dir.x += 1; }

        dir = Vector2Normalize(dir);
        is_dir_invalid = is_vec2_zero(dir);

        player_pos->x += dir.x * GetFrameTime() * state->stats.player_speed;
        player_pos->y += dir.y * GetFrameTime() * state->stats.player_speed;

        if (!is_dir_invalid) {
            state->player_heading_dir = Vector2Normalize(
                Vector2Subtract(*player_pos, old_pos));
        }
    }

    // joystick movement
    {
        if (state->is_joystick_enabled && is_dir_invalid) {
            player_pos->x += state->joystick_direction.x * GetFrameTime() * state->stats.player_speed;
            player_pos->y += state->joystick_direction.y * GetFrameTime() * state->stats.player_speed;
            state->player_heading_dir = Vector2Normalize(Vector2Subtract(*player_pos, old_pos));
        }
    }
}

void handle_extra_inputs() {
    GameScreen *screen = &state->screen;
    // pause
    if (IsKeyPressed(KEY_SPACE)) {
        if (*screen == PAUSE_MENU) {
            resume_timers();
            *screen = IN_GAME;
        } else if (*screen == IN_GAME) {
            pause_timers();
            *screen = PAUSE_MENU;
        }
        return;
    }
}

void handle_debug_inputs() {
    if (!DEBUG) return;

    GameScreen *screen = &state->screen;
    if (IsKeyPressed(KEY_TAB)) {
        state->is_show_debug_gui = !state->is_show_debug_gui;
    }
    if (IsKeyPressed(KEY_RIGHT_CONTROL)) {
        state->wave_index += 10;
        state->num_enemies_per_tick = state->wave_index;
    }
    if (IsKeyPressed(KEY_RIGHT_SHIFT) || IsKeyPressedRepeat(KEY_RIGHT_SHIFT)) {
        state->mana_count += 10;
    }
    if (IsKeyPressed(KEY_BACKSLASH) || IsKeyPressedRepeat(KEY_BACKSLASH)) {
        state->timer.game_start_ts -= 1000;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER)) {
        state->player_health = MAX_PLAYER_HEALTH;
    }
    if (IsKeyPressed(KEY_GRAVE) && *screen == IN_GAME) {
        toast("This is a sample Toast");
    }

    // Camera scroll
    float mouse_diff = GetMouseWheelMove();
    if (mouse_diff != 0 && *screen == IN_GAME) {
        if (mouse_diff > 0) {
            state->camera.zoom += 0.1;
        } else {
            state->camera.zoom -= 0.1;
        } 
    }
}

// :resize
void handle_window_resize() {
    int ww = GetScreenWidth();
    int wh = GetScreenHeight();

    float target_aspect = (float)VIRT_WIDTH / (float)VIRT_HEIGHT;
    int scaled_width, scaled_height;

    if ((float)ww / wh > target_aspect) {
        // Window wider than target aspect ratio
        scaled_height = wh;
        scaled_width = (int)(scaled_height * target_aspect);
    } else {
        // Window taller than target aspect ratio
        scaled_width = ww;
        scaled_height = (int)(scaled_width / target_aspect);
    }

    state->dest_rect = (Rect) {
        (ww - scaled_width) / 2,
        (wh - scaled_height) / 2,
        scaled_width, scaled_height
    };
}

// :joystick
void handle_virtual_joystick_input() {
    // button 0 / touch input -> left click

    if (IsMouseButtonPressed(0)) {
        state->is_joystick_enabled = true;
        state->touch_down_pos = GetMousePosition();
    }
    if (IsMouseButtonReleased(0)) {
        state->is_joystick_enabled = false;
        state->touch_down_pos = Vector2Zero();
    }

    if (state->is_joystick_enabled && IsMouseButtonDown(0)) {
        state->joystick_direction = Vector2Subtract(GetMousePosition(), state->touch_down_pos);
        state->joystick_direction = Vector2Normalize(state->joystick_direction);
    }
}

// MARK: :quadtree :qtree
/**
 * This is a simple quadtree impl following
 * https://www.youtube.com/watch?v=OJxEcs0w_kE
 * Original code: https://editor.p5js.org/codingtrain/sketches/CDMjU0GIK
 * 
 * Its used to store all enemy locations that are currently visible on the screen,
 * When the player moves around, the boundaries of the qtree are also updated
 * TODO might have to actually free the quadtree after certain clears/boundary-resets?
 */

QTree *qtree_create(QRect boundary) {
    QTree *qtree = malloc(sizeof(QTree));
    if (!qtree) return NULL;
    
    qtree->points = malloc(POINTS_PER_QUAD * sizeof(QPoint));
    qtree->boundary = boundary;
    qtree->is_divided = false;
    qtree->num_points = 0;
    qtree->tl = qtree->tr = qtree->bl = qtree->br = NULL;
    
    return qtree;
}

void qtree_destroy(QTree *qtree) {
    if (!qtree) return;

    if (qtree->is_divided) {
        qtree_destroy(qtree->tl);
        qtree_destroy(qtree->tr);
        qtree_destroy(qtree->bl);
        qtree_destroy(qtree->br);
    }

    free(qtree->points);
    free(qtree);
}

void qtree_clear(QTree *qtree) {
    if (!qtree) return;
    
    qtree->num_points = 0;
    if (qtree->is_divided) {
        qtree_clear(qtree->tl);
        qtree_clear(qtree->tr);
        qtree_clear(qtree->bl);
        qtree_clear(qtree->br);
    }
}

void qtree_reset_boundary(QTree *qtree, QRect rect) {
    qtree->boundary.x = rect.x;
    qtree->boundary.y = rect.y;
    qtree->boundary.w = rect.w;
    qtree->boundary.h = rect.h;
    
    if (qtree->is_divided) {
        float w = rect.w/2;
        float h = rect.h/2;
        
        qtree->tl->boundary.x = rect.x - w/2;
        qtree->tl->boundary.y = rect.y - h/2;
        qtree->tl->boundary.w = w;
        qtree->tl->boundary.h = h;
        qtree_reset_boundary(
            qtree->tl, 
            (QRect) { qtree->tl->boundary.x, qtree->tl->boundary.y, w, h }
        );
        
        qtree->tr->boundary.x = rect.x + w/2;
        qtree->tr->boundary.y = rect.y - h/2;
        qtree->tr->boundary.w = w;
        qtree->tr->boundary.h = h;
        qtree_reset_boundary(
            qtree->tr, 
            (QRect) { qtree->tr->boundary.x, qtree->tr->boundary.y, w, h }
        );
        
        qtree->bl->boundary.x = rect.x - w/2;
        qtree->bl->boundary.y = rect.y + h/2;
        qtree->bl->boundary.w = w;
        qtree->bl->boundary.h = h;
        qtree_reset_boundary(
            qtree->bl, 
            (QRect) { qtree->bl->boundary.x, qtree->bl->boundary.y, w, h }
        );
        
        qtree->br->boundary.x = rect.x + w/2;
        qtree->br->boundary.y = rect.y + h/2;
        qtree->br->boundary.w = w;
        qtree->br->boundary.h = h;
        qtree_reset_boundary(
            qtree->br, 
            (QRect) { qtree->br->boundary.x, qtree->br->boundary.y, w, h }
        );
    }
}

bool qtree_insert(QTree *tree, QPoint pt) {
    if (!tree)
        return false;
    if (!is_rect_contains_point(tree->boundary, pt))
        return false;

    if (tree->num_points < POINTS_PER_QUAD) {
        tree->points[tree->num_points] = pt;
        tree->num_points += 1;
        return true;
    }

    if (!tree->is_divided) {
        _qtree_subdivide(tree);
        for (int i = 0; i < tree->num_points; i++) {
            qtree_insert(tree->tl, tree->points[i]);
            qtree_insert(tree->tr, tree->points[i]);
            qtree_insert(tree->bl, tree->points[i]);
            qtree_insert(tree->br, tree->points[i]);
        }
        tree->num_points = 0;
    }

    return qtree_insert(tree->tl, pt) || 
        qtree_insert(tree->tr, pt) || 
        qtree_insert(tree->bl, pt) || 
        qtree_insert(tree->br, pt);
}

bool qtree_remove(QTree *qtree, QPoint pt) {
    if (!qtree || !is_rect_contains_point(qtree->boundary, pt))
        return false;
        
    for (int i = 0; i < qtree->num_points; i++) {

        // we don't do the id check here since the id of the items aren't valid
        // ie when you remove a pickup item, the id of other pickups may change
        if (qtree->points[i].x != pt.x && qtree->points[i].y != pt.y) {
            continue;
        }

        // Don't do an unordered remove here since the id values in the qtree are hardcoded
        qtree->points[i] = qtree->points[qtree->num_points - 1];
        qtree->num_points--;
        return true;
    }
    
    if (qtree->is_divided) {
        return qtree_remove(qtree->tl, pt) ||
               qtree_remove(qtree->tr, pt) ||
               qtree_remove(qtree->bl, pt) ||
               qtree_remove(qtree->br, pt);
    }
    return false;
}

void qtree_query(QTree *qtree, QRect range, QPoint *result, int *num_points) {
    if (!qtree || !result || !num_points) 
        return;
    if (*num_points >= MAX_ENEMIES)
        return;
    if (!is_rect_overlap(qtree->boundary, range))
        return;

    for (int i = 0; i < qtree->num_points; i ++) {
        if (is_rect_contains_point(range, qtree->points[i])) {
            result[*num_points] = qtree->points[i];
            *num_points += 1;
        }
    }

    if (qtree->is_divided) {
        qtree_query(qtree->tl, range, result, num_points);
        qtree_query(qtree->tr, range, result, num_points);
        qtree_query(qtree->bl, range, result, num_points);
        qtree_query(qtree->br, range, result, num_points);
    }
}

void _qtree_subdivide(QTree *qtree) {
    float x = qtree->boundary.x;
    float y = qtree->boundary.y;
    float w = qtree->boundary.w;
    float h = qtree->boundary.h;

    QRect tl = (QRect) { x - w/2, y - h/2, w/2, h/2 };
    QRect tr = (QRect) { x + w/2, y - h/2, w/2, h/2 };
    QRect bl = (QRect) { x - w/2, y + h/2, w/2, h/2 };
    QRect br = (QRect) { x + w/2, y + h/2, w/2, h/2 };

    qtree->tl = qtree_create(tl);
    qtree->tr = qtree_create(tr);
    qtree->bl = qtree_create(bl);
    qtree->br = qtree_create(br);

    if (!qtree->tl || !qtree->tr || !qtree->bl || !qtree->br) {
        free(qtree->tl);
        free(qtree->tr);
        free(qtree->bl);
        free(qtree->br);
        qtree->tl = qtree->tr = qtree->bl = qtree->br = NULL;
        return;
    }

    qtree->is_divided = true;
}

// MARK: :data :switch

Vec2 get_enemy_sprite_pos(EnemyType type, bool is_shiny) {
    switch (type) {
        case BAT:
            return is_shiny ? (Vec2) {0, 4} : (Vec2) {0, 3};
        case RAM:
            return (Vec2) {1, 3};
        case MAGE:
            return (Vec2) {2, 3};
        case DEMON:
            return (Vec2) {4, 3};
        case DEMON_PUP:
            return (Vec2) {5, 3};
        case REAPER:
            return (Vec2) {3, 3};
        default:
            return (Vec2) {0, 0};
    }
}

float get_enemy_scale(EnemyType type) {
    switch (type) {
        case BAT:
            return DEFAULT_SPRITE_SCALE;
        case RAM:
            return DEFAULT_SPRITE_SCALE + 0.3;
        case MAGE:
            return DEFAULT_SPRITE_SCALE + 0.3;
        case REAPER:
            return DEFAULT_SPRITE_SCALE + 0.3;
        case DEMON:
            return DEFAULT_SPRITE_SCALE + 0.6;
        case DEMON_PUP:
            return DEFAULT_SPRITE_SCALE;
        default:
            return DEFAULT_SPRITE_SCALE;
    }
}

float get_enemy_health(EnemyType type, bool is_shiny) {
    switch (type) {
        case BAT:
            return is_shiny ? 200 : 15;
        case RAM:
            return 150;
        case MAGE:
            return 80;
        case DEMON:
            return 500;
        case DEMON_PUP:
            return 5;
        case REAPER:
            return 50000;
        default:
            return 100;
    }
}

float get_enemy_speed(EnemyType type) {
    switch (type) {
        case BAT:
            return ENEMY_SPEED * 0.9;
        case RAM:
            return ENEMY_SPEED * 1.2;
        case MAGE:
            return ENEMY_SPEED * 0.7;
        case DEMON:
            return ENEMY_SPEED * 1.0;
        case REAPER:
            return ENEMY_SPEED * 0.6;
        default:
            return ENEMY_SPEED * 0.9;
    }
}

float get_enemy_damage(EnemyType type) {
    switch (type) {
        case BAT:
            return 1;
        case RAM:
            return 3;
        case MAGE:
            return 2;
        case DEMON:
            return 3;
        case REAPER:
            return 5;
        default:
            return 1;
    }
}

Vec2 get_attack_sprite(AttackType type) {
    switch (type) {
        case BULLET:
            return (Vec2) {0, 6};
        case MAGE_BULLET:
            return (Vec2) {4, 6};
        case DEMON_BULLET:
            return (Vec2) {4, 6};
        case SPLINTER:
            return (Vec2) {1, 6};
        case SPIKE:
            return (Vec2) {2, 6};
        case FLAME:
        case FROST_WAVE:
            return (Vec2) {3, 6};
        case ORBS:
            return (Vec2) {6, 6};
        default:
            return (Vec2) {0, 0};
    }
}

int get_attack_range(AttackType type) {
    switch (type) {
        case BULLET:
            return state->stats.bullet_range;
        case SPLINTER:
            return state->stats.splinter_range;
        case SPIKE:
            return INT_MAX;
        case ORBS:
            return 3000;
        case MAGE_BULLET:
            return 1200;
        case DEMON_BULLET:
            return 3500;
        default:
            return 200;
    }
}

int get_attack_speed(AttackType type) {
    switch (type) {
        case BULLET:
            return 400;
        case MAGE_BULLET:
            return 100;
        case DEMON_BULLET:
            return 35;
        case SPLINTER:
            return 200;
        case SPIKE:
            // this is a dummy value,
            // there's a stat for this
            return 5;
        case ORBS:
            return 120;
        default:
            return 100;
    }
}

// MARK: :stat

float get_base_stat_value(ShopUpgradeType type) {
    switch (type) {
        case BULLET_INTERVAL: return 400;
        case BULLET_COUNT: return GOD ? 10 : 1;
        case BULLET_SPREAD: return GOD ? 30 : 15;
        case BULLET_RANGE: return GOD ? 400 : 200;
        case BULLET_DAMAGE: return 15;
        case BULLET_PENETRATION: return 1;

        case SPLINTER_INTERVAL: return 1000;
        case SPLINTER_RANGE: return 1000;
        case SPLINTER_COUNT: return GOD ? 20 : 3;
        case SPLINTER_DAMAGE: return 10;
        case SPLINTER_PENETRATION: return 3;

        case SPIKE_INTERVAL: return GOD ? 1 : 200;
        case SPIKE_SPEED: return 5;
        case SPIKE_COUNT: return GOD ? 5000 : 3;
        case SPIKE_DAMAGE: return 3;

        case FLAME_INTERVAL: return GOD ? 100 : 4800;
        case FLAME_LIFETIME: return 800;
        case FLAME_SPREAD: return 25;
        case FLAME_DISTANCE: return 200;
        case FLAME_DAMAGE: return 0.5f;

        case FROST_WAVE_INTERVAL: return 3000;
        case FROST_WAVE_LIFETIME: return 300;
        case FROST_WAVE_DAMAGE: return 5;

        case ORBS_INTERVAL: return 3200;
        case ORBS_DAMAGE: return 1;
        case ORBS_COUNT: return 2;
        case ORBS_SIZE: return 5;

        case PLAYER_SPEED: return 40;
        case SHINY_CHANCE: return 1;

        default:
            return 0;
    }
}

// :increment
float get_stat_increment(ShopUpgradeType type) {
    switch (type) {
        case BULLET_INTERVAL: return -5;
        case BULLET_COUNT: return 1;
        case BULLET_SPREAD: return 1;
        case BULLET_RANGE: return 4;
        case BULLET_DAMAGE: return 3;
        case BULLET_PENETRATION: return 1;

        case SPLINTER_INTERVAL: return -3;
        case SPLINTER_RANGE: return 4;
        case SPLINTER_COUNT: return 2;
        case SPLINTER_DAMAGE: return 1;
        case SPLINTER_PENETRATION: return 0;

        case SPIKE_INTERVAL: return -10;
        case SPIKE_SPEED: return 0.2;
        case SPIKE_COUNT: return 5;
        case SPIKE_DAMAGE: return 5;

        case FLAME_INTERVAL: return -20;
        case FLAME_LIFETIME: return 25;
        case FLAME_SPREAD: return 0.5;
        case FLAME_DISTANCE: return 3;
        case FLAME_DAMAGE: return 0.2f;

        case FROST_WAVE_INTERVAL: return -100;
        case FROST_WAVE_LIFETIME: return 10;
        case FROST_WAVE_DAMAGE: return 1;

        case ORBS_INTERVAL: return -20;
        case ORBS_DAMAGE: return 1;
        case ORBS_COUNT: return 1;
        case ORBS_SIZE: return 1;

        case PLAYER_SPEED: return 1;
        case SHINY_CHANCE: return 1;

        default: return 1.0;
    }
}

// MARK: :impl :func

EnemyType get_next_enemy_spawn_type() {
    int time_elapsed = get_current_time_millis() - state->timer.game_start_ts;
    int prob = GetRandomValue(0, 100);

    /**
     * Remember to sequence the following logic in the reverse time order
     */

    // more reaper
    if (time_elapsed > 60 * 12 * 1000) {
        if (prob > 99) return REAPER;
        if (prob > 98) return DEMON;
        if (prob > 97) return MAGE;
        if (prob > 96) return RAM;
        return BAT;
    }

    // reaper
    if (time_elapsed > 60 * 10 * 1000) {
        if (prob > 99) {
            if (GetRandomValue(0, 100) > 80) return REAPER;
        }
        if (prob > 98) return DEMON;
        if (prob > 97) return MAGE;
        if (prob > 96) return RAM;
        return BAT;
    }

    // more mages
    if (time_elapsed > 60 * 5 * 1000) {
        if (prob > 99) return DEMON;
        if (prob > 98) return MAGE;
        if (prob > 97) return RAM;
        return BAT;
    }

    // mage
    if (time_elapsed > 60 * 3 * 1000) {
        if (prob > 99) return MAGE;
        if (prob > 98) return RAM;
        return BAT;
    }

    // rams
    if (time_elapsed > 60 * 1 * 1000) {
        if (prob > 99) return RAM;
    }

    return BAT;
}

int get_num_enemies_per_tick() {
    int elapsed = get_current_time_millis() - state->timer.game_start_ts;
    int mins = elapsed / (60 * 1000);

    if (mins < 3) {
        return 3;
    }

    if (mins < 7) {
        return mins - 3 + 8;
    }

    return mins - 5 + 25;
}

int get_mana_value() {
    // int elapsed = get_current_time_millis() - state->timer.game_start_ts;
    // int mins = elapsed / (60 * 1000);

    // if (mins < 3) {
    //     return 1;
    // }

    // if (mins < 5) {
    //     return 2;
    // }

    // if (mins < 8) {
    //     return 3;
    // }

    // if (mins < 10) {
    //     return 5;
    // }

    return 1;
}

PickupType get_pickup_spawn_type(bool is_shiny) {
    if (is_shiny) return MANA_SHINY;
    if (GetRandomValue(0, 8000) > 7999) {
        if (get_current_time_millis() - state->timer.last_heart_spawn_ts > 20 * 1000) {
            state->timer.last_heart_spawn_ts = get_current_time_millis();
            return HEALTH;
        }
    }
    return MANA;
}

void update_available_upgrades() {
    // TODO use the state to decide which ones are upgradable

    // reset options
    state->available_upgrades.bullet_level = 0;
    state->available_upgrades.splinter_level = 0;
    state->available_upgrades.spike_level = 0;
    state->available_upgrades.flame_level = 0;
    state->available_upgrades.frost_wave_level = 0;
    state->available_upgrades.orbs_level = 0;
    state->available_upgrades.speed_level = 0;
    state->available_upgrades.shiny_level = 0;

    Upgrades *upgrades = &state->available_upgrades;
    int *fields[] = {
        &upgrades->bullet_level,
        &upgrades->splinter_level,
        &upgrades->spike_level,
        &upgrades->flame_level,
        &upgrades->frost_wave_level,
        &upgrades->orbs_level,
        &upgrades->speed_level,
        &upgrades->shiny_level,
    };
    size_t num_fields = sizeof(fields) / sizeof(fields[0]);

    // shuffle
    for (size_t i = 0; i < num_fields - 1; i++) {
        size_t j = i + GetRandomValue(0, num_fields - i - 1);
        int *temp = fields[i];
        fields[i] = fields[j];
        fields[j] = temp;
    }

    // pick 4 for now
    for (size_t i = 0; i < 4; i++) {
        *fields[i] = 1;
    }
}

void upgrade_stat(UpgradeType type) {
    int rand_val = GetRandomValue(0, 100);
    switch (type)
    {
        case BULLET_UPGRADE: {
            state->stats.bullet_count += get_stat_increment(BULLET_COUNT);
            if (rand_val > 50) state->stats.bullet_interval += get_stat_increment(BULLET_INTERVAL);
            if (rand_val > 50) state->stats.bullet_spread += get_stat_increment(BULLET_SPREAD);
            state->stats.bullet_damage += get_stat_increment(BULLET_DAMAGE);
            state->stats.bullet_penetration += get_stat_increment(BULLET_PENETRATION);
            state->upgrades.bullet_level += 1;

            toast("Bullet upgraded");
            break;
        }
        case SPLINTER_UPGRADE: {
            state->stats.splinter_count += get_stat_increment(SPLINTER_COUNT);
            if (rand_val > 50) state->stats.splinter_interval += get_stat_increment(SPLINTER_INTERVAL);
            if (rand_val > 50) state->stats.splinter_range += get_stat_increment(SPLINTER_RANGE);
            state->stats.splinter_damage += get_stat_increment(SPLINTER_DAMAGE);
            state->stats.splinter_penetration += get_stat_increment(SPLINTER_PENETRATION);
            state->upgrades.splinter_level += 1;

            (state->upgrades.splinter_level <= 1)
                ? toast("Splinter unlocked!")
                : toast("Splinter upgraded");
            break;
        }
        case SPIKE_UPGRADE: {
            state->stats.spike_count += get_stat_increment(SPIKE_COUNT);
            if (rand_val > 50) state->stats.spike_interval += get_stat_increment(SPIKE_INTERVAL);
            state->stats.spike_speed += get_stat_increment(SPIKE_SPEED);
            state->stats.spike_damage += get_stat_increment(SPIKE_DAMAGE);
            state->upgrades.spike_level += 1;

            (state->upgrades.spike_level <= 1)
                ? toast("Spike unlocked!")
                : toast("Spike upgraded");
            break;
        }
        case FLAME_UPGRADE: {
            if (rand_val > 50) state->stats.flame_interval += get_stat_increment(FLAME_INTERVAL);
            state->stats.flame_lifetime += get_stat_increment(FLAME_LIFETIME);
            if (rand_val > 50) state->stats.flame_spread += get_stat_increment(FLAME_SPREAD);
            state->stats.flame_distance += get_stat_increment(FLAME_DISTANCE);
            state->stats.flame_damage += get_stat_increment(FLAME_DAMAGE);
            state->upgrades.flame_level += 1;

            (state->upgrades.flame_level <= 1)
                ? toast("Flame unlocked!")
                : toast("Flame upgraded");
            break;
        }
        case FROST_UPGRADE: {
            if (rand_val > 50) state->stats.frost_wave_interval += get_stat_increment(FROST_WAVE_INTERVAL);
            if (rand_val > 50) state->stats.frost_wave_lifetime += get_stat_increment(FROST_WAVE_LIFETIME);
            state->stats.frost_wave_damage += get_stat_increment(FROST_WAVE_DAMAGE);
            state->upgrades.frost_wave_level += 1;

            (state->upgrades.frost_wave_level <= 1)
                ? toast("Frost wave unlocked!")
                : toast("Frost wave upgraded");
            break;
        }
        case ORBS_UPGRADE: {
            if (rand_val > 50) state->stats.orbs_interval += get_stat_increment(ORBS_INTERVAL);
            state->stats.orbs_damage += get_stat_increment(ORBS_DAMAGE);
            state->stats.orbs_count += get_stat_increment(ORBS_COUNT);
            if (rand_val > 70) state->stats.orbs_size += get_stat_increment(ORBS_SIZE);
            state->upgrades.orbs_level += 1;

            (state->upgrades.orbs_level <= 1)
                ? toast("Orbs unlocked!")
                : toast("Orbs upgraded");
            break;
        }
        case SPEED_UPGRADE: {
            state->stats.player_speed += get_stat_increment(PLAYER_SPEED);
            state->upgrades.speed_level += 1;
            toast("Speed upgraded");
            break;
        }
        case SHINY_UPGRADE: {
            state->stats.shiny_chance += get_stat_increment(SHINY_CHANCE);
            state->upgrades.shiny_level += 1;
            toast("Shiny Probability upgraded");
            break;
        }
        default:
            printe(TextFormat("Unknown upgrade: %d", type));
            break;
    }
}

int get_level_mana_threshold(int level) {
    return (int) ((5 * level) + (level * 4));
}

// MARK: :button :btn

bool main_menu_button(Button btn) {
    NPatchInfo patch_info = {
        .source = {0, 16 * 12, 48, 48},
        .left = 16, .top = 16, .right = 16, .bottom = 16,
        .layout = NPATCH_NINE_PATCH
    };
    Vec2 text_size = MeasureTextEx(state->custom_font, btn.label, btn.font_size, 2);
    Vec2 text_pos = { btn.pos.x + (btn.size.x - text_size.x) / 2, btn.pos.y + (btn.size.y - text_size.y) / 2 };

    bool is_mouse_hovered = CheckCollisionPointRec(
        GetMousePosition(), (Rect) {btn.pos.x, btn.pos.y, btn.size.x, btn.size.y});
    DrawTextureNPatch(
        state->sprite_sheet,
        patch_info,
        (Rect) { btn.pos.x, btn.pos.y, btn.size.x, btn.size.y },
        Vector2Zero(), 0.0f, COLOR_WHITE
    );
    draw_text_with_shadow(btn.label, text_pos, btn.font_size, COLOR_WHITE, COLOR_CEMENT);

    return is_mouse_hovered && IsMouseButtonPressed(0);
}

bool upgrade_menu_button(Button btn) {
    // 9patch
    NPatchInfo patch_info = {
        .source = {0, 16 * 12, 48, 48},
        .left = 16, .top = 16, .right = 16, .bottom = 16,
        .layout = NPATCH_NINE_PATCH
    };

    int padding = 1;
    DrawRectangleV(
        Vector2SubtractValue(btn.pos, padding), 
        Vector2AddValue(btn.size, padding * 2), 
        COLOR_DARK_BLUE
    );
    DrawTextureNPatch(
        state->sprite_sheet,
        patch_info,
        (Rect) { btn.pos.x, btn.pos.y, btn.size.x, btn.size.y },
        Vector2Zero(), 0.0f, COLOR_WHITE
    );

    Vec2 text_size = MeasureTextEx(state->custom_font, btn.label, btn.font_size, 2);
    Vec2 text_pos = { btn.pos.x + (btn.size.x - text_size.x) / 2, btn.pos.y + (btn.size.y - text_size.y) / 2 };
    DrawTextEx(state->custom_font, btn.label, text_pos, btn.font_size, 2, btn.fg);

    bool is_mouse_hovered = CheckCollisionPointRec(
        GetMousePosition(), (Rect) { btn.pos.x, btn.pos.y, btn.size.x, btn.size.y });
    return is_mouse_hovered && IsMouseButtonPressed(0);
}

bool button(Button btn) {
    Vec2 text_size = MeasureTextEx(state->custom_font, btn.label, btn.font_size, 2);
    Vec2 text_pos = { btn.pos.x + (btn.size.x - text_size.x) / 2, btn.pos.y + (btn.size.y - text_size.y) / 2 };
    Vec2 pos_offset = Vector2AddValue(btn.pos, 3);
    Vec2 pos_offset_rev = Vector2SubtractValue(btn.pos, 3);
    Vec2 size_offset = Vector2AddValue(btn.size, 6);

    bool is_mouse_hovered = CheckCollisionPointRec(
        GetMousePosition(), (Rect) {btn.pos.x, btn.pos.y, btn.size.x, btn.size.y});

    DrawRectangleV(
        is_mouse_hovered ? pos_offset_rev : pos_offset,
        is_mouse_hovered ? size_offset : btn.size,
        is_mouse_hovered ? COLOR_WHITE : btn.shadow
    );
    DrawRectangleV(btn.pos, btn.size, btn.bg);
    DrawTextEx(state->custom_font, btn.label, text_pos, btn.font_size, 2, btn.fg);

    return is_mouse_hovered && IsMouseButtonPressed(0);
}

//:slider
void slider(float ypos, const char *label, float *slider_offset, bool *is_drag) {
    float ww = (float) GetScreenWidth();
    float wh = (float) GetScreenHeight();
    Vec2 slider_size = { 300, 10 };
    Vec2 handle_size = { 10, 30 };
    float max_offset_val = slider_size.x - handle_size.x;
    int font_size = 35;

    Vec2 pos = { ww / 2 - slider_size.x / 2, wh * ypos };
    Vec2 text_pos = { pos.x, pos.y + 40 };
    Vec2 handle_pos = { 
        pos.x + (*slider_offset * max_offset_val),
        pos.y - handle_size.y / 2 + slider_size.y / 2 
    };

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vec2 mouse_pos = GetMousePosition();
        Rectangle handle_rect = { handle_pos.x, handle_pos.y, handle_size.x, handle_size.y };

        if (*is_drag || CheckCollisionPointRec(mouse_pos, handle_rect)) {
            *is_drag = true;
            *slider_offset = Clamp(
                mouse_pos.x - pos.x - handle_size.x / 2, 0, max_offset_val);
            *slider_offset /= max_offset_val;
        }
    } else {
        *is_drag = false;
    }

    DrawTextEx(state->custom_font, label, text_pos, font_size, 2, COLOR_WHITE);
    DrawTextEx(
        state->custom_font, 
        TextFormat("%d%%", (int) (*slider_offset * 100)), 
        (Vec2) { text_pos.x, text_pos.y + 40 }, 
        font_size, 2, COLOR_WHITE
    );
    DrawRectangleV(pos, slider_size, COLOR_BROWN);
    DrawRectangleV(
        (Vec2) {pos.x + (*slider_offset * max_offset_val), handle_pos.y},
        handle_size, COLOR_WHITE
    );
}

void toast(const char *message) {
    if (state->toasts_count >= MAX_NUM_TOASTS) {
        return;
    }

    state->toasts[state->toasts_count] = (Toast) {
        .message = message,
        .spawn_ts = get_current_time_millis()
    };
    state->toasts_count += 1;
}

void draw_text_with_shadow(const char *text, Vec2 pos, int size, Color fg, Color bg) {
    Vec2 pos_offset = Vector2AddValue(pos, 2);
    DrawTextEx(state->custom_font, text, pos_offset, size, 2, bg);
    DrawTextEx(state->custom_font, text, pos, size, 2, fg);
}

// MARK: :utils

int get_window_width() {
    // return GetScreenWidth();
    return VIRT_WIDTH;
}

int get_window_height() {
    // return GetScreenHeight();
    return VIRT_HEIGHT;
}

QRect get_visible_rect(Vec2 center, float zoom) {
    float view_width = get_window_width();
    float view_height = get_window_height();
    return (QRect) { center.x, center.y, view_width + 10, view_height + 10 };
}

int get_current_time_millis() {
    return GetTime() * 1000;
}

bool is_vec2_zero(Vec2 vec) {
    return vec.x == 0.0f && vec.y == 0.0f;
}

Vec2 rotate_vector(Vec2 v, float angle) {
    float angle_rad = angle * DEG2RAD;
    float cosa = cosf(angle_rad);
    float sina = sinf(angle_rad);
    return (Vec2) { 
        v.x * cosa - v.y * sina,
        v.x * sina + v.y * cosa
    };
}

Vec2 get_line_center(Vec2 a, Vec2 b) {
    return (Vec2) {
        (a.x + b.x) / 2,
        (a.y + b.y) / 2
    };
}

Vec2 get_rand_unit_vec2() {
    Vec2 rand_vec = (Vec2) { GetRandomValue(-100, 100), GetRandomValue(-100, 100) };
    return Vector2Normalize(rand_vec);
}

bool is_rect_contains_point(QRect rect, QPoint pt) {
    return (pt.x >= rect.x - rect.w && pt.x <= rect.x + rect.w) && 
           (pt.y >= rect.y - rect.h && pt.y <= rect.y + rect.h);
}

bool is_rect_overlap(QRect first, QRect second) {
    return !(
        first.x - first.w > second.x + second.w || 
        first.x + first.w < second.x - second.w || 
        first.y - first.h > second.y + second.h || 
        first.y + first.h < second.y - second.h
    );
}

bool are_colors_equal(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

Vec2 get_rand_pos_around_point(Vec2 pt, float min_dist, float max_dist) {
    float angle = GetRandomValue(0, 360) * (PI / 180.0f);
    float distance = min_dist + (float) GetRandomValue(0, 100) / 100.0f * (max_dist - min_dist);

    return (Vec2) {
        pt.x + distance * cosf(angle),
        pt.y + distance * sinf(angle)
    };
}

Vec2 point_at_dist(Vec2 pt, Vec2 dir, float dist) {
    return (Vec2) { pt.x + dir.x * dist, pt.y + dir.y * dist };
}

bool is_point_in_triangle(Vec2 pt, Vec2 a, Vec2 b, Vec2 c) {
    // Barycentric weights
    // https://en.wikipedia.org/wiki/Barycentric_coordinate_system
    float w1 = ((b.y - a.y) * (pt.x - a.x) - (b.x - a.x) * (pt.y - a.y)) /
               ((b.y - a.y) * (c.x - a.x) - (b.x - a.x) * (c.y - a.y));
    float w2 = ((c.y - a.y) * (pt.x - a.x) - (c.x - a.x) * (pt.y - a.y)) /
               ((c.y - a.y) * (b.x - a.x) - (c.x - a.x) * (b.y - a.y));
    return w1 >= 0 && w2 >= 0 && (w1 + w2) <= 1;
}

float vec2_to_angle(Vec2 dir) {
    float angle = atan2f(-dir.y, dir.x) * RAD2DEG;
    if (angle < 0) angle += 360.0f;
    return angle;
}

void print(const char *text) {
    if (!DEBUG) return;
    TraceLog(LOG_INFO, text);
}

void printe(const char *text) {
    TraceLog(LOG_ERROR, text);
}

// MARK: :ana

void* post_analytics_thread(void* arg) {
    AnalyticsType type = *(AnalyticsType*)arg;
    free(arg);

    char url[256] = SERVER_URL;
    switch (type) {
        case GAME_OPEN:
            sprintf(url, "%s/game-open", SERVER_URL);
            break;
        case GAME_START:
            sprintf(url, "%s/game-start", SERVER_URL);
            break;
        case GAME_OVER:
            sprintf(url, "%s/game-over", SERVER_URL);
            break;
    }

    if (DEBUG) {
        sprintf(url, "%s/debug", url);
    } else if (IS_MOBILE) {
        sprintf(url, "%s/mobile", url);
    }

    #ifndef WASM
    {
        CURL *curl = curl_easy_init();
        if (!curl) {
            printe("Curl init failed");
            return NULL;
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printe(TextFormat("curl failed: %s\n", curl_easy_strerror(res)));
        }
        curl_easy_cleanup(curl);
    }
    #else
    {
        // WASM API GET request using emscripten_fetch
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        // no success/failure callbacks
        // emscripten will automatically clean up the fetch resources
        attr.onsuccess = NULL;
        attr.onerror = NULL;
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        emscripten_fetch(&attr, url);
    }
    #endif

    return NULL;
}

void post_analytics_async(AnalyticsType type) {
    #ifndef WASM
    {
        pthread_t thread;
        AnalyticsType* data = malloc(sizeof(AnalyticsType));
        if (!data) {
            printe("malloc for thread failed");
            return;
        }
        *data = type;

        if (pthread_create(&thread, NULL, post_analytics_thread, data) != 0) {
            printe("failed to create thread");
            free(data);
        } else {
            // automatically cleans up resources when thread exits
            pthread_detach(thread); 
        }
    }
    #else
    {
        AnalyticsType* data = malloc(sizeof(AnalyticsType));
        if (!data) {
            printe("malloc for thread failed");
            return;
        }
        *data = type;
        post_analytics_thread(data);
    }
    #endif
}

// MARK: :main

void game_update() {
    BeginDrawing();

    bool in_game = state->screen != MAIN_MENU;
    if (in_game) {
        update_game();
        draw_game();
    }

    if (state->screen == MAIN_MENU) {
        update_main_menu_enemies();
        draw_main_menu();
    }

    handle_debug_inputs();
    handle_extra_inputs();
    EndDrawing();
}

#ifndef WASM
int main(void) {
    // Init Window
    // use FLAG_VSYNC_HINT for vsync
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "cgame1");
    SetWindowMinSize(VIRT_WIDTH, VIRT_HEIGHT);
    SetTargetFPS(FPS);

    // disable escape key for non debug builds
    if (!DEBUG) SetExitKey(KEY_NULL);

    gamestate_create();
    while (!WindowShouldClose()) {
        if (IsWindowResized() || (state->is_fullscreen != IsWindowFullscreen())) {
            handle_window_resize();
            state->is_fullscreen = IsWindowFullscreen();
        }

        game_update();
    }

    gamestate_destroy();
    CloseWindow();
    return 0;
}
#endif
