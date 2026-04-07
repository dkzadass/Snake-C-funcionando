/*
 * SNAKE INFERNO
 * Jogo da Cobra - Tema Fogo
 *
 * Compile: gcc snake_inferno.c -o snake_inferno.exe
 * Requer: PowerShell ou Windows Terminal (UTF-8)
 *
 * ARQUITETURA:
 *   main()        -> loop principal de estados
 *   input()       -> captura de teclas
 *   update()      -> logica do jogo (colisao, movimento, pontuacao)
 *   render()      -> desenho na tela (separado da logica)
 *
 * ESTRUTURAS DE DADOS:
 *   Lista encadeada dupla -> corpo da cobra (O(1) insercao/remocao)
 *   Tabela hash (array)   -> deteccao de colisao (O(1))
 *   Fila circular (array) -> obstaculos dinamicos (O(1) enqueue/dequeue)
 *
 * REQUISITOS FUNCIONAIS:
 *   RF01 - Controle por teclado (WASD / setas)
 *   RF02 - Crescimento ao comer fruta
 *   RF03 - Colisao com paredes encerra o jogo
 *   RF04 - Colisao com o proprio corpo encerra o jogo
 *   RF05 - Pontuacao e HUD em tempo real
 *   RF06 - Sistema de niveis (a cada 200 pontos)
 *   RF07 - Power-up a cada 150 pontos (frutas extras + animacao)
 *   RF08 - Diamante raro com spawn aleatorio (+50pts, 15s)
 *   RF09 - Menu, placar e reinicio sem fechar o programa
 *   RF10 - Obstaculos dinamicos
 *   RF11 - Combo multiplier: frutas consecutivas aumentam bonus de pontos
 *
 * REQUISITOS NAO FUNCIONAIS:
 *   RNF01 - Resposta < 200ms por frame
 *   RNF02 - Codigo modular com responsabilidades separadas
 *   RNF03 - Memoria dinamica sem leaks (malloc/free gerenciados)
 *   RNF04 - Colisao em O(1) via hash, sem percurso linear
 *   RNF05 - Renderizacao sem flickering (gotoxy seletivo)
 *   RNF06 - Fila circular com capacidade fixa, sem alocacao dinamica
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

/* ===================== DIMENSOES ===================== */
#define WIDTH          42
#define HEIGHT         22
#define TOP_OFFSET      6

#define ARENA_L         1
#define ARENA_R        (ARENA_L + WIDTH * 2)
#define PANEL_X        (ARENA_R + 3)
#define PANEL_W        22
#define CON_COLS       (PANEL_X + PANEL_W + 2)
#define CON_ROWS       36

/* ===================== JOGO ===================== */
#define FRUIT_BASE      4
#define FRUIT_EXTRA    10
#define FRUIT_TOTAL    (FRUIT_BASE + FRUIT_EXTRA)
#define MAX_SNAKE      900
#define MAX_RECORDS     5
#define RECORD_FILE    "ranking.txt"

#define SPEED_BASE     150
#define SPEED_MIN       45
#define SPEED_STEP      15
#define PTS_LEVEL      200
#define PTS_POWERUP    150
#define PTS_DIAMOND     50
#define PTS_APPLE       10
#define DIAMOND_TTL     15   /* segundos ate sumir */

/* ===================== OBSTACULOS (FILA CIRCULAR) ===================== */
#define OBS_CAP        60
#define OBS_PER_LEVEL   3

/* ===================== COMBO ===================== */
#define COMBO_MAX       8
#define COMBO_DECAY     5

/* ===================== EMOJIS UTF-8 ===================== */
#define SYM_HEAD       "\xF0\x9F\x94\xA5"
#define SYM_BODY       "\xF0\x9F\x9F\xA7"
#define SYM_BODY_PU    "\xF0\x9F\x9F\xA5"
#define SYM_APPLE      "\xF0\x9F\x8D\x96"
#define SYM_DIAMOND    "\xF0\x9F\x92\x9C"
#define SYM_POWERUP    "\xF0\x9F\x92\x80"
#define SYM_SKULL      "\xF0\x9F\x92\x80"
#define SYM_TROPHY     "\xF0\x9F\x8F\x86"
#define SYM_CROWN      "\xF0\x9F\x91\x91"
#define SYM_FIRE       "\xF0\x9F\x94\xA5"
#define SYM_CTRL       "\xF0\x9F\x8E\xAE"
#define SYM_SNAKE      "\xF0\x9F\x90\x89"
#define SYM_STAR       "\xE2\xAD\x90"
#define SYM_HEART      "\xE2\x9D\xA4"
#define SYM_ZAP        "\xE2\x9A\xA1"
#define SYM_ROCK       "\xF0\x9F\xAA\xA8"
#define SYM_COMBO      "\xF0\x9F\x94\xA5"
#define SYM_MULTI      "\xC3\x97"
#define BOX_TL   "\xE2\x94\x8C"
#define BOX_TR   "\xE2\x94\x90"
#define BOX_BL   "\xE2\x94\x94"
#define BOX_BR   "\xE2\x94\x98"
#define BOX_H    "\xE2\x94\x80"
#define BOX_V    "\xE2\x94\x82"
#define BOX_TLH  "\xE2\x95\x94"
#define BOX_TRH  "\xE2\x95\x97"
#define BOX_BLH  "\xE2\x95\x9A"
#define BOX_BRH  "\xE2\x95\x9D"
#define BOX_DH   "\xE2\x95\x90"
#define BOX_DV   "\xE2\x95\x91"
#define BOX_LM   "\xE2\x95\xA0"
#define BOX_RM   "\xE2\x95\xA3"
#define BLOCK    "\xE2\x96\x93"

