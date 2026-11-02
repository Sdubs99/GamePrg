/*
 * ================================================================
 *  콘솔 바둑 & 오목  v2.0
 * ----------------------------------------------------------------
 *  원본 : 19x19 콘솔 바둑판 착수 프로그램
 *  환경 : Windows 콘솔 (Visual Studio / MinGW, Dev-C++)
 *  주의 : 소스 파일은 CP949(EUC-KR)로 저장되어 있어야 하며,
 *         기본 콘솔 창(conhost)에서 실행해야 판이 정확히 보입니다.
 * ================================================================
 */
#define _CRT_SECURE_NO_WARNINGS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600          /* SetCurrentConsoleFontEx 사용 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <wchar.h>
#include <conio.h>
#include <windows.h>

/* ================================================================
 *  상수 정의 (원본의 매직 넘버 정리)
 * ================================================================ */
#define N             19            /* 판 크기 19 x 19 */
#define EMPTY         0
#define BLACK         1
#define WHITE         2
#define OPP(c)        (3 - (c))     /* 상대 색 */

#define MODE_OMOK     1
#define MODE_GO       2
#define KOMI          6.5           /* 바둑 덤 (백) */

/* 화면 배치 */
#define SCREEN_COLS   90
#define SCREEN_LINES  32
#define BOARD_X       5             /* 판 왼쪽 위 교차점의 화면 열 */
#define BOARD_Y       3             /* 판 왼쪽 위 교차점의 화면 행 */
#define PANEL_X       49            /* 오른쪽 정보 패널 시작 열 */
#define PANEL_W       40
#define MSG_Y         25            /* 안내 메시지 행 */

/* 키 코드 (get_key 에서 정규화) */
#define KEY_ENTER     13
#define KEY_ESC       27
#define KEY_SPACE     32
#define KEY_UP        1001
#define KEY_DOWN      1002
#define KEY_LEFT      1003
#define KEY_RIGHT     1004

/* 색상 (상위 4비트 = 배경, 하위 4비트 = 글자) */
#define COL_DEFAULT   0x07
#define COL_BOARD     0x60          /* 황토색 판 + 검은 선 */
#define FG_BLACK      0x00
#define FG_WHITE      0x0F
#define BG_BOARD      0x60
#define BG_WIN        0xA0          /* 승리한 5목 하이라이트 (연두) */
#define BG_CURSOR     0xC0          /* 돌 위의 커서 (빨강) */
#define COL_LABEL     0x0E
#define COL_TITLE     0x0B
#define COL_WARN      0x0C
#define COL_INFO      0x0E
#define COL_DIM       0x08
#define COL_SELECT    0x70

/* 착수 오류 */
#define ERR_NONE      0
#define ERR_OCCUPIED  1
#define ERR_KO        2
#define ERR_SUICIDE   3

/* 게임 종료 후 행동 */
#define ACT_RESTART   0
#define ACT_MENU      1
#define ACT_QUIT      2

#define MAX_HISTORY   1024

/* ================================================================
 *  자료구조
 * ================================================================ */
typedef struct {
    int board[N][N];        /* board[y][x] = EMPTY / BLACK / WHITE */
    int turn;               /* 현재 차례 */
    int moveNo;             /* 지금까지 진행된 수 (패스 포함) */
    int captured[3];        /* captured[c] = c 가 따낸 돌 수 */
    int lastX, lastY;       /* 마지막 착수 위치 (-1 = 없음) */
    int koX, koY;           /* 패로 금지된 자리 (-1 = 없음) */
    int passCount;          /* 연속 패스 횟수 */
} GameState;

typedef struct {
    int color;
    int x, y;               /* x == -1 이면 패스 */
} Move;

/* ================================================================
 *  전역 변수
 * ================================================================ */
static HANDLE    hOut;
static GameState g;
static GameState history[MAX_HISTORY];   /* 무르기용 상태 스택 */
static Move      kifu[MAX_HISTORY];      /* 기보 */
static int       histCount;

static int  gameMode;
static int  curX, curY;
static int  cursorVisible;
static int  winMark[N][N];
static int  shown[N][N];                 /* 화면에 그려진 셀 상태 캐시 */

static int  gameOver;
static int  winner;                      /* BLACK / WHITE / 0(무승부) */
static char resultReason[96];
static char resultDetail[2][96];

static int  wins[3], draws;              /* 전적 */

static const char *COL_NAME = "ABCDEFGHJKLMNOPQRST";   /* 바둑 관례상 I 생략 */
static const int DX[4] = { 1, -1, 0, 0 };
static const int DY[4] = { 0, 0, 1, -1 };

/* ================================================================
 *  함수 원형
 * ================================================================ */
