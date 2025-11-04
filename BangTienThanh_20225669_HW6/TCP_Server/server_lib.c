// server_lib.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define MAXLINE 2048
#define MAX_USER 256
#define USERNAME_LEN 64
#define POST_FILE "posts.txt"

/* Account information
 * username - account name
 * status   - 0 = locked, 1 = active
 */
typedef struct {
    char username[USERNAME_LEN];
    int status; // 0 locked, 1 active
} account_t;

/* Session information for each client connection
 * client_fd   - client's socket descriptor
 * client_addr - client's address
 * username    - logged-in username (if any)
 * logged_in   - flag indicating whether the client is logged in
 */
typedef struct session {
    int client_fd;
    struct sockaddr_in client_addr;
    char username[USERNAME_LEN];
    bool logged_in;
} session_t;

/* Global account list and logged-in map */
static account_t accounts[MAX_USER];
static int account_count = 0;
static bool logged_in_flags[MAX_USER];
static pthread_mutex_t accounts_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Utility: remove trailing CR/LF and whitespace from a string.
 * Modifies the input string in place.
 */
static void trim_line(char *s) {
    int n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
        s[--n] = '\0';
    }
}

/* Load account list from a file.
 * File format: <username> <status> per line
 * Returns 0 on success, -1 if file cannot be opened.
 */
int load_accounts(const char *filename) {
    FILE *f = fopen(filename, "r");
    char altpath[PATH_MAX];
    /* If opening the given filename fails, try the TCP_Server/ subfolder.
     * This allows running the program from the repository root while the
     * account file remains in the TCP_Server directory next to server.c.
     */
    if (!f) {
        int ret = snprintf(altpath, sizeof(altpath), "TCP_Server/%s", filename);
        if (ret >= 0 && (size_t)ret < sizeof(altpath)) {
            f = fopen(altpath, "r");
        }
    }
    if (!f) return -1;
    char line[256];
    account_count = 0;
    while (fgets(line, sizeof(line), f) && account_count < MAX_USER) {
        trim_line(line);
        if (line[0] == '\0') continue;
        char user[USERNAME_LEN];
        int status;
        if (sscanf(line, "%63s %d", user, &status) == 2) {
            strncpy(accounts[account_count].username, user, USERNAME_LEN-1);
            accounts[account_count].username[USERNAME_LEN-1] = '\0';
            accounts[account_count].status = status ? 1 : 0;
            logged_in_flags[account_count] = false;
            account_count++;
        }
    }
    fclose(f);
    return 0;
}

/* Find account index by username. Returns -1 if not found. */
static int find_account_index(const char *username) {
    for (int i = 0; i < account_count; ++i) {
        if (strcmp(accounts[i].username, username) == 0) return i;
    }
    return -1;
}

/* Send a single line (append CRLF) to socket.
 * Returns number of bytes sent, or -1 on error.
 */
static ssize_t send_line(int fd, const char *s) {
    char buf[MAXLINE];
    int n = snprintf(buf, sizeof(buf), "%s\r\n", s);
    if (n < 0) return -1;
    return send(fd, buf, n, 0);
}

/* Save a post to POST_FILE.
 * Format: [YYYY-MM-DD hh:mm:ss] username: article
 */
static void save_post(const char *username, const char *article) {
    FILE *f = fopen(POST_FILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            username, article);
    fclose(f);
}

/* Receive a line from socket until '\n'.
 * Returns number of bytes read, 0 if connection closed, or -1 on error.
 */
static ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t idx = 0;
    char c;
    ssize_t n;
    while (idx + 1 < maxlen) {
        n = recv(fd, &c, 1, 0);
        if (n == 1) {
            buf[idx++] = c;
            if (c == '\n') break;
        } else if (n == 0) {
            if (idx == 0) return 0;
            break;
        } else {
            if (errno == EINTR) continue;
            return -1;
        }
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

/* Set the logged-in flag for account at index idx. */
static void set_logged_flag(int idx, bool val) {
    if (idx >= 0 && idx < account_count) logged_in_flags[idx] = val;
}

/* Per-client thread handler.
 * Handles commands from the client: USER, POST, BYE, etc., and sends
 * appropriate numeric responses. Thread exits and frees resources when
 * the client disconnects or sends BYE.
 */
static void *client_thread(void *arg) {
    session_t sess = *((session_t *)arg);
    free(arg);
    char rbuf[MAXLINE];
    char cmd[MAXLINE];

    send_line(sess.client_fd, "100");

    while (1) {
        ssize_t rc = recv_line(sess.client_fd, rbuf, sizeof(rbuf));
        if (rc == 0) { /* client closed */
            pthread_mutex_lock(&accounts_mutex);
            if (sess.logged_in) {
                int idx = find_account_index(sess.username);
                if (idx != -1) set_logged_flag(idx, false);
            }
            pthread_mutex_unlock(&accounts_mutex);
            close(sess.client_fd);
            return NULL;
        } else if (rc < 0) {
            pthread_mutex_lock(&accounts_mutex);
            if (sess.logged_in) {
                int idx = find_account_index(sess.username);
                if (idx != -1) set_logged_flag(idx, false);
            }
            pthread_mutex_unlock(&accounts_mutex);
            close(sess.client_fd);
            return NULL;
        }

        trim_line(rbuf);
        if (strlen(rbuf) == 0) {
            send_line(sess.client_fd, "300");
            continue;
        }

        char *space = strchr(rbuf, ' ');
        char verb[32];
        if (space) {
            size_t vlen = (size_t)(space - rbuf);
            if (vlen >= sizeof(verb)) vlen = sizeof(verb)-1;
            memcpy(verb, rbuf, vlen);
            verb[vlen] = '\0';
            snprintf(cmd, sizeof(cmd), "%s", space + 1);
        } else {
            snprintf(verb, sizeof(verb), "%s", rbuf);
            cmd[0] = '\0';
        }

        for (char *p = verb; *p; ++p) *p = toupper((unsigned char)*p);

        if (strcmp(verb, "USER") == 0) {
            if (cmd[0] == '\0') {
                send_line(sess.client_fd, "300");
                continue;
            }
            pthread_mutex_lock(&accounts_mutex);
            if (sess.logged_in) {
                pthread_mutex_unlock(&accounts_mutex);
                send_line(sess.client_fd, "213");
                continue;
            }
            int idx = find_account_index(cmd);
            if (idx == -1) {
                pthread_mutex_unlock(&accounts_mutex);
                send_line(sess.client_fd, "212");
                continue;
            }
            if (accounts[idx].status == 0) {
                pthread_mutex_unlock(&accounts_mutex);
                send_line(sess.client_fd, "211");
                continue;
            }
            if (logged_in_flags[idx]) {
                pthread_mutex_unlock(&accounts_mutex);
                send_line(sess.client_fd, "214");
                continue;
            }
            set_logged_flag(idx, true);
            strncpy(sess.username, accounts[idx].username, USERNAME_LEN-1);
            sess.username[USERNAME_LEN-1] = '\0';
            sess.logged_in = true;
            pthread_mutex_unlock(&accounts_mutex);
            send_line(sess.client_fd, "110");
        }
        else if (strcmp(verb, "POST") == 0) {
            if (cmd[0] == '\0') {
                send_line(sess.client_fd, "300");
                continue;
            }
            pthread_mutex_lock(&accounts_mutex);
            bool is_logged = sess.logged_in;
            pthread_mutex_unlock(&accounts_mutex);
            if (!is_logged) {
                send_line(sess.client_fd, "221");
                continue;
            }
            save_post(sess.username, cmd);
            send_line(sess.client_fd, "120");
        }
        else if (strcmp(verb, "BYE") == 0) {
            pthread_mutex_lock(&accounts_mutex);
            if (!sess.logged_in) {
                pthread_mutex_unlock(&accounts_mutex);
                send_line(sess.client_fd, "221");
                continue;
            }
            int idx = find_account_index(sess.username);
            if (idx != -1) set_logged_flag(idx, false);
            sess.logged_in = false;
            sess.username[0] = '\0';
            pthread_mutex_unlock(&accounts_mutex);
            send_line(sess.client_fd, "130");
            close(sess.client_fd);
            return NULL;
        }
        else {
            send_line(sess.client_fd, "300");
        }
    }
}

/* Start listening on port and serve incoming connections.
 * Each accepted connection is handled in a detached thread.
 * Returns 0 on normal exit, 1 on setup error.
 */
int server_listen_and_serve(int port) {
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, 16) < 0) {
        perror("listen");
        close(listenfd);
        return 1;
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        session_t *sess = malloc(sizeof(session_t));
        if (!sess) {
            close(connfd);
            continue;
        }
        sess->client_fd = connfd;
        sess->client_addr = cliaddr;
        sess->username[0] = '\0';
        sess->logged_in = false;

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&tid, &attr, client_thread, sess) != 0) {
            perror("pthread_create");
            close(connfd);
            free(sess);
        }
        pthread_attr_destroy(&attr);
    }

    close(listenfd);
    return 0;
}