/* ===================== CORES ===================== */
#define C_RESET     7
#define C_BORDER   12
#define C_PANEL    12
#define C_HDR      14
#define C_VAL      15
#define C_SCORE    14
#define C_LEVEL    12
#define C_SPEED    12
#define C_TITLE    12
#define C_SELECT   12
#define C_NORMAL    7
#define C_DIM       8
#define C_GAMEOVER 12
#define C_GOOD      6
#define C_FLASH    15
#define C_PU       13
#define C_FIRE     12

/* ===================== TIPOS ===================== */
typedef struct Node {
    int x, y;
    struct Node *next, *prev;
} Node;

typedef struct {
    int x, y;
    int type;
    int active;
} Fruit;

typedef struct {
    char name[32];
    int  score;
} Record;

typedef enum { DIR_UP=0, DIR_RIGHT, DIR_DOWN, DIR_LEFT } Direction;

typedef enum {
    ST_MENU=0, ST_PLAYING, ST_RANKING, ST_GAMEOVER, ST_EXIT
} State;

typedef struct { int x, y; } Pt;

/* ===================== FILA CIRCULAR - OBSTACULOS ===================== */
typedef struct {
    Pt  data[OBS_CAP];
    int head, tail, size;
} CircQueue;

CircQueue obsQueue;

int cqEnqueue(CircQueue *q, int x, int y) {
    if (q->size == OBS_CAP) return 0;
    q->data[q->tail].x = x;
    q->data[q->tail].y = y;
    q->tail = (q->tail + 1) % OBS_CAP;
    q->size++;
    return 1;
}

int cqDequeue(CircQueue *q, Pt *out) {
    if (q->size == 0) return 0;
    *out = q->data[q->head];
    q->head = (q->head + 1) % OBS_CAP;
    q->size--;
    return 1;
}

void cqClear(CircQueue *q) { q->head = q->tail = q->size = 0; }

/* ===================== GLOBAIS ===================== */
HANDLE hConsole;

Node   *snakeHead = NULL, *snakeTail = NULL;
int     snakeLen  = 0;

#define HASH_H (HEIGHT + TOP_OFFSET + 4)
int occupancy[HASH_H][WIDTH + 2];

Fruit fruits[FRUIT_TOTAL];
int   diamondActive = 0;
time_t diamondTime;

Direction dir     = DIR_RIGHT;
Direction nextDir = DIR_RIGHT;
int score         = 0;
int level         = 1;
int speedMs       = SPEED_BASE;
int gameOver      = 0;
int paused        = 0;
time_t startTime;
time_t pauseStart;
int    pausedSec  = 0;

int lastMilestonePU   = 0;
int lastMilestoneLVL  = 0;
int powerUpActive     = 0;
time_t powerUpStart;

/* --- Combo (RF11) --- */
int combo        = 0;
int comboDecay   = 0;
int comboPopX    = -1;   /* posicao X do popup de combo na tela */
int comboPopY    = -1;   /* posicao Y do popup de combo na tela */

/* --- Obstaculos (RF10) --- */
int lastMilestoneObs = 0;

int menuIndex    = 0;
int goIndex      = 0;
Record ranking[MAX_RECORDS];
int rankCount    = 0;

int needRedrawMenu   = 1;
int needRedrawPanel  = 1;
int needRedrawArena  = 1;

/* ===================== UTILITARIOS ===================== */
void setColor(int c) { SetConsoleTextAttribute(hConsole, c); }

void gotoxy(int x, int y) {
    COORD p = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hConsole, p);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(hConsole, &ci);
}

void showCursor() {
    CONSOLE_CURSOR_INFO ci = {1, TRUE};
    SetConsoleCursorInfo(hConsole, &ci);
}

void clearScreen() {
    COORD c = {0,0};
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole, ' ', size, c, &written);
    FillConsoleOutputAttribute(hConsole, C_RESET, size, c, &written);
    SetConsoleCursorPosition(hConsole, c);
}

void printCenter(int l, int r, int y, int color, const char *s) {
    int len = (int)strlen(s);
    int x   = l + (r - l - len) / 2;
    if (x < l) x = l;
    gotoxy(x, y);
    setColor(color);
    printf("%s", s);
}

/* ===================== LISTA ENCADEADA (COBRA) ===================== */

void insertHead(int x, int y) {
    Node *n = (Node*)malloc(sizeof(Node));
    if (!n) return;
    n->x = x; n->y = y;
    n->next = snakeHead; n->prev = NULL;
    if (snakeHead) snakeHead->prev = n;
    snakeHead = n;
    if (!snakeTail) snakeTail = n;
    occupancy[y][x] = 1;
    snakeLen++;
}