/* 콘솔 */
void gotoxy(int x, int y);
void set_color(int color);
void hide_cursor(void);
void show_cursor(void);
void clear_screen(void);
void setup_console(void);
int  get_key(void);
/* 소리 */
void snd_place(void);
void snd_error(void);
void snd_select(void);
void snd_win(void);
void snd_draw(void);
/* 화면 */
void show_intro(void);
int  main_menu(void);
void show_help(void);
int  select_menu(int x, int y, const char *items[], int count);
void draw_labels(void);
void draw_cell(int x, int y, int v);
void refresh_board(void);
void invalidate_board(void);
void redraw_all(void);
void draw_panel(void);
void clear_panel(void);
void panel_line(int row, const char *fmt, ...);
void show_msg(int color, const char *fmt, ...);
void clear_msg(void);
int  confirm(const char *question);
/* 게임 */
void init_game(int mode);
int  play_game(int mode);
int  play_omok(int x, int y);
int  play_go(int x, int y);
void do_pass(void);
void do_undo(void);
int  check_omok(int x, int y);
int  collect_group(int x, int y, int gx[], int gy[], int *size);
void score_go(void);
int  show_result(void);
void push_history(const GameState *s, int color, int x, int y);
int  save_kifu(char *outName);
void coord_str(int x, int y, char *buf);
const char *color_name(int c);

/* ================================================================
 *  main
 * ================================================================ */
int main(void)
{
    int choice, act = ACT_MENU;

    setup_console();
    show_intro();

    while (1) {
        choice = main_menu();
        if (choice == 0 || choice == 1) {
            int mode = (choice == 0) ? MODE_OMOK : MODE_GO;
            do {
                act = play_game(mode);
            } while (act == ACT_RESTART);
            if (act == ACT_QUIT) break;
        }
        else if (choice == 2) {
            show_help();
        }
        else {
            break;
        }
    }

    clear_screen();
    gotoxy(1, 1);
    printf("게임을 종료합니다. 플레이해 주셔서 감사합니다!\n");
    printf("최종 전적 : 흑 %d승 / 백 %d승 / 무 %d\n", wins[BLACK], wins[WHITE], draws);
    show_cursor();
    return 0;
}

/* ================================================================
 *  콘솔 제어
 * ================================================================ */
void gotoxy(int x, int y)                /* 원본과 같이 1부터 시작하는 좌표 */
{
    COORD pos;
    pos.X = (SHORT)(x - 1);
    pos.Y = (SHORT)(y - 1);
    SetConsoleCursorPosition(hOut, pos);
}

void set_color(int color)
{
    SetConsoleTextAttribute(hOut, (WORD)color);
}

void hide_cursor(void)
{
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 1;
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &ci);
}

void show_cursor(void)
{
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 20;
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &ci);
}

void clear_screen(void)
{
    set_color(COL_DEFAULT);
    system("cls");
    hide_cursor();
}

void setup_console(void)
{
    CONSOLE_FONT_INFOEX fi;
    char cmd[64];

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    /* 한글·괘선 문자를 위해 코드 페이지를 CP949로 고정 */
    SetConsoleOutputCP(949);
    SetConsoleCP(949);

    /* 창 제목 */
    SetConsoleTitleA("콘솔 바둑 & 오목 v2.0");

    /* 폰트 : 굴림체 20px (전각 문자 폭이 정확히 2칸인 고정폭 글꼴) */
    memset(&fi, 0, sizeof(fi));
    fi.cbSize = sizeof(fi);
    fi.dwFontSize.X = 0;
    fi.dwFontSize.Y = 20;
    fi.FontFamily = FF_DONTCARE;
    fi.FontWeight = FW_NORMAL;
    wcscpy(fi.FaceName, L"\xAD74\xB9BC\xCCB4");     /* "굴림체" */
    SetCurrentConsoleFontEx(hOut, FALSE, &fi);

    /* 창 크기 */
    sprintf(cmd, "mode con cols=%d lines=%d", SCREEN_COLS, SCREEN_LINES);
    system(cmd);

    hide_cursor();
}

/*
 * 키 입력 정규화
 *  - 방향키는 224(또는 0) 다음에 실제 코드가 오므로 두 번 읽어 처리
 *  - WASD 도 방향키로 변환
 *  - 영문자는 소문자로 통일
 */
int get_key(void)
{
    int c = _getch();

    if (c == 0 || c == 224) {
        c = _getch();
        switch (c) {
        case 72: return KEY_UP;
        case 80: return KEY_DOWN;
        case 75: return KEY_LEFT;
        case 77: return KEY_RIGHT;
        default: return 0;            /* 그 밖의 확장 키는 무시 */
        }
    }
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';

    switch (c) {
    case 'w': return KEY_UP;
    case 's': return KEY_DOWN;
    case 'a': return KEY_LEFT;
    case 'd': return KEY_RIGHT;
    }
    return c;
}

