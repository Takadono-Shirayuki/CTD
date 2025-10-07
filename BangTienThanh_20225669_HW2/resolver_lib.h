// resolver_lib.h
#ifndef RESOLVER_LIB_H
#define RESOLVER_LIB_H
#include <stdio.h>
#include <netinet/in.h>

struct Ipv4AddressList
{
    struct sockaddr_in **addresses;
    int count;
};

struct Ipv4AddressList resolve_domain(const char *domain);
char* reverse_DNS_lookup(const struct sockaddr_in *ip_address);
char* ipv4_to_string(const struct sockaddr_in *ipv4);
struct sockaddr_in* string_to_ipv4(const char *ip_str);

#endif // RESOLVER_LIB_H