void removeTail() {
    if (!snakeTail) return;
    occupancy[snakeTail->y][snakeTail->x] = 0;
    Node *tmp = snakeTail;
    snakeTail = snakeTail->prev;
    if (snakeTail) snakeTail->next = NULL;
    else snakeHead = NULL;
    free(tmp);
    snakeLen--;
}

void freeSnake() {
    Node *t = snakeHead;
    while (t) {
        Node *nx = t->next;
        free(t);
        t = nx;
    }
    snakeHead = snakeTail = NULL;
    snakeLen = 0;
}

/* ===================== FRUTAS ===================== */

int arenaX(int gx) { return ARENA_L + gx * 2; }
int arenaY(int gy) { return TOP_OFFSET + 1 + gy; }

void eraseFruitAt(int gx, int gy) {
    gotoxy(arenaX(gx), arenaY(gy));
    printf("  ");
}

void drawFruit(int i) {
    if (!fruits[i].active) return;
    gotoxy(arenaX(fruits[i].x), arenaY(fruits[i].y));
    if (fruits[i].type == 1) { setColor(12); printf("%s", SYM_APPLE);   }
    if (fruits[i].type == 2) { setColor(8);  printf("%s", SYM_POWERUP); }
    if (fruits[i].type == 3) { setColor(13); printf("%s", SYM_DIAMOND); }
    setColor(C_RESET);
}

void spawnFruit(int i, int type) {
    int x, y, tries = 0;
    do {
        x = rand() % WIDTH;
        y = rand() % HEIGHT;
        if (++tries > 2000) return;
    } while (occupancy[arenaY(y)][x] != 0 ||
             occupancy[TOP_OFFSET + 1 + y][x] != 0);

    fruits[i].x = x; fruits[i].y = y;
    fruits[i].type = type; fruits[i].active = 1;
    occupancy[arenaY(y)][x] = type + 1;
    drawFruit(i);
}

void removeFruitSlot(int i) {
    if (!fruits[i].active) return;
    occupancy[arenaY(fruits[i].y)][fruits[i].x] = 0;
    eraseFruitAt(fruits[i].x, fruits[i].y);
    fruits[i].active = 0;
}

void spawnInitialFruits() {
    for (int i = 0; i < FRUIT_BASE; i++) spawnFruit(i, 1);
}

/* ===================== RENDER: HEADER ===================== */

void drawHeader() {
    int aw = WIDTH * 2 + 2;
    int L  = ARENA_L - 1;
    int R  = L + aw - 1;

    setColor(C_FIRE);
    gotoxy(L, 0);
    printf("%s", BOX_TLH);
    for (int i = 0; i < aw - 2; i++) printf("%s", BOX_DH);
    printf("%s", BOX_TRH);

    gotoxy(L, 1); setColor(C_FIRE); printf("%s", BOX_DV);
    setColor(15);
    { const char *s = "Aluno: RAFAEL DE OLIVEIRA FERREIRA";
      int x = L + 1 + (aw - 2 - (int)strlen(s)) / 2;
      gotoxy(x, 1); printf("%s", s); }
    gotoxy(R, 1); setColor(C_FIRE); printf("%s", BOX_DV);

    gotoxy(L, 2); setColor(C_FIRE); printf("%s", BOX_DV);
    setColor(14);
    { const char *s = "Materia: ESTRUTURA DE DADOS";
      int x = L + 1 + (aw - 2 - (int)strlen(s)) / 2;
      gotoxy(x, 2); printf("%s", s); }
    gotoxy(R, 2); setColor(C_FIRE); printf("%s", BOX_DV);

    gotoxy(L, 3); setColor(C_FIRE); printf("%s", BOX_DV);
    gotoxy(R, 3); setColor(C_FIRE); printf("%s", BOX_DV);
    gotoxy(L, 4); setColor(C_FIRE); printf("%s", BOX_DV);
    gotoxy(R, 4); setColor(C_FIRE); printf("%s", BOX_DV);

    setColor(C_FIRE);
    gotoxy(L, 5);
    printf("%s", BOX_BLH);
    for (int i = 0; i < aw - 2; i++) printf("%s", BOX_DH);
    printf("%s", BOX_BRH);

    setColor(C_RESET);
}

/* ===================== OBSTACULOS ===================== */

void drawObstacle(int gx, int gy) {
    gotoxy(arenaX(gx), arenaY(gy));
    setColor(8);
    printf("%s", SYM_ROCK);
    setColor(C_RESET);
}

void eraseObstacle(int gx, int gy) {
    gotoxy(arenaX(gx), arenaY(gy));
    printf("  ");
    occupancy[arenaY(gy)][gx] = 0;
}

