
/**
 * @brief 待ち受けソケットの作成 (リッスンの開始まで行う)
 * @param port 待ち受けポート番号
 * @param backlog 待ち受けキューの長さ
 * @return ソケットのファイルディスクリプタ, エラー時 -1
 */
int make_listen_socket(int port, int backlog);

/**
 * @brief 待ち受けソケットの作成 (リッスンの開始まで行う)
 * @param port 待ち受けポート番号の格納先
 * @param backlog 待ち受けキューの長さ
 * @return ソケットのファイルディスクリプタ, エラー時 -1
 */
int make_dynamic_listen_socket(int *port, int backlog);

/**
 * @brief 接続ソケットの作成
 * @param host_name ホスト名
 * @param server_port ポート番号
 * @return ソケットのファイルディスクリプタ, エラー時 -1
 */
int make_connected_socket(const char* host_name, const char* server_port);

/**
 * @brief STDOUT_FILENO にフォーマットされた文字列を書き込む
 * @return 書き込んだ文字数, 異常時 -1
 */
int std_write(const char *format, ...);
