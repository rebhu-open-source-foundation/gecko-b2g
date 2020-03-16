# SystemMessage API

System messages are messages from the system, they are sent by the system based on their types, for example, a system message with name *alarm* is sent to its subscriber when a timer goes off; a system message with name *sms-received* is sent when the system receives an sms message. **SystemMessage API** gives applications the ability to subscribe system messages of particular names, and events `SystemMessageEvent` will dispatched to its registered ServiceWorker in different scenarios.

## Message types

| **Name** | **Permission**  |
| :---:    | :---------:     |
| activity |                 |
| alarm    |                 |

## For front-end and application developers

To subscribe and receive system messages, an app has to have an active service worker.

### Subscribe system messages from application manifest

Not yet implemented.

### Subscribe system messages from service worker

Apps can use `SystemMessageManager.subscribe()` to subscribe system messages on an active service worker. `systemMessageManager` is a property of `ServiceWorkerRegistration`, which returns a reference to the `SystemMessageManager` interface.

##### Use from main scripts
```javascript
navigator.serviceWorker.register("sw.js").then(registration => {
  registration.systemMessageManager.subscribe("alarm").then(
    rv => {
      console.log('Successfully subscribe system messages of name "alarm".');
    },  
    error => {
      console.log("Fail to subscribe system message, error: " + error);
    }   
  );  
});
```

##### Use from service worker script
```javascript
self.registration.systemMessageManager.subscribe("alarm").then(
  rv => {
    console.log('Successfully subscribe system messages of name "alarm".');
  },  
  error => {
    console.log("Fail to subscribe system message, error: " + error);
  }
);
```

### Receive system messages

System messages are delivered to the `ServiceWorkerGlobalScope.onsystemmessage` event handler, in the format of `SystemMessageEvent`.

<dl>
<b>SystemMessageEvent</b>
    <dt>name</dt>
    <dd>The type of this system message.</dd>
    <dt>data</dt>
    <dd>Returns an object <code>SystemMessageData</code> which wraps the details of this system message.</dd>
</dl>

<dl>
<b>SystemMessageData</b>
    <dt>json()</dt>
    <dd>Extracts the message data as a JSON object, not available for <em>activity</em> messages.</dd>
    <dt>WebActivityRequestHandler()</dt>
    <dd>Only available for <em>activity</em> messages, details stated in <code>WebActivity API</code>.</dd>
</dl>

```javascript
self.onsystemmessage = evt => {
  console.log("Receive systemmessage event with name: " + evt.name);
  console.log(" message data: " + evt.data);
  console.log("  data detail:");
  try {
    console.log(evt.data.json());
  } catch (err) {
    console.log(err);
  }
};
```

## For gecko developers

`SystemMessageService` provides an interface for module developers to send system message to target application (broadcast messages is not yet implemented).

**Syntax**
```
void sendMessage(in AString messageName, in jsval message, in ACString origin);
```

**Example**
```javascript
let sm = Cc["@mozilla.org/systemmessage-service;1"].getService(
  Ci.nsISystemMessageService
);
let msg = { test1: "aaa", test2: "bbb" };
let appOrigin = "https://calculator.local";
sm.sendMessage("alarm", msg, appOrigin);
```
