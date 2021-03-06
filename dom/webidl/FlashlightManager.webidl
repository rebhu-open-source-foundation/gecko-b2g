/*
 * Copyright (C) 2012-2015 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * FlashlightManager reports the current flashlight status, and dispatch a
 * flashlightchange event when device has flashlight opened or closed.
 */
[Exposed=Window, Pref="dom.flashlight.enabled"]
interface FlashlightManager : EventTarget {
    attribute boolean flashlightEnabled;

    attribute EventHandler onflashlightchange;
};