void spawnObstacles(int count) {
    for (int k = 0; k < count; k++) {
        if (obsQueue.size == OBS_CAP) {
            Pt old;
            if (cqDequeue(&obsQueue, &old))
                eraseObstacle(old.x, old.y);
        }

        int x, y, tries = 0;
        do {
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
            if (++tries > 3000) return;
        } while (occupancy[arenaY(y)][x] != 0);

        occupancy[arenaY(y)][x] = 5;
        cqEnqueue(&obsQueue, x, y);
        drawObstacle(x, y);
    }
}

void clearAllObstacles() {
    Pt p;
    while (cqDequeue(&obsQueue, &p))
        eraseObstacle(p.x, p.y);
}

void redrawAllObstacles() {
    for (int i = 0; i < obsQueue.size; i++) {
        int idx = (obsQueue.head + i) % OBS_CAP;
        drawObstacle(obsQueue.data[idx].x, obsQueue.data[idx].y);
    }
}

/* ===================== RENDER: ARENA ===================== */

void drawBorder() {
    setColor(C_FIRE);

    gotoxy(ARENA_L - 1, TOP_OFFSET);
    printf("%s", BOX_TLH);
    for (int i = 0; i < WIDTH; i++) printf("%s%s", BOX_DH, BOX_DH);
    printf("%s", BOX_TRH);

    for (int y = 1; y <= HEIGHT; y++) {
        gotoxy(ARENA_L - 1, TOP_OFFSET + y);
        printf("%s", BOX_DV);
        gotoxy(ARENA_L + WIDTH * 2, TOP_OFFSET + y);
        printf("%s", BOX_DV);
    }

    gotoxy(ARENA_L - 1, TOP_OFFSET + HEIGHT + 1);
    printf("%s", BOX_BLH);
    for (int i = 0; i < WIDTH; i++) printf("%s%s", BOX_DH, BOX_DH);
    printf("%s", BOX_BRH);

    setColor(C_RESET);
}

void renderSnake() {
    Node *cur = snakeHead;
    int   i   = 0;
    while (cur) {
        gotoxy(arenaX(cur->x), cur->y);
        if (i == 0) {
            setColor(C_FIRE);
            printf("%s", SYM_HEAD);
        } else {
            if (powerUpActive) {
                int ciclo = (i + (int)(clock() / 120)) % 3;
                setColor(ciclo == 0 ? 12 : ciclo == 1 ? 14 : 6);
                printf("%s", SYM_BODY_PU);
            } else {
                setColor(i < 4 ? 6 : 4);
                printf("%s", SYM_BODY);
            }
        }
        cur = cur->next;
        i++;
    }
    setColor(C_RESET);
}

/* ===================== RENDER: PAINEL LATERAL ===================== */

void drawPanelBox() {
    setColor(C_PANEL);

    gotoxy(PANEL_X, TOP_OFFSET);
    printf("%s", BOX_TLH);
    for (int i = 0; i < PANEL_W; i++) printf("%s", BOX_DH);
    printf("%s", BOX_TRH);

    for (int y = TOP_OFFSET + 1; y <= TOP_OFFSET + HEIGHT; y++) {
        gotoxy(PANEL_X, y);           printf("%s", BOX_DV);
        gotoxy(PANEL_X + PANEL_W + 1, y); printf("%s", BOX_DV);
    }

    gotoxy(PANEL_X, TOP_OFFSET + HEIGHT + 1);
    printf("%s", BOX_BLH);
    for (int i = 0; i < PANEL_W; i++) printf("%s", BOX_DH);
    printf("%s", BOX_BRH);

    gotoxy(PANEL_X, TOP_OFFSET + 2);
    printf("%s", BOX_LM);
    for (int i = 0; i < PANEL_W; i++) printf("%s", BOX_DH);
    printf("%s", BOX_RM);

    setColor(C_RESET);
}

