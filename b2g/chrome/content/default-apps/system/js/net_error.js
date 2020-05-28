function getErrorFromURI() {
  let _error = {};
  var uri = document.documentURI;

  // Quick check to ensure it's the URI format we're expecting.
  if (!uri.startsWith("about:neterror?")) {
    // A blank error will generate the default error message (no network).
    return uri;
  }

  // Small hack to get the URL object to parse the URI correctly.
  var url = new URL(uri.replace("about:", "http://"));

  // Set the error attributes.
  ['e', 'u', 'm', 'c', 'd', 'f'].forEach(
    function (v) {
      _error[v] = url.searchParams.get(v);
    }
  );

  switch (_error.e) {
    case "connectionFailure":
    case "netInterrupt":
    case "netTimeout":
    case "netReset":
      _error.e = 'connectionFailed';
      break;

    case "unknownSocketType":
    case "unknownProtocolFound":
    case "cspFrameAncestorBlocked":
      this._error.e = "invalidConnection";
      break;
  }

  return _error;
}

let node = document.getElementById("view");
node.textContent = getErrorFromURI().e;