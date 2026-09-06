#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>
#include "pokemon.h"
#include "maps.h"
#include "sprites.h"

#define SCREEN_W 128
#define SCREEN_H 64

typedef enum {
    SCENE_TITLE,
    SCENE_STARTER,
    SCENE_EXPLORE,
    SCENE_BATTLE,
} Scene;

typedef enum {
    BATTLE_INTRO,
    BATTLE_ACTION,
    BATTLE_MOVE,
    BATTLE_RESULT,
} BattleState;

typedef struct {
    FuriMessageQueue* queue;
    Gui* gui;
    ViewPort* viewport;
    bool running;

    Scene scene;
    BattleState battle_state;

    GameMap route1;
    GameMap pallet;
    GameMap* map;

    int trainer_x;
    int trainer_y;
    int dir;

    int starter_cursor;
    int action_cursor;
    int move_cursor;

    Pokemon player;
    Pokemon wild;
    char battle_text[48];
} Game;

static int clamp_i(int v, int lo, int hi) {
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static void draw_hp(Canvas* c, int x, int y, int hp, int max_hp) {
    if(max_hp < 1) max_hp = 1;
    int fill = (hp * 38) / max_hp;
    fill = clamp_i(fill, 0, 38);
    canvas_draw_frame(c, x, y, 40, 6);
    if(fill) canvas_draw_box(c, x+1, y+1, fill, 4);
}

static void draw_title(Canvas* c) {
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 17, 16, "POKEMON PURPLE");
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 31, 31, "FLIPPER EDITION");
    canvas_draw_circle(c, 64, 45, 8);
    canvas_draw_line(c, 56,45,72,45);
    canvas_draw_circle(c,64,45,2);
    canvas_draw_str(c, 19, 63, "OK START  HOLD BACK EXIT");
}

static void draw_starter(Canvas* c, Game* g) {
    const char* options[3] = {"BULBASAUR","CHARMANDER","SQUIRTLE"};
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 20, 12, "CHOOSE STARTER");
    canvas_set_font(c, FontSecondary);
    for(int i=0;i<3;i++) {
        int y=29+i*11;
        if(i == g->starter_cursor) canvas_draw_str(c, 7, y, ">");
        canvas_draw_str(c, 16, y, options[i]);
    }
}

static void draw_explore(Canvas* c, Game* g) {
    int cam_x = g->trainer_x - SCREEN_W/2;
    int cam_y = g->trainer_y - SCREEN_H/2;
    cam_x = clamp_i(cam_x, 0, MAP_W*TILE_SIZE - SCREEN_W);
    cam_y = clamp_i(cam_y, 0, MAP_H*TILE_SIZE - SCREEN_H);

    int sx = cam_x / TILE_SIZE;
    int sy = cam_y / TILE_SIZE;
    int ex = (cam_x + SCREEN_W - 1) / TILE_SIZE;
    int ey = (cam_y + SCREEN_H - 1) / TILE_SIZE;

    for(int ty=sy; ty<=ey; ty++) {
        for(int tx=sx; tx<=ex; tx++) {
            Tile* t = &g->map->tiles[tx][ty];
            int px = tx*TILE_SIZE - cam_x;
            int py = ty*TILE_SIZE - cam_y;
            if(t->type == TILE_WALL) {
                canvas_draw_xbm(c, px, py, 16,16,tile_wall);
            } else if(t->type == TILE_GRASS) {
                canvas_draw_xbm(c, px, py, 16,16,tile_grass);
            }
        }
    }

    int dx = g->trainer_x - cam_x;
    int dy = g->trainer_y - cam_y;
    canvas_draw_xbm(c, dx,dy,16,16,(g->dir==1||g->dir==3)?trainer_side:trainer_front);

    canvas_set_font(c, FontSecondary);
    canvas_set_color(c, ColorWhite);
    canvas_draw_box(c, 0,0,128,9);
    canvas_set_color(c, ColorBlack);
    canvas_draw_str(c,2,8,g->map->name);
}