/* ================================================================
 *  효과음
 * ================================================================ */
void snd_place(void)  { Beep(880, 35); }
void snd_error(void)  { Beep(220, 150); }
void snd_select(void) { Beep(660, 30); }
void snd_win(void)
{
    Beep(523, 120); Beep(659, 120); Beep(784, 120); Beep(1046, 300);
}
void snd_draw(void)
{
    Beep(440, 150); Beep(440, 150);
}

/* ================================================================
 *  인트로 / 메뉴 / 도움말
 * ================================================================ */
static const char *G_B[5] = { "###### ", "#     #", "###### ", "#     #", "###### " };
static const char *G_A[5] = { "   #   ", "  # #  ", " #   # ", "#######", "#     #" };
static const char *G_D[5] = { "###### ", "#     #", "#     #", "#     #", "###### " };
static const char *G_U[5] = { "#     #", "#     #", "#     #", "#     #", " ##### " };
static const char *G_K[5] = { "#    # ", "#   #  ", "####   ", "#   #  ", "#    # " };
static const char *G_O[5] = { " ##### ", "#     #", "#     #", "#     #", " ##### " };
static const char *G_M[5] = { "#     #", "##   ##", "# # # #", "#  #  #", "#     #" };

static void print_banner(int x, int y, const char **word[], int len, int color)
{
    int r, i;
    const char *p;

    set_color(color);
    for (r = 0; r < 5; r++) {
        gotoxy(x, y + r);
        for (i = 0; i < len; i++) {
            for (p = word[i][r]; *p; p++)
                printf("%s", (*p == '#') ? "■" : "  ");
            if (i < len - 1) printf("  ");
        }
    }
    set_color(COL_DEFAULT);
}

void show_intro(void)
{
    const char **baduk[5] = { G_B, G_A, G_D, G_U, G_K };
    const char **omok[4]  = { G_O, G_M, G_O, G_K };
    int i, on = 1, t;

    clear_screen();
    print_banner(7, 3, baduk, 5, 0x0E);

    set_color(0x0F);
    gotoxy(36, 9);
    printf("─────  &  ─────");

    print_banner(15, 11, omok, 4, 0x0B);

    /* 장식용 돌 줄 */
    gotoxy(27, 18);
    for (i = 0; i < N; i++) {
        set_color(BG_BOARD | ((i % 2) ? FG_WHITE : FG_BLACK));
        printf("●");
    }

    set_color(COL_DIM);
    gotoxy(30, 24);
    printf("Console Baduk & Omok  ·  v2.0");
    set_color(COL_DEFAULT);

    /* PRESS ANY KEY 깜빡임 */
    while (1) {
        gotoxy(38, 21);
        set_color(on ? 0x0F : COL_DIM);
        printf("PRESS ANY KEY");
        on = !on;
        for (t = 0; t < 10 && !_kbhit(); t++) Sleep(50);
        if (_kbhit()) break;
    }
    get_key();
    snd_select();
    set_color(COL_DEFAULT);
}

/* 방향키 + Enter / 숫자키로 고르는 세로 메뉴 */
int select_menu(int x, int y, const char *items[], int count)
{
    int sel = 0, i, key;

    while (1) {
        for (i = 0; i < count; i++) {
            gotoxy(x, y + i * 2);
            set_color(i == sel ? COL_SELECT : COL_DEFAULT);
            printf(" %s %d. %-20s ", i == sel ? "▶" : "  ", i + 1, items[i]);
        }
        set_color(COL_DEFAULT);

        key = get_key();
        if (key == KEY_UP)        { sel = (sel + count - 1) % count; snd_select(); }
        else if (key == KEY_DOWN) { sel = (sel + 1) % count;         snd_select(); }
        else if (key == KEY_ENTER || key == KEY_SPACE) { snd_select(); return sel; }
        else if (key >= '1' && key < '1' + count)      { snd_select(); return key - '1'; }
        else if (key == KEY_ESC)  return count - 1;      /* 마지막 항목 = 종료/뒤로 */
    }
}

int main_menu(void)
{
    static const char *items[4] = { "오목 시작", "바둑 시작", "조작법 / 규칙", "종료" };

    clear_screen();
    set_color(COL_INFO);
    gotoxy(34, 4);
    printf("●○  바둑 & 오목  ○●");
    set_color(COL_DIM);
    gotoxy(28, 6);
    printf("방향키로 고르고 Enter 를 누르세요");
    set_color(COL_DEFAULT);
    gotoxy(28, 20);
    printf("이번 실행 전적 : 흑 %d승  백 %d승  무 %d", wins[BLACK], wins[WHITE], draws);

    return select_menu(32, 9, items, 4);
}

