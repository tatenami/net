#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>
#include "net.h"

#define PORT_NUMBER 61003
#define BACKLOG_COUNT 10
#define MAX_CLIENTS 3
#define BUF_LEN 10

#define MAX_NAME_LEN 70
#define MAX_ADDRESS_LEN 70
#define MAX_LINE_LEN 1024

struct date {
  int y; // year
  int m; // month
  int d; // day of month
};

struct profile {
    int id;                        // ID
    char name[MAX_NAME_LEN];       // 氏名
    struct date birthday;          // 誕生日
    char address[MAX_ADDRESS_LEN]; // 住所
    char* note;                    // 備考
};

struct client_data {
    int socket;
    struct profile profile_data_store[10000];
    int profile_data_nitems;
    int modifying_i;
};

/**
 * クライアントを初期化
 */
void init_client(struct client_data* client, int socket) {
    client->socket = socket;
    client->profile_data_nitems = 0;
    client->modifying_i = -1;
}

/**
 * クライアントに送信する
 * 戻り値: 成功時 0, 失敗時 1
 */
int client_printf(struct client_data* client, const char *format, ...) {
    char buf[MAX_LINE_LEN * 2];
    int len;
    va_list args;

    va_start(args, format);
    len = vsprintf(buf, format, args);
    va_end(args);

    if (write(client->socket, buf, len) == -1) {
        perror("write failed");
        return 1;
    }
    return 0;
}

/**
 * 文字列 STR を文字列中の文字 SEP で区切る．
 * 区切った結果の文字列は， RET[] に格納される．
 * 引数 MAX は，分割後の要素数の最大値である．
 * 戻り値: 実際に分割された要素数である．
 */
int split(char *str, char *ret[], char sep, int max) {
    int count = 0;
    ret[0] = str;
    while (*str != '\0') {
        if (*str == sep) {
            *str = '\0';
            count++;
            ret[count] = str+1;
            if (count+1 == max) return count+1;
        }
        str++;
    }
    
    return count+1;
}

/**
 * ソケットから1行受信して，文字配列 LINE に格納する．
 * 戻り値: 成功時 ヌル文字を含む LINE の長さ, 失敗時 -1
 */
ssize_t get_line(int s, char *line) {
    char* buf;
    ssize_t len, total_len = 0;
    for (buf = line;; buf += BUF_LEN) {
        len = recv(s, buf, BUF_LEN, 0);
        if (len < 0) {
            perror("recv failed");
            return -1;
        } else if (len == 0) { // EOF(クライアントが Ctrl-C で終了した場合など)
            return -1;
        }
        if (buf[len - 1] == '\0') break;
        total_len += len;
    }
    return total_len;
}

/**
 * I 番目のプロファイルを登録．
 * 戻り値: 成功時 0, 失敗時 1
 */
int register_profile(struct client_data* client, int i, char* line) {
    char *elem[5], *date_elem[3];
    struct profile* prof = &client->profile_data_store[i];

    if (split(line, elem, ',', 5) != 5) {
        client_printf(client, "Invalid CSV #%d: CSV format error: ignored.\n", i);
        return 1;
    }
    if (split(elem[2], date_elem, '-', 3) != 3) {
        client_printf(client, "Invalid CSV #%d: Date format error: ignored.\n", i);
        return 1;
    }

    prof->id = atoi(elem[0]);

    strcpy(prof->name, elem[1]);
    prof->name[MAX_NAME_LEN-1] = '\0';

    prof->birthday.y = atoi(date_elem[0]);
    prof->birthday.m = atoi(date_elem[1]);
    prof->birthday.d = atoi(date_elem[2]);

    strcpy(prof->address, elem[3]);
    prof->address[MAX_ADDRESS_LEN-1] = '\0';

    prof->note = (char*)malloc(strlen(elem[4])+1);
    strcpy(prof->note, elem[4]);
    return 0;
}

/**
 * 新規プロファイルを作成．
 */
void new_profile(struct client_data* client, char* line) {
    if (register_profile(client, client->profile_data_nitems, line)) return;
    client->profile_data_nitems++;
}

/**
 * I 番目のプロファイルを出力．
 */