static void draw_battle(Canvas* c, Game* g) {
    canvas_set_font(c, FontSecondary);

    char line[32];
    snprintf(line,sizeof(line),"%s L%d",g->wild.name,g->wild.level);
    canvas_draw_str(c,3,8,line);
    draw_hp(c,3,11,g->wild.current_hp,g->wild.max_hp);
    canvas_draw_xbm(c,83,0,42,42,monster_front);

    canvas_draw_xbm(c,4,20,42,42,monster_back);
    snprintf(line,sizeof(line),"%s L%d",g->player.name,g->player.level);
    canvas_draw_str(c,58,35,line);
    draw_hp(c,58,38,g->player.current_hp,g->player.max_hp);

    canvas_set_color(c,ColorWhite);
    canvas_draw_box(c,0,45,128,19);
    canvas_set_color(c,ColorBlack);
    canvas_draw_frame(c,0,45,128,19);

    if(g->battle_state == BATTLE_INTRO || g->battle_state == BATTLE_RESULT) {
        canvas_draw_str(c,4,57,g->battle_text);
    } else if(g->battle_state == BATTLE_ACTION) {
        const char* o[4]={"FIGHT","PKMN","ITEM","RUN"};
        for(int i=0;i<4;i++) {
            int x=8+(i%2)*58, y=54+(i/2)*9;
            if(i==g->action_cursor) canvas_draw_str(c,x-6,y,">");
            canvas_draw_str(c,x,y,o[i]);
        }
    } else if(g->battle_state == BATTLE_MOVE) {
        for(int i=0;i<4;i++) {
            int x=7+(i%2)*61, y=54+(i/2)*9;
            if(i==g->move_cursor) canvas_draw_str(c,x-6,y,">");
            const char* n = g->player.moves[i].name ? g->player.moves[i].name : "";
            canvas_draw_str(c,x,y,n);
        }
    }
}

static void draw_cb(Canvas* c, void* ctx) {
    Game* g = ctx;
    canvas_clear(c);
    switch(g->scene) {
        case SCENE_TITLE: draw_title(c); break;
        case SCENE_STARTER: draw_starter(c,g); break;
        case SCENE_EXPLORE: draw_explore(c,g); break;
        case SCENE_BATTLE: draw_battle(c,g); break;
    }
}

static void input_cb(InputEvent* event, void* ctx) {
    Game* g = ctx;
    furi_message_queue_put(g->queue,event,0);
}

static void start_battle(Game* g, PokemonSpecies s, uint8_t level) {
    g->wild = pokemon_create(s,level);
    g->battle_state = BATTLE_INTRO;
    snprintf(g->battle_text,sizeof(g->battle_text),"Wild %s appeared!",g->wild.name);
    g->action_cursor=0;
    g->move_cursor=0;
    g->scene=SCENE_BATTLE;
}

static void maybe_encounter(Game* g) {
    int tx = g->trainer_x/TILE_SIZE;
    int ty = g->trainer_y/TILE_SIZE;
    Tile* t = &g->map->tiles[tx][ty];
    if(t->encounter_rate == 0) return;
    if((rand()%100) >= t->encounter_rate) return;

    PokemonSpecies s = t->encounters[rand()%3];
    uint8_t lvl = t->min_level;
    if(t->max_level > t->min_level) lvl += rand()%(t->max_level-t->min_level+1);
    start_battle(g,s,lvl);
}

static void move_explore(Game* g, InputKey k) {
    int nx=g->trainer_x, ny=g->trainer_y;
    if(k==InputKeyUp) {ny-=TILE_SIZE; g->dir=0;}
    else if(k==InputKeyRight) {nx+=TILE_SIZE; g->dir=1;}
    else if(k==InputKeyDown) {ny+=TILE_SIZE; g->dir=2;}
    else if(k==InputKeyLeft) {nx-=TILE_SIZE; g->dir=3;}
    else return;

    nx=clamp_i(nx,0,MAP_W*TILE_SIZE-TILE_SIZE);
    ny=clamp_i(ny,0,MAP_H*TILE_SIZE-TILE_SIZE);

    Tile* next=&g->map->tiles[nx/TILE_SIZE][ny/TILE_SIZE];
    if(next->obstacle) return;

    g->trainer_x=nx;
    g->trainer_y=ny;
    maybe_encounter(g);
}

static void use_move(Game* g) {
    Move* m=&g->player.moves[g->move_cursor];
    if(!m->name || m->name[0]=='\0') return;

    int dmg=pokemon_calculate_damage(&g->player,&g->wild,m);
    g->wild.current_hp-=dmg;
    if(g->wild.current_hp<0) g->wild.current_hp=0;

    if(g->wild.current_hp==0) {
        snprintf(g->battle_text,sizeof(g->battle_text),"Wild %s fainted!",g->wild.name);
        g->battle_state=BATTLE_RESULT;
        return;
    }

    int enemy_index=rand()%4;
    Move* em=&g->wild.moves[enemy_index];
    if(!em->name || em->name[0]=='\0') em=&g->wild.moves[0];

    int edmg=pokemon_calculate_damage(&g->wild,&g->player,em);
    g->player.current_hp-=edmg;
    if(g->player.current_hp<0) g->player.current_hp=0;

    if(g->player.current_hp==0) {
        snprintf(g->battle_text,sizeof(g->battle_text),"%s fainted!",g->player.name);
        g->battle_state=BATTLE_RESULT;
    } else {
        snprintf(g->battle_text,sizeof(g->battle_text),"%s did %d dmg",m->name,dmg);
        g->battle_state=BATTLE_RESULT;
    }
}

