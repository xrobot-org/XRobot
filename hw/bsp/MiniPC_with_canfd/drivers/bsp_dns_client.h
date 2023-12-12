#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <netdb.h>

#include "bsp.h"

typedef char bsp_dns_addr_t[INET_ADDRSTRLEN];

bsp_status_t bsp_dns_prase_domain(const char* domain_name,
                                  bsp_dns_addr_t* result);

#ifdef __cplusplus
}
#endif
