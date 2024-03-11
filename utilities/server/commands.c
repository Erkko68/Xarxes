/**
 * @file server:commands.c
 * @brief Functions to execute server commands.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */
#include "../commons.h"

void sanitizeString(char *str) {
    int start = 0, end = strlen(str) - 1;

    /*Find the first non-space character*/
    while (str[start] == ' ') {
        start++;
    }
    /*Find the last non-space character*/
    while (end > start && (str[end] == ' ')) {
        end--;
    }
    /*Shift the substring to the beginning of the original string*/
    memmove(str, str + start, end - start + 1);
    str[end - start + 1] = '\0'; /*Null-terminate the new string*/
}