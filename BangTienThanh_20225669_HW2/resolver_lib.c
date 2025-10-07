#include "resolver_lib.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/**
 * @function resolve_domain: convert domain name to IPv4 address, returns list of IPv4 addresses
 * @param domain: domain name to resolve
 * @return: list of IPv4 addresses
 */
Ipv4AddressList resolve_domain(const char *domain) {
    struct addrinfo hints, *res, *p;
    char ipstr[INET_ADDRSTRLEN];
    Ipv4AddressList result;
    result.addresses = NULL;
    result.count = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(domain, NULL, &hints, &res) != 0) {
        return result;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        struct sockaddr_in **temp = realloc(result.addresses, sizeof(struct sockaddr_in *) * (result.count + 1));
        // Error handling for realloc
        if (temp == NULL) {
            freeaddrinfo(res);
            for (int i = 0; i < result.count; i++) {
                free(result.addresses[i]);
            }
            free(result.addresses);
            result.addresses = NULL;
            result.count = 0;
            return result;
        }
        result.addresses = temp;
        result.addresses[result.count] = ipv4;
        result.count++;
    }

    // Free the linked list
    freeaddrinfo(res);
    return result;
}

/**
 * @function reverse_DNS_lookup: convert IPv4 address to domain name
 * @param ip_address: IPv4 address to resolve
 * @return: domain name, NULL if not found
 */
char* reverse_DNS_lookup(const struct sockaddr_in *ip_address) {
    char host[NI_MAXHOST];
    if (getnameinfo((const struct sockaddr *)ip_address, sizeof(struct sockaddr_in), host, sizeof(host), NULL, 0, 0) != 0) {
        return NULL;
    }

    char *result = strdup(host);
    return result;
}

/**
 * @function ipv4_to_string: convert IPv4 address to string
 * @param ipv4: IPv4 address to convert
 * @return: string representation of IPv4 address
 */
char* ipv4_to_string(const struct sockaddr_in *ipv4) {
    char *ip_str = malloc(INET_ADDRSTRLEN);
    if (ip_str == NULL) {
        return NULL;
    }
    inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
    return ip_str;
}

struct sockaddr_in string_to_ipv4(const char *ip_str) {
    struct sockaddr_in ipv4;
    memset(&ipv4, 0, sizeof(ipv4));
    ipv4.sin_family = AF_INET;
    inet_pton(AF_INET, ip_str, &(ipv4.sin_addr));
    return ipv4;
}