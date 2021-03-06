#include "GlobalFunctions.h"
#include "wxCrafterLCDPanel.h"
#include "MainFrame.h"
#include "CncLCDPositionPanel.h"

///////////////////////////////////////////////////////////////////
CncLCDPositionPanel::CncLCDPositionPanel(wxWindow* parent)
: CncLCDPositionPanelBase(parent)
, lcdW(NULL)
, lcdX(NULL)
, lcdY(NULL)
, lcdZ(NULL)
///////////////////////////////////////////////////////////////////
{
	lcdW = new wxLCDWindow(this, wxDefaultPosition, wxDefaultSize);
	GblFunc::replaceControl(m_lcdPlaceholderW, lcdW);
	
	lcdX = new wxLCDWindow(this, wxDefaultPosition, wxDefaultSize);
	GblFunc::replaceControl(m_lcdPlaceholderX, lcdX);
	
	lcdY = new wxLCDWindow(this, wxDefaultPosition, wxDefaultSize);
	GblFunc::replaceControl(m_lcdPlaceholderY, lcdY);
	
	lcdZ = new wxLCDWindow(this, wxDefaultPosition, wxDefaultSize);
	GblFunc::replaceControl(m_lcdPlaceholderZ, lcdZ);
	
	lcdW->SetNumberDigits(8);
	lcdX->SetNumberDigits(8);
	lcdY->SetNumberDigits(8);
	lcdZ->SetNumberDigits(8);
	
	lcdW->SetValue(   "0.0" ); 
	lcdX->SetValue( "0.000" ); 
	lcdY->SetValue( "0.000" ); 
	lcdZ->SetValue( "0.000" ); 
}
///////////////////////////////////////////////////////////////////
CncLCDPositionPanel::~CncLCDPositionPanel() {
///////////////////////////////////////////////////////////////////
	delete lcdW;
	delete lcdX;
	delete lcdY;
	delete lcdZ;
}
///////////////////////////////////////////////////////////////////
void CncLCDPositionPanel::onStartupTimer(wxTimerEvent& event) {
///////////////////////////////////////////////////////////////////
	//SetClientSize(wxSize(100, 500));
	Refresh();
}
///////////////////////////////////////////////////////////////////
void CncLCDPositionPanel::onPaint(wxPaintEvent& event) {
///////////////////////////////////////////////////////////////////
	event.Skip();
	
	lcdW->Refresh();
	lcdX->Refresh();
	lcdY->Refresh();
	lcdZ->Refresh();
}
///////////////////////////////////////////////////////////////////
void CncLCDPositionPanel::onSize(wxSizeEvent& event) {
///////////////////////////////////////////////////////////////////
	event.Skip();
	
	lcdW->Refresh();
	lcdX->Refresh();
	lcdY->Refresh();
	lcdZ->Refresh();
}
///////////////////////////////////////////////////////////////////
void CncLCDPositionPanel::onUpdateTimer(wxTimerEvent& event) {
///////////////////////////////////////////////////////////////////
	lcdW->SetValue(THE_APP->GetConfiguredFeedSpeed()->GetValue());
	
	lcdX->SetValue(THE_APP->GetXAxisCtl()->GetValue());
	lcdY->SetValue(THE_APP->GetYAxisCtl()->GetValue());
	lcdZ->SetValue(THE_APP->GetZAxisCtl()->GetValue());
	
	const wxString unit(THE_APP->GetUnit()->GetValue());
	if ( m_unitX->GetLabel() != unit) {
		m_unitX->SetLabel(unit);
		m_unitY->SetLabel(unit);
		m_unitZ->SetLabel(unit);
	}
}
