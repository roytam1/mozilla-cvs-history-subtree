
#include "widgetry.h"
#include "pw_public.h"

class CXPProgressDialog: public CDialog
{
public:
	CXPProgressDialog(CWnd *pParent =NULL);

	void SetCancelCallback(PW_CancelCallback cb, void*closure);
	void SetProgressValue(int32 value);
	int SetRange(int32 min,int32 max);
	virtual  BOOL OnInitDialog( );
	BOOL PreTranslateMessage( MSG* pMsg );
    CProgressMeter m_ProgressMeter;
	CStatic	m_PercentComplete;
	int32 m_Min;
	int32 m_Max;
	int32 m_Range;
	PW_CancelCallback m_cancelCallback;
	void * m_cancelClosure;

protected:
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange*);
	afx_msg int OnCreate( LPCREATESTRUCT );

	DECLARE_MESSAGE_MAP()
};

extern CWnd *FE_GetDialogOwnerFromContext(MWContext *context);
