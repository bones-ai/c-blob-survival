#ifndef GAME_H
#define GAME_H

// MARK: :flags

#define IS_MOBILE false
#define DEBUG true
#define GOD false

// MARK: :configs

#define VERSION "v0.0.1"
#if IS_MOBILE
    #define VIRT_WIDTH 144
    #define VIRT_HEIGHT 320
    #define SCREEN_WIDTH VIRT_WIDTH * 3
    #define SCREEN_HEIGHT VIRT_HEIGHT * 3
#else
    #define VIRT_WIDTH 320
    #define VIRT_HEIGHT 180
    #define SCREEN_WIDTH VIRT_WIDTH * 4
    #define SCREEN_HEIGHT VIRT_HEIGHT * 4
#endif
#define FPS 144

#define TILE_SIZE 16
#define DEFAULT_SPRITE_SCALE 1
#define SPRITE_SHEET_PATH "assets/proj.png"
#define CUSTOM_FONT_PATH "assets/alagard.ttf"
#define SOUND_BG_1 "assets/sounds/fright_bg.ogg"
#define SOUND_BULLET_FIRE "assets/sounds/bullet_fire.ogg"
#define SOUND_SPLINTER "assets/sounds/splinter.wav"
#define SOUND_FROST_WAVE "assets/sounds/frost_wave.mp3"
#define SOUND_ORB "assets/sounds/orb.wav"
#define SOUND_HURT "assets/sounds/hurt.wav"
#define SOUND_HEALTH_PICKUP "assets/sounds/health_pickup.wav"
#define SOUND_UPGRADE "assets/sounds/upgrade.wav"
#define SOUND_FLAME_THROWER "assets/sounds/flamethrower.ogg"
#define SOUND_HEARTBEAT "assets/sounds/heartbeat.wav"

#define WORLD_W 32
#define WORLD_H 24
#define NUM_DECORATIONS 25
#define TOTAL_NUM_DECORATIONS 14

#define MAX_PICKUPS 50000
#define PICKUPS_CLEANUP_INTERVAL 1000 * 5
#define PICKUPS_LIFETIME 1000 * 60 * 1

#define PARTICLE_SPEED 200
#define PARTICLE_LIFETIME 400
#define MAX_MANA_PARTICLES 1000
#define MAX_FLAME_PARTICLES 1000
#define MAX_FROST_WAVE_PARTICLES 1000

#define MAX_ENEMIES 50000
#define MAX_MAIN_MENU_ENEMIES 200
#define ENEMY_SPEED 30
#define WAVE_DURATION_MILLIS 1000 * 15
#define WAVE_DURATION_INCREMENT_MILLIS 1000 * 3
#define QTREE_UPDATE_INTERVAL_MILLIS 100
#define DEFAULT_ENEMY_SPAWN_INTERVAL_MS 1000

#define MAX_BULLETS 10000
#define MAX_ENEMY_BULLETS 5000
#define GUN_VISION 70
#define SPIKE_RADIUS 30

#define POINTS_PER_QUAD 10

#define TOAST_LIEFTIME_MS 1500
#define MAX_NUM_TOASTS 15

#define MAX_PLAYER_HEALTH 500.0f

#define SERVER_URL ""

// MARK: :colors

#define COLOR_DARK_BLUE (Color) { 16, 20, 31, 255 }
#define COLOR_BROWN (Color) { 123, 105, 96, 255 }
#define COLOR_WHITE (Color) { 255, 248, 237, 255 }
#define COLOR_RED (Color) { 255, 41, 97, 255 }
#define COLOR_SKY_BLUE (Color) { 81, 202, 243, 255 }
#define COLOR_CEMENT (Color) { 53, 52, 65, 255 }
#define COLOR_LIGHT_PINK (Color) { 243, 216, 216, 255 }

#define COLOR_FLASH_HURT (Color) { 212, 129, 129, 255 }
#define COLOR_FLASH_FROST COLOR_SKY_BLUE

// MARK: :alias

typedef Vector2 Vec2;
typedef Rectangle Rect;

// MARK: :enums

