/**********************************************************************
 LILoginAdvancedDialog.h
 By Daniel Malmer
 5/4/98

**********************************************************************/

#ifndef __LILoginAdvancedDialog_h
#define __LILoginAdvancedDialog_h

#include "Dialog.h"
#include "PrefsPageLIServer.h"
#include "PrefsPageLIFiles.h"

class XFE_LILoginAdvancedDialog : public XFE_Dialog
{
public:

	// Constructors, Destructors

	XFE_LILoginAdvancedDialog(Widget parent);

	virtual ~XFE_LILoginAdvancedDialog();

	// Accessor functions

	// Modifier functions

	// virtual void show();

	// virtual void hide();

	void okCallback(Widget, XtPointer);
	void cancelCallback(Widget, XtPointer);

	static void ok_callback(Widget, XtPointer, XtPointer);
	static void cancel_callback(Widget, XtPointer, XtPointer);

private:
	XFE_PrefsPageLIServer* m_serverFrame;
	XFE_PrefsPageLIFiles* m_filesFrame;	
};

#endif /* __LILoginAdvancedDialog_h */
