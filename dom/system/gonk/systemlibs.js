/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

if (!this.ctypes) {
  // We're likely being loaded as a JSM.
  this.EXPORTED_SYMBOLS = ["libcutils", "netHelpers"];
  const { ctypes } = ChromeUtils.import("resource://gre/modules/ctypes.jsm");
  this.ctypes = ctypes;
}

const SYSTEM_PROPERTY_KEY_MAX = 32;
const SYSTEM_PROPERTY_VALUE_MAX = 92;

// We leave this as 'undefined' instead of setting it to 'false'. That
// way a file that includes us can have it defined already without us
// overriding the value here.
var DEBUG;

/**
 * Expose some system-level functions.
 */
this.libcutils = (function() {
  let lib;
  try {
    lib = ctypes.open("libcutils.so");
  } catch (ex) {
    // Return a fallback option in case libcutils.so isn't present (e.g.
    // when building Firefox with MOZ_B2G_RIL.
    if (DEBUG) {
      dump("Could not load libcutils.so. Using fake propdb.\n");
    }
    let fake_propdb = Object.create(null);
    return {
      property_get(key, defaultValue) {
        if (key in fake_propdb) {
          return fake_propdb[key];
        }
        return defaultValue === undefined ? null : defaultValue;
      },
      property_set(key, value) {
        fake_propdb[key] = value;
      },
    };
  }

  let c_property_get = lib.declare(
    "property_get",
    ctypes.default_abi,
    ctypes.int, // return value: length
    ctypes.char.ptr, // key
    ctypes.char.ptr, // value
    ctypes.char.ptr
  ); // default
  let c_property_set = lib.declare(
    "property_set",
    ctypes.default_abi,
    ctypes.int, // return value: success
    ctypes.char.ptr, // key
    ctypes.char.ptr
  ); // value
  let c_value_buf = ctypes.char.array(SYSTEM_PROPERTY_VALUE_MAX)();

  return {
    /**
     * Get a system property.
     *
     * @param key
     *        Name of the property
     * @param defaultValue [optional]
     *        Default value to return if the property isn't set (default: null)
     */
    property_get(key, defaultValue) {
      if (defaultValue === undefined) {
        defaultValue = null;
      }
      c_property_get(key, c_value_buf, defaultValue);
      return c_value_buf.readString();
    },

    /**
     * Set a system property
     *
     * @param key
     *        Name of the property
     * @param value
     *        Value to set the property to.
     */
    property_set(key, value) {
      let rv = c_property_set(key, value);
      if (rv) {
        throw Error(
          `libcutils.property_set("${key}", "${value}") failed with error ${rv}`
        );
      }
    },
  };
})();

/**
 * Helpers for conversions.
 */
this.netHelpers = {
  /**
   * Swap byte orders for 32-bit value
   */
  swap32(n) {
    return (
      (((n >> 24) & 0xff) << 0) |
      (((n >> 16) & 0xff) << 8) |
      (((n >> 8) & 0xff) << 16) |
      (((n >> 0) & 0xff) << 24)
    );
  },

  /**
   * Convert network byte order to host byte order
   * Note: Assume that the system is little endian
   */
  ntohl(n) {
    return this.swap32(n);
  },

  /**
   * Convert host byte order to network byte order
   * Note: Assume that the system is little endian
   */
  htonl(n) {
    return this.swap32(n);
  },

  /**
   * Convert integer representation of an IP address to the string
   * representation.
   *
   * @param ip
   *        IP address in number format.
   */
  ipToString(ip) {
    return (
      ((ip >> 0) & 0xff) +
      "." +
      ((ip >> 8) & 0xff) +
      "." +
      ((ip >> 16) & 0xff) +
      "." +
      ((ip >> 24) & 0xff)
    );
  },

  /**
   * Convert string representation of an IP address to the integer
   * representation (network byte order).
   *
   * @param string
   *        String containing the IP address.
   */
  stringToIP(string) {
    if (!string) {
      return null;
    }
    let ip = 0;
    let start,
      end = -1;
    for (let i = 0; i < 4; i++) {
      start = end + 1;
      end = string.indexOf(".", start);
      if (end == -1) {
        end = string.length;
      }
      let num = parseInt(string.slice(start, end), 10);
      if (isNaN(num)) {
        return null;
      }
      ip |= num << (i * 8);
    }
    return ip;
  },

  /**
   * Make a subnet mask.
   */
  makeMask(len) {
    let mask = 0;
    for (let i = 0; i < len; ++i) {
      mask |= 0x80000000 >> i;
    }
    return this.ntohl(mask);
  },

  /**
   * Get Mask length from given mask address
   */
  getMaskLength(mask) {
    let len = 0;
    let netmask = this.ntohl(mask);
    while (netmask & 0x80000000) {
      len++;
      netmask = netmask << 1;
    }
    return len;
  },
};
