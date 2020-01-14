/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

dictionary ContactAddress {
  sequence<DOMString>? type = null;
  DOMString? streetAddress = null;
  DOMString? locality = null;
  DOMString? region = null;
  DOMString? postalCode = null;
  DOMString? countryName = null;
  boolean? pref = null;
};

dictionary ContactField {
  sequence<DOMString>? type = null;
  DOMString?           value = null;
  boolean?             pref = null;
};

dictionary ContactTelField : ContactField {
  DOMString? carrier = null;
};

dictionary ContactProperties {
  Date?                          bday = null;
  Date?                          anniversary = null;

  DOMString?                     sex = null;
  DOMString?                     genderIdentity = null;

  sequence<Blob>?                photo = null;

  DOMString?                     ringtone = null;

  sequence<ContactAddress>?  adr = null;

  sequence<ContactField>?    email = null;
  sequence<ContactField>?    url = null;
  sequence<ContactField>?    impp = null;

  sequence<ContactTelField>? tel = null;

  sequence<DOMString>?           name = null;
  sequence<DOMString>?           honorificPrefix = null;
  sequence<DOMString>?           givenName = null;
  sequence<DOMString>?           phoneticGivenName = null;
  sequence<DOMString>?           additionalName = null;
  sequence<DOMString>?           familyName = null;
  sequence<DOMString>?           phoneticFamilyName = null;
  sequence<DOMString>?           honorificSuffix = null;
  sequence<DOMString>?           nickname = null;
  sequence<DOMString>?           category = null;
  sequence<DOMString>?           org = null;
  sequence<DOMString>?           jobTitle = null;
  sequence<DOMString>?           note = null;
  sequence<DOMString>?           key = null;
  sequence<DOMString>?           group;
};

[Exposed=Window,
 JSImplementation="@mozilla.org/contact;1"]
interface mozContact {

  constructor(optional ContactProperties properties={});
                 attribute DOMString  id;
        readonly attribute Date?      published;
        readonly attribute Date?      updated;

                 attribute Date?      bday;
                 attribute Date?      anniversary;

                 attribute DOMString? sex;
                 attribute DOMString? genderIdentity;

                 attribute DOMString? ringtone;
/*
  [Cached, Pure] attribute sequence<Blob>?            photo;

  [Cached, Pure] attribute sequence<ContactAddress>?  adr;

  [Cached, Pure] attribute sequence<ContactField>?    email;
  [Cached, Pure] attribute sequence<ContactField>?    url;
  [Cached, Pure] attribute sequence<ContactField>?    impp;

  [Cached, Pure] attribute sequence<ContactTelField>? tel;

  [Cached, Pure] attribute sequence<DOMString>?       name;
  [Cached, Pure] attribute sequence<DOMString>?       honorificPrefix;
  [Cached, Pure] attribute sequence<DOMString>?       givenName;
  [Cached, Pure] attribute sequence<DOMString>?       phoneticGivenName;
  [Cached, Pure] attribute sequence<DOMString>?       additionalName;
  [Cached, Pure] attribute sequence<DOMString>?       familyName;
  [Cached, Pure] attribute sequence<DOMString>?       phoneticFamilyName;
  [Cached, Pure] attribute sequence<DOMString>?       honorificSuffix;
  [Cached, Pure] attribute sequence<DOMString>?       nickname;
  [Cached, Pure] attribute sequence<DOMString>?       category;
  [Cached, Pure] attribute sequence<DOMString>?       org;
  [Cached, Pure] attribute sequence<DOMString>?       jobTitle;
  [Cached, Pure] attribute sequence<DOMString>?       note;
  [Cached, Pure] attribute sequence<DOMString>?       key;
  [Cached, Pure] attribute sequence<DOMString>?       group;
*/
  void init(optional ContactProperties properties={});

  [ChromeOnly]
  void setMetadata(DOMString id, Date? published, Date? updated);

  //jsonifier;
};

dictionary ContactFindSortOptions {
  /**
   * Possible values for contacts: "givenName", "familyName"
   * Possible values for group: "name"
   * Possible values for block number: "number"
   */
  DOMString sortBy = "";

  /**
   * Possible values: "ascending", "descending"
   */
  DOMString sortOrder = "ascending";

  DOMString sortLanguage = "en";    // e.g. "de", "zh-Hant-TW"
};

