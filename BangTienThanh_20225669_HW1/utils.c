// utils.c
#include <stdio.h>
#include "utils.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//---------- Hàm không export bên ngoài ----------
/**
 * @function compare_accounts: So sánh hai tài khoản theo username
 * @param a: Con trỏ đến tài khoản thứ nhất
 * @param b: Con trỏ đến tài khoản thứ hai
 * @return Giá trị âm nếu a < b, dương nếu a > b, 0 nếu a == b
 * @note Hàm này dùng để sắp xếp trong qsort và tìm kiếm tài khoản trong tìm kiếm nhị phân
 */
int compare_accounts(const void *a, const void *b) {
    if (a == NULL) return -1;
    if (b == NULL) return 1;
    struct account *accA = (struct account *)a;
    struct account *accB = (struct account *)b;
    return strcmp(accA->username, accB->username);
}

//---------- Hàm export bên ngoài ----------
/**
 * @function get_long_stdin_input: Đọc chuỗi dài từ stdin đến ký tự kết thúc end_char, EOF hoặc xuống dòng
 * @param end_char: Ký tự kết thúc
 * @return Chuỗi đọc được, NULL nếu lỗi hoặc không có dữ liệu
 */
char *get_long_stdin_input(char end_char) {
    int size = 8;
    char *str = malloc(size);
    int len = 0;
    int ch;
    if (!str) {
        return NULL;
    }
    while (1) {
        ch = getchar();
        if (ch == end_char || ch == EOF || ch == '\n' || ch == '\r') break;
        str[len++] = ch;
        if (len == size - 1) {
            size *= 2;
            char *new_str = realloc(str, size);
            if (!new_str) {
                free(str);
                return NULL;
            }
            str = new_str;
        }
    }
    if (len == 0) {
        free(str);
        return NULL;
    }
    str[len] = '\0';
    return str;
}

/**
 * @function get_long_file_input: Đọc chuỗi dài từ file đến ký tự kết thúc end_char, EOF hoặc xuống dòng
 * @param fp: Con trỏ file
 * @param end_char: Ký tự kết thúc
 * @return Chuỗi đọc được, NULL nếu lỗi hoặc không có dữ liệu
 */
char *get_long_file_input(FILE *fp, char end_char) {
    if (!fp) return NULL;

    int size = 8;
    char *str = malloc(size);
    if (!str) return NULL;

    int len = 0;
    int ch;

    while (1) {
        ch = fgetc(fp);
        if (ch == end_char || ch == EOF || ch == '\n' || ch == '\r') break;

        str[len++] = ch;

        if (len == size - 1) {
            size *= 2;
            char *new_str = realloc(str, size);
            if (!new_str) {
                free(str);
                return NULL;
            }
            str = new_str;
        }
    }
    if (len == 0) {
        free(str);
        return NULL;
    }
    str[len] = '\0';
    return str;
}

/**
 * @function read_accounts_from_file: Đọc danh sách tài khoản từ file
 * @param filename: Tên file
 * @return Con trỏ đến struct account_list chứa danh sách tài khoản, NULL nếu lỗi
 */
struct account_list *read_accounts_from_file(const char *filename) {
    // Mở file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Không thể mở file %s\n", filename);
        return NULL;
    }

    // Đọc dữ liệu từ file
    int count = 0, capacity = 10;
    struct account *accounts = malloc(capacity * sizeof(struct account));
    while (!feof(file)) {
        // Đọc username và status
        char *username = get_long_file_input(file, ' ');
        if (username != NULL) {
            char *status = get_long_file_input(file, '\n');
            if (status != NULL) {
                accounts[count].username = username;
                accounts[count].status = status;
                count++;
                // Xin cấp phát thêm bộ nhớ nếu cần
                if (count == capacity) {
                    capacity += 10;
                    struct account *new_accounts = realloc(accounts, capacity * sizeof(struct account));
                    if (new_accounts == NULL) {
                        printf("Lỗi cấp phát bộ nhớ\n");
                        free(accounts);
                        fclose(file);
                        return NULL;
                    }
                    accounts = new_accounts;
                }
            } else {
                free(username);
            }
        }
    }
    fclose(file);
    // Đóng gói kết quả vào struct account_list
    struct account_list *result = malloc(sizeof(struct account_list));
    result->accounts = accounts;
    result->count = count;
    // Sắp xếp danh sách tài khoản theo username để sử dụng tìm kiếm nhị phân
    qsort(result->accounts, result->count, sizeof(struct account), compare_accounts);
    return result;
}

// Tìm tài khoản theo username sử dụng tìm kiếm nhị phân
struct account* get_account_by_username(struct account_list *list, const char *username) {
    if (list == NULL || username == NULL) return NULL;

    int left = 0, right = list->count - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = compare_accounts(&list->accounts[mid], &(struct account){.username = (char *)username, .status = NULL});
        if (cmp == 0) {
            return &list->accounts[mid];
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return NULL; // Không tìm thấy
}

/**
 * @function log_message: Ghi log vào file với định dạng [dd/mm/yyyy hh:mm:ss] $ Chức năng lựa chọn $ Giá trị người dùng cung cấp $ Mã kết quả
 * @param fp: Con trỏ file log
 * @param command_type: Chức năng lựa chọn ('1' - Đăng nhập, '2' - Đăng bài, '3' - Đăng xuất, '4' - Kết thúc)
 * @param message: Giá trị người dùng cung cấp (username hoặc nội dung bài đăng)
 * @param result_code: Mã kết quả ("+OK" hoặc "-ERR")
 * @return void
 */
void log_message(FILE *fp, char command_type, char *message, char *result_code) {
    // Ghi log vào file với định dạng [dd/mm/yyyy hh:mm:ss] $ Chức năng lựa chọn $ Giá trị người dùng cung cấp $ Mã kết quả
    if (!fp || !message || !result_code) return;

    // Lấy thời gian hiện tại
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%d/%m/%Y %H:%M:%S", t);

    // Nếu message không rỗng thì thêm dấu cách vào cuối
    if (strlen(message) > 0 && message[strlen(message) - 1] != ' ') {
        size_t len = strlen(message);
        char *new_message = malloc(len + 2);
        if (!new_message) return;
        strcpy(new_message, message);
        new_message[len] = ' ';
        new_message[len + 1] = '\0';
        message = new_message;
    }

    // Ghi log vào file
    fprintf(fp, "[%s] $ %c $ %s$ %s\n", time_str, command_type, message, result_code);
    fflush(fp);
}