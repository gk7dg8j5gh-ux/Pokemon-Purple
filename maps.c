#include "maps.h"

static Tile wall_tile(void) {
    Tile t = {0};
    t.type = TILE_WALL;
    t.obstacle = true;
    return t;
}

static Tile path_tile(void) {
    Tile t = {0};
    t.type = TILE_PATH;
    return t;
}

static Tile grass_tile(void) {
    Tile t = {0};
    t.type = TILE_GRASS;
    t.encounter_rate = 20;
    t.encounters[0] = POKEMON_PIDGEY;
    t.encounters[1] = POKEMON_RATTATA;
    t.encounters[2] = POKEMON_CATERPIE;
    t.min_level = 2;
    t.max_level = 5;
    return t;
}

void maps_init(GameMap* route1, GameMap* pallet) {
    route1->name = "ROUTE 1";
    pallet->name = "PALLET";

    for(int y=0; y<MAP_H; y++) {
        for(int x=0; x<MAP_W; x++) {
            bool border = x==0 || y==0 || x==MAP_W-1 || y==MAP_H-1;
            route1->tiles[x][y] = border ? wall_tile() : grass_tile();
            pallet->tiles[x][y] = border ? wall_tile() : path_tile();
        }
    }

    // Route path through grass.
    for(int y=1; y<MAP_H-1; y++) {
        route1->tiles[9][y] = path_tile();
        route1->tiles[10][y] = path_tile();
    }

    // Small grass patch in town.
    for(int y=10; y<15; y++) {
        for(int x=3; x<8; x++) {
            pallet->tiles[x][y] = grass_tile();
            pallet->tiles[x][y].encounter_rate = 8;
        }
    }
}