typedef enum {
    MAIN_MENU,
    UPGRADE_MENU,
    IN_GAME,
    PAUSE_MENU,
    DEATH
} GameScreen;

typedef enum {
    BAT,
    RAM,
    MAGE,
    REAPER,
    DEMON,
    DEMON_PUP,
} EnemyType;

typedef enum {
    // One shot bullets
    BULLET,
    SPLINTER,

    // Revolvers
    SPIKE,

    // Particle effects
    FLAME,
    FROST_WAVE,
    FIRE_WAVE,

    // Area
    ORBS,
    METEOR,
    GARLIC,

    // Enemy Attacks
    MAGE_BULLET,
    DEMON_BULLET,
} AttackType;

// :shop
typedef enum {
    BULLET_INTERVAL,
    BULLET_COUNT,
    BULLET_SPREAD,
    BULLET_RANGE,
    BULLET_DAMAGE,
    BULLET_PENETRATION,

    SPLINTER_INTERVAL,
    SPLINTER_RANGE,
    SPLINTER_COUNT,
    SPLINTER_DAMAGE,
    SPLINTER_PENETRATION,

    SPIKE_INTERVAL,
    SPIKE_SPEED,
    SPIKE_COUNT,
    SPIKE_DAMAGE,

    FLAME_INTERVAL,
    FLAME_LIFETIME,
    FLAME_SPREAD,
    FLAME_DISTANCE,
    FLAME_DAMAGE,

    FROST_WAVE_INTERVAL,
    FROST_WAVE_LIFETIME,
    FROST_WAVE_DAMAGE,

    ORBS_INTERVAL,
    ORBS_DAMAGE,
    ORBS_COUNT,
    ORBS_SIZE,

    PLAYER_SPEED,
    SHINY_CHANCE
} ShopUpgradeType;

typedef enum {
    MANA,
    MANA_SHINY,
    HEALTH,
    MAGNET
} PickupType;

typedef enum {
    /**
     * When adding a new upgrade,
     * - add the enum value here
     * - add the new value to the draw switch statements
     * - actually call the draw func when showing options
     * - add the new options to the randomization list
     * - ensure the stats are initialized
     */
    BULLET_UPGRADE,
    SPLINTER_UPGRADE,
    SPIKE_UPGRADE,
    FLAME_UPGRADE,
    FROST_UPGRADE,
    ORBS_UPGRADE,
    SPEED_UPGRADE,
    SHINY_UPGRADE,
} UpgradeType;

typedef enum {
    GAME_OPEN,
    GAME_START,
    GAME_OVER,
} AnalyticsType;

// MARK: :structs

typedef struct {
    float x, y, w, h;
} QRect;

typedef struct {
    float x, y;
    int id;
} QPoint;

typedef struct QTree_ {
    bool is_divided;
    QRect boundary;
    QPoint *points;
    int num_points;

    struct QTree_ *tl;
    struct QTree_ *tr;
    struct QTree_ *bl;
    struct QTree_ *br;
} QTree;

// :bullet
typedef struct {
    Vec2 pos;
    Vec2 direction;
    float strength;
    int penetration;
    int spawnTs;
    int speed;
    AttackType type;

    // For revolving type bullets
    float angle;
} Bullet;

typedef struct {
    EnemyType type;
    Vec2 pos;
    float health;
    float damage;
    float speed;
    int spawn_ts;
    bool is_shiny;

    // Flash
    bool is_frozen;
    int frozen_ts;
    bool is_taking_damage;
    int damage_ts;

    // Enemy type specific
    bool is_player_found;
    int player_found_ts;
    // Used by RAM
    Vec2 charge_dir;
    // Used by DEMON
    int last_spawn_ts;
} Enemy;

typedef struct {
    Vec2 pos;
    int decoration_idx;
} Decoration;

typedef struct {
    Vec2 pos;
    PickupType type;
    int spawn_ts;
    bool is_collected;
} Pickup;

typedef struct {
    Vec2 pos;
    Vec2 dir;
    int spawn_ts;
    int lifetime;
} Particle;

typedef struct {
    const char *label;
    Vec2 pos;
    Vec2 size;
    Color bg;
    Color fg;
    Color shadow;
    float font_size;
} Button;

