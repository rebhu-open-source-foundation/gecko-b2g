/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

enum IccImageCodingScheme { "basic", "color", "color-transparency" };

dictionary StkIcon
{
  /*
   * Width of the icon.
   */
  unsigned long width = 0;

  /*
   * Height of the icon.
   */
  unsigned long height = 0;

  /*
   * Image coding scheme of the icon.
   */
  IccImageCodingScheme codingScheme = "basic";

  /*
   * Array of pixels. Each pixel represents a color in the RGBA format made up
   * of four bytes, that is, the Red sample in the highest 8 bits, followed by
   * the Green sample, Blue sample and the Alpha sample in the lowest 8 bits.
   */
  sequence<unsigned long> pixels = [];
};

dictionary StkIconContainer
{
  /**
   * Indicates how the icon is to be used.
   *
   * @see TS 11.14, clause 12.31, Icon Identifier.
   *
   * true: icon replaces the text string.
   * false: icon shall be displayed together with the text string.
   */
  boolean iconSelfExplanatory = true;

  /**
   * Icon(s) that replaces or accompanies the text string.
   *
   * @see TS 11.14, clause 12.31, Icon Identifier.
   *
   * Array of icons, basically of a same image, that may differ in size,
   * resolution or coding scheme. The first icon should be the default one.
   */
  sequence<StkIcon> icons = [];
};

[Pref="dom.icc.enabled",
 Exposed=Window]
 //CheckAnyPermissions="mobileconnection",
 // AvailableIn="CertifiedApps",
interface StkCommandEvent : Event
{
  constructor(DOMString type, optional StkCommandEventInit eventInitDict={});
  readonly attribute any command;
};

dictionary StkCommandEventInit : EventInit
{
  any command = null;
};

dictionary StkTextMessage : StkIconContainer
{
  /**
   * Text String.
   *
   * @see TS 11.14, clause 12.15, Text String.
   */
  DOMString text = "";

  /**
   * The length of time for which the ME shall display the dialog.
   */
  StkDuration duration = {};

  /**
   * Indicate this text message is high priority or normal priority.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, Display Text, bit 1.
   *
   * High priority text shall be displayed on the screen immediately, except if
   * there is a conflict of priority level of alerting such as incoming calls
   * or a low battery warning. In that situation, the resolution is left to
   * the terminal. If the command is rejected in spite of the high priority,
   * the terminal shall inform the ICC with resultCode is
   * IccManager.STK_RESULT_TERMINAL_CRNTLY_UNABLE_TO_PROCESS in StkResponse.
   *
   * true: high priority
   * false: normal priority
   */
  boolean isHighPriority = false;

  /**
   * Need to wait for user to clear message or not.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, Display Text, bit 8.
   *
   * If this attribute is true, but user doesn't give any input within a period
   * of time(said 30 secs), the terminal shall inform the ICC with resultCode
   * is IccManager.STK_RESULT_NO_RESPONSE_FROM_USER in StkResponse.
   *
   * true: Wait for user to clear message.
   * false: clear message after a delay.
   */
  boolean userClea = false;

  /**
   * Need to response immediately or not.
   *
   * @see TS 11.14, clause 12.43, Immediate response.
   *
   * When this attribute is true, the terminal shall immediately send
   * StkResponse with resultCode is OK.
   *
   * true: The terminal shall send response immediately.
   * false: otherwise.
   */
  boolean responseNeeded = false;
};

dictionary StkItem : StkIconContainer
{
  /**
   * Identifier of item.
   *
   * The identifier is a single byte between '01' and 'FF'. Each item shall
   * have a unique identifier within an Item list.
   */
  unsigned short identifier = 0;

  /**
   * Text string of item.
   */
  DOMString text = "";
};

dictionary StkMenu : StkIconContainer
{
  /**
   * Array of StkItem.
   *
   * @see TS 11.14, clause 12.9
   */
  sequence<StkItem> items = [];

  /**
   * Presentation type, one of IccManager.STK_MENU_TYPE_*.
   */
  unsigned short presentationType = 0;

  /**
   * Title of the menu.
   */
  DOMString title = "";

  /**
   * Default item identifier of the menu.
   */
  unsigned short defaultItem = 0;

  /**
   * Help information available or not.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, SET UP MENU, bit 8.
   *
   * true: help information available.
   * false: no help information available.
   */
  boolean isHelpAvailable = false;

  /**
   * List of Next Action Indicators.
   * Each element should be one of IccManager.STK_CMD_*
   *                            or IccManager.STK_NEXT_ACTION_*
   * If it's IccManager.STK_NEXT_ACTION_NULL, the terminal should ignore this
   * action in corresponding item.
   *
   * @see TS 11.14, clause 12.24, Items Next Action Indicator.
   */
  sequence<unsigned short> nextActionList = [];
};

