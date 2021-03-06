#include "wxCrafterImages.h"
#include "GlobalFunctions.h"
#include "MainFrameProxy.h"
#include "CncOpenGLContextObserver.h"

//////////////////////////////////////////////////////////////////
CncOpenGLContextObserver::CncOpenGLContextObserver(wxWindow* parent)
: CncOpenGLContextObserverBase(parent)
, heartbeatFlag(false)
, infoCounter(0)
, currentCtxList(NULL)
, registeredCtxList(NULL)
, historyList(NULL)
, historyStage()
//////////////////////////////////////////////////////////////////
{
	CncGLContextObserver::getInstance()->setCallbackInterface(this);
	
	currentCtxList 		= new CncOpenGLCurrentContextObserverListCtrl	(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL);
	registeredCtxList 	= new CncOpenGLRegisteredContextObserverListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL);
	historyList		 	= new CncOpenGLContextObserverHistoryListCtrl	(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL);
	
	GblFunc::replaceControl(m_currentContextListPlaceholder, 	currentCtxList);
	GblFunc::replaceControl(m_allContextListPlaceholder, 		registeredCtxList);
	GblFunc::replaceControl(m_historyInfoPlaceholder, 			historyList);
	
	historyList->setDetaiInfoControl(m_historyDetailInfo);
	
	if ( m_startupTimer->IsRunning() )
		m_startupTimer->Stop();

	if ( m_continuousTimer->IsRunning() )
		m_continuousTimer->Stop();
	
	m_startupTimer->Start();
}
//////////////////////////////////////////////////////////////////
CncOpenGLContextObserver::~CncOpenGLContextObserver() {
//////////////////////////////////////////////////////////////////
	historyStage.clear();
	
	if ( m_startupTimer->IsRunning() )
		m_startupTimer->Stop();

	if ( m_continuousTimer->IsRunning() )
		m_continuousTimer->Stop();

	wxDELETE( currentCtxList );
	wxDELETE( registeredCtxList );
	wxDELETE( historyList );
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::onCloseWindow(wxCloseEvent& event) {
//////////////////////////////////////////////////////////////////
	APP_PROXY::GetMiOpenGLContextObserver()->Check(false);
	Show(false);
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::onStartupTimer(wxTimerEvent& event) {
//////////////////////////////////////////////////////////////////
	if ( m_startupTimer->IsRunning() )
		m_startupTimer->Stop();
	
	// fixing layout problems
	Update();
	
	m_continuousTimer->Start();
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::onContinuousTimer(wxTimerEvent& event) {
//////////////////////////////////////////////////////////////////
	if ( IsShownOnScreen() ) {
		
		if ( heartbeatFlag )	{ heartbeatFlag = false; m_bmpHeartbeat->SetBitmap(ImageLibHeartbeat().Bitmap("BMP_HEART")); }
		else					{ heartbeatFlag = true;  m_bmpHeartbeat->SetBitmap(ImageLibHeartbeat().Bitmap("BMP_HEART_PLUS")); }
		
		m_bmpHeartbeat->Refresh();
		
		// update context list
		if ( currentCtxList->IsShownOnScreen() )
			currentCtxList->Refresh();
		
		// update registered list
		if ( registeredCtxList->IsShownOnScreen() )
			registeredCtxList->Refresh();
		
		// update history  list
		if ( historyList->IsShownOnScreen() ) {
			// move stage content
			if ( historyStage.size() > 0 ) {
				for (auto it= historyStage.begin(); it != historyStage.end(); ++it ) 
					historyList->appendItem(*it);
				historyStage.clear();
			}
			historyList->Refresh();
		}
	}
	
	if ( infoCounter > 0 ) {
		
		if ( --infoCounter == 0 )
			m_infoText->Clear();
			
		const wxColour colour = infoCounter%2 != 0 ? wxColour(239, 228, 176) : wxColour(200, 200, 200);
		m_infoText->SetBackgroundColour(colour);
		m_infoText->Refresh();
	}
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::addHistoryItem(const wxString& ctxName, const wxString& functName, const wxString& item, const char type, const char mode) {
//////////////////////////////////////////////////////////////////
	CncColumContainer cc(CncOpenGLContextObserverHistoryListCtrl::TOTAL_COL_COUNT);
	cc.updateItem(CncOpenGLContextObserverHistoryListCtrl::COL_TIME, 	wxDateTime::UNow().FormatISOTime());
	cc.updateItem(CncOpenGLContextObserverHistoryListCtrl::COL_TYPE, 	wxString::Format("%c", type));
	cc.updateItem(CncOpenGLContextObserverHistoryListCtrl::COL_MODE, 	wxString::Format("%c", mode));
	cc.updateItem(CncOpenGLContextObserverHistoryListCtrl::COL_CTX, 	ctxName);
	cc.updateItem(CncOpenGLContextObserverHistoryListCtrl::COL_FUNCT, 	functName);
	cc.updateItem(CncOpenGLContextObserverHistoryListCtrl::COL_MSG, 	item);
	
	if ( false ) {
		// negtive peformance !!!
		historyList->appendHistoryItem(cc);
		historyList->Refresh();
	} else {
		if ( historyList->IsShownOnScreen() )	{ historyStage.push_back(cc); }
		else 									{ historyList->appendItem(cc); }
	}
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::nofifyForCurrent(const wxString& newCtxName) {
//////////////////////////////////////////////////////////////////
	m_infoText->ChangeValue(wxString::Format("Current Context changed to '%s'. . . ", newCtxName));
	addHistoryItem(GL_CTX_OBS->getCurrentContextName(), CNC_LOG_FUNCT, m_infoText->GetValue(), 'S', 'N');
	
	activateInfoBell();
	
	currentCtxList->SetItemCount(GL_CTX_OBS->getContextValueCount());
	currentCtxList->Refresh();
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::nofifyForRegistered(const wxString& newCtxName) {
//////////////////////////////////////////////////////////////////
	m_infoText->ChangeValue(wxString::Format("New Context '%s' registered . . . ", newCtxName));
	addHistoryItem(GL_CTX_OBS->getCurrentContextName(), CNC_LOG_FUNCT, m_infoText->GetValue(), 'I', 'N');
	
	activateInfoBell();
	
	registeredCtxList->SetItemCount(GL_CTX_OBS->getContextCount());
	registeredCtxList->Refresh();
}
//////////////////////////////////////////////////////////////////
void CncOpenGLContextObserver::nofifyMessage(const char type, const wxString& curCtxName, const wxString& functName, const wxString& msg) {
//////////////////////////////////////////////////////////////////
	addHistoryItem(curCtxName, functName, msg, type, 'M');
}