void print_profile(struct client_data* client, int i) {
    struct profile* prof = &client->profile_data_store[i];

    client_printf(client, "Id    : %d\n", prof->id);
    client_printf(client, "Name  : %s\n", prof->name);
    client_printf(client, "Birth : %04d-%02d-%02d\n", prof->birthday.y,
        prof->birthday.m, prof->birthday.d);
    client_printf(client, "Addr. : %s\n", prof->address);
    client_printf(client, "Comm. : %s\n\n", prof->note);
}

/**
 * プロファイルをCSV形式で文字列化．
 */
void sprint_profile_csv(char line[MAX_LINE_LEN], struct profile* prof) {
    snprintf(line, MAX_LINE_LEN, "%d,%s,%d-%02d-%02d,%s,%s",
             prof->id,
             prof->name,
             prof->birthday.y, prof->birthday.m, prof->birthday.d,
             prof->address,
             prof->note);
}

/**
 * プロファイルをCSV形式で出力．
 */
void fprint_profile_csv(FILE *stream, struct profile* prof) {
    char line[MAX_LINE_LEN];
    sprint_profile_csv(line, prof);
    fprintf(stream, "%s\n", line);
}

/**
 * INDEX 番目のプロファイルを削除
*/
void remove_profile(struct client_data* client, int index) {
    int i;
    free(client->profile_data_store[index].note);
    for (i=index; i < client->profile_data_nitems-1; i++)
        client->profile_data_store[i] = client->profile_data_store[i+1];
    client->profile_data_nitems--;
}

/**
 * プロファイルを id で比較．
 */
int compare_id(struct profile* p1, struct profile* p2) {
    return p1->id < p2->id;
}

/**
 * プロファイルを name で比較．
 */
int compare_name(struct profile* p1, struct profile* p2) {
    return strcmp(p1->name, p2->name) < 0 ? 1 : 0;
}

/**
 * プロファイルを birthday で比較．
 */
int compare_birthday(struct profile* p1, struct profile* p2) {
    int tmp;
    if ((tmp = p1->birthday.y - p2->birthday.y)) return tmp < 0;
    if ((tmp = p1->birthday.m - p2->birthday.m)) return tmp < 0;
    if ((tmp = p1->birthday.d - p2->birthday.d)) return tmp < 0;
    return 0;
}

/**
 * プロファイルを address で比較．
 */
int compare_address(struct profile* p1, struct profile* p2) {
    return strcmp(p1->address, p2->address) < 0 ? 1 : 0;
}

/**
 * プロファイルを note で比較．
 */
int compare_note(struct profile* p1, struct profile* p2) {
    return strcmp(p1->note, p2->note) < 0 ? 1 : 0;
}

void cmd_check(struct client_data* client) {
    client_printf(client, "%d profile(s)\n", client->profile_data_nitems);
}

void cmd_print(struct client_data* client, int nitems) {
    int idx, end;
    if (nitems > 0) {
        idx = 0;
        end = nitems;
        if (end > client->profile_data_nitems) end = client->profile_data_nitems;
    } else if (nitems < 0) {
        idx = client->profile_data_nitems + nitems;
        if (idx < 0) idx = 0;
        end = client->profile_data_nitems;
    } else {
        idx = 0;
        end = client->profile_data_nitems;
    }
    for (; idx < end; idx++)
        print_profile(client, idx);
}

int cmd_read(struct client_data* client, char *path) {
    FILE *fp;
    char line[1024];
    fp = fopen(path, "r");
    if (fp == NULL) {
        client_printf(client, "%%R: file open error %s.\n", path);
        return 1;
    }

    while (fgets(line, MAX_LINE_LEN, fp) != NULL) {
        int last = strlen(line)-1;
        if (last >= 0 && line[last] == '\n') line[last] = '\0';
        new_profile(client, line);
    }

    fclose(fp);
    return 0;
}

int cmd_write(struct client_data* client, char *path) {
    FILE *fp;
    int idx;
    fp = fopen(path, "w");
    if (fp == NULL) {
        client_printf(client, "%%W: file open error %s.\n", path);
        return 1;
    }

    for (idx=0; idx < client->profile_data_nitems; idx++) {
        fprint_profile_csv(fp, &client->profile_data_store[idx]);
    }

    fclose(fp);
    return 0;
}