void updatePanel() {
    int px = PANEL_X + 2;
    int py = TOP_OFFSET + 1;

    setColor(C_FIRE);
    gotoxy(PANEL_X + 3, py); printf("%s  SNAKE  INFERNO  %s", SYM_FIRE, SYM_FIRE);

    py = TOP_OFFSET + 3;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "PONTOS");
    setColor(C_SCORE);gotoxy(px+12, py);printf("%-6d", score);
    py++;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "NIVEL");
    setColor(C_LEVEL);gotoxy(px+12, py);printf("%-6d", level);
    py++;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "VELOCIDADE");
    setColor(C_SPEED);gotoxy(px+12, py);printf("%-6dms", speedMs);
    py++;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "COBRA");
    setColor(C_VAL);  gotoxy(px+12, py);printf("%-6d", snakeLen);
    py++;

    time_t elapsed = time(NULL) - startTime - pausedSec;
    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "TEMPO");
    setColor(C_VAL);  gotoxy(px+12, py);printf("%02d:%02d ", (int)elapsed/60, (int)elapsed%60);
    py += 2;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "MODO");
    setColor(C_GOOD); gotoxy(px+12, py); printf("%s MANUAL", SYM_CTRL);
    py += 2;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "COMBO");
    if (combo > 1) {
        setColor(combo >= 6 ? 12 : combo >= 4 ? 14 : 6);
        gotoxy(px+12, py);
        printf("%s x%d     ", SYM_COMBO, combo);
    } else {
        setColor(C_DIM); gotoxy(px+12, py); printf("---     ");
    }
    py++;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "ROCHAS");
    setColor(8);      gotoxy(px+12, py); printf("%s %-4d  ", SYM_ROCK, obsQueue.size);
    py++;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "POWER-UP");
    if (powerUpActive) {
        int rem = 10 - (int)difftime(time(NULL), powerUpStart);
        setColor(C_PU); gotoxy(px+12, py); printf("%s %ds  ", SYM_ZAP, rem > 0 ? rem : 0);
    } else {
        setColor(C_DIM); gotoxy(px+12, py); printf("OFF     ");
    }
    py++;

    setColor(C_DIM);  gotoxy(px, py);   printf("%-12s", "BONUS");
    if (diamondActive) {
        int rem = DIAMOND_TTL - (int)difftime(time(NULL), diamondTime);
        setColor(13); gotoxy(px+12, py); printf("%s %ds  ", SYM_DIAMOND, rem > 0 ? rem : 0);
    } else {
        setColor(C_DIM); gotoxy(px+12, py); printf("---     ");
    }
    py += 2;

    setColor(C_DIM);
    gotoxy(px, py++); printf("P: pausa            ");
    gotoxy(px, py++); printf("Q: menu             ");
    gotoxy(px, py++); printf("WASD / setas: mover ");

    py++;
    int proxPU  = PTS_POWERUP  - (score % PTS_POWERUP);
    int proxLVL = PTS_LEVEL    - (score % PTS_LEVEL);
    setColor(C_DIM);
    gotoxy(px, py++); printf("%-13s %s+%d  ", "Prox. PU:", SYM_STAR, proxPU);
    gotoxy(px, py++); printf("%-13s %s+%d  ", "Prox. niv:", SYM_ZAP, proxLVL);
}

/* ===================== RENDER: MENU ===================== */

void renderMenu() {
    if (!needRedrawMenu) return;
    clearScreen();

    int midX = CON_COLS / 2;

    setColor(C_FIRE);
    gotoxy(4, 1);
    for (int i = 0; i < CON_COLS - 8; i++) printf("%s", BOX_DH);

    setColor(14);
    gotoxy(midX - 18, 2);
    printf("%s  S N A K E   I N F E R N O  %s", SYM_FIRE, SYM_FIRE);

    setColor(C_DIM);
    gotoxy(midX - 15, 3);
    printf("Power-Ups  %s  Bonus  %s  Ranking  %s  Desafio",
           SYM_SKULL, SYM_DIAMOND, SYM_TROPHY);

    setColor(C_FIRE);
    gotoxy(4, 4);
    for (int i = 0; i < CON_COLS - 8; i++) printf("%s", BOX_DH);

    int ML = midX - 16, MR = midX + 16;
    int MT = 7, MB = 16;

    setColor(C_BORDER);
    gotoxy(ML, MT); printf("%s", BOX_TLH);
    for (int i = ML+1; i < MR; i++) { gotoxy(i, MT); printf("%s", BOX_DH); }
    gotoxy(MR, MT); printf("%s", BOX_TRH);

    gotoxy(ML, MT+1); printf("%s", BOX_LM);
    for (int i = ML+1; i < MR; i++) { gotoxy(i, MT+1); printf("%s", BOX_DH); }
    gotoxy(MR, MT+1); printf("%s", BOX_RM);

    for (int y = MT+2; y < MB; y++) {
        gotoxy(ML, y); printf("%s", BOX_DV);
        gotoxy(MR, y); printf("%s", BOX_DV);
    }

    gotoxy(ML, MB); printf("%s", BOX_BLH);
    for (int i = ML+1; i < MR; i++) { gotoxy(i, MB); printf("%s", BOX_DH); }
    gotoxy(MR, MB); printf("%s", BOX_BRH);

    setColor(C_HDR);
    gotoxy(ML + 5, MT); printf(" %s MENU PRINCIPAL %s ", SYM_CROWN, SYM_CROWN);

    const char *labels[] = {
        "      JOGAR      ",
        "    RANKING      ",
        "      SAIR       "
    };
    int cols[] = { C_FIRE, C_HDR, C_DIM };

    for (int i = 0; i < 3; i++) {
        int oy = MT + 3 + i * 2;
        if (i == menuIndex) {
            setColor(cols[i]);
            gotoxy(ML + 2, oy);
            printf("%s [ %s ] %s", SYM_FIRE, labels[i], SYM_FIRE);
        } else {
            setColor(C_DIM);
            gotoxy(ML + 5, oy);
            printf("  %s  ", labels[i]);
        }
    }

    setColor(C_DIM);
    gotoxy(midX - 16, MB + 2);
    printf("Use W/S ou setas para navegar  |  ENTER para confirmar");

    setColor(C_RESET);
    needRedrawMenu = 0;
}

/* ===================== RENDER: RANKING ===================== */

void loadRanking() {
    rankCount = 0;
    FILE *f = fopen(RECORD_FILE, "r");
    if (!f) return;
    while (rankCount < MAX_RECORDS &&
           fscanf(f, "%31s %d", ranking[rankCount].name, &ranking[rankCount].score) == 2)
        rankCount++;
    fclose(f);
}

