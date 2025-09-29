#include <stdio.h>
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#define CMD_LOGIN "1"
#define CMD_POST "2"
#define CMD_LOGOUT "3"
#define CMD_EXIT "4"

void log_in(char **current_user, struct account_list *account_list, FILE *log_file)
{
    printf("Username: ");
    char *username = get_long_stdin_input('\n');
    if (*current_user != NULL) {
        printf("You have already logged in\n");
        log_message(log_file, '1', username, "-ERR");
        free(username);
        return;
    }
    struct account *acc = get_account_by_username(account_list, username);
    if (acc == NULL) {
        printf("Account is not exist\n");
        log_message(log_file, '1', username, "-ERR");
        free(username);
        return;
    }
    if (strcmp(acc->status, "0") == 0) {
        printf("Account is banned\n");
        log_message(log_file, '1', username, "-ERR");
        free(username);
        return;
    }
    *current_user = username;
    printf("Hello %s\n", *current_user);
    log_message(log_file, '1', username, "+OK");
    return;
}

void post_message(char *current_user, FILE *log_file)
{
printf("Post message: ");
    char *post_message = get_long_stdin_input('\n');
    if (post_message == NULL) {
        printf("Error reading post message\n");
        log_message(log_file, '2', "", "-ERR");
        return;
    }
    if (current_user == NULL) {
        printf("You have not logged in\n");
        log_message(log_file, '2', post_message, "-ERR");
        return;
    }
    printf("Successful post\n");
    log_message(log_file, '2', post_message, "+OK");
    free(post_message);
    return;
}

void log_out(char **current_user, FILE *log_file)
{
    if (*current_user == NULL) {
        printf("You have not logged in\n");
        log_message(log_file, '3', "", "-ERR");
        return;
    }
    printf("Successful log out\n");
    log_message(log_file, '3', "", "+OK");
    free(*current_user);
    *current_user = NULL;
    return;
}

void free_memory(struct account_list *account_list, char *current_user, FILE *log_file)
{
    // Giải phóng bộ nhớ
    for (int i = 0; i < account_list->count; i++) {
        free(account_list->accounts[i].username);
        free(account_list->accounts[i].status);
    }
    free(account_list->accounts);
    free(account_list);
    if (current_user != NULL) {
        free(current_user);
    }
    fclose(log_file);
}

int main() {
    // Mở file log để ghi lại các hoạt động
    FILE *log_file = fopen("log_20225669.txt", "a");
    if (log_file == NULL) {
        printf("Không thể mở file log\n");
        return 1;
    }

    struct account_list *account_list = read_accounts_from_file("account.txt");
    if (account_list == NULL) {
        return 1; // Lỗi đọc file hoặc cấp phát bộ nhớ
    }
    
    // Chương trình thực hiện các chức năng Đăng nhập, Đăng bài, Đăng xuất và Kết thúc
    char *current_user = NULL;
    while (1) {
        printf("--------------------------------------------------\n 1. Login \n 2. Post \n 3. Logout \n 4. Exit\nInput command: ");
        char *command = get_long_stdin_input('\n');
        if (command == NULL) {
            printf("Error reading command\n");
            continue;
        }
        if (strcmp(command, CMD_LOGIN) == 0)
        {
            log_in(&current_user, account_list, log_file);
            free(command);
            continue;
        }
        if (strcmp(command, CMD_POST) == 0)
        {
            post_message(current_user, log_file);
            free(command);
            continue;
        }
        if (strcmp(command, CMD_LOGOUT) == 0)
        {
            log_out(&current_user, log_file);
            free(command);
            continue;
        }
        if (strcmp(command, CMD_EXIT) == 0)
        {
            log_message(log_file, '4', "", "+OK");
            free(command);
            break;
        }
        printf("Invalid command\n");
        free(command);
    }

    // Giải phóng bộ nhớ và đóng file log
    free_memory(account_list, current_user, log_file);
    return 0;
}