dictionary ContactFindOptions : ContactFindSortOptions {
  DOMString      filterValue = "";  // e.g. "Tom"
  /**
   * Possible values: equals, contains, match, startsWith, fuzzyMatch.
   * contains, match and fuzzyMatch only supported by filterValue 'tel'.
   * contains: *<filterValue>*.
   * match: exact match in international, national formats.
   * fuzzyMatch: *<filterValue>, right to left digit comparison.
   */
  DOMString      filterOp = "";     // e.g. "startsWith"
  any            filterBy = null;     // e.g. ["givenName", "nickname"]
  unsigned long  filterLimit = 0;
};

enum ContactType {"device"};

dictionary ContactClearOptions {
  /**
   * The contact type to delete, possible values are 'device'
   * If no given, it will clear all conacts including device and sim.
   */
  ContactType type = "device";
  /**
   * To delete contacts only.
   * Default false.
   * If false, to clear all contacts including contacts, group and block number info.
   * If true, to clear contacts and keep group, block number info.
   */
  boolean contactOnly = false;
};

[NoInterfaceObject, Exposed=Window,
 JSImplementation="@mozilla.org/contactManager;1"]
 //CheckAnyPermissions="contacts-read contacts-write contacts-create"]
interface ContactManager : EventTarget {
  DOMRequest find(optional ContactFindOptions options={});
//  DOMCursor  getAll(optional ContactFindSortOptions options);
//  DOMCursor  findWithCursor(optional ContactFindOptions options);
  /**
   * To clear all contacts and related data.
   * @param options: the clear options
   */
  DOMRequest clear(optional ContactClearOptions options={});

  /**
   * @param contact The contact object be saved to db.
   *                Could be new or update.
   * @return DOMRequest contact id could be retrieved via DOMRequest.result if succeed.
   */
  DOMRequest save(mozContact contact);

  /**
   * @param contactOrId The contact object or id you are going to remove from db.
   * @return DOMRequest contact id could be retrieved via DOMRequest.result if succeed.
   */
  DOMRequest remove((mozContact or DOMString) contactOrId);
  DOMRequest getRevision();
  DOMRequest getCount();
  DOMRequest getSpeedDials();
  DOMRequest setSpeedDial(DOMString speedDial, DOMString tel, optional DOMString contactId);
  DOMRequest removeSpeedDial(DOMString speedDial);

  /**
   * To get all groups with given parameter.
   * @return An array of {id, name}.
   */
  //DOMCursor getAllGroups(optional ContactFindSortOptions options);

  /**
   * @return An array of {id, name}
   */
  DOMRequest findGroups(optional ContactFindOptions options={});

  /**
   * @param name The group name.
   * @param id The group id. If no id is given, it means create a new group.
   * @return DOMRequest group id can be retrieved via DOMRequest.result if succeed.
   */
  DOMRequest saveGroup(DOMString name, optional DOMString id);

  /**
   * To remove a group.
   * @param id The group id to be removed.
   * @return DOMRequest group id can be retrieved via DOMRequest.result if succeed.
   */
  DOMRequest removeGroup(DOMString id);

  /**
   * To get all blocked numbers with given parameter.
   * @param options The sorting option.
   * @return DOMCursor, every entry is a blocked number string.
   */
  //DOMCursor getAllBlockedNumbers(optional ContactFindSortOptions options);

  /**
   * To find blocked numbers with given parameter.
   * @param options The query option.
   *   To simplify the number search, we always apply fuzzyMatch on field 'number'.
   * @return DOMRequest, result is an array of blobked number string.
   */
  DOMRequest findBlockedNumbers(optional ContactFindOptions options={});

  /**
   * To save a number as blocked number.
   * If saving number is already exists, event onblocknumberchange wouldn't be triggered.
   * User always get success if saving identical number multiple times.
   * @param number The number to be blocked.
   */
  DOMRequest saveBlockedNumber(DOMString number);

  /**
   * To unblock a number.
   * @param number The number to be unblocked.
   */
  DOMRequest removeBlockedNumber(DOMString number);

  attribute EventHandler oncontactchange;
  attribute EventHandler onspeeddialchange;
  /* possible reasons:
   *   create, update, remove
   */
  attribute EventHandler oncontactgroupchange;
  /**
   * The event struct is {number, reason}
   * possible reasons:
   *   create, remove
   */
  attribute EventHandler onblockednumberchange;
};