void cmd_find(struct client_data* client, char *word) {
    int idx;
    for (idx=0; idx < client->profile_data_nitems; idx++) {
        struct profile* prof = &client->profile_data_store[idx];
        struct date* birth = &(prof->birthday);
        char* candidates[5];
        int i;

        candidates[0] = malloc(sizeof(char)*12);
        sprintf(candidates[0], "%d", prof->id);

        candidates[1] = prof->name;

        candidates[2] = malloc(sizeof(char)*11);
        sprintf(candidates[2], "%04d-%02d-%02d", birth->y, birth->m, birth->d);

        candidates[3] = prof->address;

        candidates[4] = prof->note;

        for (i=0; i < 5; i++) {
            if (strcmp(word, candidates[i]) == 0) {
                print_profile(client, idx);
                continue;
            }
        }

        free(candidates[0]);
        free(candidates[2]);
    }
}

/**
 * X, Y の値を入れ替える．
 */
void swap(struct profile* x, struct profile* y) {
    struct profile tmp;
    tmp = *x;
    *x = *y;
    *y = tmp;
}

/**
 * ARRAY の要素をシャッフルする．
 */
void shuffle(struct profile array[], int nitems) {
    int i;
    srand(time(NULL));
    for (i=nitems-1; i > 0; i--) swap(&array[rand() % (i+1)], &array[i]);
}

/**
 * X, Y, Z の中央値を求める．
 */
struct profile* median3(struct profile* x, struct profile* y, struct profile* z,
                        int (*compare)(struct profile*, struct profile*)) {
    if (compare(x,y)) {
        return compare(y,z) ? y
        : compare(x,z) ? z
        : x;
    } else {
        return compare(x,z) ? x
        : compare(y,z) ? z
        : y;
    }
}

/**
 * クイックソートの並び替え関数．分割位置を返す．
 */
int partition(struct profile array[], int l, int r,
              int (*compare)(struct profile*, struct profile*)) {
    struct profile* pivot = median3(&array[l],&array[r],&array[(l+r)/2], compare);
    int i = l;

    while (1) {
        while (compare(&array[i], pivot)) i++;
        while (compare(pivot, &array[r])) r--;
        if (i >= r) break;
        
        swap(&array[i], &array[r]);
        i++; r--;
    }
    return i == l ? i+1 : i;
}

/**
 * ARRAY をクイックソートで整列させる
 */
void quick_sort(struct profile array[], int l, int r,
                int (*compare)(struct profile*, struct profile*)) {
    if (l >= r) return;
    int p = partition(array, l, r, compare);
    quick_sort(array, l, p-1, compare);
    quick_sort(array, p, r, compare);
}

void cmd_sort(struct client_data* client, int n) {
    struct profile* profs = client->profile_data_store;
    int nitem = client->profile_data_nitems;
    switch (n) {
    case -1:
        shuffle(profs, nitem);
        break;

    case 1:
        quick_sort(profs, 0, nitem-1, compare_id);
        break;

    case 2:
        quick_sort(profs, 0, nitem-1, compare_name);
        break;

    case 3:
        quick_sort(profs, 0, nitem-1,compare_birthday);
        break;

    case 4:
        quick_sort(profs, 0, nitem-1, compare_address);
        break;

    case 5:
        quick_sort(profs, 0, nitem-1, compare_note);
        break;

    default:
        client_printf(client, "%%S: Invalid column number %d: ignored.\n", n);
    }
}

// 戻り値: 正常時 0, 異常時 1
int cmd_modify(struct client_data* client, int index) {
    char line[MAX_LINE_LEN];
    if (index < 0 || index > client->profile_data_nitems-1) {
        client_printf(client, "%%M: Invalid index #%d: ignored.\n", index);
        return 0;
    }
    sprint_profile_csv(line, &client->profile_data_store[index]);

    if (client_printf(client, line)) return 1;

    client->modifying_i = index;
    return 0;
}

