/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Membuf server code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Joe Hewitt <hewitt@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsWidgetsCID.h"

#include "membufAppShell.h"
#include "membufWindow.h"
#include "membufToolkit.h"
#include "membufLookAndFeel.h"
#include "membufBidiKeyboard.h"
#include "membufClipboard.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(membufAppShell)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufWindow)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufToolkit)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufLookAndFeel)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufBidiKeyboard)
NS_GENERIC_FACTORY_CONSTRUCTOR(membufClipboard)

static const nsModuleComponentInfo components[] =
{
  { "Membuf AppShell",
    NS_APPSHELL_CID,
    "@mozilla.org/widget/appshell/membuf;1",
    membufAppShellConstructor },
  { "Membuf Toolkit",
    NS_TOOLKIT_CID,
    "@mozilla.org/widget/toolkit/membuf;1",
    membufToolkitConstructor },
  { "Membuf nsWindow",
    NS_WINDOW_CID,
    "@mozilla.org/widgets/window/membuf;1",
    membufWindowConstructor },
  { "Membuf child nsWindow",
    NS_CHILD_CID,
    "@mozilla.org/widgets/child_window/membuf;1",
    membufWindowConstructor },
  { "Membuf Look And Feel",
    NS_LOOKANDFEEL_CID,
    "@mozilla.org/widget/lookandfeel/membuf;1",
    membufLookAndFeelConstructor },
  { "Membuf Bidi Keyboard",
    NS_BIDIKEYBOARD_CID,
    "@mozilla.org/widget/bidikeyboard;1",
    membufBidiKeyboardConstructor },
  { "Membuf Clipboard",
    NS_CLIPBOARD_CID,
    "@mozilla.org/widget/clipboard;1",
    membufClipboardConstructor },
};

PR_STATIC_CALLBACK(void)
nsWidgetMembufModuleDtor(nsIModule *self)
{
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsWidgetMembufModule,
                              components,
                              nsWidgetMembufModuleDtor)
