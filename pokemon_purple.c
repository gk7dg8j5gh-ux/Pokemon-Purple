#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <storage/storage.h>

#define SAVE_PATH "/ext/apps_data/pokemon_purple/save.bin"
#define SAVE_MAGIC 0x50555250u
#define SAVE_VERSION 1u
#define PARTY_MAX 6
#define SPECIES_COUNT 151
#define DEX_BYTES 19

typedef enum {
    ScreenTitle,
    ScreenStarter,
    ScreenWorld,
    ScreenBattle,
    ScreenBattleMenu,
    ScreenParty,
    ScreenDex,
    ScreenCenter,
    ScreenGym,
    ScreenWin,
} GameScreen;

typedef struct {
    uint8_t species; // 0..150
    uint8_t level;
    uint8_t hp;
    uint8_t max_hp;
    uint16_t xp;
} Mon;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t player_x;
    uint8_t player_y;
    uint8_t zone;
    uint8_t badges;
    uint16_t money;
    uint8_t balls;
    uint8_t potions;
    uint8_t party_count;
    Mon party[PARTY_MAX];
    uint8_t owned[DEX_BYTES];
    uint8_t seen[DEX_BYTES];
} SaveData;

typedef struct {
    Gui* gui;
    ViewPort* view;
    FuriMessageQueue* queue;
    Storage* storage;
    bool running;
    GameScreen screen;
    SaveData save;

    uint8_t starter_cursor;
    uint8_t menu_cursor;
    uint8_t party_cursor;
    uint8_t dex_page;

    uint8_t enemy_species;
    uint8_t enemy_level;
    uint8_t enemy_hp;
    uint8_t enemy_max_hp;

    uint32_t rng;
    char message[32];
    uint8_t message_ticks;
} PurpleGame;

static const char* const species_names[SPECIES_COUNT] = {
    "Bulbasaur",
    "Ivysaur",
    "Venusaur",
    "Charmander",
    "Charmeleon",
    "Charizard",
    "Squirtle",
    "Wartortle",
    "Blastoise",
    "Caterpie",
    "Metapod",
    "Butterfree",
    "Weedle",
    "Kakuna",
    "Beedrill",
    "Pidgey",
    "Pidgeotto",
    "Pidgeot",
    "Rattata",
    "Raticate",
    "Spearow",
    "Fearow",
    "Ekans",
    "Arbok",
    "Pikachu",
    "Raichu",
    "Sandshrew",
    "Sandslash",
    "NidoranF",
    "Nidorina",
    "Nidoqueen",
    "NidoranM",
    "Nidorino",
    "Nidoking",
    "Clefairy",
    "Clefable",
    "Vulpix",
    "Ninetales",
    "Jigglypuff",
    "Wigglytuff",
    "Zubat",
    "Golbat",
    "Oddish",
    "Gloom",
    "Vileplume",
    "Paras",
    "Parasect",
    "Venonat",
    "Venomoth",
    "Diglett",
    "Dugtrio",
    "Meowth",
    "Persian",
    "Psyduck",
    "Golduck",
    "Mankey",
    "Primeape",
    "Growlithe",
    "Arcanine",
    "Poliwag",
    "Poliwhirl",
    "Poliwrath",
    "Abra",
    "Kadabra",
    "Alakazam",
    "Machop",
    "Machoke",
    "Machamp",
    "Bellsprout",
    "Weepinbell",
    "Victreebel",
    "Tentacool",
    "Tentacruel",
    "Geodude",
    "Graveler",
    "Golem",
    "Ponyta",
    "Rapidash",
    "Slowpoke",
    "Slowbro",
    "Magnemite",
    "Magneton",
    "Farfetchd",
    "Doduo",
    "Dodrio",
    "Seel",
    "Dewgong",
    "Grimer",
    "Muk",
    "Shellder",
    "Cloyster",
    "Gastly",
    "Haunter",
    "Gengar",
    "Onix",
    "Drowzee",
    "Hypno",
    "Krabby",
    "Kingler",
    "Voltorb",
    "Electrode",
    "Exeggcute",
    "Exeggutor",
    "Cubone",
    "Marowak",
    "Hitmonlee",
    "Hitmonchan",
    "Lickitung",
    "Koffing",
    "Weezing",
    "Rhyhorn",
    "Rhydon",
    "Chansey",
    "Tangela",
    "Kangaskhan",
    "Horsea",
    "Seadra",
    "Goldeen",
    "Seaking",
    "Staryu",
    "Starmie",
    "MrMime",
    "Scyther",
    "Jynx",
    "Electabuzz",
    "Magmar",
    "Pinsir",
    "Tauros",
    "Magikarp",
    "Gyarados",
    "Lapras",
    "Ditto",
    "Eevee",
    "Vaporeon",
    "Jolteon",
    "Flareon",
    "Porygon",
    "Omanyte",
    "Omastar",
    "Kabuto",
    "Kabutops",
    "Aerodactyl",
    "Snorlax",
    "Articuno",
    "Zapdos",
    "Moltres",
    "Dratini",
    "Dragonair",
    "Dragonite",
    "Mewtwo",
    "Mew"
};