dictionary StkInput : StkIconContainer
{
  /**
   * Text for the ME to display in conjunction with asking the user to respond.
   */
  DOMString text = "";

  /**
   * The length of time for which the ME shall display the dialog. This field
   * is used only for GET INKEY.
   *
   * @see TS 11.14, clause 11.8, duration, GET INKEY T.C 27.22.4.2.8.4.2
   */
  StkDuration duration = {};

  /**
   * Minimum length of response.
   */
  unsigned short minLength = 0;

  /**
   * Maximum length of response.
   */
  unsigned short maxLength = 0;

  /**
   * Text for the ME to display, corresponds to a default text string offered
   * by the ICC.
   */
  DOMString defaultText = "";

  /**
   * Input format.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, GET INPUT, bit 1.
   *
   * true: Alphabet set.
   * false: Digits only.
   */
  boolean isAlphabet = false;

  /**
   * Alphabet encoding.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, GET INPUT, bit 2.
   *
   * true: UCS2 alphabet.
   * false: default SMS alphabet.
   */
  boolean isUCS2 = false;

  /**
   * Visibility of input.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, GET INPUT, bit 3.
   *
   * true: User input shall not be revealed in any way.
   * false: ME may echo user input on the display.
   */
  boolean hideInput = false;

  /**
   * Yes/No response is requested.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, GET INKEY, bit 3.
   *
   * true: Yes/No response is requested, and character sets
   *       (Alphabet set and UCS2) are disabled.
   * false: Character sets (Alphabet set and UCS2) are enabled.
   */
  boolean isYesNoRequested = false;

  /**
   * User input in packed or unpacked format.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, GET INPUT, bit 4.
   *
   * true: User input to be in SMS packed format.
   * false: User input to be in unpacked format.
   */
  boolean isPacked = false;

  /**
   * Help information available or not.
   *
   * @see TS 11.14, clause 12.6, Command Qualifier, GET INPUT/GET INKEY, bit 8.
   *
   * true: help information available.
   * false: no help information available.
   */
  boolean isHelpAvailable = false;
};

dictionary StkBrowserSetting
{
  /**
   * Confirm message to launch browser.
   */
  StkTextMessage confirmMessage = {};

  /**
   * The URL to be opened by browser.
   */
  DOMString url = "";

  /**
   * One of IccManager.STK_BROWSER_MODE_*.
   */
  unsigned short mode = 0;
};

dictionary StkSetUpCall
{
  /**
   * The Dialling number.
   */
  DOMString address = "";

  /**
   * The text message used in user confirmation phase.
   */
  StkTextMessage confirmMessage = {};

  /**
   * The text message used in call set up phase.
   */
  StkTextMessage callMessage = {};

  /**
   * The Optional maximum duration for the redial mechanism.
   * The time elapsed since the first call set-up attempt has exceeded the duration
   * requested by the UICC, the redial mechanism is terminated.
   */
  StkDuration duration = {};
};

dictionary StkSetUpEventList
{
  /**
   * The list of events that needs to provide details to ICC when they happen.
   * When this valus is null, means an indication to remove the existing list
   * of events in ME.
   *
   * @see IccManager.STK_EVENT_TYPE_*
   */
   sequence<unsigned short> eventList = [];
};

dictionary StkLocationInfo
{
  /**
   * Mobile Country Code (MCC) of the current serving operator.
   */
  DOMString mcc = "";

  /**
   * Mobile Network Code (MNC) of the current serving operator.
   */
  DOMString mnc = "";

  /**
   * Mobile Location Area Code (LAC) for the current serving operator.
   */
  unsigned short gsmLocationAreaCode = 0;

  /**
   * Mobile Cell ID for the current serving operator.
   */
  unsigned long gsmCellId = 0;
};

dictionary StkDuration
{
  /**
   * Time unit used, should be one of IccManager.STK_TIME_UNIT_*.
   */
  unsigned short timeUnit = 0;

  /**
   * The length of time required, expressed in timeUnit.
   */
  octet timeInterval = 0;
};

