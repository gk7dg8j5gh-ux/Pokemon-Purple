#include "pokemon.h"
#include <stdlib.h>

const char* const pokemon_names[POKEMON_COUNT + 1] = {
    [0] = "MissingNo",
    [1] = "Bulbasaur",
    [2] = "Ivysaur",
    [3] = "Venusaur",
    [4] = "Charmander",
    [5] = "Charmeleon",
    [6] = "Charizard",
    [7] = "Squirtle",
    [8] = "Wartortle",
    [9] = "Blastoise",
    [10] = "Caterpie",
    [11] = "Metapod",
    [12] = "Butterfree",
    [13] = "Weedle",
    [14] = "Kakuna",
    [15] = "Beedrill",
    [16] = "Pidgey",
    [17] = "Pidgeotto",
    [18] = "Pidgeot",
    [19] = "Rattata",
    [20] = "Raticate",
    [21] = "Spearow",
    [22] = "Fearow",
    [23] = "Ekans",
    [24] = "Arbok",
    [25] = "Pikachu",
    [26] = "Raichu",
    [27] = "Sandshrew",
    [28] = "Sandslash",
    [29] = "NidoranF",
    [30] = "Nidorina",
    [31] = "Nidoqueen",
    [32] = "NidoranM",
    [33] = "Nidorino",
    [34] = "Nidoking",
    [35] = "Clefairy",
    [36] = "Clefable",
    [37] = "Vulpix",
    [38] = "Ninetales",
    [39] = "Jigglypuff",
    [40] = "Wigglytuff",
    [41] = "Zubat",
    [42] = "Golbat",
    [43] = "Oddish",
    [44] = "Gloom",
    [45] = "Vileplume",
    [46] = "Paras",
    [47] = "Parasect",
    [48] = "Venonat",
    [49] = "Venomoth",
    [50] = "Diglett",
    [51] = "Dugtrio",
    [52] = "Meowth",
    [53] = "Persian",
    [54] = "Psyduck",
    [55] = "Golduck",
    [56] = "Mankey",
    [57] = "Primeape",
    [58] = "Growlithe",
    [59] = "Arcanine",
    [60] = "Poliwag",
    [61] = "Poliwhirl",
    [62] = "Poliwrath",
    [63] = "Abra",
    [64] = "Kadabra",
    [65] = "Alakazam",
    [66] = "Machop",
    [67] = "Machoke",
    [68] = "Machamp",
    [69] = "Bellsprout",
    [70] = "Weepinbell",
    [71] = "Victreebel",
    [72] = "Tentacool",
    [73] = "Tentacruel",
    [74] = "Geodude",
    [75] = "Graveler",
    [76] = "Golem",
    [77] = "Ponyta",
    [78] = "Rapidash",
    [79] = "Slowpoke",
    [80] = "Slowbro",
    [81] = "Magnemite",
    [82] = "Magneton",
    [83] = "Farfetchd",
    [84] = "Doduo",
    [85] = "Dodrio",
    [86] = "Seel",
    [87] = "Dewgong",
    [88] = "Grimer",
    [89] = "Muk",
    [90] = "Shellder",
    [91] = "Cloyster",
    [92] = "Gastly",
    [93] = "Haunter",
    [94] = "Gengar",
    [95] = "Onix",
    [96] = "Drowzee",
    [97] = "Hypno",
    [98] = "Krabby",
    [99] = "Kingler",
    [100] = "Voltorb",
    [101] = "Electrode",
    [102] = "Exeggcute",
    [103] = "Exeggutor",
    [104] = "Cubone",
    [105] = "Marowak",
    [106] = "Hitmonlee",
    [107] = "Hitmonchan",
    [108] = "Lickitung",
    [109] = "Koffing",
    [110] = "Weezing",
    [111] = "Rhyhorn",
    [112] = "Rhydon",
    [113] = "Chansey",
    [114] = "Tangela",
    [115] = "Kangaskhan",
    [116] = "Horsea",
    [117] = "Seadra",
    [118] = "Goldeen",
    [119] = "Seaking",
    [120] = "Staryu",
    [121] = "Starmie",
    [122] = "MrMime",
    [123] = "Scyther",
    [124] = "Jynx",
    [125] = "Electabuzz",
    [126] = "Magmar",
    [127] = "Pinsir",
    [128] = "Tauros",
    [129] = "Magikarp",
    [130] = "Gyarados",
    [131] = "Lapras",
    [132] = "Ditto",
    [133] = "Eevee",
    [134] = "Vaporeon",
    [135] = "Jolteon",
    [136] = "Flareon",
    [137] = "Porygon",
    [138] = "Omanyte",
    [139] = "Omastar",
    [140] = "Kabuto",
    [141] = "Kabutops",
    [142] = "Aerodactyl",
    [143] = "Snorlax",
    [144] = "Articuno",
    [145] = "Zapdos",
    [146] = "Moltres",
    [147] = "Dratini",
    [148] = "Dragonair",
    [149] = "Dragonite",
    [150] = "Mewtwo",
    [151] = "Mew",
};