static uint32_t prng(PurpleGame* g) {
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

static uint8_t clamp_u8(int v, int lo, int hi) {
    if(v < lo) return lo;
    if(v > hi) return hi;
    return (uint8_t)v;
}

static uint8_t species_base_hp(uint8_t s) {
    // Gives legendaries / late dex slightly more bulk without a huge data table.
    uint16_t dex = (uint16_t)s + 1;
    uint8_t base = 10 + (dex % 8);
    if(dex >= 144) base += 6;
    else if(dex >= 130) base += 3;
    return base;
}

static uint8_t species_power(uint8_t s) {
    uint16_t dex = (uint16_t)s + 1;
    uint8_t p = 3 + (dex % 5);
    if(dex >= 144) p += 3;
    else if(dex >= 130) p += 2;
    return p;
}

static void dex_set(uint8_t* bits, uint8_t species) {
    bits[species >> 3] |= (1u << (species & 7));
}

static bool dex_get(const uint8_t* bits, uint8_t species) {
    return (bits[species >> 3] & (1u << (species & 7))) != 0;
}

static uint16_t dex_count(const uint8_t* bits) {
    uint16_t count = 0;
    for(uint8_t s = 0; s < SPECIES_COUNT; s++) if(dex_get(bits, s)) count++;
    return count;
}

static uint8_t mon_max_hp(uint8_t species, uint8_t level) {
    return clamp_u8(species_base_hp(species) + level * 2, 1, 99);
}

static uint16_t xp_needed(uint8_t level) {
    return (uint16_t)(level * level * 3u);
}

static bool save_game(PurpleGame* g) {
    File* f = storage_file_alloc(g->storage);
    bool ok = storage_file_open(f, SAVE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        ok = storage_file_write(f, &g->save, sizeof(g->save)) == sizeof(g->save);
        storage_file_close(f);
    }
    storage_file_free(f);
    return ok;
}

static bool load_game(PurpleGame* g) {
    File* f = storage_file_alloc(g->storage);
    bool ok = storage_file_open(f, SAVE_PATH, FSAM_READ, FSOM_OPEN_EXISTING);
    if(ok) {
        SaveData tmp;
        ok = storage_file_read(f, &tmp, sizeof(tmp)) == sizeof(tmp);
        storage_file_close(f);
        if(ok && tmp.magic == SAVE_MAGIC && tmp.version == SAVE_VERSION) {
            g->save = tmp;
        } else {
            ok = false;
        }
    }
    storage_file_free(f);
    return ok;
}

static void new_game(PurpleGame* g) {
    memset(&g->save, 0, sizeof(g->save));
    g->save.magic = SAVE_MAGIC;
    g->save.version = SAVE_VERSION;
    g->save.player_x = 7;
    g->save.player_y = 6;
    g->save.zone = 0;
    g->save.money = 500;
    g->save.balls = 10;
    g->save.potions = 3;
    g->save.party_count = 0;
}

static void add_party_mon(PurpleGame* g, uint8_t species, uint8_t level) {
    dex_set(g->save.owned, species);
    dex_set(g->save.seen, species);
    if(g->save.party_count < PARTY_MAX) {
        Mon* m = &g->save.party[g->save.party_count++];
        m->species = species;
        m->level = level;
        m->max_hp = mon_max_hp(species, level);
        m->hp = m->max_hp;
        m->xp = 0;
    }
}

static void heal_party(PurpleGame* g) {
    for(uint8_t i = 0; i < g->save.party_count; i++) {
        Mon* m = &g->save.party[i];
        m->max_hp = mon_max_hp(m->species, m->level);
        m->hp = m->max_hp;
    }
}

static void level_check(Mon* m) {
    while(m->level < 100 && m->xp >= xp_needed(m->level)) {
        m->xp -= xp_needed(m->level);
        m->level++;
        m->max_hp = mon_max_hp(m->species, m->level);
        m->hp = m->max_hp;
    }
}

static uint8_t current_zone(PurpleGame* g) {
    // Eight horizontal regions across one continuous 30x14 overworld.
    uint8_t x = g->save.player_x;
    if(x < 4) return 0;
    if(x < 8) return 1;
    if(x < 12) return 2;
    if(x < 16) return 3;
    if(x < 20) return 4;
    if(x < 24) return 5;
    if(x < 28) return 6;
    return 7;
}

static bool is_center_tile(uint8_t x, uint8_t y) {
    return (x == 2 && y == 2) || (x == 10 && y == 2) || (x == 18 && y == 2) || (x == 26 && y == 2);
}

static bool is_gym_tile(uint8_t x, uint8_t y) {
    return (x == 6 && y == 2) || (x == 14 && y == 2) || (x == 22 && y == 2) || (x == 29 && y == 2);
}

static bool is_grass_tile(uint8_t x, uint8_t y) {
    if(y < 4 || y > 11) return false;
    if((x + y) % 5 == 0) return false;
    return true;
}

static uint8_t encounter_species(PurpleGame* g) {
    uint8_t z = current_zone(g);
    // Every regular Gen 1 species is obtainable across the eight regions.
    uint16_t start = z * 19u;
    uint16_t span = 19u;
    uint16_t idx = start + (prng(g) % span);
    if(idx >= 143) idx = 130 + (prng(g) % 13); // keep regular zones below legends
    // After 8 badges, late route can spawn legends + Mewtwo + Mew.
    if(g->save.badges >= 8 && z == 7 && (prng(g) % 6 == 0)) {
        idx = 143 + (prng(g) % 8);
    }
    return (uint8_t)idx;
}

static uint8_t encounter_level(PurpleGame* g) {
    uint8_t z = current_zone(g);
    uint8_t lv = 3 + z * 5 + (prng(g) % 6);
    if(g->save.badges >= 8 && z == 7) lv += 10;
    return clamp_u8(lv, 2, 70);
}

static void start_encounter(PurpleGame* g) {
    g->enemy_species = encounter_species(g);
    g->enemy_level = encounter_level(g);
    g->enemy_max_hp = mon_max_hp(g->enemy_species, g->enemy_level);
    g->enemy_hp = g->enemy_max_hp;
    dex_set(g->save.seen, g->enemy_species);
    g->menu_cursor = 0;
    g->screen = ScreenBattle;
}

static void show_msg(PurpleGame* g, const char* msg) {
    snprintf(g->message, sizeof(g->message), "%s", msg);
    g->message_ticks = 12;
}

static Mon* lead_mon(PurpleGame* g) {
    if(g->save.party_count == 0) return NULL;
    return &g->save.party[0];
}

static void enemy_turn(PurpleGame* g) {
    Mon* m = lead_mon(g);
    if(!m) return;
    uint8_t dmg = 1 + (species_power(g->enemy_species) + g->enemy_level / 5 + (prng(g)%4)) / 3;
    if(dmg >= m->hp) m->hp = 0;
    else m->hp -= dmg;

    if(m->hp == 0) {
        bool found = false;
        for(uint8_t i = 1; i < g->save.party_count; i++) {
            if(g->save.party[i].hp) {
                Mon tmp = g->save.party[0];
                g->save.party[0] = g->save.party[i];
                g->save.party[i] = tmp;
                found = true;
                break;
            }
        }
        if(!found) {
            heal_party(g);
            g->save.player_x = 2;
            g->save.player_y = 3;
            g->screen = ScreenWorld;
            show_msg(g, "Party fainted!");
            save_game(g);
        }
    }
}

static void player_attack(PurpleGame* g) {
    Mon* m = lead_mon(g);
    if(!m) return;
    uint8_t dmg = 2 + (species_power(m->species) + m->level / 4 + (prng(g)%5)) / 2;
    if(dmg >= g->enemy_hp) g->enemy_hp = 0;
    else g->enemy_hp -= dmg;

    if(g->enemy_hp == 0) {
        uint16_t gain = 8 + g->enemy_level * 4u;
        m->xp += gain;
        level_check(m);
        g->save.money += 20 + g->enemy_level;
        show_msg(g, "You won!");
        g->screen = ScreenWorld;
        save_game(g);
    } else {
        enemy_turn(g);
    }
}

static void try_catch(PurpleGame* g) {
    if(g->save.balls == 0) {
        show_msg(g, "No Purple Balls!");
        return;
    }
    g->save.balls--;
    uint16_t hp_factor = (uint16_t)(g->enemy_max_hp - g->enemy_hp + 1) * 100u / g->enemy_max_hp;
    uint16_t rarity = 20 + ((uint16_t)g->enemy_species * 37u % 45u);
    if(g->enemy_species >= 143) rarity += 25;
    uint16_t chance = 30 + hp_factor - rarity / 2;
    if(chance > 90) chance = 90;
    if((prng(g) % 100) < chance) {
        add_party_mon(g, g->enemy_species, g->enemy_level);
        show_msg(g, "Caught!");
        g->screen = ScreenWorld;
        save_game(g);
    } else {
        show_msg(g, "It broke free!");
        enemy_turn(g);
    }
}

static void use_potion(PurpleGame* g) {
    if(g->save.potions == 0) {
        show_msg(g, "No potions!");
        return;
    }
    Mon* m = lead_mon(g);
    if(!m) return;
    g->save.potions--;
    uint8_t heal = 15;
    m->hp = clamp_u8(m->hp + heal, 0, m->max_hp);
    show_msg(g, "HP restored");
    enemy_turn(g);
}

static void run_battle(PurpleGame* g) {
    if(g->enemy_species >= 143 && g->save.badges >= 8) {
        show_msg(g, "Can't run!");
        enemy_turn(g);
        return;
    }
    if((prng(g)%100) < 80) {
        g->screen = ScreenWorld;
        show_msg(g, "Got away");
    } else {
        show_msg(g, "Couldn't escape");
        enemy_turn(g);
    }
}

static void draw_header(Canvas* c, PurpleGame* g) {
    char buf[32];
    canvas_set_font(c, FontSecondary);
    snprintf(buf, sizeof(buf), "B:%u $%u P:%u", g->save.badges, g->save.money, g->save.balls);
    canvas_draw_str(c, 2, 8, buf);
}

static void draw_title(Canvas* c, PurpleGame* g) {
    UNUSED(g);
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 16, 17, "POKEMON PURPLE");
    canvas_set_font(c, FontSecondary);
    canvas_draw_str(c, 29, 31, "Flipper Edition");
    canvas_draw_str(c, 22, 47, "OK: Continue/New");
    canvas_draw_str(c, 31, 59, "Back: Exit");
}

