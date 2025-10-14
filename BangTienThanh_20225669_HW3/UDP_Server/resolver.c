#include "resolver_lib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    // get domain or IP address from command line
    if (argc != 2) {
        printf("Usage: %s <domain|IPv4 address>\n", argv[0]);
        return 1;
    }

    char* input = argv[1];
    // check if input is an IPv4 address
    struct sockaddr_in *ipv4 = string_to_ipv4(input);
    if (ipv4 != NULL) {
        // reverse DNS lookup
        char *domain = reverse_DNS_lookup(ipv4);
        if (domain != NULL) {
            printf("Result: \n%s\n", domain);
        } else {
            printf("Not found information\n");
        }
        free(ipv4);
    } else {
        // resolve domain to IPv4 addresses
        struct Ipv4AddressList list = resolve_domain(input);
        if (list.count == 0) {
            printf("Not found information\n");
        } else {
            printf("Result: \n");
            for (int i = 0; i < list.count; i++) {
                char *ip_str = ipv4_to_string(list.addresses[i]);
                if (ip_str != NULL) {
                    printf("%s\n", ip_str);
                    free(ip_str);
                }
            }
            // Free allocated memory for addresses
            free(list.addresses);
        }
    }
}