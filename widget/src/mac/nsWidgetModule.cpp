/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Christopher
 * Blizzard.  Portions created by Christopher Blizzard are
 * Copyright (C) 2000 Christopher Blizzard. All Rights Reserved.
 *
 * Contributor(s): 
 *   Christopher Blizzzard <blizzard@mozilla.org>
 *   Stuart Parmenter <pavlov@netscape.com>
 *   Dan Rosen <dr@netscape.com>
 */

#include "nsIGenericFactory.h"

#include "nsISupports.h"
#include "nsIButton.h"
#include "nsITextWidget.h"
#include "nsWidgetsCID.h"

#include "nsToolkit.h"
#include "nsWindow.h"
#include "nsMacWindow.h"
#include "nsAppShell.h"
#include "nsButton.h"
#include "nsTextWidget.h"
#include "nsLabel.h"
#include "nsFilePicker.h"
#include "nsFileSpecWithUIImpl.h"
#include "nsScrollbar.h"

#if TARGET_CARBON
#include "nsMenuBarX.h"
#include "nsMenuX.h"
#include "nsMenuItemX.h"

#define nsMenuBar nsMenuBarX
#define nsMenu nsMenuX
#define nsMenuItem nsMenuItemX

#else
#include "nsMenuBar.h"
#include "nsMenu.h"
#include "nsMenuItem.h"
#endif

#include "nsFileWidget.h"

#include "nsClipboard.h"
#include "nsClipboardHelper.h"
#include "nsTransferable.h"
#include "nsHTMLFormatConverter.h"
#include "nsDragService.h"

#if USE_NATIVE_VERSION
# include "nsTextAreaWidget.h"
# include "nsListBox.h"
# include "nsComboBox.h"
# include "nsRadioButton.h"
# include "nsCheckButton.h"
#endif

#include "nsLookAndFeel.h"

#include "nsIComponentManager.h"

#include "nsSound.h"
#include "nsTimerMac.h"

#ifdef IBMBIDI
#include "nsBidiKeyboard.h"
#endif


NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimerImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(ChildWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsButton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFilePicker)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFileWidget)

#if USE_NATIVE_VERSION
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCheckButton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsComboBox)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRadioButton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsListBox)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTextAreaWidget)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsHorizScrollbar)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsVertScrollbar)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTextWidget)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLabel)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMenuBar)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMenu)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMenuItem)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSound)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFileSpecWithUIImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransferable)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClipboardHelper)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsHTMLFormatConverter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDragService)
#ifdef IBMBIDI
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBidiKeyboard)
#endif

#if 0 // added by thesteve
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontRetrieverService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsTimerManager)
#endif

