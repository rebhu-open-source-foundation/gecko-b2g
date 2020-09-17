# \<web-view\> custom element API.

## Attribute

The <web-view> element supports the following attributes:
- `src` : the url of the page to load.
- `remote` : a boolean to decide if that browser should load content in a content process.
- `ignorefocus` : a boolean that when set let the browser get pointer events without focusing it. This is useful for virtual keyboard frames.
- `transparent` : if true, the background of the browser will be transparent instead of white.

## Methods

- `focus() : void` : focuses the browser.
- `blur() : void` : blurs the browser.
- `getBackgroundColor() : Promise<String>`: returns the CSS value of the page's background color.
- `getScreenshot(max_width, max_height, mime_type) : Promise<Blob>` : takes a screenshot of the current page.
- `goForward() : void` : navigates one step forwared in the history.
- `goBack() : void` : navigates one step back in the history.
- `reload(forced) : void` : reload the patch, bypassing the cache if `forced` is true.
- `stop() : void` : stops the current page loading.
- `enableCursor() : void` : enables the virtual cursor.
- `disableCursor() : void` : disables the virtual cursor.
- `getCursorEnabled() : Promise<boolean>` : query whether the virtual cursor is enabled.
- `startVirtualCursorPanning() : void` : starts panning simulation of virtual cursor with keyboard events.
- `stopVirtualCursorPanning() : void` : stops panning simulation of virtual cursor with keyboard events.

## Properties

- `(readonly) frame` : returns a handle to the underlying browser. Use with care!
- `src : string` : mirror of the `src` attribute.
- ` (readonly) canGoForward : boolean` : return `true` if calling `goForward()` would be effective.
- ` (readonly) canGoBack : boolean` : return `true` if calling `goBack()` would be effective.
- `active : boolean` : control the active state of the browser's docShell.
- `processid : int` : returns the process ID of its content process if there is, or -1 if there is not.
- `visible : boolean`: currently similar to `active`.
- `isVirtualCursorPanning : boolean` : query whether the panning simulation of virtual cursor is enabled.

## Events

Each event type is prefixed with `mozbrowser` for historical compatibility reasons. All events are CustomEvents, with an event payload specific to each type.

- `close` : `{}`
- `contextmenu` : `{ <depends on the element that was selected> }`
- `documentfirstpaint` : `{}`
- `error` : `{ type: string }`
- `iconchange` : `{ href: string, sizes: string, rel: string }`
- `loadend` : `{ backgroundColor: string}`
- `loadstart` : `{}`
- `locationchange` : `{ url: string, canGoBack: boolean, cangoForward: boolean}`
- `manifestchange` : `{ href: string}`
- `opensearch` : `{ title: string, href: string}`
- `processready`: `{ processid: int}`
- `resize` : `{ width: int, height: int}`
- `scroll` : `{ top: int, left: int}`
- `securitychange` : `{ state: string, mixedState: string, extendedValidation: boolean, mixedContent: boolean }`
- `titlechange` : `{ title: string }`
- `visibilitychange` : `{ visible: boolean }`
