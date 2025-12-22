#ifndef TG_T9_DICT_H
#define TG_T9_DICT_H

#include "../shared/platform.h"
#include <ctype.h>
#include <string.h>

// T9 Predictive Dictionary
// ~600 common words, alphabetically sorted
// Focus: common nouns, names, game terms, short words

const char T9_DICT[] PLATFORM_PROGMEM =
    // A
    "ACE\0" "ADAM\0" "ADD\0" "ADVENTURE\0" "AGE\0" "AIM\0" "AIR\0" "ALEX\0"
    "ALL\0" "ALPHA\0" "AMY\0" "AND\0" "ANN\0" "ANT\0" "APE\0" "APP\0"
    "APPLE\0" "ARC\0" "ARK\0" "ARM\0" "ART\0" "ASH\0" "AXE\0"
    // B
    "BACK\0" "BAD\0" "BAG\0" "BALL\0" "BAN\0" "BAR\0" "BASE\0" "BAT\0"
    "BAY\0" "BEACH\0" "BEAR\0" "BED\0" "BEE\0" "BELL\0" "BEN\0" "BEST\0"
    "BIG\0" "BILL\0" "BIRD\0" "BIT\0" "BLACK\0" "BLADE\0" "BLOCK\0" "BLUE\0"
    "BOB\0" "BOAT\0" "BODY\0" "BOLD\0" "BONE\0" "BOOK\0" "BOSS\0" "BOW\0"
    "BOX\0" "BOY\0" "BRAVE\0" "BRICK\0" "BROOK\0" "BUD\0" "BUG\0" "BUILD\0"
    "BURN\0" "BUSH\0"
    // C
    "CABIN\0" "CAKE\0" "CALL\0" "CALM\0" "CAMP\0" "CAN\0" "CAP\0" "CAR\0"
    "CARD\0" "CARE\0" "CARL\0" "CASE\0" "CAST\0" "CAT\0" "CAVE\0" "CHAR\0"
    "CHEF\0" "CHEST\0" "CHOP\0" "CITY\0" "CLAY\0" "CLIFF\0" "CLOUD\0" "CLUE\0"
    "COAL\0" "COAST\0" "COAT\0" "CODE\0" "COIN\0" "COLD\0" "COOK\0" "COOL\0"
    "COPY\0" "CORE\0" "CORN\0" "COW\0" "CRAB\0" "CRAFT\0" "CRASH\0" "CREEK\0"
    "CROW\0" "CUP\0" "CUT\0"
    // D
    "DAN\0" "DANGER\0" "DARK\0" "DATA\0" "DAVE\0" "DAWN\0" "DAY\0" "DEEP\0"
    "DEER\0" "DEN\0" "DESK\0" "DEW\0" "DIG\0" "DIRT\0" "DOC\0" "DOG\0"
    "DOOR\0" "DOT\0" "DOWN\0" "DRAGON\0" "DRAW\0" "DREAM\0" "DROP\0" "DRUM\0"
    "DUCK\0" "DUNE\0" "DUST\0"
    // E
    "EAGLE\0" "EAR\0" "EARTH\0" "EAST\0" "EASY\0" "EAT\0" "ECHO\0" "EDGE\0"
    "EGG\0" "ELF\0" "ELM\0" "EMU\0" "END\0" "ERA\0" "EVE\0" "EVIL\0"
    "EXIT\0" "EYE\0"
    // F
    "FACE\0" "FACT\0" "FAIR\0" "FALL\0" "FAME\0" "FAN\0" "FAR\0" "FARM\0"
    "FAST\0" "FAT\0" "FATE\0" "FEAR\0" "FEED\0" "FEEL\0" "FERN\0" "FIELD\0"
    "FIG\0" "FILE\0" "FILL\0" "FIND\0" "FINE\0" "FIRE\0" "FISH\0" "FIST\0"
    "FIT\0" "FIVE\0" "FLAG\0" "FLAME\0" "FLAT\0" "FLEE\0" "FLIP\0" "FLOAT\0"
    "FLOOR\0" "FLOW\0" "FLY\0" "FOG\0" "FOLD\0" "FOOD\0" "FOOT\0" "FORD\0"
    "FOREST\0" "FORK\0" "FORM\0" "FORT\0" "FOUR\0" "FOX\0" "FRANK\0" "FREE\0"
    "FRESH\0" "FROG\0" "FRONT\0" "FROST\0" "FRUIT\0" "FUEL\0" "FULL\0" "FUN\0"
    // G
    "GAIN\0" "GALE\0" "GAME\0" "GAP\0" "GATE\0" "GEM\0" "GHOST\0" "GIFT\0"
    "GIRL\0" "GIVE\0" "GLAD\0" "GLASS\0" "GLEN\0" "GLOW\0" "GOAL\0" "GOAT\0"
    "GOLD\0" "GONE\0" "GOOD\0" "GRACE\0" "GRAIN\0" "GRAND\0" "GRAPE\0" "GRASS\0"
    "GRAVE\0" "GRAY\0" "GREEN\0" "GREY\0" "GRID\0" "GRILL\0" "GROVE\0" "GROW\0"
    "GUARD\0" "GULF\0" "GUN\0" "GUY\0"
    // H
    "HACK\0" "HAIL\0" "HAIR\0" "HALF\0" "HALL\0" "HAM\0" "HAMMER\0" "HAND\0"
    "HAPPY\0" "HARD\0" "HARM\0" "HAT\0" "HATE\0" "HAWK\0" "HAY\0" "HEAD\0"
    "HEAL\0" "HEART\0" "HEAT\0" "HELM\0" "HELP\0" "HENRY\0" "HERB\0" "HERO\0"
    "HIGH\0" "HILL\0" "HIT\0" "HOLD\0" "HOLE\0" "HOME\0" "HONEY\0" "HOOD\0"
    "HOOK\0" "HOPE\0" "HORN\0" "HORSE\0" "HOST\0" "HOT\0" "HOUR\0" "HOUSE\0"
    "HUB\0" "HUNT\0" "HUT\0"
    // I
    "ICE\0" "IDEA\0" "IMP\0" "INK\0" "INN\0" "ION\0" "IRON\0" "ISLAND\0"
    "IVY\0"
    // J
    "JACK\0" "JADE\0" "JAIL\0" "JAKE\0" "JAM\0" "JAMES\0" "JAR\0" "JAW\0"
    "JAY\0" "JET\0" "JIM\0" "JOB\0" "JOE\0" "JOIN\0" "JOKE\0" "JOY\0"
    "JUDGE\0" "JUG\0" "JUMP\0" "JUNE\0" "JUST\0"
    // K
    "KATE\0" "KEN\0" "KEY\0" "KICK\0" "KID\0" "KILL\0" "KIM\0" "KING\0"
    "KISS\0" "KIT\0" "KITE\0" "KNEE\0" "KNIFE\0" "KNIGHT\0" "KNOB\0" "KNOT\0"
    // L
    "LAB\0" "LACK\0" "LAKE\0" "LAMP\0" "LAND\0" "LANE\0" "LAST\0" "LATE\0"
    "LAW\0" "LEAD\0" "LEAF\0" "LEAN\0" "LEAP\0" "LEFT\0" "LEG\0" "LEO\0"
    "LIFE\0" "LIFT\0" "LIGHT\0" "LILY\0" "LINE\0" "LINK\0" "LION\0" "LIST\0"
    "LIVE\0" "LOAD\0" "LOCK\0" "LOG\0" "LONE\0" "LONG\0" "LOOK\0" "LOOP\0"
    "LORD\0" "LOST\0" "LOVE\0" "LOW\0" "LUCK\0" "LUNA\0"
    // M
    "MAD\0" "MAGIC\0" "MAIN\0" "MAKE\0" "MAN\0" "MAP\0" "MARK\0" "MARY\0"
    "MASK\0" "MASS\0" "MASTER\0" "MATCH\0" "MATT\0" "MAX\0" "MAY\0" "MAZE\0"
    "MEAL\0" "MEAT\0" "MEGA\0" "MELT\0" "METAL\0" "MICE\0" "MIKE\0" "MILD\0"
    "MILE\0" "MILK\0" "MILL\0" "MIND\0" "MINE\0" "MINT\0" "MIST\0" "MIX\0"
    "MOB\0" "MODE\0" "MOON\0" "MOOR\0" "MOSS\0" "MOST\0" "MOTH\0" "MOUNT\0"
    "MOUSE\0" "MOVE\0" "MUD\0" "MUG\0" "MULE\0" "MYTH\0"
    // N
    "NAIL\0" "NAME\0" "NEAR\0" "NECK\0" "NEST\0" "NET\0" "NEW\0" "NEWS\0"
    "NICK\0" "NIGHT\0" "NINE\0" "NODE\0" "NOON\0" "NORM\0" "NORTH\0" "NOSE\0"
    "NOTE\0" "NOVA\0" "NULL\0" "NUT\0"
    // O
    "OAK\0" "OAT\0" "OCEAN\0" "ODD\0" "OIL\0" "OLD\0" "OLIVE\0" "ONE\0"
    "OPEN\0" "ORB\0" "ORE\0" "OWL\0" "OX\0"
    // P
    "PACK\0" "PAD\0" "PAGE\0" "PAIL\0" "PAIN\0" "PAIR\0" "PALM\0" "PAN\0"
    "PARK\0" "PART\0" "PASS\0" "PAST\0" "PAT\0" "PATH\0" "PAUL\0" "PAW\0"
    "PAY\0" "PEAK\0" "PEAR\0" "PEN\0" "PEST\0" "PET\0" "PETE\0" "PICK\0"
    "PIE\0" "PIG\0" "PIKE\0" "PILE\0" "PILL\0" "PIN\0" "PINE\0" "PINK\0"
    "PIT\0" "PLACE\0" "PLAIN\0" "PLAN\0" "PLANT\0" "PLAY\0" "PLOT\0" "PLUM\0"
    "PLUS\0" "POD\0" "POINT\0" "POLE\0" "POND\0" "POOL\0" "POOR\0" "POP\0"
    "PORT\0" "POST\0" "POT\0" "POWER\0" "PRIME\0" "PRIZE\0" "PRO\0" "PUFF\0"
    "PULL\0" "PUMP\0" "PURE\0" "PUSH\0" "PUT\0"
    // Q
    "QUEEN\0" "QUEST\0" "QUICK\0" "QUIET\0" "QUIT\0"
    // R
    "RACE\0" "RACK\0" "RAFT\0" "RAGE\0" "RAIL\0" "RAIN\0" "RAKE\0" "RAM\0"
    "RAMP\0" "RANCH\0" "RANGE\0" "RANK\0" "RARE\0" "RAT\0" "RATE\0" "RAW\0"
    "RAY\0" "READ\0" "REAL\0" "RED\0" "REED\0" "REEF\0" "REIGN\0" "REST\0"
    "REX\0" "RIB\0" "RICH\0" "RICK\0" "RIDE\0" "RIDGE\0" "RIFT\0" "RIGHT\0"
    "RIM\0" "RING\0" "RISE\0" "RISK\0" "RIVER\0" "ROAD\0" "ROAM\0" "ROB\0"
    "ROBE\0" "ROBIN\0" "ROCK\0" "ROD\0" "ROLE\0" "ROLL\0" "RON\0" "ROOF\0"
    "ROOM\0" "ROOT\0" "ROPE\0" "ROSE\0" "ROT\0" "ROW\0" "ROY\0" "RUB\0"
    "RUG\0" "RUIN\0" "RULE\0" "RUN\0" "RUSH\0" "RUST\0"
    // S
    "SACK\0" "SAD\0" "SAFE\0" "SAGE\0" "SAIL\0" "SALT\0" "SAM\0" "SAME\0"
    "SAND\0" "SARA\0" "SAVE\0" "SAW\0" "SAY\0" "SCALE\0" "SCAR\0" "SCENE\0"
    "SEA\0" "SEAL\0" "SEAT\0" "SEED\0" "SEEK\0" "SELF\0" "SELL\0" "SEND\0"
    "SET\0" "SHADE\0" "SHADOW\0" "SHAKE\0" "SHAPE\0" "SHARE\0" "SHARK\0" "SHARP\0"
    "SHED\0" "SHEEP\0" "SHELL\0" "SHIFT\0" "SHINE\0" "SHIP\0" "SHOCK\0" "SHOE\0"
    "SHOOT\0" "SHOP\0" "SHORE\0" "SHORT\0" "SHOT\0" "SHOW\0" "SHRUB\0" "SHUT\0"
    "SICK\0" "SIDE\0" "SIEGE\0" "SIGN\0" "SILK\0" "SILVER\0" "SIMPLE\0" "SINK\0"
    "SIT\0" "SITE\0" "SIX\0" "SIZE\0" "SKI\0" "SKILL\0" "SKIN\0" "SKY\0"
    "SLAM\0" "SLAP\0" "SLATE\0" "SLEEP\0" "SLICE\0" "SLIDE\0" "SLIM\0" "SLIP\0"
    "SLOPE\0" "SLOT\0" "SLOW\0" "SMALL\0" "SMART\0" "SMELL\0" "SMILE\0" "SMITH\0"
    "SMOKE\0" "SMOOTH\0" "SNAKE\0" "SNAP\0" "SNOW\0" "SOFT\0" "SOIL\0" "SOLAR\0"
    "SOLO\0" "SOME\0" "SON\0" "SONG\0" "SOON\0" "SORT\0" "SOUL\0" "SOUND\0"
    "SOUTH\0" "SPACE\0" "SPAN\0" "SPARK\0" "SPEAK\0" "SPEED\0" "SPELL\0" "SPEND\0"
    "SPICE\0" "SPIDER\0" "SPIKE\0" "SPIN\0" "SPIRIT\0" "SPLIT\0" "SPORT\0" "SPOT\0"
    "SPRAY\0" "SPRING\0" "SPY\0" "SQUAD\0" "STAFF\0" "STAGE\0" "STAIR\0" "STAKE\0"
    "STALL\0" "STAMP\0" "STAND\0" "STAR\0" "START\0" "STATE\0" "STAY\0" "STEAM\0"
    "STEEL\0" "STEEP\0" "STEM\0" "STEP\0" "STEVE\0" "STICK\0" "STILL\0" "STOCK\0"
    "STONE\0" "STOP\0" "STORE\0" "STORM\0" "STORY\0" "STOVE\0" "STREAM\0" "STREET\0"
    "STRETCH\0" "STRIKE\0" "STRIP\0" "STROKE\0" "STRONG\0" "STUDY\0" "STYLE\0" "SUM\0"
    "SUN\0" "SUPER\0" "SURF\0" "SURGE\0" "SWAMP\0" "SWAN\0" "SWAP\0" "SWARM\0"
    "SWEET\0" "SWIFT\0" "SWIM\0" "SWING\0" "SWORD\0"
    // T
    "TABLE\0" "TAIL\0" "TAKE\0" "TALE\0" "TALK\0" "TALL\0" "TANK\0" "TAP\0"
    "TAPE\0" "TASK\0" "TEA\0" "TEAM\0" "TEAR\0" "TED\0" "TELL\0" "TEN\0"
    "TENT\0" "TERM\0" "TEST\0" "TEXT\0" "THICK\0" "THIN\0" "THING\0" "THINK\0"
    "THREE\0" "THROW\0" "TIDE\0" "TIE\0" "TIGER\0" "TILE\0" "TIME\0" "TIM\0"
    "TIN\0" "TIP\0" "TOAST\0" "TODAY\0" "TOE\0" "TOM\0" "TOMB\0" "TONE\0"
    "TOOL\0" "TOOTH\0" "TOP\0" "TORCH\0" "TORN\0" "TOTAL\0" "TOUCH\0" "TOUGH\0"
    "TOUR\0" "TOWER\0" "TOWN\0" "TOY\0" "TRACK\0" "TRADE\0" "TRAIL\0" "TRAIN\0"
    "TRAP\0" "TRASH\0" "TRAVEL\0" "TREE\0" "TREND\0" "TRIAL\0" "TRIBE\0" "TRICK\0"
    "TRIM\0" "TRIP\0" "TROLL\0" "TROOP\0" "TRUCK\0" "TRUE\0" "TRUNK\0" "TRUST\0"
    "TRUTH\0" "TRY\0" "TUBE\0" "TUNE\0" "TURN\0" "TWIN\0" "TWIST\0" "TWO\0"
    // U
    "UGLY\0" "ULTRA\0" "UNDER\0" "UNIT\0" "UP\0" "UPPER\0" "URBAN\0" "USE\0"
    // V
    "VALE\0" "VALLEY\0" "VALUE\0" "VAN\0" "VAST\0" "VAULT\0" "VEIN\0" "VERSE\0"
    "VEST\0" "VIEW\0" "VINE\0" "VOICE\0" "VOID\0" "VOTE\0"
    // W
    "WADE\0" "WAGE\0" "WAIT\0" "WAKE\0" "WALK\0" "WALL\0" "WAND\0" "WANT\0"
    "WAR\0" "WARM\0" "WARN\0" "WARP\0" "WASH\0" "WASTE\0" "WATCH\0" "WATER\0"
    "WAVE\0" "WAX\0" "WAY\0" "WEAK\0" "WEALTH\0" "WEAR\0" "WEATHER\0" "WEB\0"
    "WEED\0" "WEEK\0" "WELL\0" "WEST\0" "WET\0" "WHEAT\0" "WHEEL\0" "WHITE\0"
    "WHOLE\0" "WIDE\0" "WIFE\0" "WILD\0" "WILL\0" "WIN\0" "WIND\0" "WINDOW\0"
    "WINE\0" "WING\0" "WINTER\0" "WIRE\0" "WISE\0" "WISH\0" "WITCH\0" "WOLF\0"
    "WOMAN\0" "WONDER\0" "WOOD\0" "WOOL\0" "WORD\0" "WORK\0" "WORLD\0" "WORM\0"
    "WORRY\0" "WORTH\0" "WOUND\0" "WRAP\0" "WRATH\0" "WRECK\0" "WRITE\0" "WRONG\0"
    // X
    "XRAY\0"
    // Y
    "YARD\0" "YARN\0" "YEAR\0" "YELL\0" "YES\0" "YET\0" "YIELD\0" "YOUNG\0"
    "YOUR\0" "YOUTH\0"
    // Z
    "ZACK\0" "ZEAL\0" "ZERO\0" "ZONE\0" "ZOO\0"
    "\0";  // End marker