const Move move_tackle       = {"Tackle", TYPE_NORMAL, 40, 100};
const Move move_scratch      = {"Scratch", TYPE_NORMAL, 40, 100};
const Move move_growl        = {"Growl", TYPE_NORMAL, 0, 100};
const Move move_ember        = {"Ember", TYPE_FIRE, 40, 100};
const Move move_water_gun    = {"Water Gun", TYPE_WATER, 40, 100};
const Move move_vine_whip    = {"Vine Whip", TYPE_GRASS, 45, 100};
const Move move_gust         = {"Gust", TYPE_FLYING, 40, 100};
const Move move_quick_attack = {"Quick Atk", TYPE_NORMAL, 40, 100};
const Move move_bug_bite     = {"Bug Bite", TYPE_BUG, 60, 100};
const Move move_thundershock = {"Thunder", TYPE_ELECTRIC, 40, 100};

static void base_stats(PokemonSpecies s, uint8_t* hp, uint8_t* atk, uint8_t* def, uint8_t* spd) {
    // Exact-ish Gen 1 values for the initial playable set; safe generic values for the rest.
    *hp=50; *atk=50; *def=50; *spd=50;
    switch(s) {
        case POKEMON_BULBASAUR: *hp=45; *atk=49; *def=49; *spd=45; break;
        case POKEMON_CHARMANDER: *hp=39; *atk=52; *def=43; *spd=65; break;
        case POKEMON_SQUIRTLE: *hp=44; *atk=48; *def=65; *spd=43; break;
        case POKEMON_CATERPIE: *hp=45; *atk=30; *def=35; *spd=45; break;
        case POKEMON_PIDGEY: *hp=40; *atk=45; *def=40; *spd=56; break;
        case POKEMON_RATTATA: *hp=30; *atk=56; *def=35; *spd=72; break;
        case POKEMON_PIKACHU: *hp=35; *atk=55; *def=30; *spd=90; break;
        case POKEMON_ZUBAT: *hp=40; *atk=45; *def=35; *spd=55; break;
        case POKEMON_ODDISH: *hp=45; *atk=50; *def=55; *spd=30; break;
        default: break;
    }
}

Pokemon pokemon_create(PokemonSpecies species, uint8_t level) {
    Pokemon p = {0};
    if(species < 1 || species > POKEMON_COUNT) species = POKEMON_BULBASAUR;

    uint8_t bhp, batk, bdef, bspd;
    base_stats(species, &bhp, &batk, &bdef, &bspd);

    p.species = species;
    p.name = pokemon_names[species];
    p.level = level;
    p.max_hp = (bhp * 2 * level) / 100 + level + 10;
    p.current_hp = p.max_hp;
    p.attack = (batk * 2 * level) / 100 + 5;
    p.defense = (bdef * 2 * level) / 100 + 5;
    p.speed = (bspd * 2 * level) / 100 + 5;

    p.moves[0] = move_tackle;
    p.moves[1] = move_growl;

    switch(species) {
        case POKEMON_BULBASAUR:
            p.moves[2] = move_vine_whip;
            break;
        case POKEMON_CHARMANDER:
            p.moves[0] = move_scratch;
            p.moves[2] = move_ember;
            break;
        case POKEMON_SQUIRTLE:
            p.moves[2] = move_water_gun;
            break;
        case POKEMON_PIDGEY:
            p.moves[1] = move_gust;
            p.moves[2] = move_quick_attack;
            break;
        case POKEMON_CATERPIE:
            p.moves[1] = move_bug_bite;
            break;
        case POKEMON_PIKACHU:
            p.moves[1] = move_thundershock;
            p.moves[2] = move_quick_attack;
            break;
        default:
            break;
    }
    return p;
}

int pokemon_calculate_damage(const Pokemon* attacker, const Pokemon* defender, const Move* move) {
    if(!attacker || !defender || !move || move->power == 0) return 0;
    int defense = defender->defense > 0 ? defender->defense : 1;
    int damage = (2 * attacker->level * move->power * attacker->attack) / (defense * 50) + 2;
    damage = (damage * (85 + (rand() % 16))) / 100;
    return damage > 0 ? damage : 1;
}