static void draw_starter(Canvas* c, PurpleGame* g) {
    static const char* opts[3] = {"Bulbasaur","Charmander","Squirtle"};
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 7, 12, "Choose a starter");
    canvas_set_font(c, FontSecondary);
    for(uint8_t i=0;i<3;i++) {
        char line[24];
        snprintf(line,sizeof(line),"%c %s", i==g->starter_cursor?'>':' ', opts[i]);
        canvas_draw_str(c, 12, 30+i*11, line);
    }
}

static void draw_world(Canvas* c, PurpleGame* g) {
    draw_header(c,g);
    uint8_t px = g->save.player_x;
    uint8_t py = g->save.player_y;
    uint8_t z = current_zone(g);

    char zbuf[16];
    snprintf(zbuf,sizeof(zbuf),"ZONE %u",z+1);
    canvas_draw_str(c, 91, 8, zbuf);

    // 15x7 viewport around player
    int sx = (int)px - 7;
    int sy = (int)py - 3;
    for(int vy=0;vy<7;vy++) {
        for(int vx=0;vx<15;vx++) {
            int wx=sx+vx, wy=sy+vy;
            int x=2+vx*8, y=12+vy*7;
            if(wx<0 || wy<0 || wx>=32 || wy>=14) {
                canvas_draw_box(c,x,y,7,6);
                continue;
            }
            if(is_center_tile(wx,wy)) {
                canvas_draw_frame(c,x,y,7,6);
                canvas_draw_str(c,x+1,y+5,"C");
            } else if(is_gym_tile(wx,wy)) {
                canvas_draw_frame(c,x,y,7,6);
                canvas_draw_str(c,x+1,y+5,"G");
            } else if(is_grass_tile(wx,wy)) {
                canvas_draw_dot(c,x+2,y+2);
                canvas_draw_dot(c,x+5,y+4);
            }
        }
    }
    canvas_draw_circle(c, 2+7*8+3, 12+3*7+3, 2);

    if(g->message_ticks) {
        canvas_draw_box(c, 1, 52, 126, 12);
        canvas_set_color(c, ColorWhite);
        canvas_draw_str(c, 4, 61, g->message);
        canvas_set_color(c, ColorBlack);
    }
}

