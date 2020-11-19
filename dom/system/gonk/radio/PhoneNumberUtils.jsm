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

"use strict";

this.EXPORTED_SYMBOLS = ["PhoneNumberUtils"];

//TODO: MIN_MATCH should support customization for different project/region.
const MIN_MATCH = 7;
const MAX_PHONE_NUMBER_LENGTH = 50;
const NON_DIALABLE_CHARS = /[^,#+\*\d]/g;
const NON_DIALABLE_CHARS_ONCE = new RegExp(NON_DIALABLE_CHARS.source);
const UNICODE_DIGITS = /[\uFF10-\uFF19\u0660-\u0669\u06F0-\u06F9]/g;
const VALID_ALPHA_PATTERN = /[a-zA-Z]/g;
const LEADING_PLUS_CHARS_PATTERN = /^[+\uFF0B]+/g;

// Map letters to numbers according to the ITU E.161 standard
var E161 = {
  a: 2,
  b: 2,
  c: 2,
  d: 3,
  e: 3,
  f: 3,
  g: 4,
  h: 4,
  i: 4,
  j: 5,
  k: 5,
  l: 5,
  m: 6,
  n: 6,
  o: 6,
  p: 7,
  q: 7,
  r: 7,
  s: 7,
  t: 8,
  u: 8,
  v: 8,
  w: 9,
  x: 9,
  y: 9,
  z: 9,
};

this.PhoneNumberUtils = {
  normalize(aNumber, numbersOnly) {
    if (typeof aNumber !== "string") {
      return "";
    }

    aNumber = aNumber.replace(UNICODE_DIGITS, function(ch) {
      return String.fromCharCode(48 + (ch.charCodeAt(0) & 0xf));
    });
    if (!numbersOnly) {
      aNumber = aNumber.replace(VALID_ALPHA_PATTERN, function(ch) {
        return String(E161[ch.toLowerCase()] || 0);
      });
    }
    aNumber = aNumber.replace(LEADING_PLUS_CHARS_PATTERN, "+");
    aNumber = aNumber.replace(NON_DIALABLE_CHARS, "");
    return aNumber;
  },

  isPlainPhoneNumber(aNumber) {
    if (typeof aNumber !== "string") {
      return false;
    }

    var length = aNumber.length;
    var isTooLong = length > MAX_PHONE_NUMBER_LENGTH;
    var isEmpty = length === 0;
    return !(isTooLong || isEmpty || NON_DIALABLE_CHARS_ONCE.test(aNumber));
  },

  match(aNumber1, aNumber2) {
    if (typeof aNumber1 !== "string" || typeof aNumber2 !== "string") {
      return false;
    }

    let normalizedNumber1 = this.normalize(aNumber1);
    let normalizedNumber2 = this.normalize(aNumber2);

    if (normalizedNumber1 == normalizedNumber2) {
      return true;
    }

    let index1 = normalizedNumber1.length - 1;
    let index2 = normalizedNumber2.length - 1;
    let matched = 0;

    while (index1 >= 0 && index2 >= 0) {
      if (normalizedNumber1[index1] != normalizedNumber2[index2]) {
        break;
      }
      index1--;
      index2--;
      matched++;
    }

    if (matched >= MIN_MATCH) {
      return true;
    }
    return false;
  },
};
