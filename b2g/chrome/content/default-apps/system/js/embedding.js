/* eslint-disable quotes */
"use strict";

(function(exports) {
  const kPreallocLaunchDelay = 5000; // ms of wait time before launching a new preallocated process.

  function log(msg) {
    console.log(`Embedding: ${msg}`);
  }

  function error(msg) {
    console.error(`Embedding: ${msg}`);
  }

  const windowProvider = {
    openURI(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp) {
      log(`browserWindow::openURI ${aURI.spec}`);
      throw "NOT IMPLEMENTED";
    },

    createContentWindow(
      aURI,
      aOpener,
      aWhere,
      aFlags,
      aTriggeringPrincipal,
      aCsp
    ) {
      log(`browserWindow::createContentWindow ${aURI.spec}`);
      throw "NOT IMPLEMENTED";
    },

    openURIInFrame(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName) {
      // We currently ignore aNextRemoteTabId on mobile.  This needs to change
      // when Fennec starts to support e10s.  Assertions will fire if this code
      // isn't fixed by then.
      //
      // We also ignore aName if it is set, as it is currently only used on the
      // e10s codepath.
      log(
        `browserWindow::openURIInFrame ${aURI.spec} ${aParams} ${aWhere} ${aFlags} ${aName}`
      );

      // Need to return the new WebView here.
      return null;
    },

    // Called to open a window for window.open(url, "_blank")
    createContentWindowInFrame(
      aURI,
      aParams,
      aWhere,
      aFlags,
      aNextRemoteTabId,
      aName
    ) {
      let url = aURI.spec;
      log(
        `browserWindow::createContentWindowInFrame ${url} ${aParams.features} ${aNextRemoteTabId}`
      );

      let wm = exports["wm"];
      let web_view = wm.open_frame(url, { activate: true });
      return web_view;
    },

    isTabContentWindow(aWindow) {
      log(`browserWindow::isTabContentWindow`);
      return false;
    },

    canClose() {
      log(`browserWindow::canClose`);
      return true;
    }
  };

  const processSelector = {
    NEW_PROCESS: -1,

    provideProcess(aType, aOpener, aProcesses, aMaxCount) {
      log(
        `provideProcess ${aType} ${JSON.stringify(
          aProcesses
        )} (max=${aMaxCount})`
      );

      // If we find an existing process with no tab, use it.
      for (let i = 0; i < aProcesses.length; i++) {
        if (aProcesses[i].tabCount == 0) {
          log(`Re-using process #${i}`);
          // If we re-use a preallocated process, spawn a new one.
          window.setTimeout(() => {
            embedder.launch_preallocated_process();
          }, kPreallocLaunchDelay);
          return i;
        }
      }

      // Fallback to creating a new process.
      log(`No reusable process found, will create a new one.`);
      return processSelector.NEW_PROCESS;
    }
  };

  const embedder = new WebEmbedder({
    window_provider: windowProvider,
    process_selector: processSelector
  });
  embedder.addEventListener("runtime-ready", e => {
    log(`Embedder event: ${e.type}`);
    embedder.launch_preallocated_process();
  });

  // Hacks.
  const { Services } = ChromeUtils.import(
    "resource://gre/modules/Services.jsm"
  );
  // Force a Mobile User Agent string.
  Services.prefs.setCharPref(
    "general.useragent.override",
    "Mozilla/5.0 (Mobile; rv:76.0) Gecko/20100101 Firefox/76.0 B2GOS/3.0"
  );
})(window);