static nsModuleComponentInfo components[] =
{
  { "Mac nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/mac;1",
    nsMacWindowConstructor },
  { "Mac Child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/mac;1",
    ChildWindowConstructor },
  { "Mac File Widget",
    NS_FILEWIDGET_CID,
    "@mozilla.org/widgets/filewidget/mac;1",
    nsFileWidgetConstructor },
  { "Mac File Widget",
    NS_FILEPICKER_CID,
        // thesteve changed: "@mozilla.org/widgets/filepicker/mac;1",
    "@mozilla.org/widgets/filepicker;1",
    nsFilePickerConstructor },
  { "Mac Horiz Scrollbar",
    NS_HORZSCROLLBAR_CID,
    "@mozilla.org/widgets/horizscroll/mac;1",
    nsHorizScrollbarConstructor },
  { "Mac Vert Scrollbar",
    NS_VERTSCROLLBAR_CID,
    "@mozilla.org/widgets/vertscroll/mac;1",
    nsVertScrollbarConstructor },
  { "Mac AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/mac;1",
    nsAppShellConstructor },
  { "Mac Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/mac;1",
    nsToolkitConstructor },
  { "Mac Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel/mac;1",
    nsLookAndFeelConstructor },
  { "Mac Sound",
    NS_SOUND_CID,
    //    "@mozilla.org/widget/sound/mac;1"
    "@mozilla.org/sound;1",
    nsSoundConstructor },
  { "File Spec with UI",
    NS_FILESPECWITHUI_CID,
    //    "@mozilla.org/widget/filespecwithui/mac;1",
    "@mozilla.org/filespecwithui;1",
    nsFileSpecWithUIImplConstructor },
  { "Transferable",
    NS_TRANSFERABLE_CID,
    //    "@mozilla.org/widget/transferable/mac;1",
    "@mozilla.org/widget/transferable;1",
    nsTransferableConstructor },
  { "HTML Format Converter",
    NS_HTMLFORMATCONVERTER_CID,
    "@mozilla.org/widget/htmlformatconverter/mac;1",
    nsHTMLFormatConverterConstructor },
  { "Mac Clipboard",
    NS_CLIPBOARD_CID,
    //    "@mozilla.org/widget/clipboard/mac;1",
    "@mozilla.org/widget/clipboard;1",
    nsClipboardConstructor },
  { "Clipboard Helper",
    NS_CLIPBOARDHELPER_CID,
    "@mozilla.org/widget/clipboardhelper;1",
    nsClipboardHelperConstructor },
  { "Mac Drag Service",
    NS_DRAGSERVICE_CID,
    //    "@mozilla.org/widget/dragservice/mac;1",
    "@mozilla.org/widget/dragservice;1",
    nsDragServiceConstructor },
#if 0  // added by thesteve, influenced by nsWinWidgetFactory.cpp
  { "Font Retriever Service",
    NS_FONTRETRIEVERSERVICE_CID,
    "@mozilla.org/widget/fontretrieverservice/win;1",
    nsFontRetrieverServiceConstructor },
#endif // 1
  { "Mac nsTimer",
    NS_TIMER_CID,
    "@mozilla.org/timer;1",
    nsTimerImplConstructor },

#if 1  // thesteve: not found in nsWinWidgetFactory.cpp
  { "Mac Popup nsWindow",
    NS_POPUP_CID,
    "@mozilla.org/widgets/popup/mac;1",
    nsMacWindowConstructor },
  { "Mac Button",
    NS_BUTTON_CID,
    "@mozilla.org/widgets/button/mac;1",
    nsButtonConstructor },
#if USE_NATIVE_VERSION
  { "Mac Native Button Widget",
    NS_CHECKBUTTON_CID,
    "@mozilla.org/widgets/checkbutton/mac;1",
    nsCheckButtonConstructor },
  { "Mac Native Combobox Widget",
    NS_COMBOBOX_CID,
    "@mozilla.org/widgets/combobox/mac;1",
    nsCheckButtonConstructor },
  { "Mac Native Radio Button Widget",
    NS_RADIOBUTTON_CID,
    "@mozilla.org/widgets/radiobutton/mac;1",
    nsCheckButtonConstructor },
  { "Mac Native Listbox Widget",
    NS_LISTBOX_CID,
    "@mozilla.org/widgets/listbox/mac;1",
    nsCheckButtonConstructor },
  { "Mac Native Textarea Widget",
    NS_TEXTAREA_CID,
    "@mozilla.org/widgets/textarea/mac;1",
    nsCheckButtonConstructor },
#endif
  { "Mac Text Widget",
    NS_TEXTFIELD_CID,
    "@mozilla.org/widgets/textwidget/mac;1",
    nsTextWidgetConstructor },
  { "Mac Label",
    NS_LABEL_CID,
    "@mozilla.org/widget/label/mac;1",
    nsLabelConstructor },

  { "Mac Menubar",
    NS_MENUBAR_CID,
    "@mozilla.org/widget/menubar/mac;1",
    nsMenuBarConstructor },

  { "Mac Menu",
    NS_MENU_CID,
    "@mozilla.org/widget/menu/mac;1",
    nsMenuConstructor },

  { "Mac Menu Item",
    NS_MENUITEM_CID,
    "@mozilla.org/widget/menuitem/mac;1",
    nsMenuItemConstructor },
#endif 0

#ifdef IBMBIDI
    { "Mac Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    nsBidiKeyboardConstructor },
#endif // IBMBIDI
};

PR_STATIC_CALLBACK(void)
nsWidgetModuleDtor(nsIModule *self)
{
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsWidgetModule,
                              components,
                              nsWidgetModuleDtor)
