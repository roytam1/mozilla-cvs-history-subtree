/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s):  Ed Burns <edburns@acm.org>
 */

package org.mozilla.webclient;

import java.util.Properties;


/**

 * The custom app must implement this interface in order to supply the
 * underlying browser with basic authentication behavior.  The custom
 * app must tell webclient about its Prompt implementation by calling
 * Navigation.setPrompt().  This must be done FOR EACH BrowserControl
 * instance!

 */

public interface Prompt
{

/**

 * Puts up a username/password dialog with OK and Cancel buttons.

 * @param fillThis a pre-allocated properties object
 * that the callee fills in.

 * keys: userName, password
 
 * @return true for OK, false for Cancel

 */

public boolean promptUsernameAndPassword(String dialogTitle,
					 String text,
					 String passwordRealm,
					 int savePassword,
					 Properties fillThis);
    
} // end of interface History
