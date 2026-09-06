#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>

typedef enum {
    SceneTitle,
    SceneStarter,
    SceneWorld,
    SceneBattle,
    SceneResult,
} Scene;

typedef struct {
    Gui* gui;
    ViewPort* view;
    FuriMessageQueue* queue;
    bool running;

    Scene scene;
    uint8_t starter;
    uint8_t x;
    uint8_t y;
    uint8_t hp;
    uint8_t enemy_hp;
    uint8_t enemy;
    uint32_t rng;
    char msg[28];
} Game;

static const char* starters[] = {"BULBASAUR","CHARMANDER","SQUIRTLE"};
static const char* wilds[] = {"PIDGEY","RATTATA","CATERPIE","WEEDLE","PIKACHU","ODDISH"};

static uint32_t rnd(Game* g) {
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

static bool grass(uint8_t x, uint8_t y) {
    return (y >= 3 && y <= 6 && x >= 2 && x <= 11);
}

static bool blocked(uint8_t x, uint8_t y) {
    if(x >= 14 || y >= 8) return true;
    if(x == 0 || y == 0) return true;
    if((x == 12 || x == 13) && y < 7) return true;
    return false;
}

static void mon_enemy(Canvas* c, int x, int y, uint8_t kind) {
    // 1-bit stylized silhouettes
    if(kind == 0) {
        canvas_draw_circle(c, x+12, y+9, 7);
        canvas_draw_box(c, x+4, y+11, 16, 8);
        canvas_draw_box(c, x+7, y+18, 3, 4);
        canvas_draw_box(c, x+15, y+18, 3, 4);
        canvas_draw_line(c, x+17, y+6, x+22, y+2);
    } else if(kind == 1) {
        canvas_draw_box(c, x+5, y+6, 18, 12);
        canvas_draw_box(c, x+9, y+2, 10, 6);
        canvas_draw_line(c, x+6, y+5, x+2, y+1);
        canvas_draw_line(c, x+22, y+5, x+26, y+1);
        canvas_draw_box(c, x+8, y+18, 3, 5);
        canvas_draw_box(c, x+18, y+18, 3, 5);
    } else {
        canvas_draw_circle(c, x+13, y+10, 8);
        canvas_draw_box(c, x+7, y+15, 12, 7);
        canvas_draw_line(c, x+8, y+4, x+3, y);
        canvas_draw_line(c, x+18, y+4, x+23, y);
    }
    canvas_draw_dot(c, x+10, y+9);
    canvas_draw_dot(c, x+16, y+9);
}

static void mon_back(Canvas* c, int x, int y, uint8_t starter) {
    if(starter == 0) {
        canvas_draw_circle(c, x+11, y+8, 7);
        canvas_draw_box(c, x+4, y+12, 16, 10);
        canvas_draw_circle(c, x+20, y+6, 5);
    } else if(starter == 1) {
        canvas_draw_circle(c, x+11, y+9, 7);
        canvas_draw_box(c, x+5, y+13, 14, 9);
        canvas_draw_line(c, x+7, y+4, x+4, y);
        canvas_draw_line(c, x+15, y+3, x+18, y);
        canvas_draw_line(c, x+18, y+17, x+24, y+13);
    } else {
        canvas_draw_circle(c, x+11, y+9, 8);
        canvas_draw_box(c, x+5, y+14, 14, 8);
        canvas_draw_circle(c, x+11, y+17, 5);
    }
}

static void hp_bar(Canvas* c, int x, int y, uint8_t hp, uint8_t maxhp) {
    canvas_draw_frame(c, x, y, 43, 6);
    uint8_t w = maxhp ? (hp * 41 / maxhp) : 0;
    if(w) canvas_draw_box(c, x+1, y+1, w, 4);
}

static void draw_title(Canvas* c) {
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 18, 15, "POKEMON PURPLE");
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 34, 29, "DX DEMO");

    // simple pokeball logo
    canvas_draw_circle(c, 64, 44, 9);
    canvas_draw_line(c, 55, 44, 73, 44);
    canvas_draw_circle(c, 64, 44, 3);

    canvas_draw_str(c, 23, 62, "OK START  HOLD BACK EXIT");
}