dictionary StkPlayTone : StkIconContainer
{
  /**
   * Text String.
   */
  DOMString text = "";

  /**
   * One of IccManager.STK_TONE_TYPE_*.
   */
  unsigned short tone = 0;

  /**
   * The length of time for which the ME shall generate the tone.
   */
  StkDuration duration = {};

  /**
   * Need to vibrate or not.
   * true: vibrate alert, if available, with the tone.
   * false: use of vibrate alert is up to the ME.
   */
  boolean isVibrate = false;
};

dictionary StkProvideLocalInfo
{
  /**
   * Indicate which local information is required.
   * It shall be one of following:
   *  - IccManager.STK_LOCAL_INFO_LOCATION_INFO
   *  - IccManager.STK_LOCAL_INFO_IMEI
   *  - IccManager.STK_LOCAL_INFO_DATE_TIME_ZONE
   *  - IccManager.STK_LOCAL_INFO_LANGUAGE
   */
  unsigned short localInfoType = 0;
};

dictionary StkLocationEvent
{
  /**
   * The type of this event.
   * It shall be IccManager.STK_EVENT_TYPE_LOCATION_STATUS;
   */
  unsigned short eventType = 0;

  /**
   * Indicate current service state of the MS with one of the values listed
   * below:
   *  - IccManager.STK_SERVICE_STATE_NORMAL
   *  - IccManager.STK_SERVICE_STATE_LIMITED
   *  - IccManager.STK_SERVICE_STATE_UNAVAILABLE
   */
  unsigned short locationStatus = 0;

  /**
   * See StkLocationInfo.
   * This value shall only be provided if the locationStatus indicates
   * IccManager.STK_SERVICE_STATE_NORMAL.
   */
  StkLocationInfo locationInfo = {};
};

dictionary StkTimer
{
  /**
   * Identifier of a timer.
   */
  octet timerId = 0;

  /**
   * Length of time during which the timer has to run.
   * The resolution of a timer is 1 second.
   */
  unsigned long timerValue = 0;

  /**
   * The action requested from UICC.
   * It shall be one of below:
   * - IccManager.STK_TIMER_START
   * - IccManager.STK_TIMER_DEACTIVATE
   * - IccManager.STK_TIMER_GET_CURRENT_VALUE
   */
  unsigned short timerAction = 0;
};

dictionary StkBipMessage : StkIconContainer
{
  /**
   * Text String
   */
  DOMString text = "";
};

dictionary StkCommand
{
  /**
   * The number of command issued by ICC. And it is assigned
   * by ICC may take any hexadecimal value betweean '01' and 'FE'.
   *
   * @see TS 11.14, clause 6.5.1
   */
  unsigned short commandNumber = 0;

  /**
   * One of IccManager.STK_CMD_*
   */
  unsigned short typeOfCommand = 0;

  /**
   * Qualifiers specific to the command.
   */
  unsigned short commandQualifier = 0;

  /**
   * options varies accrording to the typeOfCommand in StkCommand.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_DISPLAY_TEXT
   * - IccManager.STK_CMD_SET_UP_IDLE_MODE_TEXT
   * - IccManager.STK_CMD_SEND_{SS|USSD|SMS|DTMF},
   * options is StkTextMessage.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_SELECT_ITEM
   * - IccManager.STK_CMD_SET_UP_MENU
   * options is StkMenu.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_GET_INKEY
   * - IccManager.STK_CMD_GET_INPUT,
   * options is StkInput.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_LAUNCH_BROWSER
   * options is StkBrowserSetting.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_SET_UP_CALL
   * options is StkSetUpCall.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_SET_UP_EVENT_LIST
   * options is StkSetUpEventList.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_PLAY_TONE
   * options is StkPlayTone.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_POLL_INTERVAL
   * options is StkDuration.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_PROVIDE_LOCAL_INFO
   * options is StkProvideLocalInfo.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_TIMER_MANAGEMENT
   * option is StkTimer
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_OPEN_CHANNEL
   * - IccManager.STK_CMD_CLOSE_CHANNEL
   * - IccManager.STK_CMD_SEND_DATA
   * - IccManager.STK_CMD_RECEIVE_DATA
   * options is StkBipMessage
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_POLL_OFF
   * options is null.
   *
   * When typeOfCommand is
   * - IccManager.STK_CMD_REFRESH
   * options is null.
   */
  any options = null;
};