static void draw_battle(Canvas* c, PurpleGame* g) {
    Mon* m = lead_mon(g);
    canvas_set_font(c, FontSecondary);
    char buf[32];

    snprintf(buf,sizeof(buf),"%s L%u",species_names[g->enemy_species],g->enemy_level);
    canvas_draw_str(c, 2, 9, buf);
    snprintf(buf,sizeof(buf),"HP %u/%u",g->enemy_hp,g->enemy_max_hp);
    canvas_draw_str(c, 2, 19, buf);
    canvas_draw_frame(c,2,22,60,6);
    uint8_t ew = g->enemy_max_hp ? (g->enemy_hp*58/g->enemy_max_hp) : 0;
    if(ew) canvas_draw_box(c,3,23,ew,4);

    if(m) {
        snprintf(buf,sizeof(buf),"%s L%u",species_names[m->species],m->level);
        canvas_draw_str(c, 66, 37, buf);
        snprintf(buf,sizeof(buf),"HP %u/%u",m->hp,m->max_hp);
        canvas_draw_str(c, 66, 47, buf);
        canvas_draw_frame(c,66,50,60,6);
        uint8_t pw = m->max_hp ? (m->hp*58/m->max_hp) : 0;
        if(pw) canvas_draw_box(c,67,51,pw,4);
    }
    canvas_draw_str(c,2,63,"OK: Menu");
}