// :timers
typedef struct {
    /**
     * When adding new timers,
     * remember to update the timer state to handle 
     * pause/resume properly
     */

    // General
    int game_start_ts;
    int pause_ts;
    int pickups_cleanup_ts;
    int last_hurt_sound_ts;
    int last_heart_spawn_ts;

    // Weapon
    int bullet_ts;
    int splinter_ts;
    int spike_ts;
    int flame_ts;
    int frost_wave_ts;
    int orbs_ts;

    // Enemy
    int qtree_update_ts;
    int enemy_spawn_ts;
    int enemy_spawn_interval;

} Timers;

// :stat
typedef struct {
    /**
     * Steps to add a new shop item:
     * - Add a new enum field
     * - Updated all :stat functions with the new enum
     * - Add a new stat var
     * - Init the newly added stat var
     * - Add item to :shop items list
     * - Add a new clause to the :shop switch
     */
    float bullet_interval;
    float bullet_count;
    float bullet_spread;
    float bullet_range;
    float bullet_damage;
    float bullet_penetration;

    float splinter_interval;
    float splinter_range;
    float splinter_count;
    float splinter_damage;
    float splinter_penetration;

    float spike_interval;
    float spike_speed;
    float spike_count;
    float spike_damage;

    float flame_interval;
    float flame_lifetime;
    float flame_spread;
    float flame_distance;
    float flame_damage;

    float frost_wave_interval;
    float frost_wave_lifetime;
    float frost_wave_damage;

    float orbs_interval;
    float orbs_damage;
    float orbs_count;
    float orbs_size;

    float player_speed;
    float shiny_chance;
} Stats;

typedef struct {
    int bullet_level;
    int splinter_level;
    int spike_level;
    int flame_level;
    int frost_wave_level;
    int orbs_level;
    int speed_level;
    int shiny_level;
} Upgrades;

typedef struct {
    const char *message;
    int spawn_ts;
} Toast;

typedef struct {
    int ct;
    int num_enemies_drawn;
} Temp;

// MARK: :gamestate

typedef struct {
    // Raylib
    Camera2D camera;
    Texture2D sprite_sheet;
    Font custom_font;
    RenderTexture2D render_texture;
    Rect source_rect;
    Rect dest_rect;

    // Music, Sound
    Music sound_bg;
    Music sound_heartbeat;
    Sound sound_upgrade;
    Sound sound_bullet_fire;
    Sound sound_splinter;
    Sound sound_flamethrower;
    Sound sound_frost_wave;
    Sound sound_orb;
    Sound sound_hurt;
    Sound sound_health_pickup;
    float sound_intensity;

    // General
    GameScreen screen;
    Timers timer;
    Stats stats;
    Upgrades upgrades;
    Upgrades available_upgrades;
    Toast *toasts;
    int toasts_count;
    int wave_index;
    int player_level;
    long kill_count;
    bool is_fullscreen;

    // Player
    Vec2 player_pos;
    float player_health;
    Vec2 player_heading_dir;
    bool is_joystick_enabled;
    Vec2 touch_down_pos;
    Vec2 joystick_direction;
    int mana_count;
    Vec2 *mana_particles;
    int mana_particles_count;
    int death_ts;

    // Weapon
    Bullet *bullets;
    int bullet_count;
    Bullet *enemy_bullets;
    int enemy_bullet_count;
    Particle *flame_particles;
    int flame_particles_count;
    Particle *frost_wave_particles;
    int frost_wave_particles_count;

    // Enemy
    Enemy *enemies;
    int enemy_count;
    QTree *enemy_qtree;
    int num_enemies_per_tick;

    // World
    Rect world_dims;
    Decoration *decorations;
    Pickup *pickups;
    int pickups_count;
    QTree *pickups_qtree;

    // UI
    float master_volume;
    bool is_master_volume_dragging;
    Enemy *main_menu_enemies;
    int main_menu_enemies_count;

    // Others
    QPoint *query_points;
    int num_query_points;

    // Debug
    bool is_show_debug_gui;
    Temp temp;

} GameState;