static void handle_short(Game* g, InputKey key) {
    if(g->scene==SCENE_TITLE) {
        if(key==InputKeyOk) g->scene=SCENE_STARTER;
        return;
    }

    if(g->scene==SCENE_STARTER) {
        if(key==InputKeyUp) g->starter_cursor=(g->starter_cursor+2)%3;
        else if(key==InputKeyDown) g->starter_cursor=(g->starter_cursor+1)%3;
        else if(key==InputKeyBack) g->scene=SCENE_TITLE;
        else if(key==InputKeyOk) {
            PokemonSpecies s = g->starter_cursor==0 ? POKEMON_BULBASAUR :
                               g->starter_cursor==1 ? POKEMON_CHARMANDER : POKEMON_SQUIRTLE;
            g->player=pokemon_create(s,5);
            g->scene=SCENE_EXPLORE;
        }
        return;
    }

    if(g->scene==SCENE_EXPLORE) {
        if(key==InputKeyBack) g->scene=SCENE_TITLE;
        else move_explore(g,key);
        return;
    }

    if(g->scene==SCENE_BATTLE) {
        if(g->battle_state==BATTLE_INTRO) {
            if(key==InputKeyOk) g->battle_state=BATTLE_ACTION;
        } else if(g->battle_state==BATTLE_ACTION) {
            if(key==InputKeyUp) g->action_cursor=(g->action_cursor+2)%4;
            else if(key==InputKeyDown) g->action_cursor=(g->action_cursor+2)%4;
            else if(key==InputKeyLeft) g->action_cursor=(g->action_cursor+3)%4;
            else if(key==InputKeyRight) g->action_cursor=(g->action_cursor+1)%4;
            else if(key==InputKeyBack) g->scene=SCENE_EXPLORE;
            else if(key==InputKeyOk) {
                if(g->action_cursor==0) g->battle_state=BATTLE_MOVE;
                else if(g->action_cursor==3) g->scene=SCENE_EXPLORE;
                else {
                    snprintf(g->battle_text,sizeof(g->battle_text),"Not implemented yet");
                    g->battle_state=BATTLE_RESULT;
                }
            }
        } else if(g->battle_state==BATTLE_MOVE) {
            if(key==InputKeyUp) g->move_cursor=(g->move_cursor+2)%4;
            else if(key==InputKeyDown) g->move_cursor=(g->move_cursor+2)%4;
            else if(key==InputKeyLeft) g->move_cursor=(g->move_cursor+3)%4;
            else if(key==InputKeyRight) g->move_cursor=(g->move_cursor+1)%4;
            else if(key==InputKeyBack) g->battle_state=BATTLE_ACTION;
            else if(key==InputKeyOk) use_move(g);
        } else if(g->battle_state==BATTLE_RESULT) {
            if(key==InputKeyOk) {
                if(g->wild.current_hp<=0) {
                    g->scene=SCENE_EXPLORE;
                } else if(g->player.current_hp<=0) {
                    g->player.current_hp=g->player.max_hp;
                    g->trainer_x=2*TILE_SIZE;
                    g->trainer_y=2*TILE_SIZE;
                    g->scene=SCENE_EXPLORE;
                } else {
                    g->battle_state=BATTLE_ACTION;
                }
            }
        }
    }
}

int32_t pokemon_purple_app(void* p) {
    UNUSED(p);

    Game* g=malloc(sizeof(Game));
    memset(g,0,sizeof(Game));
    g->running=true;
    g->scene=SCENE_TITLE;
    g->map=&g->route1;
    g->trainer_x=9*TILE_SIZE;
    g->trainer_y=15*TILE_SIZE;
    g->player=pokemon_create(POKEMON_BULBASAUR,5);

    maps_init(&g->route1,&g->pallet);

    g->queue=furi_message_queue_alloc(8,sizeof(InputEvent));
    g->gui=furi_record_open(RECORD_GUI);
    g->viewport=view_port_alloc();
    view_port_draw_callback_set(g->viewport,draw_cb,g);
    view_port_input_callback_set(g->viewport,input_cb,g);
    gui_add_view_port(g->gui,g->viewport,GuiLayerFullscreen);

    InputEvent ev;
    while(g->running) {
        if(furi_message_queue_get(g->queue,&ev,100)==FuriStatusOk) {
            if(ev.type==InputTypeLong && ev.key==InputKeyBack) {
                g->running=false;
            } else if(ev.type==InputTypeShort || ev.type==InputTypePress) {
                handle_short(g,ev.key);
            }
            view_port_update(g->viewport);
        }
    }

    gui_remove_view_port(g->gui,g->viewport);
    view_port_free(g->viewport);
    furi_message_queue_free(g->queue);
    furi_record_close(RECORD_GUI);
    free(g);
    return 0;
}
