#include "bsp_dns_client.h"

#include <arpa/inet.h>
#include <stdio.h>

#include "bsp.h"

bsp_status_t bsp_dns_prase_domain(const char* domain_name,
                                  bsp_dns_addr_t* result) {
  struct addrinfo hints;
  struct addrinfo* tmp = NULL;

  char ipbuf[INET_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */

  int ans = getaddrinfo(domain_name, NULL, &hints, &tmp);

  if (ans) {
    return BSP_ERR;
  }

  struct sockaddr_in* addr = (struct sockaddr_in*)tmp->ai_addr;

  const char* addr_str = inet_ntop(AF_INET, &addr->sin_addr, ipbuf, 16);

  snprintf((char*)result, 16, "%s", addr_str);

  freeaddrinfo(tmp);

  return BSP_OK;
}