// MARK: :func

void gamestate_create();
void gamestate_destroy();

void pause_timers();
void resume_timers();

// :draw :render
void draw_main_menu();
void draw_game();
void draw_ui();
void draw_upgrade_menu();
void draw_upgrade_option(UpgradeType type, int level, Vec2 *pos);
void draw_player();
void draw_enemies();
void draw_particles();
void draw_bullets();
void draw_decorations();
void draw_pickups();
void draw_toasts();
void draw_health_bar(Vec2 pos, float health, float max_health);
void draw_sprite(Texture2D *sprite_sheet, Vec2 tile, Vec2 pos, float scale, float rotation, Color tint);
void draw_spritev(Texture2D *sprite_sheet, Vec2 tile, Vec2 pos, float scale, float rotation, bool is_flip_x, bool is_bob, Color tint, int bob_id);

// :update
void update_game();
void update_player();
void update_enemies();
void update_particles();
void update_bullets();
void update_pickups();
void update_toasts();

// :sound :music
void sounds_init();
void sounds_destroy();
void sounds_update();
void play_sound_modulated(Sound *sound, float volume);

// :decorations
void decorations_init();
void update_decorations();

// :input
void handle_player_input();
void handle_debug_inputs();
void handle_extra_inputs();
void handle_window_resize();
void handle_virtual_joystick_input();

// :quadtree :qtree
QTree* qtree_create(QRect boundary);
void qtree_destroy(QTree *qtree);
void qtree_clear(QTree *qtree);
void qtree_reset_boundary(QTree *qtree, QRect rect);
bool qtree_insert(QTree *qtree, QPoint pt);
bool qtree_remove(QTree *qtree, QPoint pt);
void qtree_query(QTree *qtree, QRect range, QPoint *result, int *num_points);
void _qtree_subdivide(QTree *qtree);

// :data
Vec2 get_enemy_sprite_pos(EnemyType type, bool is_shiny);
float get_enemy_scale(EnemyType type);
float get_enemy_health(EnemyType type, bool is_shiny);
float get_enemy_speed(EnemyType type);
float get_enemy_damage(EnemyType type);
Vec2 get_attack_sprite(AttackType type);
int get_attack_range(AttackType type);
int get_attack_speed(AttackType type);
float get_base_stat_value(ShopUpgradeType type);
float get_stat_increment(ShopUpgradeType type);

// :impl
EnemyType get_next_enemy_spawn_type();
int get_num_enemies_per_tick();
int get_mana_value();
PickupType get_pickup_spawn_type(bool is_shiny);
void update_available_upgrades();
void upgrade_stat(UpgradeType type);
int get_level_mana_threshold(int level);

// :ui
bool main_menu_button(Button btn);
void reset_main_menu_enemies();
void update_main_menu_enemies();
bool upgrade_menu_button(Button btn);
bool button(Button btn);
void slider(float ypos, const char *label, float *slider_offset, bool *is_drag);
void toast(const char *message);
void draw_text_with_shadow(const char *text, Vec2 pos, int size, Color fg, Color bg);

// :utils
int get_window_width();
int get_window_height();
QRect get_visible_rect(Vec2 center, float zoom);
int get_current_time_millis();
bool is_vec2_zero(Vec2 vec);
Vec2 rotate_vector(Vec2 vec, float angle);
Vec2 get_line_center(Vec2 a, Vec2 b);
Vec2 get_rand_unit_vec2();
bool is_rect_contains_point(QRect rect, QPoint pt);
bool is_rect_overlap(QRect first, QRect second);
bool are_colors_equal(Color a, Color b);
Vec2 get_rand_pos_around_point(Vec2 pt, float minDist, float maxDist);
Vec2 point_at_dist(Vec2 pt, Vec2 dir, float dist);
bool is_point_in_triangle(Vec2 pt, Vec2 a, Vec2 b, Vec2 c);
float vec2_to_angle(Vec2 dir);
void print(const char *text);
void printe(const char *text);

// :ana
void* post_analytics_thread(void* arg);
void post_analytics_async(AnalyticsType type);

// :main
void game_update();

#endif