// 戻り値: 正常時 0, 異常時 1
int cmd_modify2(struct client_data* client, char* line) {
    int index = client->modifying_i;
    if (line[0]) {
        client_printf(client, "%s\n", line);
        register_profile(client, index, line);
    } else {
        client_printf(client, "\n");
        remove_profile(client, index);
    }
    client->modifying_i = -1;
    return 0;
}

/**
 * コマンド文字 CMD と引数 PARAM で示された処理を実行．
 * 戻り値: 通常時 0, Quit時 1
 */
int exec_command(struct client_data* client, char cmd, char *param) {
    switch (cmd) {
    case 'Q':
        return 1;

    case 'C':
        cmd_check(client);
        break;

    case 'P':
        cmd_print(client, atoi(param));
        break;

    case 'R':
        cmd_read(client, param);
        break;

    case 'W':
        cmd_write(client, param);
        break;

    case 'F':
        cmd_find(client, param);
        break;

    case 'S':
        cmd_sort(client, atoi(param));
        break;

    case 'M':
        return cmd_modify(client, atoi(param)-1);

    default:
        client_printf(client, "Invalid command %c: ignored.\n", cmd);
    }
    return 0;
}

/**
 * 読み込んだ行 LINE の処理をする．
 * 戻り値: 通常時 0, Quit時 1
 */
int parse_line(struct client_data* client, char *line) {
    if (client->modifying_i >= 0) {
        return cmd_modify2(client, line);
    } else if (*line == '%') {
        char cmd, *param;
        cmd = *(line+1);
        param = line+3;
        if (!*(line+2)) *param = '\0';
        return exec_command(client, cmd, param);
    } else {
        new_profile(client, line);
        return 0;
    }
}

int main() {
    char line[MAX_LINE_LEN + 1];
    int listen_sock;
    struct client_data clients[MAX_CLIENTS];
    struct client_data* client;
    int i, max_fd, new_sock;
    fd_set read_fds;

    // クライアントソケットを -1 に初期化
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
    }

    // 待ち受け用ソケットの作成
    listen_sock = make_listen_socket(PORT_NUMBER, BACKLOG_COUNT);

    std_write("[INFO] Listening on port %d\n", PORT_NUMBER);

    while (1) {
        // ファイルディスクリプタセットの初期化
        FD_ZERO(&read_fds);

        // 待ち受け用ソケット と クライアントソケット をセットに追加
        FD_SET(listen_sock, &read_fds);
        max_fd = listen_sock;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket < 0) continue;
            FD_SET(clients[i].socket, &read_fds);
            if (clients[i].socket > max_fd) {
                max_fd = clients[i].socket;
            }
        }

        // select() の呼び出し
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        // 待ち受け用ソケットに新しい接続があるかチェック
        if (FD_ISSET(listen_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);
            new_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &addrlen);
            if (new_sock < 0) {
                perror("accept failed");
                continue;
            }

            std_write("[INFO] New connection accepted: socket %d\n", new_sock);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == -1) {
                    init_client(&clients[i], new_sock);
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                std_write("[WARN] Max clients reached. Connection rejected: socket %d\n", new_sock);
                close(new_sock);
            }
        }

        // クライアントソケットからのデータをチェック
        for (i = 0; i < MAX_CLIENTS; i++) {
            client = &clients[i];
            if (client->socket == -1) continue;
            if (!FD_ISSET(client->socket, &read_fds)) continue;

            // データの受信
            if (get_line(client->socket, line) < 0) {
                std_write("[INFO] Client disconnected: socket %d\n", client->socket);
                close(client->socket);
                client->socket = -1;
            } else {
                std_write("[INFO] Received from socket %d: \"%s\"\n", client->socket, line);
                if (parse_line(client, line)) {
                    // Quit時
                    std_write("[INFO] Connection closed by parse_line: socket %d\n", client->socket);
                    close(client->socket);
                    client->socket = -1;
                    continue;
                }

                // 送信終了を表すヌル文字の送信
                if (write(client->socket, "\0", 1) == -1) {
                    perror("write failed");
                    close(client->socket);
                    client->socket = -1;
                    continue;
                }
                std_write("[INFO] Finished sending to socket %d.\n", client->socket);
            }
        }
    }

    close(listen_sock);
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == -1) continue;
        close(clients[i].socket);
    }

    return 0;
}
