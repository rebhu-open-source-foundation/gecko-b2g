/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "WifiCommon"

#include "WifiCommon.h"
#include <iomanip>

template <typename T>
std::string ConvertMacToString(const T& mac) {
  size_t len = mac.size();
  if (len != 6) {
    return std::string();
  }
  std::ostringstream stream;
  stream << std::hex << std::uppercase;
  for (size_t i = 0; i < len; i++) {
    stream << std::setw(2) << std::setfill('0') << unsigned(mac[i]);
    if (i != len - 1) stream << ":";
  }
  return stream.str();
}
