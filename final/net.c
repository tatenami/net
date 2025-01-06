#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "net.h"

int _make_listen_socket(int *port, int backlog) {
    int sock;
    struct sockaddr_in listen_addr;
    const int yes = 1;

    if (port == NULL) {
        fprintf(stderr, "port is NULL\n");
        return -1;
    }

    // ソケット作成
    sock = socket(
        AF_INET, // IPv4
        SOCK_STREAM, // TCP
        0 // standard family
    );
    if(sock < 0) {
        perror("socket failed");
        return -1;
    }

    // ソケットオプションの設定（再利用可能にする）
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt failed");
        close(sock); return -1;
    }

    memset(&listen_addr, 0, sizeof(listen_addr));
    // IPv4
    listen_addr.sin_family = AF_INET;
    // 待ち受けポート番号
    listen_addr.sin_port = htons(*port);
    // どのIPアドレスを持つ相手とでも通信を許可
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // バインド
    if(bind(sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind failed");
        close(sock); return -1;
    }

    if (*port == 0) {
        // ポート番号を取得
        socklen_t len = sizeof(listen_addr);
        if(getsockname(sock, (struct sockaddr*)&listen_addr, &len) < 0) {
            perror("getsockname failed");
            close(sock); return -1;
        }
        *port = ntohs(listen_addr.sin_port);
    }

    // リッスン開始
    if (listen(sock, backlog) < 0) {
        perror("listen failed");
        close(sock); return -1;
    }

    return sock;
}

int make_listen_socket(int port, int backlog) {
    return _make_listen_socket(&port, backlog);
}

int make_dynamic_listen_socket(int *port, int backlog) {
    *port = 0;
    return _make_listen_socket(port, backlog);
}

int make_connected_socket(const char* host_name, const char* server_port) {
    int sock;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;  // IPv4
    if (getaddrinfo(host_name, server_port, &hints, &res)) {
        perror("getaddrinfo failed");
        return -1;
    }

    // ソケット作成
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        freeaddrinfo(res); return -1;
    }

    // 接続
    if(connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect failed");
        freeaddrinfo(res); close(sock); return -1;
    }

    freeaddrinfo(res);
    return sock;
}

int std_write(const char *format, ...) {
    char buf[1024];
    int len;
    va_list args;

    va_start(args, format);
    len = vsprintf(buf, format, args);
    va_end(args);

    if (write(STDOUT_FILENO, buf, len) < 0) {
        perror("write failed");
        return -1;
    }
    return len;
}
