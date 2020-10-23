/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window, Pref="dom.inputmethod.enabled"]
interface InputMethod {

  Promise<boolean> setComposition(DOMString text);

  Promise<boolean> endComposition(optional DOMString text);

  Promise<boolean> sendKey(DOMString key);
  Promise<boolean> keydown(DOMString key);
  Promise<boolean> keyup(DOMString key);

  /**
   * Delete a text/node/selection before the cursor
   */
  Promise<boolean> deleteBackward();

  /**
   * Select the <select> option specified by index.
   * If this method is called on a <select> that support multiple
   * selection, then the option specified by index will be added to
   * the selection.
   * If this method is called for a select that does not support multiple
   * selection the previous element will be unselected.
   */
  void setSelectedOption(long index);

  /**
   * Select the <select> options specified by indexes. All other options
   * will be deselected.
   * If this method is called for a <select> that does not support multiple
   * selection, then the last index specified in indexes will be selected.
   */
  void setSelectedOptions(sequence<long> indexes);

  /**
   * Remove focus from the current input, usable by Gaia System app, globally,
   * regardless of the current focus state.
   */
  void removeFocus();

  };
