/*

Game design ideas:
 - enemy movement patterns
 - enemy attack one by one
 - power ups
    - double shot
    - invulnerable
 - spaceships to protect?
 - more enemy types
    - ones that have a shell that reflects bullets, and open/close it
    - ones that are wide and will block shots, but have one weak point at a precise location
    - ones that run towards you when you get too close (to use the vertical movement a bit)
    - swarms of disposable ennemies that fly in boids fashon and sometimes rush in your direction. You have to go up/down too to avoid them
    - dish that flies by at the top of the screen and always drops a powerup
    - enemies that are explosive, when they die they take out other enemies in a radius. So it's an incentive to take them out first or when
      there are many other enemies nearby. If they expode next to you they also hurt you ofc
 - shoot charge

Feel:
 - screeshake
 - sound

*/

#include <stdio.h>
#include "MiniFB.h"
#include <stdlib.h>
#include <memory.h>

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    #include <unistd.h>
#else
    #include <io.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>

#define SOKOL_IMPL
#include "sokol_log.h"
#include "sokol_audio.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

#define ARR_LEN(arr) ((int) (sizeof(arr) / sizeof(*arr)))
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#ifdef EMSCRIPTEN
    #define WINDOW_FAC 1
#else
    #define WINDOW_FAC 4
#endif

#define FRAME_SX 128
#define FRAME_SY 128

#define WINDOW_SX (FRAME_SX*WINDOW_FAC)
#define WINDOW_SY (FRAME_SY*WINDOW_FAC)

#include "input.cpp"
#include "draw.cpp"
#include "audio.cpp"

#define MAX_CHARGE 300

int spaceship_x = 50;
int spaceship_y = FRAME_SY - 20;
int player_health = 3;
int score = 0;
int player_invicibility_frame = 0;
int player_charge = 0;
int player_last_bullet_shot_timer = 0;

sound_clip_t explode_clip;

typedef struct bullet_t
{
    int x;
    int y;
}
bullet_t;

bullet_t bullets[20];
int      bullet_count;

void spawn_bullet(int x, int y)
{
    printf("Shot!!\n");
    if (bullet_count == ARR_LEN(bullets)) {
        return;
    }

    audio_play_sound(440 * 2 + rand() % 30, false);

    bullets[bullet_count] = (bullet_t) { .x = x, .y = y };
    bullet_count++;
}

typedef struct enemy_t
{
    int x;
    int y;
    int base_x; // the base pos for patterns (real pos oscilating around base)
    int base_y;
    int hit_anim;
    int threshold_y;
    int health;
}
enemy_t;

int enemies_count = 0;
enemy_t enemies[40];

void spawn_enemy(int x, int y, int threshold_y)
{
    if (enemies_count == ARR_LEN(enemies)) {
        return;
    }

    enemies[enemies_count++] = (enemy_t) {
        .x = x, .y = y,
        .base_x = x, .base_y = y,
        .threshold_y = threshold_y,
        .health = 2
    };
}

typedef struct explosion_t
{
    int x;
    int y;
    int frame;
}
explosion_t;

int explosions_count = 0;
explosion_t explosions[40];

void spawn_explosion(int x, int y)
{
    if (explosions_count == ARR_LEN(explosions)) {
        return;
    }

    audio_play_sound_clip(explode_clip);
    //audio_play_sound(0, true);

    explosions[explosions_count++] = (explosion_t) { .x = x, .y = y };
}

typedef struct star_t
{
    int x;
    int y;
}
star_t;

star_t stars[20];

rect_t centered_rect(int x, int y, int sx, int sy)
{
    return (rect_t) { x - (sx/2), y - (sy/2), sx, sy };
}

int frame_count;
bool game_running;
int game_over_timer;

void move_towards(int* x, int* y, int target_x, int target_y, float max_dist)
{
    int dx = target_x - *x;
    int dy = target_y - *y;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist == 0) {
        return;
    }
    float fac = max_dist / dist;
    *x += fac * dx;
    *y += fac * dy;
}