void show_help(void)
{
    static const char *lines[] = {
        "[ 조작법 ]",
        "  방향키 / W A S D   커서 이동",
        "  Space / Enter      돌 놓기",
        "  U                  무르기 (한 수 되돌리기)",
        "  P                  패스 (바둑)",
        "  R                  기권",
        "  H                  도움말",
        "  ESC                메인 메뉴로",
        "",
        "[ 오목 규칙 ]",
        "  - 흑이 먼저 둡니다.",
        "  - 가로, 세로, 대각선으로 정확히 5개를 먼저 이으면 승리합니다.",
        "  - 6개 이상 이어진 장목은 승리로 인정하지 않습니다.",
        "  - 판이 가득 차면 무승부입니다.",
        "",
        "[ 바둑 규칙 ]",
        "  - 흑이 먼저 두고, 백은 덤 6.5집을 받습니다.",
        "  - 상대 돌의 활로(이웃한 빈 칸)를 모두 막으면 따냅니다.",
        "  - 놓자마자 잡히는 자리(자충수)에는 둘 수 없습니다.",
        "  - 패 : 방금 한 점을 따낸 자리를 곧바로 되따낼 수 없습니다.",
        "  - 두 사람이 연속으로 패스하면 대국이 끝나고 계가합니다.",
        "  - 계가 : 판 위의 내 돌 + 내 돌로만 둘러싸인 빈 칸 (중국식)",
        "  - 죽은 돌은 자동 판정되지 않으니 끝내기 전에 따내 주세요.",
    };
    int i, count = (int)(sizeof(lines) / sizeof(lines[0]));

    clear_screen();
    for (i = 0; i < count; i++) {
        gotoxy(5, 2 + i);
        set_color(lines[i][0] == '[' ? COL_TITLE : COL_DEFAULT);
        printf("%s", lines[i]);
    }
    set_color(COL_DIM);
    gotoxy(5, SCREEN_LINES - 2);
    printf("아무 키나 누르면 돌아갑니다.");
    set_color(COL_DEFAULT);
    get_key();
}

/* ================================================================
 *  판 그리기
 * ================================================================ */
static int is_hoshi(int x, int y)
{
    return (x == 3 || x == 9 || x == 15) && (y == 3 || y == 9 || y == 15);
}

static const char *grid_char(int x, int y)
{
    if (y == 0)     return x == 0 ? "┌" : (x == N - 1 ? "┐" : "┬");
    if (y == N - 1) return x == 0 ? "└" : (x == N - 1 ? "┘" : "┴");
    if (x == 0)     return "├";
    if (x == N - 1) return "┤";
    if (is_hoshi(x, y)) return "╋";
    return "┼";
}

void draw_labels(void)
{
    int i;

    set_color(COL_LABEL);
    for (i = 0; i < N; i++) {
        gotoxy(BOARD_X + i * 2, BOARD_Y - 1); printf("%c", COL_NAME[i]);
        gotoxy(BOARD_X + i * 2, BOARD_Y + N); printf("%c", COL_NAME[i]);
        gotoxy(BOARD_X - 3, BOARD_Y + i);     printf("%2d", N - i);
        gotoxy(BOARD_X + N * 2 + 1, BOARD_Y + i); printf("%d", N - i);
    }
    set_color(COL_DEFAULT);
}

/*
 * 셀의 "보이는 상태"를 정수 하나로 표현
 *   bit 0-1 : 돌 (EMPTY/BLACK/WHITE)
 *   bit 2   : 마지막 착수
 *   bit 3   : 커서
 *   bit 4   : 승리 5목
 *   bit 5-6 : 커서가 있을 때 현재 차례
 */
static int cell_visual(int x, int y)
{
    int v = g.board[y][x];

    if (x == g.lastX && y == g.lastY)          v |= 4;
    if (cursorVisible && x == curX && y == curY) v |= 8 | (g.turn << 5);
    if (winMark[y][x])                          v |= 16;
    return v;
}

