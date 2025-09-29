// utils.h
#ifndef UTILS_H
#define UTILS_H

struct account
{
    char *username;
    char *status;
};

struct account_list
{
    struct account *accounts;
    int count;
};

char *get_long_stdin_input(char end_char);
char *get_long_file_input(FILE *fp, char end_char);
void log_message(FILE *log_file, char command_type, char *message, char *result_code);
struct account_list *read_accounts_from_file(const char *filename);
struct account* get_account_by_username(struct account_list *list, const char *username);

#endif // UTILS_H