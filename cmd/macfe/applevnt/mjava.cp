/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "mjava.h"

#if defined (JAVA)

extern "C" {
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Component.h"
#include "s_a_m__MComponentPeer.h"
};

#include "MComponentPeer.h"
#include "MToolkit.h"

CJavaView *
CJavaView::CreateJavaView(LStream *inStream)
{
	return new CJavaView(inStream);
}

CJavaView::
CJavaView(LStream *inStream):CPluginView(inStream)
{
	if ( MToolkit::GetMToolkit() )
		(MToolkit::GetMToolkit())->AddFrame((LPane *)this);
}

CJavaView::~CJavaView()
{
	Hsun_awt_macos_MComponentPeer *peer = PaneToPeer(this);
	
	if (peer != NULL)
		unhand(PaneToPeer(this))->mOwnerPane = NULL;
	
	if ( MToolkit::GetMToolkit() )
		(MToolkit::GetMToolkit())->RemoveFrame((LPane *)this);

}

void
CJavaView::Show()
{
	inherited::Show();
 	if ( MToolkit::GetMToolkit() )
	 	(MToolkit::GetMToolkit())->AddFrame((LPane *)this);
}

void
CJavaView::Hide()
{
	inherited::Hide();
	if ( MToolkit::GetMToolkit() )
		(MToolkit::GetMToolkit())->RemoveFrame((LPane *)this);
}

void
CJavaView::ClickSelf(const SMouseDownEvent &inMouseDown)
{
}

#endif	//JAVA