bool check_aabb_col(rect_t a, rect_t b)
{
    return
        a.x < b.x + b.sx && a.x + a.sx > b.x
        &&
        a.y < b.y + b.sy && a.y + a.sy > b.y
    ;
}

int check_bullet_col(rect_t rect)
{
    int col_count = 0;
    for (int i = 0; i < bullet_count; i++) {
        bullet_t bullet = bullets[i];
        rect_t bullet_rect = centered_rect(bullet.x, bullet.y, 2, 5); // TODO
        if (check_aabb_col(rect, bullet_rect)) {
            col_count++;

            // destroy bullet
            bullets[i] = bullets[bullet_count-1];
            bullet_count--;
        }
    }
    return col_count;
}

void spawn_wave()
{
    for (int x = 24; x < FRAME_SX - 10; x += 20) {
        spawn_enemy(x, 10, 30 + rand() % 80);
    }
}

rect_t get_player_rect()
{
    return centered_rect(spaceship_x, spaceship_y, 12, 6);
}

void reset_game()
{
    game_over_timer = 0;
    bullet_count = 0;
    enemies_count = 0;
    frame_count = 0;
    explosions_count = 0;
    player_health = 3;
    score = 0;
    player_charge = 0;
}

int main()
{
    printf("Hello world!\n");

    // NOTE: on the web platform the title of the window should match the html canvas id! (wierd but ok...)
    struct mfb_window* window = mfb_open_ex("Game", WINDOW_SX, WINDOW_SY, 0);
    if (!window) {
        printf("Error, couldn't open miniFB window!\n");
    }

    font_map = load_bitmap("assets/font_map.png");

    buffer = (uint32_t*) malloc(FRAME_SX * FRAME_SY * 4);

    window_buffer = (uint32_t*) malloc(WINDOW_SX * WINDOW_SY * 4);

    mfb_set_keyboard_callback(window, on_keyboard_event);

    explode_clip = load_sound_clip("assets/jump.wav");

    reset_game();

    // load bitmaps
    bitmap_t invader[] = {
        load_bitmap("assets/invader_001.png"),
        load_bitmap("assets/invader_002.png"),
        load_bitmap("assets/invader_001.png"),
        load_bitmap("assets/invader_003.png"),
    };

    bitmap_t invader_mad[] = {
        load_bitmap("assets/invader_mad_001.png"),
        load_bitmap("assets/invader_mad_002.png"),
        load_bitmap("assets/invader_mad_001.png"),
        load_bitmap("assets/invader_mad_003.png"),
    };

    bitmap_t expl_anim[] = {
        load_bitmap("assets/expl_001.png"),
        load_bitmap("assets/expl_002.png"),
        load_bitmap("assets/expl_003.png"),
        load_bitmap("assets/expl_004.png"),
    };

    bitmap_t player_ship_imgs[] = {
        load_bitmap("assets/player_ship_left.png"),
        load_bitmap("assets/player_ship.png"),
        load_bitmap("assets/player_ship_right.png"),
    }; 

    // Init stars
    for (int i = 0; i < ARR_LEN(stars); i++) {
        stars[i].x += rand() % FRAME_SX;
        stars[i].y += rand() % FRAME_SY;
    }

    audio_init();

    #ifdef __EMSCRIPTEN__
    mfb_timer* timer = mfb_timer_create();
    double last_rendered_frame_time = -1000;
    #endif

    while (true) {
        #ifdef __EMSCRIPTEN__
        double now_time = mfb_timer_now(timer);
        double delta_time = now_time - last_rendered_frame_time;
        double target_delta_time = 1.f/60.f;
        if (delta_time < target_delta_time) {
            mfb_wait_sync(window);
            continue;
        }
        last_rendered_frame_time = now_time;
        #else
        mfb_set_target_fps(60);
        #endif

        // Clear screen
        for (int i = 0; i < FRAME_SX * FRAME_SY; i++) {
            buffer[i] = MFB_ARGB(255,30,30,70);
            //memset(buffer, 0,  * 4);
        }

        if ((frame_count % 300) == 0) {
            printf("Spawning wave\n");
            spawn_wave();
        }

        frame_count++;

        //draw_text(0, 0, "Fist gather " FOOD_ICN_STR ", then " STONE_ICN_STR ", then " GOLD_ICN_STR ".");

        int player_move_x = 0;
        int player_move_y = 0;
        if (game_running) {
            player_last_bullet_shot_timer++;
            if (key_is_down(KB_KEY_RIGHT)) {
                player_move_x = 1;
                spaceship_x += 1;
                spaceship_x = MIN(spaceship_x, FRAME_SX - 6);
            }
            if (key_is_down(KB_KEY_LEFT)) {
                player_move_x = -1;
                spaceship_x -= 1;
                spaceship_x = MAX(spaceship_x, 6);
            }
            if (key_is_down(KB_KEY_DOWN)) {
                player_move_y = 1;
                spaceship_y += 1;
                spaceship_y = MIN(spaceship_y, FRAME_SY - 3);
            }
            if (key_is_down(KB_KEY_UP)) {
                player_move_y = -1;
                spaceship_y -= 1;
                spaceship_y = MAX(spaceship_y, 3);
            }
            if (key_was_just_pressed(KB_KEY_SPACE)) {
                spawn_bullet(spaceship_x, spaceship_y - 5);
                player_last_bullet_shot_timer = 0;
                player_charge = 0;
                // spawn_bullet(spaceship_x+4, FRAME_SY - 16);
                // spawn_bullet(spaceship_x-4, FRAME_SY - 16);
            }
            if (key_is_down(KB_KEY_SPACE) && player_last_bullet_shot_timer > 20) {
                player_charge += 2;
            } else {
                if (player_charge > 0 && player_last_bullet_shot_timer > 3) {
                    player_last_bullet_shot_timer = 0;
                    spawn_bullet(spaceship_x, spaceship_y - 5);
                }
                player_charge -= 3;
            }
            player_charge = MIN(MAX_CHARGE, MAX(0, player_charge));


            UNUSED(player_move_y);
        }
        if (key_was_just_pressed(KB_KEY_ESCAPE)) {
            exit(0);
        }

        // Draw stars
        for (int i = 0; i < ARR_LEN(stars); i++) {
            star_t* s = &stars[i];
            int update_period = (i % 4) + 1;
            if (frame_count % update_period == 0) {
                s->y += 1;
                if (s->y >= FRAME_SY) {
                    s->y -= FRAME_SY;
                }
            }
            draw_box((rect_t) { .x = s->x, .y = s->y, .sx = 1, .sy = 1 }, 0x55FFFFFF);
        }

        // Draw player ship
        bitmap_t player_img = player_ship_imgs[player_move_x + 1];
        rect_t player_ship_rect = centered_rect(spaceship_x, spaceship_y, player_img.size_px_x, player_img.size_px_y);
        if (player_invicibility_frame > 0) {
            player_invicibility_frame -= 1;
        }
        if (player_health > 0 && ((player_invicibility_frame / 4) & 1) == 0) {
            draw_bitmap(player_ship_rect.x, player_ship_rect.y, player_img);

            // Draw charge
            int charge_size = player_charge * 6 / MAX_CHARGE;
            draw_box(((rect_t) { spaceship_x - 1 + player_move_x, spaceship_y + 4 - charge_size, 2, charge_size }), 0xFF24B6CF);
        }
 
        // Draw bullets
        for (int i = 0; i < bullet_count; i++) {
            bullet_t* bullet = &bullets[i];
            if (game_running) {
                bullet->y -= 3;
            }
            draw_box(centered_rect(bullet->x, bullet->y, 2, 5), 0xFF24B6CF);
            if (bullet->y < 0) {
                bullets[i] = bullets[bullet_count-1];
                bullet_count--;
                i--;
            }
        }


        // Update + draw enemies
        for (int i = 0; i < enemies_count; i++) {
            enemy_t* enemy = &enemies[i];
            bool attacking = enemy->y >= enemy->threshold_y;
            
            if (game_running && enemy->health > 0) {
                if (!attacking) {
                    // normal progression
                    enemy->base_y += frame_count % 10 == 0;
                    enemy->y = enemy->base_y + sinf(frame_count / 30.f) * 6;
                    enemy->x = enemy->base_x + cosf(frame_count / 60.f) * 10;
                }
                else {
                    // attack
                    if ((frame_count & 0x1) == 0) {
                        move_towards(&enemy->x, &enemy->y, spaceship_x, spaceship_y - 4, 1.5f);
                    }
                }
            }

            rect_t rect = centered_rect(enemy->x, enemy->y, 8, 8);

            if (game_running && enemy->health > 0) {
                // check collisions with bullets
                int col_count = check_bullet_col(rect);
                if (col_count) {
                    enemies[i].hit_anim = 10;
                    enemies[i].health -= col_count;

                    audio_play_sound(440, false);
                }

                // check collision with player
                if (enemies[i].health > 0 && check_aabb_col(rect, get_player_rect())) {
                    if (player_invicibility_frame == 0) {
                        spawn_explosion(spaceship_x, spaceship_y);
                        player_health -= 1;
                        player_invicibility_frame = 100;
                    }
                    enemies[i].health = 0; // set enemy's health and hit anim to 0 so it's destroyed immediately
                    enemies[i].hit_anim = 0;
                }
            }

            // draw
            enemy->hit_anim = MAX(0, enemy->hit_anim - 1);
            uint32_t highlight_col = (enemy->hit_anim / 2) & 1 ? 0x00FFFFFF : 0;
            draw_bitmap(rect.x, rect.y, (attacking ? invader_mad : invader)[(frame_count >> (attacking ? 3 : 4)) & 3], highlight_col);

            // destroy
            if (enemy->health <= 0 && enemy->hit_anim <= 0) {
                score++;
                spawn_explosion(enemy->x, enemy->y);
                enemies[i] = enemies[enemies_count-1];
                enemies_count--;
                i--;
                continue;
            }
        }

        // draw explosions
        for (int i = 0; i < explosions_count; i++) {
            explosion_t* expl = &explosions[i];
            bitmap_t img = expl_anim[expl->frame/10];
            draw_bitmap(expl->x - img.size_px_x/2, expl->y - img.size_px_y/2, img, 0);
            expl->frame += 1;
            if (expl->frame == ARR_LEN(expl_anim) * 10) {
                explosions[i] = explosions[explosions_count-1];
                explosions_count--;
                i--;
            }
        }

        // check game over
        if (game_running && player_health <= 0) {
            game_over_timer = 300;
            game_running = false;
        }

        // draw lives
        char health_text[10];
        snprintf(health_text, sizeof(health_text), "%i", player_health);
        draw_text(2, FRAME_SY - 10, health_text, 0xA0FFFFFF);

        char score_text[10];
        snprintf(score_text, sizeof(score_text), "%i", score);
        draw_text(FRAME_SX - strlen(score_text) * 6, FRAME_SY - 10, score_text, 0xA0FFFFFF);

        // press space to begin
        if (!game_running) {
            if (!game_over_timer) {
                // press space to begin
                if ((frame_count >> 5) & 1) {
                    draw_text(10, 56, "Press space to begin", 0);
                    draw_text(10, 55, "Press space to begin");
                }
                if (key_was_just_pressed(KB_KEY_SPACE)) {
                    game_running = true;
                }
            }
            else {
                // game over message
                draw_text(38, 56, "Game over", 0);
                draw_text(38, 55, "Game over", COLOR_RED);

                game_over_timer--;
                if (game_over_timer == 1) {
                    reset_game();
                }
            }
        }

        tick_input();

        // feed the frame to miniFB
        resize_bitmap(window_buffer, WINDOW_SX, WINDOW_SY, buffer, FRAME_SX, FRAME_SY);
        int status = mfb_update(window, window_buffer);
        if (status < 0) {
            printf("exit! mfb status %i\n", status);
            exit(status);
        }

        mfb_wait_sync(window);
    }
}
