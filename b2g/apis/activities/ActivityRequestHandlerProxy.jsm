/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

function debug(aMsg) {
  //dump("-- ActivityRequestHandlerProxy: " + aMsg + "\n");
}

/*
 * ActivityRequestHandlerProxy is a helper of passing Activity result, from
 * WebActivityRequestHandler (running on service worker) to ActivitiesService.
 */

function ActivityRequestHandlerProxy() {
  // TODO: We used to observe "inner-window-destroyed" to moniter whether the
  // activity app window is closed before request is handled, but since
  // WebActivityRequestHandler is exposed to worker only, I'm not sure how to
  // retrieve the inner window ID out of it (innter window ID is the subject of
  // "inner-window-destroyed"). (Bug 80953)
  // TODO: We used to listen for "Activity:FireCancel" about cancelation from
  // activity requester, and should notify BrowserElementChildPreload with
  // "activity-done". however, since ActivityRequestHandlerProxy is now a proxy of
  // WebActivityRequestHandler (created on service worker), there is no way to
  // retrieve its window and turn it into docShell. (Bug 80953)
}

ActivityRequestHandlerProxy.prototype = {
  notifyActivityReady(id) {
    debug("notifyActivityReady");
    Services.cpmm.sendAsyncMessage("Activity:Ready", { id });
  },

  postActivityResult(id, result) {
    debug("postActivityResult");
    Services.cpmm.sendAsyncMessage("Activity:PostResult", {
      id,
      result,
    });
    // TODO: Should notify BrowserElementChildPreload with "activity-done"
    // that an WebActivity has handled, however, since ActivityRequestHandlerProxy is now
    // a proxy of WebActivityRequestHandler (created on service worker), there
    // is no way to retrieve its window and turn it into docShell. (Bug 80953)
  },

  postActivityError(id, error) {
    debug("postActivityError");
    Services.cpmm.sendAsyncMessage("Activity:PostError", {
      id,
      error,
    });
    // TODO: Should notify BrowserElementChildPreload with "activity-done"
    // that an WebActivity has handled, however, since ActivityRequestHandlerProxy is now
    // a proxy of WebActivityRequestHandler (created on service worker), there
    // is no way to retrieve its window and turn it into docShell. (Bug 80953)
  },

  contractID: "@mozilla.org/dom/activities/handlerproxy;1",

  classID: Components.ID("{47f2248f-2c8e-4829-a4bb-6ec1e886b2d6}"),

  QueryInterface: ChromeUtils.generateQI([Ci.nsIActivityRequestHandlerProxy]),
};

//module initialization
var EXPORTED_SYMBOLS = ["ActivityRequestHandlerProxy"];