static void draw_starter(Canvas* c, Game* g) {
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 23, 11, "CHOOSE PARTNER");

    canvas_set_font(c, FontSecondary);
    for(uint8_t i=0;i<3;i++) {
        int y = 27 + i*11;
        if(i == g->starter) {
            canvas_draw_box(c, 6, y-8, 116, 10);
            canvas_set_color(c, ColorWhite);
            canvas_draw_str(c, 11, y, starters[i]);
            canvas_set_color(c, ColorBlack);
        } else {
            canvas_draw_str(c, 11, y, starters[i]);
        }
    }
}

static void tile_tree(Canvas* c, int x, int y) {
    canvas_draw_box(c, x+2, y+1, 4, 4);
    canvas_draw_box(c, x+1, y+3, 6, 3);
    canvas_draw_line(c, x+4, y+6, x+4, y+7);
}

static void tile_grass(Canvas* c, int x, int y) {
    canvas_draw_line(c, x+1, y+6, x+3, y+3);
    canvas_draw_line(c, x+3, y+6, x+5, y+2);
    canvas_draw_line(c, x+5, y+6, x+7, y+4);
}

static void player_sprite(Canvas* c, int x, int y) {
    canvas_draw_circle(c, x+4, y+3, 2);
    canvas_draw_box(c, x+2, y+5, 5, 3);
    canvas_draw_dot(c, x+2, y+7);
    canvas_draw_dot(c, x+6, y+7);
}

static void draw_world(Canvas* c, Game* g) {
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 2, 8, "ROUTE 1");
    canvas_draw_str(c, 84, 8, "OK PARTY");

    const int ox = 3, oy = 11, tw = 8, th = 7;

    for(uint8_t y=0;y<8;y++) {
        for(uint8_t x=0;x<14;x++) {
            int px = ox + x*tw;
            int py = oy + y*th;

            if(blocked(x,y)) {
                tile_tree(c, px, py);
            } else if(grass(x,y)) {
                tile_grass(c, px, py);
            } else if((x==1 && y==1) || (x==2 && y==1)) {
                canvas_draw_frame(c, px, py, 8, 7);
                canvas_draw_str(c, px+2, py+6, "H");
            }
        }
    }

    int px = ox + g->x*tw;
    int py = oy + g->y*th;
    player_sprite(c, px, py);
}

static void draw_battle(Canvas* c, Game* g) {
    canvas_set_font(c, FontSecondary);

    canvas_draw_str(c, 2, 8, wilds[g->enemy]);
    hp_bar(c, 2, 11, g->enemy_hp, 18);

    mon_enemy(c, 86, 3, g->enemy % 3);
    mon_back(c, 8, 31, g->starter);

    canvas_draw_str(c, 61, 35, starters[g->starter]);
    hp_bar(c, 61, 38, g->hp, 20);

    // command box
    canvas_draw_frame(c, 57, 48, 70, 15);
    canvas_draw_str(c, 62, 58, "OK ATTACK");
    canvas_draw_str(c, 94, 58, "BACK RUN");
}

static void draw_result(Canvas* c, Game* g) {
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 29, 19, "BATTLE OVER");
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 30, 37, g->msg);
    canvas_draw_str(c, 29, 57, "OK RETURN");
}

static void draw_cb(Canvas* c, void* ctx) {
    Game* g = ctx;
    canvas_clear(c);
    switch(g->scene) {
        case SceneTitle: draw_title(c); break;
        case SceneStarter: draw_starter(c,g); break;
        case SceneWorld: draw_world(c,g); break;
        case SceneBattle: draw_battle(c,g); break;
        case SceneResult: draw_result(c,g); break;
    }
}

static void input_cb(InputEvent* ev, void* ctx) {
    Game* g = ctx;
    furi_message_queue_put(g->queue, ev, 0);
}