static void draw_battle_menu(Canvas* c, PurpleGame* g) {
    static const char* items[4]={"ATTACK","CATCH","POTION","RUN"};
    canvas_set_font(c, FontPrimary);
    canvas_draw_str(c, 8, 12, "Battle");
    canvas_set_font(c, FontSecondary);
    for(uint8_t i=0;i<4;i++) {
        char line[24];
        snprintf(line,sizeof(line),"%c %s",i==g->menu_cursor?'>':' ',items[i]);
        canvas_draw_str(c,18,28+i*9,line);
    }
}

static void draw_party(Canvas* c, PurpleGame* g) {
    canvas_set_font(c,FontPrimary);
    canvas_draw_str(c,2,10,"Party");
    canvas_set_font(c,FontSecondary);
    for(uint8_t i=0;i<g->save.party_count;i++) {
        Mon* m=&g->save.party[i];
        char line[32];
        snprintf(line,sizeof(line),"%c%s L%u %u/%u",
            i==g->party_cursor?'>':' ',
            species_names[m->species],m->level,m->hp,m->max_hp);
        canvas_draw_str(c,2,22+i*7,line);
    }
}

static void draw_dex(Canvas* c, PurpleGame* g) {
    canvas_set_font(c,FontPrimary);
    char hdr[32];
    snprintf(hdr,sizeof(hdr),"DEX %u/151",dex_count(g->save.owned));
    canvas_draw_str(c,2,10,hdr);
    canvas_set_font(c,FontSecondary);
    uint8_t start = g->dex_page*6;
    for(uint8_t i=0;i<6;i++) {
        uint8_t s=start+i;
        if(s>=SPECIES_COUNT) break;
        char line[32];
        char state = dex_get(g->save.owned,s)?'*':(dex_get(g->save.seen,s)?'+':'-');
        snprintf(line,sizeof(line),"%c%03u %s",state,s+1,species_names[s]);
        canvas_draw_str(c,2,21+i*7,line);
    }
}

static void draw_center(Canvas* c, PurpleGame* g) {
    UNUSED(g);
    canvas_set_font(c,FontPrimary);
    canvas_draw_str(c,12,16,"PURPLE CENTER");
    canvas_set_font(c,FontSecondary);
    canvas_draw_str(c,16,34,"Party fully healed.");
    canvas_draw_str(c,14,48,"+5 Balls, +2 Potions");
    canvas_draw_str(c,25,61,"OK: Leave");
}

