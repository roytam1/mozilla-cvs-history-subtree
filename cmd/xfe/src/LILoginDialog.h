/**********************************************************************
 LILoginDialog.h
 By Daniel Malmer
 5/4/98

**********************************************************************/

#ifndef __LILoginDialog_h
#define __LILoginDialog_h

#include "Dialog.h"
#include "PrefsPageLIGeneral.h"

class XFE_LILoginDialog : public XFE_Dialog
{
public:

	// Constructors, Destructors

	XFE_LILoginDialog(Widget parent);

	virtual ~XFE_LILoginDialog();

	// Accessor functions

	// Modifier functions

	// virtual void show();

	// virtual void hide();

	void okCallback(Widget, XtPointer);
	void cancelCallback(Widget, XtPointer);
	void advancedCallback(Widget, XtPointer);

	static void advanced_callback(Widget, XtPointer, XtPointer);
	static void ok_callback(Widget, XtPointer, XtPointer);
	static void cancel_callback(Widget, XtPointer, XtPointer);

	int selection_made() {return m_selection_made;}

private:
	XFE_PrefsPageLIGeneral* m_userFrame;
	int m_selection_made;
};

#endif /* __LILoginDialog_h */