void draw_cell(int x, int y, int v)
{
    int stone = v & 3;
    int bg = BG_BOARD;
    int color;
    const char *s;

    if (v & 16)            bg = BG_WIN;
    if ((v & 8) && stone)  bg = BG_CURSOR;     /* 돌 위의 커서는 빨간 배경 */

    if (stone == BLACK) {
        s = (v & 4) ? "◆" : "●";
        color = bg | FG_BLACK;
    }
    else if (stone == WHITE) {
        s = (v & 4) ? "◆" : "●";
        color = bg | FG_WHITE;
    }
    else if (v & 8) {                           /* 빈 칸 위의 커서 */
        s = "◎";
        color = bg | (((v >> 5) & 3) == BLACK ? FG_BLACK : FG_WHITE);
    }
    else {
        s = grid_char(x, y);
        color = bg | FG_BLACK;
    }

    gotoxy(BOARD_X + x * 2, BOARD_Y + y);
    set_color(color);
    printf("%s", s);
    set_color(COL_DEFAULT);
}

/* 바뀐 칸만 다시 그려 깜빡임 제거 */
void refresh_board(void)
{
    int x, y, v;

    for (y = 0; y < N; y++)
        for (x = 0; x < N; x++) {
            v = cell_visual(x, y);
            if (shown[y][x] != v) {
                draw_cell(x, y, v);
                shown[y][x] = v;
            }
        }
}

void invalidate_board(void)
{
    memset(shown, -1, sizeof(shown));
}

void redraw_all(void)
{
    clear_screen();
    draw_labels();
    invalidate_board();
}

/* ================================================================
 *  정보 패널 / 메시지
 * ================================================================ */
void panel_line(int row, const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    gotoxy(PANEL_X, row);
    printf("%-*s", PANEL_W, buf);
}

void clear_panel(void)
{
    int r;
    set_color(COL_DEFAULT);
    for (r = BOARD_Y; r <= BOARD_Y + N; r++)
        panel_line(r, "");
}

void draw_panel(void)
{
    char last[16];
    int  r = BOARD_Y;

    if (histCount > 0) {
        coord_str(kifu[histCount - 1].x, kifu[histCount - 1].y, last);
    } else {
        strcpy(last, "-");
    }

    set_color(COL_TITLE);
    panel_line(r, gameMode == MODE_OMOK ? "[  오  목  ]" : "[  바  둑  ]");
    set_color(COL_DEFAULT);

    panel_line(r + 2, "차례       %s", g.turn == BLACK ? "● 흑" : "○ 백");
    panel_line(r + 3, "진행       %d수", g.moveNo);
    if (histCount > 0)
        panel_line(r + 4, "마지막 수  %s %s", color_name(kifu[histCount - 1].color), last);
    else
        panel_line(r + 4, "마지막 수  -");

    if (gameMode == MODE_GO) {
        panel_line(r + 6, "따낸 돌    흑 %d  /  백 %d", g.captured[BLACK], g.captured[WHITE]);
        panel_line(r + 7, "덤         백 %.1f집", KOMI);
    } else {
        panel_line(r + 6, "규칙       정확히 5목 승리");
        panel_line(r + 7, "           (6목 이상 불인정)");
    }

    panel_line(r + 9, "전적       흑 %d승  백 %d승  무 %d", wins[BLACK], wins[WHITE], draws);

    set_color(COL_DIM);
    panel_line(r + 11, "방향키/WASD   이동");
    panel_line(r + 12, "Space/Enter   착수");
    panel_line(r + 13, "U             무르기");
    panel_line(r + 14, gameMode == MODE_GO ? "P             패스" : "");
    panel_line(r + 15, "R             기권");
    panel_line(r + 16, "H             도움말");
    panel_line(r + 17, "ESC           메인 메뉴");
    set_color(COL_DEFAULT);
}

void show_msg(int color, const char *fmt, ...)
{
    char buf[160];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    gotoxy(3, MSG_Y);
    set_color(color);
    printf("%-84s", buf);
    set_color(COL_DEFAULT);
}

void clear_msg(void)
{
    gotoxy(3, MSG_Y);
    printf("%-84s", "");
}

int confirm(const char *question)
{
    int key;

    show_msg(COL_INFO, "%s (Y/N)", question);
    while (1) {
        key = get_key();
        if (key == 'y') { clear_msg(); return 1; }
        if (key == 'n' || key == KEY_ESC) { clear_msg(); return 0; }
    }
}

/* ================================================================
 *  공통 유틸
 * ================================================================ */
void coord_str(int x, int y, char *buf)
{
    if (x < 0) strcpy(buf, "패스");
    else       sprintf(buf, "%c%d", COL_NAME[x], N - y);
}

const char *color_name(int c)
{
    return c == BLACK ? "흑" : "백";
}

void push_history(const GameState *s, int color, int x, int y)
{
    if (histCount >= MAX_HISTORY) return;
    history[histCount] = *s;
    kifu[histCount].color = color;
    kifu[histCount].x = x;
    kifu[histCount].y = y;
    histCount++;
}

