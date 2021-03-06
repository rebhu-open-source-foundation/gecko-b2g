/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

/* global is, log, ok, startTestCommon, getMozIcc */

const MARIONETTE_TIMEOUT = 30000;
const MARIONETTE_HEAD_JS = "head.js";

const TEST_DATA = [
  // mvno type, mvno data, request success, expected result
  // Emulator's hard coded IMSI: 310410000000000
  ["imsi", "3104100", true, true],
  // x and X means skip the comparison.
  ["imsi", "31041xx0", true, true],
  ["imsi", "310410x0x", true, true],
  ["imsi", "310410X00", true, true],
  ["imsi", "310410XX1", true, false],
  ["imsi", "31041012", true, false],
  ["imsi", "310410000000000", true, true],
  ["imsi", "310410000000000123", true, false],
  ["imsi", "", false, "InvalidParameter"],
  // Emulator's hard coded SPN:  Android
  ["spn", "Android", true, true],
  ["spn", "", false, "InvalidParameter"],
  ["spn", "OneTwoThree", true, false],
  // Emulator's hard coded GID1: 5a4d
  ["gid", "", false, "InvalidParameter"],
  ["gid", "A1", true, false],
  ["gid", "5A", true, true],
  ["gid", "5a", true, true],
  ["gid", "5a4d", true, true],
  ["gid", "5A4D", true, true],
  ["gid", "5a4d6c", true, false],
];

function testMatchMvno(aIcc, aMvnoType, aMvnoData, aSuccess, aExpectedResult) {
  log("matchMvno: " + aMvnoType + ", " + aMvnoData);
  return aIcc.matchMvno(aMvnoType, aMvnoData).then(
    aResult => {
      log("onsuccess: " + aResult);
      ok(aSuccess, "onsuccess while error expected");
      is(aResult, aExpectedResult);
    },
    aError => {
      log("onerror: " + aError.name);
      ok(!aSuccess, "onerror while success expected");
      is(aError.name, aExpectedResult);
    }
  );
}

// Start tests
startTestCommon(function() {
  let icc = getMozIcc();
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testMatchMvno.apply(null, [icc].concat(data)));
  }
  return promise;
});
