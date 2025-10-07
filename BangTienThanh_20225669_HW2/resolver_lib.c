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
struct Ipv4AddressList resolve_domain(const char *domain) {
    struct hostent *he = gethostbyname(domain);
    struct Ipv4AddressList list;
    list.count = 0;
    list.addresses = NULL;
    if (he == NULL || he->h_addrtype != AF_INET) {
        return list;
    }
    int count = 0;
    while (he->h_addr_list[count] != NULL) {
        count++;
        struct sockaddr_in *ipv4 = malloc(sizeof(struct sockaddr_in));
        ipv4->sin_family = AF_INET;
        memcpy(&ipv4->sin_addr, he->h_addr_list[count - 1],
                sizeof(struct in_addr));
        list.addresses = realloc(list.addresses, sizeof(struct sockaddr_in *) * count);
        list.addresses[count - 1] = ipv4;
    }
    list.count = count;
    return list;
}

/**
 * @function reverse_DNS_lookup: convert IPv4 address to domain name
 * @param ip_address: IPv4 address to resolve
 * @return: domain name, NULL if not found
 */
char* reverse_DNS_lookup(const struct sockaddr_in *ip_address) {
    struct hostent *host = gethostbyaddr(&ip_address->sin_addr, sizeof(ip_address->sin_addr), AF_INET);
    if (host == NULL) {
        return NULL;
    }
    return host->h_name;
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

struct sockaddr_in* string_to_ipv4(const char *ip_str) {
    struct sockaddr_in* ipv4 = malloc(sizeof(struct sockaddr_in));
    if (ipv4 == NULL) {
        return NULL;
    }
    ipv4->sin_family = AF_INET;

    if (inet_pton(AF_INET, ip_str, &(ipv4->sin_addr)) != 1) {
        free(ipv4);
        return NULL;
    }
    inet_pton(AF_INET, ip_str, &(ipv4->sin_addr));
    return ipv4;
}