static void maybe_battle(Game* g) {
    if(!grass(g->x,g->y)) return;
    if((rnd(g)%100) < 22) {
        g->enemy = rnd(g) % 6;
        g->enemy_hp = 18;
        g->scene = SceneBattle;
    }
}

static void world_move(Game* g, InputKey key) {
    int nx = g->x;
    int ny = g->y;
    if(key == InputKeyUp) ny--;
    else if(key == InputKeyDown) ny++;
    else if(key == InputKeyLeft) nx--;
    else if(key == InputKeyRight) nx++;
    else return;

    if(nx < 0 || ny < 0 || nx >= 14 || ny >= 8) return;
    if(blocked(nx,ny)) return;

    g->x = (uint8_t)nx;
    g->y = (uint8_t)ny;
    maybe_battle(g);
}

static void attack(Game* g) {
    uint8_t dmg = 4 + (rnd(g)%4);
    if(dmg >= g->enemy_hp) g->enemy_hp = 0;
    else g->enemy_hp -= dmg;

    if(g->enemy_hp == 0) {
        snprintf(g->msg,sizeof(g->msg),"YOU WON!");
        g->scene = SceneResult;
        return;
    }

    uint8_t edmg = 2 + (rnd(g)%3);
    if(edmg >= g->hp) g->hp = 0;
    else g->hp -= edmg;

    if(g->hp == 0) {
        snprintf(g->msg,sizeof(g->msg),"YOU FAINTED!");
        g->hp = 20;
        g->x = 2;
        g->y = 2;
        g->scene = SceneResult;
    }
}

int32_t pokemon_purple_dx_app(void* p) {
    UNUSED(p);

    Game* g = malloc(sizeof(Game));
    memset(g,0,sizeof(Game));
    g->running = true;
    g->scene = SceneTitle;
    g->starter = 0;
    g->x = 2;
    g->y = 2;
    g->hp = 20;
    g->rng = 0xB16B00B5u ^ furi_get_tick();

    g->queue = furi_message_queue_alloc(8,sizeof(InputEvent));
    g->gui = furi_record_open(RECORD_GUI);
    g->view = view_port_alloc();

    view_port_draw_callback_set(g->view,draw_cb,g);
    view_port_input_callback_set(g->view,input_cb,g);
    gui_add_view_port(g->gui,g->view,GuiLayerFullscreen);

    InputEvent ev;
    while(g->running) {
        if(furi_message_queue_get(g->queue,&ev,100)==FuriStatusOk) {
            if(ev.type == InputTypeLong && ev.key == InputKeyBack) {
                g->running = false;
                break;
            }

            if(ev.type != InputTypeShort) continue;

            switch(g->scene) {
                case SceneTitle:
                    if(ev.key == InputKeyOk) g->scene = SceneStarter;
                    break;

                case SceneStarter:
                    if(ev.key == InputKeyUp) g->starter = (g->starter+2)%3;
                    else if(ev.key == InputKeyDown) g->starter = (g->starter+1)%3;
                    else if(ev.key == InputKeyBack) g->scene = SceneTitle;
                    else if(ev.key == InputKeyOk) g->scene = SceneWorld;
                    break;

                case SceneWorld:
                    if(ev.key == InputKeyBack) g->scene = SceneTitle;
                    else world_move(g, ev.key);
                    break;

                case SceneBattle:
                    if(ev.key == InputKeyOk) attack(g);
                    else if(ev.key == InputKeyBack) {
                        snprintf(g->msg,sizeof(g->msg),"GOT AWAY!");
                        g->scene = SceneResult;
                    }
                    break;

                case SceneResult:
                    if(ev.key == InputKeyOk || ev.key == InputKeyBack) g->scene = SceneWorld;
                    break;
            }

            view_port_update(g->view);
        }
    }

    gui_remove_view_port(g->gui,g->view);
    view_port_free(g->view);
    furi_message_queue_free(g->queue);
    furi_record_close(RECORD_GUI);
    free(g);
    return 0;
}