void do_undo(void)
{
    if (histCount == 0) {
        snd_error();
        show_msg(COL_WARN, "되돌릴 수가 없습니다.");
        return;
    }
    g = history[--histCount];
    snd_select();
    show_msg(COL_INFO, "한 수 물렀습니다.");
}

int save_kifu(char *outName)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    FILE *fp;
    char c[16];
    int i;

    sprintf(outName, "kifu_%04d%02d%02d_%02d%02d%02d.txt",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    fp = fopen(outName, "w");
    if (fp == NULL) return 0;

    fprintf(fp, "# 콘솔 %s 기보\n", gameMode == MODE_OMOK ? "오목" : "바둑");
    fprintf(fp, "# 일시 : %04d-%02d-%02d %02d:%02d\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
    if (winner) fprintf(fp, "# 결과 : %s 승 - %s\n", color_name(winner), resultReason);
    else        fprintf(fp, "# 결과 : 무승부 - %s\n", resultReason);
    if (resultDetail[0][0]) fprintf(fp, "# %s\n", resultDetail[0]);
    if (resultDetail[1][0]) fprintf(fp, "# %s\n", resultDetail[1]);
    fprintf(fp, "\n");

    for (i = 0; i < histCount; i++) {
        coord_str(kifu[i].x, kifu[i].y, c);
        fprintf(fp, "%3d. %s %s\n", i + 1, color_name(kifu[i].color), c);
    }
    fclose(fp);
    return 1;
}

/* ================================================================
 *  게임 진행
 * ================================================================ */
void init_game(int mode)
{
    memset(&g, 0, sizeof(g));
    g.turn  = BLACK;                 /* 흑 선공 */
    g.lastX = g.lastY = -1;
    g.koX   = g.koY   = -1;

    histCount = 0;
    memset(winMark, 0, sizeof(winMark));
    gameMode = mode;
    curX = curY = N / 2;             /* 천원에서 시작 */
    cursorVisible = 1;

    gameOver = 0;
    winner = 0;
    resultReason[0] = '\0';
    resultDetail[0][0] = resultDetail[1][0] = '\0';
}

int play_game(int mode)
{
    int key, err;
    GameState before;

    init_game(mode);
    redraw_all();
    show_msg(COL_DIM, "흑부터 시작합니다. H 를 누르면 도움말을 볼 수 있습니다.");

    while (!gameOver) {
        refresh_board();
        draw_panel();

        key = get_key();
        clear_msg();

        switch (key) {
        case KEY_UP:    if (curY > 0)     curY--; break;
        case KEY_DOWN:  if (curY < N - 1) curY++; break;
        case KEY_LEFT:  if (curX > 0)     curX--; break;
        case KEY_RIGHT: if (curX < N - 1) curX++; break;

        case KEY_SPACE:
        case KEY_ENTER:
            before = g;
            err = (mode == MODE_OMOK) ? play_omok(curX, curY) : play_go(curX, curY);
            if (err != ERR_NONE) {
                g = before;
                snd_error();
                if (err == ERR_OCCUPIED) show_msg(COL_WARN, "이미 돌이 놓인 자리입니다.");
                else if (err == ERR_KO)  show_msg(COL_WARN, "패 : 바로 되따낼 수 없습니다. 다른 곳에 한 수 두고 오세요.");
                else                     show_msg(COL_WARN, "자충수 : 놓자마자 잡히는 자리에는 둘 수 없습니다.");
                break;
            }
            push_history(&before, before.turn, curX, curY);
            snd_place();
            break;

        case 'u':
            do_undo();
            break;

        case 'p':
            if (mode == MODE_GO) do_pass();
            else show_msg(COL_WARN, "오목에는 패스가 없습니다.");
            break;

        case 'r':
            if (confirm("정말 기권하시겠습니까?")) {
                winner = OPP(g.turn);
                sprintf(resultReason, "%s 기권", color_name(g.turn));
                gameOver = 1;
            }
            break;

        case 'h':
            show_help();
            redraw_all();
            break;

        case KEY_ESC:
            if (confirm("메인 메뉴로 나갈까요? 진행 중인 대국은 사라집니다."))
                return ACT_MENU;
            break;
        }
    }
    return show_result();
}

/* ---------------- 오목 ---------------- */
int play_omok(int x, int y)
{
    int me = g.turn;

    if (g.board[y][x] != EMPTY) return ERR_OCCUPIED;

    g.board[y][x] = me;
    g.lastX = x;
    g.lastY = y;
    g.moveNo++;
    g.turn = OPP(me);

    if (check_omok(x, y)) {
        gameOver = 1;
        winner = me;
        sprintf(resultReason, "%s 5목 완성", color_name(me));
    }
    else if (g.moveNo >= N * N) {
        gameOver = 1;
        winner = 0;
        strcpy(resultReason, "판이 가득 찼습니다");
    }
    return ERR_NONE;
}

/* (x, y)에 방금 놓은 돌이 정확히 5목을 만들었는지 검사하고, 만들었으면 winMark 에 표시 */
int check_omok(int x, int y)
{
    static const int dx[4] = { 1, 0, 1, 1 };
    static const int dy[4] = { 0, 1, 1, -1 };
    int color = g.board[y][x];
    int d, k, cnt, sx, sy;

    for (d = 0; d < 4; d++) {
        /* 뒤쪽 끝 찾기 */
        sx = x; sy = y;
        while (sx - dx[d] >= 0 && sx - dx[d] < N && sy - dy[d] >= 0 && sy - dy[d] < N
               && g.board[sy - dy[d]][sx - dx[d]] == color) {
            sx -= dx[d];
            sy -= dy[d];
        }
        /* 앞쪽으로 세기 */
        cnt = 0;
        while (sx + dx[d] * cnt >= 0 && sx + dx[d] * cnt < N &&
               sy + dy[d] * cnt >= 0 && sy + dy[d] * cnt < N &&
               g.board[sy + dy[d] * cnt][sx + dx[d] * cnt] == color) {
            cnt++;
        }
        if (cnt == 5) {                         /* 정확히 5개 : 장목 제외 */
            for (k = 0; k < 5; k++)
                winMark[sy + dy[d] * k][sx + dx[d] * k] = 1;
            return 1;
        }
    }
    return 0;
}

/* ---------------- 바둑 ---------------- */

/*
 * (x, y)가 속한 같은 색 돌 무리를 찾아 gx/gy 에 담고, 활로(이웃한 빈 칸) 수를 반환
 */
int collect_group(int x, int y, int gx[], int gy[], int *size)
{
    static int visit[N][N];
    static int libMark[N][N];
    static int stackX[N * N], stackY[N * N];
    int color = g.board[y][x];
    int top = 0, libs = 0, n = 0;
    int i, cx, cy, nx, ny;

    memset(visit, 0, sizeof(visit));
    memset(libMark, 0, sizeof(libMark));

    stackX[top] = x; stackY[top] = y; top++;
    visit[y][x] = 1;

    while (top > 0) {
        top--;
        cx = stackX[top];
        cy = stackY[top];
        gx[n] = cx;
        gy[n] = cy;
        n++;

        for (i = 0; i < 4; i++) {
            nx = cx + DX[i];
            ny = cy + DY[i];
            if (nx < 0 || ny < 0 || nx >= N || ny >= N) continue;

            if (g.board[ny][nx] == EMPTY) {
                if (!libMark[ny][nx]) { libMark[ny][nx] = 1; libs++; }
            }
            else if (g.board[ny][nx] == color && !visit[ny][nx]) {
                visit[ny][nx] = 1;
                stackX[top] = nx; stackY[top] = ny; top++;
            }
        }
    }
    *size = n;
    return libs;
}

int play_go(int x, int y)
{
    static int gx[N * N], gy[N * N];
    int me = g.turn, op = OPP(me);
    int i, k, nx, ny, size, libs;
    int total = 0, capX = -1, capY = -1;

    if (g.board[y][x] != EMPTY)    return ERR_OCCUPIED;
    if (x == g.koX && y == g.koY)  return ERR_KO;

    g.board[y][x] = me;

    /* 1) 이웃한 상대 무리 중 활로가 0이 된 것을 따냄 */
    for (i = 0; i < 4; i++) {
        nx = x + DX[i];
        ny = y + DY[i];
        if (nx < 0 || ny < 0 || nx >= N || ny >= N) continue;
        if (g.board[ny][nx] != op) continue;

        libs = collect_group(nx, ny, gx, gy, &size);
        if (libs == 0) {
            for (k = 0; k < size; k++) g.board[gy[k]][gx[k]] = EMPTY;
            total += size;
            capX = gx[0];
            capY = gy[0];
        }
    }

    /* 2) 내 무리의 활로가 0이면 자충수 (호출한 쪽에서 상태 복원) */
    libs = collect_group(x, y, gx, gy, &size);
    if (libs == 0) return ERR_SUICIDE;

    /* 3) 패 : 한 점을 따냈고, 놓은 돌이 홀로 활로 1개뿐이면 그 자리를 금지 */
    if (total == 1 && size == 1 && libs == 1) {
        g.koX = capX;
        g.koY = capY;
    } else {
        g.koX = g.koY = -1;
    }

    g.captured[me] += total;
    g.lastX = x;
    g.lastY = y;
    g.moveNo++;
    g.passCount = 0;
    g.turn = op;

    if (total > 0)
        show_msg(COL_INFO, "%s이(가) %d점을 따냈습니다!", color_name(me), total);
    return ERR_NONE;
}

void do_pass(void)
{
    GameState before = g;

    g.passCount++;
    g.koX = g.koY = -1;
    g.lastX = g.lastY = -1;
    g.moveNo++;
    g.turn = OPP(g.turn);
    push_history(&before, before.turn, -1, -1);
    snd_select();

    if (g.passCount >= 2) {
        score_go();
        gameOver = 1;
    } else {
        show_msg(COL_INFO, "%s 패스. 상대도 패스하면 대국이 끝납니다.", color_name(before.turn));
    }
}

/* 중국식 영역 계가 : 판 위의 돌 + 한 색으로만 둘러싸인 빈 칸 */
void score_go(void)
{
    static int seen[N][N];
    static int stackX[N * N], stackY[N * N];
    int x, y, i, top, size, touchB, touchW, cx, cy, nx, ny;
    int bStones = 0, wStones = 0, bTer = 0, wTer = 0;
    double b, w;

    memset(seen, 0, sizeof(seen));

    for (y = 0; y < N; y++)
        for (x = 0; x < N; x++) {
            if (g.board[y][x] == BLACK) { bStones++; continue; }
            if (g.board[y][x] == WHITE) { wStones++; continue; }
            if (seen[y][x]) continue;

            /* 빈 칸 영역 탐색 */
            top = 0; size = 0; touchB = touchW = 0;
            stackX[top] = x; stackY[top] = y; top++;
            seen[y][x] = 1;
            while (top > 0) {
                top--;
                cx = stackX[top];
                cy = stackY[top];
                size++;
                for (i = 0; i < 4; i++) {
                    nx = cx + DX[i];
                    ny = cy + DY[i];
                    if (nx < 0 || ny < 0 || nx >= N || ny >= N) continue;
                    if (g.board[ny][nx] == BLACK) touchB = 1;
                    else if (g.board[ny][nx] == WHITE) touchW = 1;
                    else if (!seen[ny][nx]) {
                        seen[ny][nx] = 1;
                        stackX[top] = nx; stackY[top] = ny; top++;
                    }
                }
            }
            if (touchB && !touchW) bTer += size;
            else if (touchW && !touchB) wTer += size;
        }

    b = bStones + bTer;
    w = wStones + wTer + KOMI;
    winner = (b > w) ? BLACK : WHITE;           /* 덤이 0.5 단위라 비김 없음 */

    sprintf(resultReason, "%s %.1f집 승 (계가)", color_name(winner), b > w ? b - w : w - b);
    sprintf(resultDetail[0], "흑 : 돌 %d + 집 %d = %.1f", bStones, bTer, b);
    sprintf(resultDetail[1], "백 : 돌 %d + 집 %d + 덤 %.1f = %.1f", wStones, wTer, KOMI, w);
}

/* ---------------- 결과 화면 ---------------- */
int show_result(void)
{
    static const char *items[3] = { "다시 하기", "메인 메뉴", "게임 종료" };
    char fname[64];
    int saved, sel, r = BOARD_Y;

    cursorVisible = 0;
    refresh_board();                             /* 승리 5목 하이라이트 표시 */

    if (winner) wins[winner]++;
    else        draws++;

    saved = save_kifu(fname);

    clear_panel();
    clear_msg();

    set_color(COL_INFO);
    panel_line(r, "=========  게 임  종 료  =========");

    if (winner) {
        set_color(0x0A);
        panel_line(r + 2, "%s 승리!", winner == BLACK ? "● 흑" : "○ 백");
    } else {
        set_color(0x0F);
        panel_line(r + 2, "무승부!");
    }
    set_color(COL_DEFAULT);
    panel_line(r + 3, "%s", resultReason);
    if (resultDetail[0][0]) panel_line(r + 5, "%s", resultDetail[0]);
    if (resultDetail[1][0]) panel_line(r + 6, "%s", resultDetail[1]);

    panel_line(r + 8, "총 %d수 / 전적 흑 %d승 백 %d승 무 %d",
               g.moveNo, wins[BLACK], wins[WHITE], draws);

    set_color(COL_DIM);
    if (saved) panel_line(r + 10, "기보 저장 : %s", fname);
    else       panel_line(r + 10, "기보 저장에 실패했습니다.");
    set_color(COL_DEFAULT);

    if (winner) snd_win();
    else        snd_draw();

    sel = select_menu(PANEL_X, r + 13, items, 3);
    if (sel == 0) return ACT_RESTART;
    if (sel == 1) return ACT_MENU;
    return ACT_QUIT;
}
