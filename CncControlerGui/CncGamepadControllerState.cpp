#include <sstream>
#include "wxCrafterImages.h"
#include "MainFrameProxy.h"
#include "CncReferencePosition.h"
#include "CncGamepadControllerState.h"

///////////////////////////////////////////////////////////////////
CncGamepadControllerState::CncGamepadControllerState(wxWindow* parent)
: CncGamepadControllerStateBase(parent)
, posCtrlMode(PCM_STICKS)
, running(false)
, xyNavigationActive(false)
, zNavigationActive(false)
, serviceShortName("Ds3Service")
, serviceLongName("SCP DSx Service")
///////////////////////////////////////////////////////////////////
{
}
///////////////////////////////////////////////////////////////////
CncGamepadControllerState::~CncGamepadControllerState() {
///////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::update(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	running = true;
		processTrace(state);
		processReferencePage(state);
		processRefPositionDlg(state);
		processPositionControlMode(state);
		processPosition(state);
	running = false;
}
///////////////////////////////////////////////////////////////////
bool CncGamepadControllerState::isRefPosDlgMode() {
///////////////////////////////////////////////////////////////////
	if ( APP_PROXY::getRefPositionDlg() == NULL )
		return false;

	return APP_PROXY::getRefPositionDlg()->IsShown();
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::processTrace(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	std::stringstream ss;
	ss << state;
	m_gamepadTrace->ChangeValue(ss.str());
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::processPositionControlMode(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	if ( state.data.buttonRightStick == true ) { 
		if ( posCtrlMode == PCM_STICKS || posCtrlMode == PCM_NAV_XY ) 	posCtrlMode = PCM_NAV_Z;
		else 															posCtrlMode = PCM_STICKS;
		
	} 
	
	if ( state.data.buttonLeftStick  == true ) { 
		if ( posCtrlMode == PCM_STICKS || posCtrlMode == PCM_NAV_Z  ) 	posCtrlMode = PCM_NAV_XY; 
		else															posCtrlMode = PCM_STICKS;
	}
	
	mangageMainView(state);
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::processReferencePage(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	if ( state.data.buttonStart ) {
		wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED);
		wxPostEvent(APP_PROXY::GetBtSelectReferences(), evt);
	}
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::processRefPositionDlg(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	if ( APP_PROXY::getRefPositionDlg() == NULL )
		return;
		
	// check what to do
	if ( APP_PROXY::getRefPositionDlg()->IsShown() == false ) {
		// open ref dialog - on demand
		if ( state.data.buttonX ) {
			APP_PROXY::showReferencePositionDlg("Opened by Gamepad");
			return;
		}
		
		// select step sensitivity - main frame
		if ( state.data.buttonA ) { 
			unsigned int sel = APP_PROXY::GetRbStepSensitivity()->GetSelection();
			unsigned int cnt = APP_PROXY::GetRbStepSensitivity()->GetCount();
		
			if ( sel + 1 >= cnt ) 	APP_PROXY::GetRbStepSensitivity()->SetSelection(0);
			else					APP_PROXY::GetRbStepSensitivity()->SetSelection(sel +1);
		}
		
		// always return here
		return;
	}
	
	// manage ref pos dlg
	
	// sensitivity
	if ( state.data.buttonA == true ) {
		wxRadioBox* rbs  = APP_PROXY::getRefPositionDlg()->GetRbStepSensitivity();
		unsigned int sel = rbs->GetSelection();
		unsigned int cnt = rbs->GetCount();
		
		// select it at the ref pos dlg
		if ( sel + 1 >= cnt ) 	rbs->SetSelection(0);
		else					rbs->SetSelection(sel +1);
		
		// select it at the main frame
		rbs  = APP_PROXY::GetRbStepSensitivity();
		if ( sel + 1 >= cnt ) 	rbs->SetSelection(0);
		else					rbs->SetSelection(sel +1);
	}
	
	// ref pos mode
	if ( state.data.buttonB ) {
		APP_PROXY::getRefPositionDlg()->shiftReferenceMode();
	}
	
	// set and close
	if ( state.data.buttonX ) {
		wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED);
		wxPostEvent(APP_PROXY::getRefPositionDlg()->GetBtSet(), evt);
	}
	
	// cancel and close
	if ( state.data.buttonY ) {
		wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED);
		wxPostEvent(APP_PROXY::getRefPositionDlg()->GetBtCancel(), evt);
	}
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::processPosition(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	if ( posCtrlMode == PCM_STICKS ) return managePositionViaStick(state);
	
	return managePositionViaNavi(state);
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::managePositionViaStick(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	typedef CncLinearDirection CLD;
	const float threshold = (float)state.data.stickResolutionFactor / 4;

	// z navigation
	if ( state.data.rightStickY <= -threshold || state.data.rightStickY >= +threshold ) {
		
		zNavigationActive = true;
		
		CLD dz = state.data.rightStickY > 0.0f ? CLD::CncPosDir : CLD::CncNegDir;
		APP_PROXY::manualContinuousMoveStart(CLD::CncNoneDir, CLD::CncNoneDir, dz);
		
	} else {
	
		// xy navigation
		if (    state.data.leftStickX <= -threshold  
		     || state.data.leftStickX >= +threshold 
		     || state.data.leftStickY <= -threshold 
		     || state.data.leftStickY >= +threshold 
		   )
		{
			xyNavigationActive = true;
			
			CLD dx = state.data.leftStickX > 0.0f ? CLD::CncPosDir : CLD::CncNegDir;
			CLD dy = state.data.leftStickY > 0.0f ? CLD::CncPosDir : CLD::CncNegDir;
			APP_PROXY::manualContinuousMoveStart(dx, dy,  CLD::CncNoneDir);
		}
	}
	
	// stop move
	if ( zNavigationActive == true ) {
		if ( state.data.rightStickY == 0.0f && state.data.rightStickY == 0.0f )
			APP_PROXY::manualContinuousMoveStop();
			
	} else if ( xyNavigationActive == true ) {
		if ( state.data.leftStickX  == 0.0f && state.data.leftStickY  == 0.0f )
			APP_PROXY::manualContinuousMoveStop();
			
	} else {
		// safety
		APP_PROXY::manualContinuousMoveStop();
	}
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::managePositionViaNavi(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	const bool left		= state.data.buttonLeft; 
	const bool right	= state.data.buttonRight;
	const bool up		= state.data.buttonUp;
	const bool down		= state.data.buttonDown;
	
	typedef CncLinearDirection CLD;
	CLD dx = CLD::CncNoneDir;
	CLD dy = CLD::CncNoneDir;
	CLD dz = CLD::CncNoneDir;
	
	switch ( posCtrlMode ) {
		case PCM_NAV_XY:	if ( left  == true ) 	dx = CLD::CncNegDir; 
							if ( right == true ) 	dx = CLD::CncPosDir; 
							if ( up    == true ) 	dy = CLD::CncPosDir;
							if ( down  == true ) 	dy = CLD::CncNegDir;
							
							APP_PROXY::manualContinuousMoveStart(dx, dy,  CLD::CncNoneDir);
							break;
							
		case PCM_NAV_Z:		if ( up    == true ) 	dz = CLD::CncPosDir;
							if ( down  == true ) 	dz = CLD::CncNegDir;
							
							APP_PROXY::manualContinuousMoveStart(CLD::CncNoneDir, CLD::CncNoneDir, dz);
							break;
							
		default:			return;
	}
	 
	if ( left == false && right == false && up == false && down == false )
		APP_PROXY::manualContinuousMoveStop();
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::mangageMainView(const GamepadEvent& state) {
///////////////////////////////////////////////////////////////////
	APP_PROXY::GetGpBmp1()->Show(false);
	APP_PROXY::GetGpBmp2()->Show(false);
	APP_PROXY::GetGpBmp3()->Show(false);
	APP_PROXY::GetGpBmp4()->Show(false);
	
	switch ( posCtrlMode ) {
		case PCM_STICKS:	APP_PROXY::GetGpBmp1()->SetBitmap(ImageLibGamepad().Bitmap("BMP_STICK_LEFT"));
							APP_PROXY::GetGpBmp2()->SetBitmap(ImageLibGamepad().Bitmap("BMP_STICK_RIGHT"));
							APP_PROXY::GetGpBmp1()->Show();
							APP_PROXY::GetGpBmp2()->Show();
							break;
							
		case PCM_NAV_XY:	APP_PROXY::GetGpBmp1()->SetBitmap(ImageLibGamepad().Bitmap("BMP_NAVI_XY"));
							APP_PROXY::GetGpBmp1()->Show();
							break;
							
		case PCM_NAV_Z:		APP_PROXY::GetGpBmp1()->SetBitmap(ImageLibGamepad().Bitmap("BMP_NAVI_Z"));
							APP_PROXY::GetGpBmp1()->Show();
							break;
	}
	
	APP_PROXY::GetGpBmp1()->Refresh();
	APP_PROXY::GetGpBmp2()->Refresh();
	APP_PROXY::GetGpBmp3()->Refresh();
	APP_PROXY::GetGpBmp4()->Refresh();
	
	APP_PROXY::GetGpBmp1()->GetParent()->Layout();
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::executeCommand(const wxString& cmd) {
///////////////////////////////////////////////////////////////////
	wxArrayString output;
	wxArrayString errors;
	
	wxExecute(cmd, output, errors);
	
	if ( output.GetCount() > 0 ) {
		for (size_t i = 0; i < output.GetCount(); i++)
			m_gamepadServiceTrace->AppendText(wxString::Format("  %s\n", output[i]));
	}
	
	if ( errors.GetCount() > 0 ) {
		
		const wxTextAttr defaultAttri = m_gamepadServiceTrace->GetDefaultStyle();
		
		wxTextAttr errorAttri = defaultAttri;
		errorAttri.SetTextColour(wxColour(241, 75, 84));
		m_gamepadServiceTrace->SetDefaultStyle(errorAttri);
		
		for (size_t i = 0; i < errors.GetCount(); i++)
			m_gamepadServiceTrace->AppendText(wxString::Format("  %s\n", errors[i]));
			
		m_gamepadServiceTrace->SetDefaultStyle(defaultAttri);
	}
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::startGamepadService(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_gamepadServiceTrace->AppendText(wxString::Format("Try to start the service [ %s(\"%s\") ] . . . \n", serviceLongName, serviceShortName));
	
	#ifdef __WXMSW__
		if ( false )	executeCommand("net start Ds3Service"); // only with admin rights possible
		else			executeCommand("cmd /C services.msc");
	#else
		m_gamepadServiceTrace->AppendText("Start Gamepad Service isn't supported");
	#endif
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::stopGamepadService(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_gamepadServiceTrace->AppendText(wxString::Format("Try to stop the service  [ %s(\"%s\") ] . . . \n", serviceLongName, serviceShortName));
	
	#ifdef __WXMSW__
		if ( false )	executeCommand("net stop Ds3Service");  // only with admin rights possible
		else			executeCommand("cmd /C services.msc");
	#else
		m_gamepadServiceTrace->AppendText("Stop Gamepad Service isn't supported");
	#endif
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::queryGamepadService(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_gamepadServiceTrace->AppendText(wxString::Format("Query the service [ %s(\"%s\") ] . . . \n", serviceLongName, serviceShortName));
	
	#ifdef __WXMSW__
		executeCommand("sc query Ds3Service");
	#else
		m_gamepadServiceTrace->AppendText("Query Gamepad Service isn't supported");
	#endif
}
///////////////////////////////////////////////////////////////////
void CncGamepadControllerState::clearGamepadServiceTrace(wxCommandEvent& event) {
///////////////////////////////////////////////////////////////////
	m_gamepadServiceTrace->Clear();
}
