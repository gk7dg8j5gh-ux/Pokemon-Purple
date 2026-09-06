#pragma once
#include "tiles.h"

typedef struct {
    const char* name;
    Tile tiles[MAP_W][MAP_H];
} GameMap;

void maps_init(GameMap* route1, GameMap* pallet);
