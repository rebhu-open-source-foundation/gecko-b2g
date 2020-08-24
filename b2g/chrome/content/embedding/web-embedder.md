# WebEmbedder API

The WebEmbedder API is exposed to the system app by including the script from `chrome://b2g/content/embedding/web-embedder.js`.

The embedder is responsible for providing delegates that are used for each functional area exposed to the embedder when it creates a `WebEmbedder` object:

```js
const embedder = new WebEmbedder({
  windowProvider: windowProvider,
  processSelector: processSelector
});
embedder.addEventListener("runtime-ready", e => {
  console.log(`Embedder event: ${e.type}`);
  ...
});
```

## WebEmbedder object

The `WebEmbedder` constructor accepts one parameter used to provide the supported delegates:
```js
const embedder = new WebEmbedder({
  windowProvider: <WindowProvider implementation>,
  processSelector: <ProcessSelector implementation>,
  notifications: <Notifications implementation>,
  activityChooser: <ActivityChooser implementation>,
});
```
All delegates are optional.

### Methods

The `WebEmbedder` object exposes the following methods:
- `launchPreallocatedProcess()`: this will create a new content process with no content.
- `addSystemEventListener(type, listener, useCapture)`: proxies the `Services.els` method of the same name, using the shell document as a target.
- `removeSystemEventListener(type, listener, useCapture)`: proxies the `Services.els` method of the same name, using the shell document as a target.
- `systemAlerts`: this object exposes methods for invoking notification features of the system
  - `resendAll()`: this will resend all stored notifications from the Notifications DB.
  - `click(data)`: this will perform click action. The data parameter is an object with the following properties:
    - `id`: the unique id for this notification
  - `close(id)`: this will perform close action with a parameter id which is the unique id for this notification.
- `isDaemonReady`: returns a boolean that is true if the api-daemon is running and is usable.
- `isGonk`: returns true if this is a device build.

### Events

Since the `WebEmbedder` object extends `EventTarget` you can attach event listeners on it. The list of events that can be dispatched is:
- `runtime-ready`: this event is dispatched once the the embedder is setup. At this point you can safely call methods on the object itself.
- `daemon-disconnected`: this event is dispatched when the api-daemon connectivity is lost.
- `daemon-reconnected`: this event is dispatched when the api-daemon connectivity is available again.

## WindowProvider delegate

This delegate is used when the plaform needs to create a new top level window. It's mostly a reflection of the `nsIBrowserDOMWindow` interface, hence the documentation at `https://searchfox.org/mozilla-central/source/dom/interfaces/base/nsIBrowserDOMWindow.idl` can be used. One key difference is that `openXXX()` and `createXXX()` methods need to return a `<web-view>` element.

### Methods

- `openURI(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp)`.
- `createContentWindow(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp)`.
- `openURIInFrame(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName)`.
- `createContentWindowInFrame(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName)`.
- `isTabContentWindow(aWindow)`.
- `canClose()`.

## ProcessSelector delegate

This delegate allows the embedder to control how content processes are reused or created when an out of process page is loaded.

## Methods

- `provideProcess(aType, aOpener, aProcesses, aMaxCount)` returns the index of the process to reuse or -1 to create a new process.

## Notifications delegate

This delegate is responsible for displaying desktop notifications UI.

## Methods

- `showNotification(notification)`. The `notification` parameter is an object with the following properties:
  - `type`: a string equals to "desktop-notification".
  - `id`: a unique id for this notification.
  - `icon`: the url of the image for this notification.
  - `title`: the notification title.
  - `text`: the notification text.
  - `dir`: whether this notification should be displayed in ltr or rtl direction.
  - `lang`: the language code for this notification.
  - `origin`: the origin the notification is sent from.
  - `timestamp`: when the notification was triggered.
  - `data`: Arbitrary data that you want associated with the notification.
  - `requireInteraction`: Indicates that a notification should remain active until the user clicks or dismisses it, rather than closing automatically.
  - `mozbehavior`: a dictionary indicates custom notification behavior.
  - `serviceWorkerRegistrationScope`: the unique identifier for the service worker registration the notification is sent from.

- `closeNotification(id)`: Notifies the system UI that it should close the notification created with this id.

## ActivityChooser delegate

This delegate let the system UI display the list of potential choices when an several app can fulfill an activity request.

## Methods

- `choseActivity(detail)` returns a promise that resolves with the chosen app as `{ id: <opaque id>, value: <index in the choice> }`. If no activity is selected, `value` should be undefined or set to -1. The `detail` parameter is an object with the following properties:
  - `id`: an opaque id representing this activity, to be used in the resolved object.
  - `name`: the activity name.
  - `choices`: an array of `{ manifest: <url>, icon: <url>}` representing the apps that can provide the activity.

## ScreenReaderProvider delegate

This delegate is responsible for screen reader.

## Methods

- `showScreenReader(data))`. The `data` parameter is an object with the following properties:
 - `type`: the event type. Valid value: `accessibility-output`.
 - `details`: the PresentationData of accessible.