void saveRanking(const char *name, int pts) {
    loadRanking();
    if (rankCount < MAX_RECORDS) {
        strncpy(ranking[rankCount].name, name, 31);
        ranking[rankCount].score = pts;
        rankCount++;
    } else if (pts > ranking[rankCount-1].score) {
        strncpy(ranking[rankCount-1].name, name, 31);
        ranking[rankCount-1].score = pts;
    } else return;

    for (int i = 0; i < rankCount-1; i++)
        for (int j = 0; j < rankCount-i-1; j++)
            if (ranking[j].score < ranking[j+1].score) {
                Record tmp = ranking[j]; ranking[j] = ranking[j+1]; ranking[j+1] = tmp;
            }

    FILE *f = fopen(RECORD_FILE, "w");
    if (f) {
        for (int i = 0; i < rankCount; i++)
            fprintf(f, "%s %d\n", ranking[i].name, ranking[i].score);
        fclose(f);
    }
}

void renderRanking() {
    clearScreen();
    int midX = CON_COLS / 2;

    setColor(14);
    gotoxy(midX - 14, 2);
    printf("%s  TOP %d  SNAKE INFERNO  %s", SYM_TROPHY, MAX_RECORDS, SYM_TROPHY);

    setColor(C_FIRE);
    gotoxy(8, 4);
    for (int i = 0; i < CON_COLS - 16; i++) printf("%s", BOX_DH);

    loadRanking();
    const char *medals[] = { "\xF0\x9F\xA5\x87", "\xF0\x9F\xA5\x88", "\xE2\xAD\x90" };
    int colors[] = { 14, 7, 6, 8, 8 };

    for (int i = 0; i < rankCount; i++) {
        gotoxy(midX - 14, 7 + i * 2);
        setColor(colors[i < 5 ? i : 4]);
        if (i < 3) printf("%s  ", medals[i]);
        else printf("   %d.  ", i+1);
        printf("%-20s  %d pts", ranking[i].name, ranking[i].score);
    }
    if (rankCount == 0) {
        setColor(C_DIM);
        gotoxy(midX - 10, 9);
        printf("Nenhum recorde ainda...");
    }

    setColor(C_DIM);
    gotoxy(midX - 14, 20);
    printf("Pressione ENTER ou ESC para voltar ao menu");
    setColor(C_RESET);
}

/* ===================== RENDER: GAME OVER ===================== */

void renderGameOver() {
    clearScreen();
    int midX = CON_COLS / 2;

    setColor(C_GAMEOVER);
    gotoxy(midX - 12, 4);
    printf("%s  FIM DE JOGO  %s", SYM_SKULL, SYM_SKULL);

    setColor(C_SCORE);
    gotoxy(midX - 8, 6);  printf("Pontuacao:  %d", score);
    gotoxy(midX - 8, 7);  printf("Nivel:      %d", level);
    gotoxy(midX - 8, 8);  printf("Tamanho:    %d", snakeLen);

    time_t elapsed = time(NULL) - startTime - pausedSec;
    gotoxy(midX - 8, 9);  printf("Tempo:      %02d:%02d", (int)elapsed/60, (int)elapsed%60);

    const char *opts[] = { "JOGAR NOVAMENTE", "VOLTAR AO MENU" };
    for (int i = 0; i < 2; i++) {
        int oy = 12 + i * 2;
        if (i == goIndex) { setColor(C_SELECT); gotoxy(midX-10, oy); printf("%s [ %s ] %s", SYM_FIRE, opts[i], SYM_FIRE); }
        else              { setColor(C_DIM);    gotoxy(midX-8,  oy); printf("  %s  ", opts[i]); }
    }

    setColor(C_DIM);
    gotoxy(midX - 16, 17);
    printf("Digite seu nome para salvar no ranking:");
    setColor(C_RESET);
}

/* ===================== INIT DO JOGO ===================== */

void initGame() {
    freeSnake();
    memset(occupancy, 0, sizeof(occupancy));
    memset(fruits, 0, sizeof(fruits));

    score = 0; level = 1; speedMs = SPEED_BASE;
    gameOver = 0; paused = 0;
    dir = DIR_RIGHT; nextDir = DIR_RIGHT;
    powerUpActive = 0; diamondActive = 0;
    lastMilestonePU = 0; lastMilestoneLVL = 0;
    pausedSec = 0;
    combo = 0; comboDecay = 0;
    comboPopX = -1; comboPopY = -1;   /* reseta popup de combo */
    lastMilestoneObs = 0;
    cqClear(&obsQueue);
    startTime = time(NULL);

    int sx = WIDTH / 2, sy = TOP_OFFSET + HEIGHT / 2;
    insertHead(sx, sy);

    spawnInitialFruits();

    clearScreen();
    drawHeader();
    drawBorder();
    drawPanelBox();
    renderSnake();
    for (int i = 0; i < FRUIT_TOTAL; i++) drawFruit(i);
    updatePanel();
}

/* ===================== UPDATE ===================== */

