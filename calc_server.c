#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8000
#define MAX_CONNECTIONS 4096 // cat /proc/sys/net/core/somaxconn
#define BUFFER_SIZE 4096

int parse_and_calculate(const char *query) {

    // query=2+10 の "query=" を飛ばす
    const char *expr = query;
    const char *equal = strchr(query, '=');
    if (equal) {
        expr = equal + 1;
    }

    int a = 0, b = 0;

    sscanf(expr, "%d+%d", &a, &b);

    return a + b;
}

int main() {
    int sockfd = socket(
        PF_INET,     // TODO: PF_INET6でIPv6に対応できる
        SOCK_STREAM,
        IPPROTO_TCP
    );
    if (sockfd < 0) {
        perror("failed to create socket"); // TODO: system call のエラーへの対処ってこれでいいのか?
        close(sockfd); // TODO: 自動でcloseしてくれている?のでいらないかも
        exit(1);
    }

    int ov = 4;
    int result_setsockopt = setsockopt(
        sockfd,
        SOL_SOCKET,   // オプションのレベル（ソケットレベル）
        SO_REUSEADDR, // アドレス（ポート）を再利用可能にするオプション
        &ov,          // TODO: ここのoptionの意味が分からない とりあえずman pageの例を真似した
        sizeof(ov) 
    );
    if (result_setsockopt < 0) {
        perror("failed to set socket options");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in addr = {
        .sin_family = PF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(PORT)};

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("failed to bind socket");
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, MAX_CONNECTIONS) < 0) {
        perror("failed to listen on socket");
        close(sockfd);
        exit(1);
    }

    while (1) {
        int accepted_sockfd = accept(sockfd, NULL, NULL);
        if (accepted_sockfd < 0) {
            perror("failed to accept connection");
            continue;
        }

        char buffer[BUFFER_SIZE];
        int n = read(accepted_sockfd, buffer, BUFFER_SIZE - 1);
        if (n <= 0) {
            perror("failed to read from socket");
            close(accepted_sockfd);
            continue;
        }
        buffer[n] = '\0';

        printf("================\n");
        printf("buffer:\n%s\n", buffer);
        printf("================\n");

        // TODO: HTTPリクエストの構造体を定義したりパース処理を関数化したい
        char request_method[8]; // TODO: mallocを使わないと動的に確保できない
        char request_path_and_query[8192];
        char http_version[16];
        sscanf(buffer, "%s %s %s\r\n", request_method, request_path_and_query, http_version);
        // printf("request_method: %s\n", request_method);
        // printf("http_version: %s\n", http_version);
        
        char request_path[8192];
        char request_query[8192];
        char *qmark = strchr(request_path_and_query, '?');
        if (qmark) {
            // path
            size_t path_len = qmark - request_path_and_query;
            strncpy(request_path, request_path_and_query, path_len);
            request_path[path_len] = '\0';

            // query
            strcpy(request_query, qmark + 1);
        } else {
            // queryなし
            strcpy(request_path, request_path_and_query);
            request_query[0] = '\0';
            // TODO: エラー起こしたいがCでは例外処理がない?のでどうしようか
        }
        // printf("request_path: %s\n", request_path);
        // printf("request_query: %s\n", request_query);

        // TODO: リクエストメソッドやパスのチェックを入れたい
        // 想定のAPI形式でなければ400 Bad Requestを返すなど

        int result = parse_and_calculate(request_query);

        char http_status_line[64];
        snprintf(http_status_line, sizeof(http_status_line), "HTTP/1.1 200 OK\r\n");


        char response_body[64];
        snprintf(response_body, sizeof(response_body), "%d\n", result);
        int content_length = strlen(response_body);

        // HTTP response 作成
        char response[256];
        int response_len = snprintf(
            response,
            sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            content_length,
            response_body
        );

        write(accepted_sockfd, response, response_len);
        close(accepted_sockfd);
    }

    close(sockfd);
    return 0;
}
