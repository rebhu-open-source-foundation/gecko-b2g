/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

/**
 * Abstraction on top of the network support from libnetutils that we
 * use to set up network connections.
 */

#ifndef DhcpUtils_h
#define DhcpUtils_h

class DhcpUtils {
 public:
  DhcpUtils();

  int GetDhcpResults(const char* interface, char* ipAddr, char* gateway,
                     uint32_t* prefixLength, char* dns[], char* server,
                     uint32_t* lease, char* vendorInfo, char* domain,
                     char* mtu);
  int DhcpStart(const char* interface);
  int DhcpStop(const char* interface);
  int DhcpRelease(const char* interface);
  int DhcpRenew(const char* interface);
  char* GetErrMsg();
};
#endif  // DhcpUtils_h