// Index table: offset where each letter starts (A=0, B=1, ... Z=25, end=26)
// Generated by counting bytes in dictionary
const uint16_t T9_DICT_INDEX[] PROGMEM = {
    0,      // A
    96,     // B
    273,    // C
    480,    // D
    600,    // E
    678,    // F
    907,    // G
    1053,   // H
    1243,   // I
    1289,   // J
    1396,   // K
    1467,   // L
    1616,   // M
    1806,   // N
    1900,   // O
    1956,   // P
    2222,   // Q
    2252,   // R
    2480,   // S
    3220,   // T
    3563,   // U
    3607,   // V
    3677,   // W
    3957,   // X
    3962,   // Y
    4017,   // Z
    4046    // End
};

// Find predictions matching prefix (case insensitive)
// Returns number of matches found (0-maxResults)
uint8_t findPredictions(const char* prefix, const char** results, uint8_t maxResults) {
    if (!prefix || prefix[0] == '\0' || maxResults == 0) {
        return 0;
    }

    uint8_t count = 0;
    uint8_t prefixLen = strlen(prefix);

    // Get first letter index
    char firstChar = toupper(prefix[0]);
    if (firstChar < 'A' || firstChar > 'Z') {
        return 0;
    }
    uint8_t letterIdx = firstChar - 'A';

    // Get search range from index
    uint16_t startOffset = pgm_read_word(&T9_DICT_INDEX[letterIdx]);
    uint16_t endOffset = pgm_read_word(&T9_DICT_INDEX[letterIdx + 1]);

    // Search through words starting with this letter
    const char* dictPtr = T9_DICT + startOffset;
    const char* dictEnd = T9_DICT + endOffset;

    while (dictPtr < dictEnd && count < maxResults) {
        // Check if this word matches prefix
        bool matches = true;
        for (uint8_t i = 0; i < prefixLen && matches; i++) {
            char dictChar = platform_pgm_read_byte(dictPtr + i);
            if (dictChar == '\0' || toupper(prefix[i]) != dictChar) {
                matches = false;
            }
        }

        if (matches) {
            results[count++] = dictPtr;
        }

        // Skip to next word (find null terminator)
        while (platform_pgm_read_byte(dictPtr) != '\0') {
            dictPtr++;
        }
        dictPtr++;  // Skip the null terminator
    }

    return count;
}

// Helper to copy a dictionary word to a buffer
void copyDictWord(const char* dictPtr, char* buffer, uint8_t maxLen) {
    uint8_t i = 0;
    char c;
    while ((c = platform_pgm_read_byte(dictPtr + i)) != '\0' && i < maxLen - 1) {
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

#endif