void update() {
    if (paused || gameOver) return;

    /* Apaga popup anterior do combo antes de qualquer coisa */
    if (comboPopX >= 0) {
        gotoxy(comboPopX, comboPopY);
        printf("    ");
        comboPopX = comboPopY = -1;
    }

    if (!((nextDir == DIR_UP    && dir == DIR_DOWN)  ||
          (nextDir == DIR_DOWN  && dir == DIR_UP)    ||
          (nextDir == DIR_LEFT  && dir == DIR_RIGHT) ||
          (nextDir == DIR_RIGHT && dir == DIR_LEFT)))
        dir = nextDir;

    int nx = snakeHead->x, ny = snakeHead->y;
    if      (dir == DIR_UP)    ny--;
    else if (dir == DIR_DOWN)  ny++;
    else if (dir == DIR_LEFT)  nx--;
    else                       nx++;

    if (nx < 0 || nx >= WIDTH ||
        ny <= TOP_OFFSET || ny >= TOP_OFFSET + HEIGHT + 1) {
        gameOver = 1; return;
    }

    if (occupancy[ny][nx] == 1) { gameOver = 1; return; }
    if (occupancy[ny][nx] == 5) { gameOver = 1; return; }

    int cell = occupancy[ny][nx];
    int ate  = 0;

    if (cell >= 2 && cell != 5) {
        for (int i = 0; i < FRUIT_TOTAL; i++) {
            if (!fruits[i].active) continue;
            if (fruits[i].x == nx && arenaY(fruits[i].y) == ny) {
                int t = fruits[i].type;
                removeFruitSlot(i);

                if (t == 1) {
                    combo++;
                    if (combo > COMBO_MAX) combo = COMBO_MAX;
                    comboDecay = 0;
                    int pts = PTS_APPLE * (combo > 1 ? combo : 1);
                    score += pts;
                    ate = 1;
                    spawnFruit(i, 1);
                } else if (t == 2) {
                    ate = 1;
                    combo = 0; comboDecay = 0;
                    powerUpActive = 1;
                    powerUpStart  = time(NULL);
                    for (int j = FRUIT_BASE; j < FRUIT_TOTAL; j++)
                        if (!fruits[j].active) spawnFruit(j, 1);
                } else if (t == 3) {
                    score += PTS_DIAMOND * (combo > 1 ? combo : 1);
                    ate = 1;
                    diamondActive = 0;
                }
                break;
            }
        }
        if (!ate) { ate = 1; }
    }

    if (!ate) {
        comboDecay++;
        if (comboDecay >= COMBO_DECAY) {
            if (combo > 0) combo--;
            comboDecay = 0;
        }
    }

    if (powerUpActive && difftime(time(NULL), powerUpStart) >= 10) {
        powerUpActive = 0;
        for (int i = FRUIT_BASE; i < FRUIT_TOTAL; i++) removeFruitSlot(i);
    }

    if (diamondActive && difftime(time(NULL), diamondTime) >= DIAMOND_TTL) {
        for (int i = 0; i < FRUIT_TOTAL; i++) {
            if (fruits[i].active && fruits[i].type == 3) {
                removeFruitSlot(i); break;
            }
        }
        diamondActive = 0;
    }

    if (score > 0 && score / PTS_POWERUP > lastMilestonePU) {
        lastMilestonePU = score / PTS_POWERUP;
        if (!powerUpActive) {
            for (int i = 0; i < FRUIT_TOTAL; i++)
                if (!fruits[i].active) { spawnFruit(i, 2); break; }
        }
    }

    if (score / PTS_LEVEL > lastMilestoneLVL) {
        lastMilestoneLVL = score / PTS_LEVEL;
        level++;
        if (speedMs > SPEED_MIN) speedMs -= SPEED_STEP;
    }

    if (level > lastMilestoneObs) {
        lastMilestoneObs = level;
        spawnObstacles(OBS_PER_LEVEL * (level - 1));
    }

    if (!diamondActive && (rand() % 400) == 0) {
        for (int i = 0; i < FRUIT_TOTAL; i++) {
            if (!fruits[i].active) {
                spawnFruit(i, 3);
                diamondActive = 1;
                diamondTime   = time(NULL);
                break;
            }
        }
    }

    if (!ate && snakeTail) {
        gotoxy(arenaX(snakeTail->x), snakeTail->y);
        printf("  ");
    }

    insertHead(nx, ny);
    if (!ate) removeTail();

    /* Exibe novo popup de combo e salva posicao para apagar no proximo frame */
    if (ate && combo > 1) {
        comboPopX = arenaX(nx) - 2;
        comboPopY = ny - 1;
        if (comboPopY < TOP_OFFSET + 1) comboPopY = ny + 1;
        setColor(combo >= 6 ? 12 : combo >= 4 ? 14 : 6);
        gotoxy(comboPopX, comboPopY);
        printf("x%d!", combo);
    }

    renderSnake();
    updatePanel();
}

/* ===================== INPUT ===================== */

