/* * This file is part of meego-im-framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */


#ifndef MINPUTMETHODNAMESPACE_H
#define MINPUTMETHODNAMESPACE_H

#include <QSharedPointer>

namespace MInputMethod {
    //! Type of toolbar widget
    enum ItemType {
        //! Undefined item type
        ItemUndefined,

        //! Item should be visualized as button
        ItemButton,

        //! Item should be visualized as label
        ItemLabel,
    };

    //! Type of visible premiss for toolbar button
    enum VisibleType {
        //! Item's visibility will not be changed automatically
        VisibleUndefined,

        //! Item's visibility depends on text selection
        VisibleWhenSelectingText,

        //! Item is always visible
        VisibleAlways,
    };

    //! Type of action
    enum ActionType {
        //! Do nothing
        ActionUndefined,

        //! Send key sequence like Ctrl+D
        ActionSendKeySequence,

        //! Send string
        ActionSendString,

        //! Send command (not implemented yet)
        ActionSendCommand,

        //! Copy selected text
        ActionCopy,

        //! Paste text from clipboard
        ActionPaste,

        //! Show some group of items
        ActionShowGroup,

        //! Hide some group of items
        ActionHideGroup,

        //! Close virtual keyboard
        ActionClose,

        //! Standard copy/paste button
        ActionCopyPaste,
    };

    /*!
     * \brief State of Copy/Paste button.
     */
    enum CopyPasteState {
        //! Copy/Paste button is hidden
        InputMethodNoCopyPaste,

        //! Copy button is accessible
        InputMethodCopy,

        //! Paste button is accessible
        InputMethodPaste
    };

    /*!
     * This enum defines direction of plugin switching
     */
    enum SwitchDirection {
        SwitchUndefined, //!< Special value for uninitialized variables
        SwitchForward, //!< Activate next plugin
        SwitchBackward //!< Activate previous plugin
    };

    enum PreeditFace {
        PreeditDefault,
        PreeditNoCandidates,
        PreeditKeyPress           //! Used for displaying the hwkbd key just pressed
    };

    enum HandlerState {
        OnScreen,
        Hardware,
        Accessory
    };

    /// \brief Key event request type for \a MInputContext::keyEvent().
    enum EventRequestType {
        EventRequestBoth,         //!< Both a Qt::KeyEvent and a signal
        EventRequestSignalOnly,   //!< Only a signal
        EventRequestEventOnly     //!< Only a Qt::KeyEvent
    };

     /*!
      * This enum contains possible values for all the modes that are shown in the
      * Input mode indicator.
      */
    enum InputModeIndicator {
        NoIndicator,    //!< No indicator should be shown
        LatinLower,     //!< Latin lower case mode
        LatinUpper,     //!< Latin upper case mode
        LatinLocked,    //!< Latin caps locked mode
        CyrillicLower,  //!< Cyrillic lower case mode
        CyrillicUpper,  //!< Cyrillic upper case mode
        CyrillicLocked, //!< Cyrillic caps locked mode
        Arabic,         //!< Arabic mode
        Pinyin,         //!< Pinyin mode
        Zhuyin,         //!< Zhuyin mode
        Cangjie,        //!< Cangjie mode
        NumAndSymLatched,   //!< Number and Symbol latched mode
        NumAndSymLocked,//!< Number and Symbol locked mode
        DeadKeyAcute,   //!< Dead key acute mode
        DeadKeyCaron,   //!< Dead key caron mode
        DeadKeyCircumflex,  //!< Dead key circumflex mode
        DeadKeyDiaeresis,   //!< Dead key diaeresis mode
        DeadKeyGrave,   //!< Dead key grave mode
        DeadKeyTilde    //!< Dead key tilde mode
    };
};

#endif

