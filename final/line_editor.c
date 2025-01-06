#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "line_editor.h"

const char KEY_ESCAPE = '\x1b';
const char KEY_BACKSPACE = '\x7f';

struct termios orig_termios;
enum SIMode { NORMAL, INSERT };

/**
 * ターミナルの設定を変更する
 */
void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/**
 * ターミナルの設定を元に戻す
 */
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/**
 * 文字列エディタをコマンドライン上から消す
 */
void hide_editor() {
    printf("\r\033[2K");    // 1行目クリア
    printf("\n\033[2K");    // 2行目クリア
    printf("\033[1A\r");    // 1行目にカーソルを戻す
    fflush(stdout);
}

/**
 * 文字列エディタをコマンドライン上に描画する
 */
void show_editor(enum SIMode mode, char line[MAX_LINE_LEN], int cursor) {
    // エディタ画面をリセット
    hide_editor();
    // 1行目を描画
    printf("%s \n", line);
    // 2行目を描画
    if (mode == INSERT) printf("\e[1m-- INSERT --\e[m");    // 太字
    // カーソルを1行上の先頭に移動
    printf("\033[1A\r");
    // cursor 回カーソルを進める
    if (cursor) printf("\033[%dC", cursor);

    fflush(stdout);
}

/**
 * 文字列 LINE をコマンドライン上で編集させる
 * LINE がマルチバイト文字を含む場合は 1 を返す
 */
int edit_line(char line[MAX_LINE_LEN]) {
    int line_len = strlen(line);
    int cursor = 0;
    char command = '\0';
    enum SIMode mode = NORMAL;
    int insert_flg = -1;
    char* p;

    p = line;
    while (*p) {
        if (*p & 0x80) return 1;
        p++;
    }

    enable_raw_mode();

    while (1) {
        char c;
        show_editor(mode, line, cursor);

        c = getchar();

        /* Enter で終了 */
        if (c == '\x0a') break;

        // ノーマルモードのキーバインド
        if (mode == NORMAL) {
            int i;

            if (command == '[') {
                int lastPos = line_len-1;
                if (insert_flg+1) {    // 挿入モードで矢印キーを押した
                    cursor = insert_flg;
                    lastPos++;
                    mode = INSERT;
                    insert_flg = -1;
                }
                /* Esc-[-C (→) カーソルを1つ右に移動 */
                if (c == 'C' && cursor < lastPos) {
                    cursor++;
                /* Esc-[-D (←) カーソルを1つ左に移動 */
                } else if (c == 'D' && cursor > 0) {
                    cursor--;
                }
                command = '\0'; continue;

            } else if (command == 'd') {
                /* d-d 行をクリア */
                if (c == 'd') {
                    line[0] = '\0';
                    cursor = 0; line_len = 0;

                /* d-e カーソル位置からCSV列末尾までをクリア */
                } else if (c == 'e' && line_len) {
                    int l = cursor, r = cursor;
                    while (line[++r] != '\0' && line[r] != ',');
                    while ((line[l++] = line[r++]));
                    line_len -= r-l;
                    if (line_len && line_len <= cursor) cursor--;
                }
                command = '\0'; continue;

            } else if (command == 'c') {
                command = (c == 'i') ? 'i' : '\0';
                continue;
            } else if (command == 'i') {

                /* c-i-w カーソル位置のCSV列をクリア */
                if (c == 'w') {
                    if (line_len) {
                        int l = cursor, r = cursor;
                        if (l > 0 && line[l] != ',' && line[l-1] != ',') {
                            while (--l > 0 && line[l-1] != ',');
                        }
                        while (line[r] != '\0' && line[r] != ',') r++;
                        cursor = l;
                        while ((line[l++] = line[r++]));
                        line_len -= r-l;
                    }
                    mode = INSERT;
                }
                command = '\0'; continue;
            }

            if (c == '[' || c == 'd' || c == 'c') {
                command = c; continue;
            }
            command = '\0'; insert_flg = -1;

            switch (c) {
            // モード切り替え
            case 'i':   /* 挿入モード(前) */
                mode = INSERT; break;

            case 'a':   /* 挿入モード(後) */
                mode = INSERT;
                if (line_len) cursor++;
                break;

            case 'I':   /* 挿入モード(行頭) */
                mode = INSERT; cursor = 0; break;

            case 'A':   /* 挿入モード(行末) */
                mode = INSERT; cursor = line_len; break;

            // カーソル移動
            case 'h':   /* カーソルを1つ左へ */
                if (cursor > 0) cursor--;
                break;

            case 'l':   /* カーソルを1つ左へ */
                if (cursor < line_len-1) cursor++;
                break;

            case '0':   /* カーソルを行頭へ */
            case '^':
                cursor = 0;
                break;

            case '$':   /* カーソルを行末へ */
                if (line_len) cursor = line_len-1;
                break;

            case 'w':   /* カーソルをCSV次列先頭へ */
                while (cursor < line_len-1 && line[cursor++] != ',');
                break;

            case 'e':   /* カーソルをCSV次列末尾へ */
                if (cursor >= line_len-1) break;
                while (++cursor < line_len-1 && line[cursor+1] != ',');
                break;

            case 'b':   /* カーソルをCSV前列先頭へ */
                if (cursor <= 0) break;
                while (--cursor > 0 && line[cursor-1] != ',');
                break;

            // 削除
            case 'x':   /* 文字を削除 */
                for (i=cursor; i < line_len-1; i++) line[i] = line[i+1];
                if (cursor > 0 && cursor == line_len-1) cursor--;
                if (line_len > 0) line[--line_len] = '\0';
                break;

            case 'C':   /* カーソル位置から文末までを削除して挿入モードへ */
                mode = INSERT;
                line[cursor] = '\0';
                line_len = cursor;
                break;

            case 'D':   /* カーソル位置から文末までを削除 */
                line[cursor] = '\0';
                line_len = cursor;
                if (cursor > 0) cursor--;
                break;

            default:
                break;
            }

        // 挿入モードのキーバインド
        } else if (mode == INSERT) {
            /* Esc Ctrl-[ ノーマルモード */
            if (c == KEY_ESCAPE) {
                mode = NORMAL; insert_flg = cursor;
                if (cursor > 0) cursor--;
                continue;

            /* BackSpace 文字を削除 */
            } else if (c == KEY_BACKSPACE) {
                int i;
                if (cursor <= 0) continue;
                for (i=cursor; i < line_len; i++) line[i-1] = line[i];
                line[line_len-1] = '\0';
                cursor--;
                line_len--;

            /* 表示可能なASCII文字を挿入 */
            } else if (c >= '\x20' && c <= '\x7E') {
                int i;
                if (line_len >= MAX_LINE_LEN - 1) continue;
                for (i=line_len+1; i > cursor; i--) line[i] = line[i-1];
                line[cursor] = c;
                cursor++;
                line_len++;
            }
        }
    }

    hide_editor();
    disable_raw_mode();
    return 0;
}