void inputMenu(State *st) {
    if (!_kbhit()) return;
    int k = _getch();
    if (k == 0 || k == 224) {
        k = _getch();
        if (k == 72) { menuIndex--; if (menuIndex < 0) menuIndex = 2; needRedrawMenu = 1; }
        if (k == 80) { menuIndex++; if (menuIndex > 2) menuIndex = 0; needRedrawMenu = 1; }
        return;
    }
    if (k=='w'||k=='W') { menuIndex--; if (menuIndex<0) menuIndex=2; needRedrawMenu=1; }
    if (k=='s'||k=='S') { menuIndex++; if (menuIndex>2) menuIndex=0; needRedrawMenu=1; }
    if (k==13) {
        if      (menuIndex==0) { initGame(); *st=ST_PLAYING; }
        else if (menuIndex==1) { *st=ST_RANKING; }
        else                   { *st=ST_EXIT; }
    }
    if (k==27) *st=ST_EXIT;
}

void inputGame(State *st) {
    if (!_kbhit()) return;
    int k = _getch();

    if (k==0||k==224) {
        k = _getch();
        if (k==72) nextDir=DIR_UP;
        if (k==77) nextDir=DIR_RIGHT;
        if (k==80) nextDir=DIR_DOWN;
        if (k==75) nextDir=DIR_LEFT;
        return;
    }

    if (k=='w'||k=='W') nextDir=DIR_UP;
    if (k=='d'||k=='D') nextDir=DIR_RIGHT;
    if (k=='s'||k=='S') nextDir=DIR_DOWN;
    if (k=='a'||k=='A') nextDir=DIR_LEFT;

    if (k=='p'||k=='P') {
        if (!paused) { paused=1; pauseStart=time(NULL); }
        else { paused=0; pausedSec+=(int)difftime(time(NULL),pauseStart); }
        updatePanel();
    }
    if (k=='q'||k=='Q') { needRedrawMenu=1; *st=ST_MENU; }
}

void inputRanking(State *st) {
    if (!_kbhit()) return;
    int k = _getch();
    if (k==13||k==27||k=='m'||k=='M') { needRedrawMenu=1; *st=ST_MENU; }
}

void inputGameOver(State *st) {
    if (!_kbhit()) return;
    int k = _getch();
    if (k==0||k==224) { k=_getch(); if(k==72)goIndex=0; if(k==80)goIndex=1; renderGameOver(); return; }
    if (k=='w'||k=='W') { goIndex=0; renderGameOver(); }
    if (k=='s'||k=='S') { goIndex=1; renderGameOver(); }
    if (k==13) {
        if (goIndex==0) { initGame(); *st=ST_PLAYING; }
        else            { needRedrawMenu=1; *st=ST_MENU; }
    }
}

/* ===================== MAIN ===================== */

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD bufSize = { (SHORT)CON_COLS, (SHORT)CON_ROWS };
    SetConsoleScreenBufferSize(hConsole, bufSize);
    SMALL_RECT wr = { 0, 0, (SHORT)(CON_COLS-1), (SHORT)(CON_ROWS-1) };
    SetConsoleWindowInfo(hConsole, TRUE, &wr);
    SetConsoleTitleA("SNAKE INFERNO");

    srand((unsigned int)time(NULL));
    hideCursor();

    State state = ST_MENU;

    while (state != ST_EXIT) {

        if (state == ST_MENU) {
            renderMenu();
            inputMenu(&state);
            Sleep(30);
        }

        else if (state == ST_RANKING) {
            renderRanking();
            inputRanking(&state);
            Sleep(50);
        }

        else if (state == ST_PLAYING) {
            inputGame(&state);
            if (state != ST_PLAYING) continue;

            if (!paused) {
                update();

                if (gameOver) {
                    for (int f = 0; f < 4; f++) {
                        setColor(f%2==0 ? C_GAMEOVER : C_FLASH);
                        Node *cur = snakeHead;
                        while (cur) {
                            gotoxy(arenaX(cur->x), cur->y);
                            printf("%s", SYM_FIRE);
                            cur = cur->next;
                        }
                        Sleep(120);
                    }

                    renderGameOver();
                    showCursor();
                    char name[32] = "Jogador";
                    gotoxy(CON_COLS/2 - 10, 19);
                    setColor(C_VAL);
                    fflush(stdout);
                    if (fgets(name, sizeof(name), stdin)) {
                        int l = (int)strlen(name);
                        if (l > 0 && name[l-1]=='\n') name[l-1]='\0';
                    }
                    if (strlen(name)==0) strcpy(name,"Jogador");
                    hideCursor();

                    saveRanking(name, score);
                    goIndex = 0;
                    state = ST_GAMEOVER;
                } else {
                    Sleep(speedMs);
                }
            } else {
                setColor(C_FIRE);
                gotoxy(ARENA_L + WIDTH - 4, TOP_OFFSET + HEIGHT/2);
                printf("  PAUSADO  ");
                Sleep(50);
            }
        }

        else if (state == ST_GAMEOVER) {
            renderGameOver();
            inputGameOver(&state);
            Sleep(50);
        }
    }

    showCursor();
    clearScreen();
    setColor(C_RESET);
    return 0;
}