static void draw_gym(Canvas* c, PurpleGame* g) {
    canvas_set_font(c,FontPrimary);
    char buf[24];
    snprintf(buf,sizeof(buf),"GYM %u",g->save.badges+1);
    canvas_draw_str(c,44,14,buf);
    canvas_set_font(c,FontSecondary);
    canvas_draw_str(c,8,31,"Win a quick boss battle");
    canvas_draw_str(c,14,44,"to earn next badge.");
    canvas_draw_str(c,25,60,"OK: Challenge");
}

static void draw_win(Canvas* c, PurpleGame* g) {
    canvas_set_font(c,FontPrimary);
    canvas_draw_str(c,20,14,"PURPLE CHAMPION");
    canvas_set_font(c,FontSecondary);
    char buf[32];
    snprintf(buf,sizeof(buf),"Caught %u / 151",dex_count(g->save.owned));
    canvas_draw_str(c,29,31,buf);
    canvas_draw_str(c,10,47,"Legendaries now spawn");
    canvas_draw_str(c,17,60,"in Zone 8. Keep going!");
}

static void draw_cb(Canvas* c, void* ctx) {
    PurpleGame* g=ctx;
    canvas_clear(c);
    switch(g->screen) {
    case ScreenTitle: draw_title(c,g); break;
    case ScreenStarter: draw_starter(c,g); break;
    case ScreenWorld: draw_world(c,g); break;
    case ScreenBattle: draw_battle(c,g); break;
    case ScreenBattleMenu: draw_battle_menu(c,g); break;
    case ScreenParty: draw_party(c,g); break;
    case ScreenDex: draw_dex(c,g); break;
    case ScreenCenter: draw_center(c,g); break;
    case ScreenGym: draw_gym(c,g); break;
    case ScreenWin: draw_win(c,g); break;
    }
}

static void input_cb(InputEvent* event, void* ctx) {
    PurpleGame* g=ctx;
    furi_message_queue_put(g->queue,event,0);
}

static void move_world(PurpleGame* g, InputKey key) {
    int nx=g->save.player_x, ny=g->save.player_y;
    if(key==InputKeyUp) ny--;
    else if(key==InputKeyDown) ny++;
    else if(key==InputKeyLeft) nx--;
    else if(key==InputKeyRight) nx++;
    else if(key==InputKeyOk) {
        if(is_center_tile(g->save.player_x,g->save.player_y)) {
            heal_party(g);
            g->save.balls = clamp_u8(g->save.balls+5,0,99);
            g->save.potions = clamp_u8(g->save.potions+2,0,99);
            save_game(g);
            g->screen=ScreenCenter;
        } else if(is_gym_tile(g->save.player_x,g->save.player_y) && g->save.badges<8) {
            g->screen=ScreenGym;
        } else {
            g->screen=ScreenParty;
            g->party_cursor=0;
        }
        return;
    } else if(key==InputKeyBack) {
        g->screen=ScreenDex;
        g->dex_page=0;
        return;
    } else return;

    if(nx>=0 && nx<32 && ny>=0 && ny<14) {
        g->save.player_x=nx;
        g->save.player_y=ny;
        g->save.zone=current_zone(g);
        if(g->message_ticks) g->message_ticks--;
        if(is_grass_tile(nx,ny) && (prng(g)%100)<20) start_encounter(g);
    }
}

static void challenge_gym(PurpleGame* g) {
    Mon* m=lead_mon(g);
    if(!m) return;
    uint8_t gym=g->save.badges;
    uint8_t boss_species=(uint8_t)(18 + gym*16);
    if(boss_species>=SPECIES_COUNT) boss_species=149;
    uint8_t boss_level=10+gym*6;
    // Quick deterministic boss check using lead HP/level/power.
    uint16_t player_score=m->level*3u+species_power(m->species)*4u+m->hp;
    uint16_t boss_score=boss_level*3u+species_power(boss_species)*4u+15u;
    player_score += prng(g)%25;
    boss_score += prng(g)%25;
    if(player_score>=boss_score) {
        g->save.badges++;
        g->save.money+=500;
        g->save.balls=clamp_u8(g->save.balls+10,0,99);
        heal_party(g);
        save_game(g);
        if(g->save.badges>=8) g->screen=ScreenWin;
        else {
            g->screen=ScreenWorld;
            show_msg(g,"Badge earned!");
        }
    } else {
        heal_party(g);
        g->save.player_x=2; g->save.player_y=3;
        g->screen=ScreenWorld;
        show_msg(g,"Gym loss - healed");
    }
}

