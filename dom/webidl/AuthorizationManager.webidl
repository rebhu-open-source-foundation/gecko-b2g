/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

dictionary HawkRestrictedToken
{
  // restricted access token stored as JWT (JSON Web Token)
  DOMString   accessToken = "";
  // type of this token, e.g. "hawk", bearer"
  DOMString   tokenType = "";
  // scope of this token, e.g. "core"
  DOMString   scope = "";
  // the key id of Hawk
  DOMString   kid = "";
  // the key to be used and matching the kid above, usually encoded in base64.
  DOMString   macKey = "";
  // the message authentication code (MAC) algorithm of Hawk,
  DOMString   macAlgorithm = "hmac-sha-256";
  // time to live of the current credential (in seconds)
  long long   expiresInSeconds = -1;
};

[
Exposed=(Window,Worker),
// TODO: to add permission control
]
interface AuthorizationManager {
  /**
   * Get a Hawk restricted token from cloud server via HTTPS.
   *
   * @param type
   *        Specify the cloud service you're asking for authorization
   * @return A promise object.
   *  Resolve params: a HawkRestrictedToken object represents an access token
   *  Reject params:  a integer represents HTTP status code
   */
  Promise<HawkRestrictedToken> getRestrictedToken(KaiServiceType type);
};

/**
 * Types of cloud service which require access token.
 * The uri (end point) and API key of individual service are configured by
 * Gecko Preference.
 */
enum KaiServiceType
{
  "service",  // configured by service.token.uri
};
