# VirtualCursor API

The **VirtualCursor API** provides the interfaces for applications to turn on/off the mouse events simulation by keyboard events. This is designed for SFP projects and allows users to control the cursor with keyboard events.

## Preconditions
1. Enable the preference `dom.virtualcursor.enabled`.
2. Add permission `virtualcursor` in the app manifest.
3. Enable the preference `dom.virtualcursor.pan_simulator.enabled` to enable pan simulation.

## Enable the cursor simulation
Apps can simply enable the virtualcursor by calling `enable`, for example:

```javascript
navigator.b2g.virtualCursor.enable();
```

## Disable the cursor simulation
Apps can simply disable the virtualcursor by calling `disable`, for example:
```javascript
navigator.b2g.virtualCursor.disable();
```

## Query current virtual cursor configuration
Apps can simply get the virtualcursor state by querying the attribute `enabled`, for example:
```javascript
if (navigator.b2g.virtualCursor.enabled) {}
```
