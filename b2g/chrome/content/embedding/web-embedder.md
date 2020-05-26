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
The `WebEmbedder` object is an `EventTarget` that will dispatch a `runtime-ready` event once it is setup. At this point you can safely call methods on the object itself.

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
- `systemAlerts`: this object exposes methods for invoking notification features of the system
  - `resendAll()`: this will resend all stored notifications from the Notifications DB.

## WindowProvider delegate

This delegate is used when the plaform needs to create a new top level window. It's a reflection of the `nsIBrowserDOMWindow` interface, hence the documentation at `https://searchfox.org/mozilla-central/source/dom/interfaces/base/nsIBrowserDOMWindow.idl` can be used.

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
  - `manifestURL`: ???.
  - `timestamp`: when the notification was triggered.
  - `data`: ???.
  - `mozbehavior`: ???.
  - `requireInteraction`: ???.

## ActivityChooser delegate

This delegate let the system UI display the list of potential choices when an several app can fulfill an activity request.

## Methods

- `choseActivity(detail)` returns a promise that resolves with the chosen app as `{ id: <opaque id>, value: <index in the choice> }`. If no activity is selected, `value` should be undefined or set to -1. The `detail` parameter is an object with the following properties:
  - `id`: an opaque id representing this activity, to be used in the resolved object.
  - `name`: the activity name.
  - `choices`: an array of `{ manifest: <url>, icon: <url>}` representing the apps that can provide the activity.
