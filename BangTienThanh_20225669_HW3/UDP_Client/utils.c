// utils.c
#include <stdio.h>
#include "utils.h"
#include <stdlib.h>

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