dictionary StkResponse
{
  /**
   * One of IccManager.STK_RESULT_*
   */
  unsigned short resultCode = 0;

  /**
   * One of IccManager.STK_ADDITIONAL_INFO_*
   */
  unsigned short additionalInformation = 0;

  /**
   * The identifier of the item selected by user.
   *
   * @see StkItem.identifier
   */
  unsigned short itemIdentifier = 0;

  /**
   * User input.
   */
  DOMString input = "";

  /**
   * YES/NO response.
   *
   * @see StkInput.isYesNoRequested
   */
  boolean isYesNo = false;

  /**
   * User has confirmed or rejected the call during
   * IccManager.STK_CMD_CALL_SET_UP.
   *
   * @see RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
   *
   * true: Confirmed by User.
   * false: Rejected by User.
   */
  boolean hasConfirmed = false;

  /**
   * The response for IccManager.STK_CMD_PROVIDE_LOCAL_INFO
   */
  StkLocalInfo localInfo = {};

  /**
   * The response for IccManager.STK_CMD_TIMER_MANAGEMENT.
   * The 'timerValue' is needed if the action of
   * IccManager.STK_CMD_TIMER_MANAGEMENT is IccManager.STK_TIMER_DEACTIVATE
   * or IccManager.STK_TIMER_GET_CURRENT_VALUE.
   * It shall state the current value of a timer. And the resolution is 1 second.
   */
  StkTimer timer = {};
};

dictionary StkCallEvent
{
  /**
   * The type of this event.
   * It shall be one of following:
   *     - IccManager.STK_EVENT_TYPE_MT_CALL,
   *     - IccManager.STK_EVENT_TYPE_CALL_CONNECTED,
   *     - IccManager.STK_EVENT_TYPE_CALL_DISCONNECTED.
   */
  unsigned short eventType = 0;

  /**
   * Remote party number.
   */
  DOMString number = "";

  /**
   * This field is available in IccManager.STK_EVENT_TYPE_CALL_CONNECTED and
   * IccManager.STK_EVENT_TYPE_CALL_DISCONNECTED events.
   * For the IccManager.STK_EVENT_TYPE_CALL_CONNECTED event, setting this to
   * true means the connection is answered by remote end, that is, this is an
   * outgoing call.
   * For the IccManager.STK_EVENT_TYPE_CALL_DISCONNECTED event, setting this
   * to true indicates the connection is hung up by remote.
   */
  boolean isIssuedByRemote = false;

  /**
   * This field is available in Call Disconnected event to indicate the cause
   * of disconnection. The cause string is passed to gaia through the error
   * listener of CallEvent. Null if there's no error.
   */
  DOMString error = "";
};

dictionary StkLocalInfo
{
  /**
   * IMEI information
   */
  DOMString imei = "";

  /**
   * Location Information
   */
  StkLocationInfo locationInfo = {};

  /**
   * Date information
   */
  DOMTimeStamp? date = null;

  /**
   * Language Information
   *
   * @see ISO 639-1, Alpha-2 code
   */
  DOMString language = "";
};

dictionary StkLanguageSelectionEvent
{
  /**
   * The type of this event.
   * It shall be IccManager.STK_EVENT_TYPE_LANGUAGE_SELECTION.
   */
  unsigned short eventType = 0;

  /**
   * Language Information
   *
   * @see ISO 639-1, Alpha-2 code
   *      "de" for German, "en" for English, "zh" for Chinese, etc.
   */
  DOMString language = "";
};

dictionary StkBrowserTerminationEvent
{
  /**
   * The type of this event.
   * It shall be IccManager.STK_EVENT_TYPE_BROWSER_TERMINATION
   */
  unsigned short eventType = 0;

  /**
   * This object shall contain the browser termination cause.
   * See TZ 102 223 8.51. It shall be one of following:
   * - IccManager.STK_BROWSER_TERMINATION_CAUSE_USER
   * - IccManager.STK_BROWSER_TERMINATION_CAUSE_ERROR
   */
  unsigned short terminationCause = 0;
};

dictionary StkGeneralEvent
{
  /**
   * The type of this event, StkGeneralEvent can be used for all Stk Event
   * requires no more parameter than event type, including
   * IccManager.STK_EVENT_TYPE_USER_ACTIVITY.
   * IccManager.STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE.
   * HCI Connectivity Event(Not defined in interface yet).
   */
  unsigned short eventType = 0;
};
