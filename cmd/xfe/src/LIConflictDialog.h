/**********************************************************************
 LIConflictDialog.h
 By Daniel Malmer
 5/18/98

**********************************************************************/

#ifndef __LIConflictDialog_h
#define __LIConflictDialog_h

#include "Dialog.h"

class XFE_LIConflictDialog : public XFE_Dialog
{
public:

	// Constructors, Destructors

	XFE_LIConflictDialog(Widget parent, const char*, const char*, const char*, const char*);

	virtual ~XFE_LIConflictDialog();

	// Accessor functions

	// Modifier functions

	// virtual void show();

	// virtual void hide();

	void useLocalCallback(Widget, XtPointer);
	void useServerCallback(Widget, XtPointer);

	static void ok_callback(Widget, XtPointer, XtPointer);
	static void cancel_callback(Widget, XtPointer, XtPointer);

	int state();

	int selection_made() {return m_selection_made;}

private:
	Widget m_message_label;
	Widget m_query_label;
	Widget m_always_toggle;
	int m_state;
	int m_selection_made;
	void setTitle(const char* title);
};

#endif /* __LIConflictDialog_h */