static void handle_short(PurpleGame* g, InputKey key) {
    switch(g->screen) {
    case ScreenTitle:
        if(key==InputKeyBack) g->running=false;
        else if(key==InputKeyOk) {
            if(load_game(g) && g->save.party_count>0) g->screen=ScreenWorld;
            else {
                new_game(g);
                g->starter_cursor=0;
                g->screen=ScreenStarter;
            }
        }
        break;

    case ScreenStarter:
        if(key==InputKeyUp) g->starter_cursor=(g->starter_cursor+2)%3;
        else if(key==InputKeyDown) g->starter_cursor=(g->starter_cursor+1)%3;
        else if(key==InputKeyBack) g->screen=ScreenTitle;
        else if(key==InputKeyOk) {
            static const uint8_t starter_species[3]={0,3,6};
            add_party_mon(g,starter_species[g->starter_cursor],5);
            save_game(g);
            g->screen=ScreenWorld;
        }
        break;

    case ScreenWorld:
        move_world(g,key);
        break;

    case ScreenBattle:
        if(key==InputKeyOk) { g->menu_cursor=0; g->screen=ScreenBattleMenu; }
        break;

    case ScreenBattleMenu:
        if(key==InputKeyUp) g->menu_cursor=(g->menu_cursor+3)%4;
        else if(key==InputKeyDown) g->menu_cursor=(g->menu_cursor+1)%4;
        else if(key==InputKeyBack) g->screen=ScreenBattle;
        else if(key==InputKeyOk) {
            uint8_t action=g->menu_cursor;
            g->screen=ScreenBattle;
            if(action==0) player_attack(g);
            else if(action==1) try_catch(g);
            else if(action==2) use_potion(g);
            else run_battle(g);
        }
        break;

    case ScreenParty:
        if(key==InputKeyUp && g->party_cursor>0) g->party_cursor--;
        else if(key==InputKeyDown && g->party_cursor+1<g->save.party_count) g->party_cursor++;
        else if(key==InputKeyBack) g->screen=ScreenWorld;
        else if(key==InputKeyOk && g->party_cursor<g->save.party_count) {
            Mon tmp=g->save.party[0];
            g->save.party[0]=g->save.party[g->party_cursor];
            g->save.party[g->party_cursor]=tmp;
            g->party_cursor=0;
            save_game(g);
        }
        break;

    case ScreenDex:
        if(key==InputKeyLeft && g->dex_page>0) g->dex_page--;
        else if(key==InputKeyRight && g->dex_page<25) g->dex_page++;
        else if(key==InputKeyBack || key==InputKeyOk) g->screen=ScreenWorld;
        break;

    case ScreenCenter:
        if(key==InputKeyOk || key==InputKeyBack) g->screen=ScreenWorld;
        break;

    case ScreenGym:
        if(key==InputKeyBack) g->screen=ScreenWorld;
        else if(key==InputKeyOk) challenge_gym(g);
        break;

    case ScreenWin:
        if(key==InputKeyOk || key==InputKeyBack) g->screen=ScreenWorld;
        break;
    }
}

int32_t pokemon_purple_app(void* p) {
    UNUSED(p);

    PurpleGame* g=malloc(sizeof(PurpleGame));
    memset(g,0,sizeof(PurpleGame));
    g->running=true;
    g->screen=ScreenTitle;
    g->rng=0xC0FFEEu ^ furi_get_tick();

    g->queue=furi_message_queue_alloc(8,sizeof(InputEvent));
    g->gui=furi_record_open(RECORD_GUI);
    g->storage=furi_record_open(RECORD_STORAGE);
    g->view=view_port_alloc();
    view_port_draw_callback_set(g->view,draw_cb,g);
    view_port_input_callback_set(g->view,input_cb,g);
    gui_add_view_port(g->gui,g->view,GuiLayerFullscreen);

    InputEvent event;
    while(g->running) {
        if(furi_message_queue_get(g->queue,&event,100)==FuriStatusOk) {
            if(event.type==InputTypeShort) {
                handle_short(g,event.key);
                view_port_update(g->view);
            }
        }
    }

    save_game(g);
    gui_remove_view_port(g->gui,g->view);
    view_port_free(g->view);
    furi_message_queue_free(g->queue);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);
    free(g);
    return 0;
}
