/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

/* global log, is, ok, mozContact, startTestCommon, getMozIcc */

const MARIONETTE_TIMEOUT = 60000;
const MARIONETTE_HEAD_JS = "head.js";
const MARIONETTE_CONTEXT = "chrome";

function testReadContacts(aIcc, aType) {
  log("testReadContacts: type=" + aType);
  let iccId = aIcc.iccInfo.iccid;
  return aIcc.readContacts(aType).then(
    aResult => {
      is(Array.isArray(aResult), true);
      is(aResult.length, 6, "Check contact number.");

      // Alpha Id(Encoded with GSM 8 bit): "Mozilla", Dialling Number: 15555218201
      is(aResult[0].name[0], "Mozilla");
      is(aResult[0].tel[0].value, "15555218201");
      is(aResult[0].id, iccId + "1");

      // Alpha Id(Encoded with UCS2 0x80: "Saßê\u9ec3", Dialling Number: 15555218202
      is(aResult[1].name[0], "Saßê黃");
      is(aResult[1].tel[0].value, "15555218202");
      is(aResult[1].id, iccId + "2");

      // Alpha Id(Encoded with UCS2 0x81): "Fire \u706b", Dialling Number: 15555218203
      is(aResult[2].name[0], "Fire 火");
      is(aResult[2].tel[0].value, "15555218203");
      is(aResult[2].id, iccId + "3");

      // Alpha Id(Encoded with UCS2 0x82): "Huang \u9ec3", Dialling Number: 15555218204
      is(aResult[3].name[0], "Huang 黃");
      is(aResult[3].tel[0].value, "15555218204");
      is(aResult[3].id, iccId + "4");

      // Alpha Id(Encoded with GSM 8 bit): "Contact001",
      // Dialling Number: 9988776655443322110001234567890123456789
      is(aResult[4].name[0], "Contact001");
      is(aResult[4].tel[0].value, "9988776655443322110001234567890123456789");
      is(aResult[4].id, iccId + "5");

      // Alpha Id(Encoded with GSM 8 bit): "Contact002",
      // Dialling Number: 0123456789012345678999887766554433221100
      is(aResult[5].name[0], "Contact002");
      is(aResult[5].tel[0].value, "0123456789012345678999887766554433221100");
      is(aResult[5].id, iccId + "6");
    },
    aError => {
      ok(false, "Cannot get " + aType + " contacts");
    }
  );
}

// Start tests
startTestCommon(function() {
  let icc = getMozIcc();

  // Test read adn contacts
  return (
    testReadContacts(icc, "adn")
      // Test read fdn contact
      .then(() => testReadContacts(icc, "fdn"))
      // Test read sdn contacts
      .then(() => testReadContacts(icc, "sdn"))
  );
});
