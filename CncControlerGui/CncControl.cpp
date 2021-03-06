#include <iostream>
#include <sstream>
#include <cfloat>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/slider.h>
#include <wx/dataview.h>
#include <wx/propgrid/manager.h>
#include <wx/evtloop.h>
#include "OSD/CncAsyncKeyboardState.h"
#include "GlobalFunctions.h"
#include "DataControlModel.h"
#include "SerialPort.h"
#include "SerialEmulatorNull.h"
#include "SerialEmulatorTextStreamer.h"
#include "SerialEmulatorSvgStreamer.h"
#include "SerialEmulatorGCodeStreamer.h"
#include "SerialEmulatorBinaryStreamer.h"
#include "SerialThreadStub.h"
#include "CncMotionMonitor.h"
#include "CncExceptions.h"
#include "CncCommon.h"
#include "CncContext.h"
#include "CncFileNameService.h"
#include "CncLoggerProxy.h"
#include "wxCrafterImages.h"
#include "MainFrame.h"
#include "CncControl.h"

static CommandTemplates CMDTPL;

///////////////////////////////////////////////////////////////////
CncControl::CncControl(CncPortType pt) 
: currentClientId(-1)
, runContinuousMove(false)
, setterMap()
, serialPort(NULL)
, zeroAppPos(0,0,0)
, startAppPos(0,0,0)
, curAppPos(0,0,0)
, realtimeFeedSpeed_MM_MIN(0.0)
, defaultFeedSpeedRapid_MM_MIN(THE_CONFIG->getDefaultRapidSpeed_MM_MIN())
, defaultFeedSpeedWork_MM_MIN(THE_CONFIG->getDefaultRapidSpeed_MM_MIN())
, configuredSpeedMode(CncSpeedRapid)
, configuredFeedSpeed_MM_MIN(0.0)
, durationCounter(0)
, interruptState(false)
, positionOutOfRangeFlag(false)
, toolPowerState(TOOL_STATE_OFF)
, stepDelay(0)
, lastCncHeartbeatValue(0)
, toolState()
, positionCheck(true)
, drawPaneMargin(30)
{
//////////////////////////////////////////////////////////////////
	// Serial factory
	if      ( pt == CncPORT ) 				serialPort = new SerialSpyPort(this);
	else if ( pt == CncEMU_NULL )			serialPort = new SerialEmulatorNULL(this);
	else if ( pt == CncEMU_TXT )			serialPort = new SerialEmulatorTextStreamer(this);
	else if ( pt == CncEMU_SVG )			serialPort = new SerialEmulatorSvgStreamer(this);
	else if ( pt == CncEMU_GCODE )			serialPort = new SerialEmulatorGCodeStreamer(this);
	else if ( pt == CncEMU_BIN )			serialPort = new SerialEmulatorBinaryStreamer(this);
	else if ( pt == CncPORT_EMU_ARDUINO )	serialPort = new SerialThreadStub(this);
	else 									serialPort = new SerialSpyPort(this);
	
	toolState.setControl(THE_APP->GetToolState());
	setToolState(true);
	
	serialPort->enableSpyOutput();
}
///////////////////////////////////////////////////////////////////
CncControl::~CncControl() {
///////////////////////////////////////////////////////////////////
	assert(serialPort);
	
	if ( getToolState() == TOOL_STATE_ON )
		switchToolOff();
	
	// safty
	disconnect();

	delete serialPort;
}
//////////////////////////////////////////////////////////////////
const CncDoublePosition CncControl::getStartPosMetric() {
//////////////////////////////////////////////////////////////////
	CncDoublePosition retValue;
	retValue.setXYZ(startAppPos.getX() * THE_CONFIG->getDisplayFactX(),
				    startAppPos.getY() * THE_CONFIG->getDisplayFactY(),
	                startAppPos.getZ() * THE_CONFIG->getDisplayFactZ());
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncDoublePosition CncControl::getCurAppPosMetric() {
//////////////////////////////////////////////////////////////////
	CncDoublePosition retValue;
	retValue.setXYZ(curAppPos.getX() * THE_CONFIG->getDisplayFactX(),
				    curAppPos.getY() * THE_CONFIG->getDisplayFactY(),
	                curAppPos.getZ() * THE_CONFIG->getDisplayFactZ());
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncDoublePosition CncControl::getCurCtlPosMetric() {
//////////////////////////////////////////////////////////////////
	CncDoublePosition retValue;
	retValue.setXYZ(curCtlPos.getX() * THE_CONFIG->getDisplayFactX(),
				    curCtlPos.getY() * THE_CONFIG->getDisplayFactY(),
	                curCtlPos.getZ() * THE_CONFIG->getDisplayFactZ());
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncLongPosition CncControl::getMinPositions() {
//////////////////////////////////////////////////////////////////
	CncLongPosition retValue;
	retValue.setXYZ(curAppPos.getXMin(), curAppPos.getYMin(), curAppPos.getZMin());
	return retValue;
}//////////////////////////////////////////////////////////////////
const CncLongPosition CncControl::getMaxPositions() {
//////////////////////////////////////////////////////////////////
	CncLongPosition retValue;
	retValue.setXYZ(curAppPos.getXMax(), curAppPos.getYMax(), curAppPos.getZMax());
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncDoublePosition CncControl::getMinPositionsMetric() {
//////////////////////////////////////////////////////////////////
	CncDoublePosition retValue;
	retValue.setXYZ(curAppPos.getXMin() * THE_CONFIG->getDisplayFactX(),
					curAppPos.getYMin() * THE_CONFIG->getDisplayFactY(),
					curAppPos.getZMin() * THE_CONFIG->getDisplayFactZ());
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncDoublePosition CncControl::getMaxPositionsMetric() {
//////////////////////////////////////////////////////////////////
	CncDoublePosition retValue;
	retValue.setXYZ(curAppPos.getXMax() * THE_CONFIG->getDisplayFactX(),
					curAppPos.getYMax() * THE_CONFIG->getDisplayFactY(),
					curAppPos.getZMax() * THE_CONFIG->getDisplayFactZ());
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncLongPosition::Watermarks CncControl::getWaterMarks() {
//////////////////////////////////////////////////////////////////
	CncLongPosition::Watermarks retValue;
	curAppPos.getWatermarks(retValue);
	return retValue;
}
//////////////////////////////////////////////////////////////////
const CncDoublePosition::Watermarks CncControl::getWaterMarksMetric() {
//////////////////////////////////////////////////////////////////
	CncDoublePosition::Watermarks retValue;

	CncLongPosition::Watermarks xyz = CncControl::getWaterMarks();
	retValue.xMin = xyz.xMin * THE_CONFIG->getDisplayFactX();
	retValue.xMax = xyz.xMax * THE_CONFIG->getDisplayFactX();

	retValue.yMin = xyz.yMin * THE_CONFIG->getDisplayFactY();
	retValue.yMax = xyz.yMax * THE_CONFIG->getDisplayFactY();
	
	retValue.zMin = xyz.zMin * THE_CONFIG->getDisplayFactZ();
	retValue.zMax = xyz.zMax * THE_CONFIG->getDisplayFactZ();

	return retValue;
}
///////////////////////////////////////////////////////////////////
bool CncControl::execute(const unsigned char* buffer, unsigned int nbByte) {
///////////////////////////////////////////////////////////////////
	return getSerial()->execute(buffer, nbByte);
}
///////////////////////////////////////////////////////////////////
bool CncControl::processSetter(unsigned char pid, int32_t value) {
///////////////////////////////////////////////////////////////////
	cnc::SetterValueList values;
	values.push_back(value);
	
	return processSetter(pid, values);
}
///////////////////////////////////////////////////////////////////
bool CncControl::processSetter(unsigned char pid, const cnc::SetterValueList& values) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() )
		return false;

	if ( isConnected() == false )
		return false;
	
	if ( pid != PID_SEPARATOR ) {
		
		if ( THE_CONFIG->getAvoidDupSetterValuesFlag() ) {
			auto it = setterMap.find((int)pid);
			if ( it != setterMap.end() ) {
				// any value(s) are changed?
				cnc::SetterValueList smvl = it->second;
				if ( smvl.size() == values.size() ) {
					bool somethingChanged = false;
					auto itvv=values.begin();
					for ( auto itmv=smvl.begin(); itmv != smvl.end(); itmv++, itvv++) {
						if ( *itmv != *itvv ) {
							somethingChanged = true;
							break;
						}
					}
					if ( somethingChanged == false )
						return true;
				}
			}
		}
			
		if ( serialPort->processSetter(pid, values) == false ) {
			std::cerr << std::endl << "CncControl::processSetter: Setter failed." << std::endl;
			std::cerr << " Id:    " << ArduinoPIDs::getPIDLabel((int)pid) << std::endl;
			std::cerr << " Value(s): ";
			cnc::traceSetterValueList(std::cerr, values, pid < PID_FLOAT_RANG_START ? 1 : FLT_FACT);
			std::cerr << std::endl;
			return false;
		}
		
		// store
		appendToSetterMap(pid, values);
	}
	
	// publish setter event
	typedef UpdateManagerThread::Event Event;
	static Event evt;
	THE_APP->umPostEvent(evt.SetterEvent(pid, values));
		
	wxCommandEvent comevt(wxEVT_COMMAND_BUTTON_CLICKED);
	wxPostEvent(THE_APP->GetBtRefreshSetterList(), comevt);
	
	return true;
}
///////////////////////////////////////////////////////////////////
void CncControl::appendToSetterMap(unsigned char pid, const cnc::SetterValueList& values) {
///////////////////////////////////////////////////////////////////
	// add all pids which are shouldn't stored 
	switch ( pid ) {
		case PID_X_POS:
		case PID_Y_POS:
		case PID_Z_POS:
							return;
	}
	
	setterMap[pid] = values;
}
///////////////////////////////////////////////////////////////////
bool CncControl::processSetterList(const Setters& setup) {
///////////////////////////////////////////////////////////////////
	for ( auto itl = setup.begin(); itl != setup.end(); ++itl) {
		cnc::SetterValueList svl = (*itl).values;
		unsigned char pid   = (*itl).pid;
		
		if ( processSetter(pid, svl) == false )
			return false;
	}
	
	return true;
}
///////////////////////////////////////////////////////////////////
void CncControl::resetSetterMap() {
///////////////////////////////////////////////////////////////////
	setterMap.clear();
}
///////////////////////////////////////////////////////////////////
bool CncControl::setup(bool doReset) {
///////////////////////////////////////////////////////////////////
	wxASSERT(serialPort);
	wxASSERT(THE_CONFIG);
	
	// always reset the map here to definitly reinitianlize the controller
	resetSetterMap();
	
	// init setup
	processSetter(PID_SEPARATOR, SEPARARTOR_SETUP);
	
	if ( serialPort->isConnected() == false ) 
		return false;
	
	// reste controller on demand
	if ( doReset == true ) {
		if ( reset() == false ) {
			std::cerr << " CncControl::setup: reset controller failed!\n";
			return false;
		}
		
		// Firmware check
		std::stringstream ss;
		processCommand(CMD_PRINT_VERSION, ss);
		std::cout << " Firmware check . . . [Available: " << ss.str() << "; Required: " << FIRMWARE_VERSION << "] . . .";
		
		if ( wxString(FIRMWARE_VERSION) == ss.str().c_str() )	std::clog << " OK" << std::endl;
		else													cnc::cex1 << " Firmware is possibly not compatible!" << std::endl;
	}
	
	// always switch the tool off - safety - but may be already done by reset();
	switchToolOff(true);
	
	// evaluate limit states
	evaluateLimitState();
	
	std::cout << " Starting controller initialization . . . \n";
	CncTextCtrl* logger = THE_CONFIG->getTheApp()->getLogger(); wxASSERT( logger != NULL );
	logger->logCurrentPosition();
	
	// setup probe mode
	if ( enableProbeMode(THE_CONTEXT->isProbeMode()) == false ) {
		std::cerr << " CncControl::setup: Probe mode configuration failed!\n";
		return false;
	}
	
	if ( THE_CONFIG->getStepsX() <= 0 ) { std::cerr << CNC_LOG_FUNCT << ": Invalid Steps configuration, X axis! Steps: " << THE_CONFIG->getStepsX() << std::endl; return false; }
	if ( THE_CONFIG->getStepsY() <= 0 ) { std::cerr << CNC_LOG_FUNCT << ": Invalid Steps configuration, Y axis! Steps: " << THE_CONFIG->getStepsX() << std::endl; return false; }
	if ( THE_CONFIG->getStepsZ() <= 0 ) { std::cerr << CNC_LOG_FUNCT << ": Invalid Steps configuration, Z axis! Steps: " << THE_CONFIG->getStepsX() << std::endl; return false; }
	
	// process initial setters
	Setters setup;
	
	cnc::SetterValueList accelList;
	accelList.push_back(FLT_FACT * (float)(THE_CONFIG->getPitchX() / THE_CONFIG->getStepsX()));
	accelList.push_back(FLT_FACT * (float)(THE_CONFIG->getPitchY() / THE_CONFIG->getStepsY()));
	accelList.push_back(FLT_FACT * (float)(THE_CONFIG->getPitchZ() / THE_CONFIG->getStepsZ()));
	accelList.push_back(FLT_FACT * (float)(THE_CONFIG->getHighPulsWidthX()));
	accelList.push_back(FLT_FACT * (float)(THE_CONFIG->getHighPulsWidthY()));
	accelList.push_back(FLT_FACT * (float)(THE_CONFIG->getHighPulsWidthZ()));
	
	#warning use config values
	accelList.push_back(FLT_FACT * 0.0);
	accelList.push_back(FLT_FACT * 0.3);
	accelList.push_back(FLT_FACT * 333.0/60);
	accelList.push_back(FLT_FACT * 0.0);
	accelList.push_back(FLT_FACT * 0.1);
	accelList.push_back(FLT_FACT * 333.0/60);
	setup.push_back(SetterTuple(PID_ACCEL_PROFILE, accelList));
	
	setup.push_back(SetterTuple(PID_POS_REPLY_THRESHOLD, THE_CONFIG->getReplyThresholdSteps()));
	
	int32_t dirValueX = THE_CONFIG->getInverseCtrlDirectionXFlag() ? INVERSED_INCREMENT_DIRECTION_VALUE : NORMALIZED_INCREMENT_DIRECTION_VALUE;
	int32_t dirValueY = THE_CONFIG->getInverseCtrlDirectionYFlag() ? INVERSED_INCREMENT_DIRECTION_VALUE : NORMALIZED_INCREMENT_DIRECTION_VALUE;
	int32_t dirValueZ = THE_CONFIG->getInverseCtrlDirectionZFlag() ? INVERSED_INCREMENT_DIRECTION_VALUE : NORMALIZED_INCREMENT_DIRECTION_VALUE;

	setup.push_back(SetterTuple(PID_INC_DIRECTION_VALUE_X, dirValueX));
	setup.push_back(SetterTuple(PID_INC_DIRECTION_VALUE_Y, dirValueY));
	setup.push_back(SetterTuple(PID_INC_DIRECTION_VALUE_Z, dirValueZ));
	
	if ( processSetterList(setup) == false) {
		std::cerr << " CncControl::setup: Calling processSetterList() failed!\n";
		return false;
	}
	
	// enable stepper motors, do this here because to initiate a defined state
	enableStepperMotors(ENABLE_STATE_ON);
	
	// speed setup
	changeSpeedToDefaultSpeed_MM_MIN(CncSpeedRapid);
	
	// check if some output was logged in between, if not 
	// remove last '\n' and put 'Ready' at the end of the
	// same line as the starting the initialization hint
	logger->skipBackIfLoggedPositionEqualCurrent();
	std::clog << "Ready - OK\n";
	serialPort->notifySetupSuccesfullyFinsihed();
	return true;
}
///////////////////////////////////////////////////////////////////
long CncControl::convertDoubleToCtrlLong(unsigned char id, float f) { 
///////////////////////////////////////////////////////////////////
	if ( f <= MIN_LONG / FLT_FACT ) {
		std::cerr << "CncControl::convertDoubleToCtrlLong(): Invalid double value: '" << f << "' for PID: " << ArduinoPIDs::getPIDLabel(id) << std::endl;
		return MIN_LONG; 
	}
		
	if ( f >= MAX_LONG / FLT_FACT ) {
		std::cerr << "CncControl::convertDoubleToCtrlLong(): Invalid double value: '" << f << "' for PID: " << ArduinoPIDs::getPIDLabel(id) << std::endl;
		return MAX_LONG; 
	}
		
	return f * FLT_FACT; 
}
///////////////////////////////////////////////////////////////////
bool CncControl::disconnect() {
///////////////////////////////////////////////////////////////////
	if ( serialPort->isConnected() ) {
		CncTextCtrl* logger = THE_CONFIG->getTheApp()->getLogger(); wxASSERT( logger != NULL );
		
		std::cout << " Disconnecting serial port . . .\n";
		logger->logCurrentPosition();
		
		serialPort->disconnect();
		
		logger->skipBackIfLoggedPositionEqualCurrent();
		std::clog << " Disconnected\n";
	}
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::connect(const char * portName) {
///////////////////////////////////////////////////////////////////
	wxASSERT(serialPort);
	disconnect();
	
	wxFileName fn(portName);
	std::clog << "Try to connect to: " << serialPort->getClassName() 
	                                   << "("
									   << fn.GetFullName() 
									   << ")" 
									   << " - " 
									   << CncFileNameService::getSession()
									   << std::endl;
									   
	bool ret = serialPort->connect(portName);
	if ( ret == true ) {
		
		std::cout << " . . . Connection established -";
		std::clog << " OK" << std::endl;
		
	} else {
		
		std::cout << " . . . Connection refused -";
		std::cerr << " ERROR" << std::endl;
		
	}

	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::isConnected() {
///////////////////////////////////////////////////////////////////
	wxASSERT(serialPort);
	return serialPort->isConnected();
}
///////////////////////////////////////////////////////////////////
void CncControl::onPeriodicallyAppEvent() {
///////////////////////////////////////////////////////////////////
	wxASSERT(serialPort);
	serialPort->onPeriodicallyAppEvent(isInterrupted());
}
///////////////////////////////////////////////////////////////////
bool CncControl::processCommand(const unsigned char c, std::ostream& txtCtl) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() == true )
		return false;
	
	wxASSERT(serialPort);
	return serialPort->processCommand(c, txtCtl);
}
///////////////////////////////////////////////////////////////////
bool CncControl::processMoveXYZ(int32_t x1, int32_t y1, int32_t z1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() == true )
		return false;

	wxASSERT(serialPort);
	return serialPort->processMoveXYZ(x1, y1, z1, alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::processMoveSequence(CncMoveSequence& moveSequence) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() == true )
		return false;

	wxASSERT(serialPort);
	return serialPort->processMoveSequence(moveSequence);
}
///////////////////////////////////////////////////////////////////
void CncControl::updateDrawControl() {
///////////////////////////////////////////////////////////////////
	updatePreview3D(true); 
}
///////////////////////////////////////////////////////////////////
inline void CncControl::setValue(wxTextCtrl *ctl, int32_t val) {
///////////////////////////////////////////////////////////////////
	if ( ctl != NULL )
		ctl->ChangeValue(wxString::Format(wxT("%i"),val));
}
///////////////////////////////////////////////////////////////////
inline void CncControl::setValue(wxTextCtrl *ctl, double val) {
///////////////////////////////////////////////////////////////////
	if ( ctl != NULL )
		ctl->ChangeValue(wxString::Format(wxT("%4.3f"),val));
}
///////////////////////////////////////////////////////////////////
inline void CncControl::setValue(wxTextCtrl *ctl, const char* val) {
///////////////////////////////////////////////////////////////////
	if ( ctl != NULL )
		ctl->ChangeValue(wxString::Format(wxT("%s"),val));
}
///////////////////////////////////////////////////////////////////
void CncControl::setZeroPos(bool x, bool y, bool z) {
///////////////////////////////////////////////////////////////////
	// -------------------------------------------------------------
	auto setToZeroPosX =[&]() {
		curAppPos.setX(0);
		zeroAppPos.setX(0);
		startAppPos.setX(0);
		
		if ( isConnected() == true ) {
			if ( processSetter(PID_X_POS, (int32_t)(0)) == false ) {
				std::cerr << "Can't zero controllers X position!" << std::endl;
				return;
			}
		}
		
		curCtlPos.setX(0);
	};
	
	// -------------------------------------------------------------
	auto setToZeroPosY = [&]() {
		curAppPos.setY(0);
		zeroAppPos.setY(0);
		startAppPos.setY(0);
		
		if ( isConnected() == true ) {
			if ( processSetter(PID_Y_POS, (int32_t)(0)) == false ) {
				std::cerr << "Can't zero controllers Y position!" << std::endl;
				return;
			}
		}
		
		curCtlPos.setY(0);
	};
	
	// -------------------------------------------------------------
	auto setToZeroPosZ = [&]() {
		int32_t val = 0L;
		
		if ( THE_CONFIG->getReferenceIncludesWpt() == true )
			val = (long)round(THE_CONFIG->getWorkpieceThickness() * THE_CONFIG->getCalculationFactZ());
		
		curAppPos.setZ(val);
		zeroAppPos.setZ(val);
		startAppPos.setZ(val);
		
		if ( isConnected() == true ) {
			if ( processSetter(PID_Z_POS, (int32_t)(val)) == false ) {
				std::cerr << "Can't zero controllers Z position!" << std::endl;
				return;
			}
		}
		
		curCtlPos.setZ(val);
	};
	
	// -------------------------------------------------------------
	if ( x ) setToZeroPosX();
	if ( y ) setToZeroPosY();
	if ( z ) setToZeroPosZ();
	
	postAppPosition(PID_XYZ_POS_MAJOR);
	postCtlPosition(PID_XYZ_POS_MAJOR);
}
///////////////////////////////////////////////////////////////////
void CncControl::setZeroPos() {
///////////////////////////////////////////////////////////////////
	setZeroPos(true, true, true);
}
///////////////////////////////////////////////////////////////////
void CncControl::setStartPos() {
///////////////////////////////////////////////////////////////////
	startAppPos = curAppPos;
}
///////////////////////////////////////////////////////////////////
void CncControl::interrupt(const char* why) {
///////////////////////////////////////////////////////////////////
	std::cerr << wxString::Format("CncControl: Interrupted: %s", why ? why : "") << std::endl;
	
	interruptState = true;
	switchToolOff(true);
	
	throw CncInterruption(why);
}
///////////////////////////////////////////////////////////////////
bool CncControl::isReadyToRun() {
///////////////////////////////////////////////////////////////////
	if ( isConnected() == false ) {
		std::cerr << "CncControl::isReadyToRun: The controller isn't connected!" << std::endl;
		return false;
	}
	
	if ( isInterrupted() == true ) {
		std::cerr << "CncControl::isReadyToRun: The controller is interrupted. A reset is required!" << std::endl;
		return false;
	}
	
	// query the serial port
	std::vector<int32_t> list;
	getSerial()->processGetter(PID_QUERY_READY_TO_RUN, list);
		
	if ( list.size() != 1 ) {
		std::cerr << "CncControl::isReadyToRun: Unable due to the corresponding state of the serial port:" << std::endl;
		return false;
	}
	
	return ( list.at(0) == 1L );
}
///////////////////////////////////////////////////////////////////
bool CncControl::isInterrupted() {
///////////////////////////////////////////////////////////////////
	return interruptState;
}
///////////////////////////////////////////////////////////////////
void CncControl::resetInterrupt() {
///////////////////////////////////////////////////////////////////
	resetDurationCounter();
	interruptState = false;
}
///////////////////////////////////////////////////////////////////
bool CncControl::resetWatermarks() {
///////////////////////////////////////////////////////////////////
	zeroAppPos.resetWatermarks();
	startAppPos.resetWatermarks();
	curAppPos.resetWatermarks();
	curCtlPos.resetWatermarks();
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::reset() {
///////////////////////////////////////////////////////////////////
	getSerial()->purge();
	resetInterrupt();
	resetPositionOutOfRangeFlag();
	
	CncLoggerProxy* logger = THE_CONFIG->getTheApp()->getLogger(); 
	wxASSERT( logger != NULL );
	
	std::cout << " Try to reset the controller . . .\n";
	logger->logCurrentPosition();
	
	bool ret = processCommand(CMD_RESET_CONTROLLER, std::cerr);
	
	logger->skipBackIfLoggedPositionEqualCurrent();
	if ( ret == true )  { std::clog << " Controller reseted - OK\n"; } 
	else 				{ std::cerr << " Controller reset failed\n"; return false; }
	
	// do this after the controller is reseted, 
	// because setZeroPos will determine a new controller position on demand
	setZeroPos();
	
	curCtlPos = requestControllerPos();
	postCtlPosition(PID_XYZ_POS_MAJOR);
	
	evaluateLimitState();
	switchToolOff(true);
	
	return true;
}
///////////////////////////////////////////////////////////////////
unsigned int CncControl::getDurationCount() {
///////////////////////////////////////////////////////////////////
	return THE_CONFIG->getDurationCount();
}
///////////////////////////////////////////////////////////////////
bool CncControl::hasNextDuration() {
///////////////////////////////////////////////////////////////////
	return ( durationCounter < getDurationCount() );
}
///////////////////////////////////////////////////////////////////
void CncControl::resetDurationCounter() {
///////////////////////////////////////////////////////////////////
	durationCounter = 0;
	THE_APP->GetPassingCount()->SetValue(wxString() << durationCounter);
}
///////////////////////////////////////////////////////////////////
void CncControl::initNextDuration() {
///////////////////////////////////////////////////////////////////
	durationCounter++;
	THE_APP->GetPassingCount()->SetValue(wxString() << durationCounter);
}
///////////////////////////////////////////////////////////////////
unsigned int CncControl::getDurationCounter() {
///////////////////////////////////////////////////////////////////
	return durationCounter;
}
///////////////////////////////////////////////////////////////////
bool CncControl::isFirstDuration() {
///////////////////////////////////////////////////////////////////
	return (durationCounter == 1);
}
///////////////////////////////////////////////////////////////////
bool CncControl::isLastDuration() {
///////////////////////////////////////////////////////////////////
	return (durationCounter == getDurationCount());
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveZToBottom() {
///////////////////////////////////////////////////////////////////
	double curZPos = curAppPos.getZ() * THE_CONFIG->getDisplayFactZ(); // we need it as mm
	double moveZ   = curZPos * (-1);
	
	bool ret = true;
	if ( prepareSimpleMove() == true ) {
		if ( moveRelMetricZ(moveZ) == false ) {
			std::cerr << "CncControl: Move Z to bottom error"<< std::endl;
			ret = false;
		}
	} else {
		ret = false;
	}
	reconfigureSimpleMove(false);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveZToTop() {
///////////////////////////////////////////////////////////////////
	double topZPos = THE_CONFIG->getCurZDistance(); //THE_CONFIG->getMaxZDistance();
	double curZPos = curAppPos.getZ() * THE_CONFIG->getDisplayFactZ(); // we need it as mm
	double moveZ   = topZPos - curZPos;
	
	bool ret = true;
	if ( prepareSimpleMove() == true ) {
		if ( moveRelMetricZ(moveZ) == false ) {
			std::cerr << "CncControl: Move Z to top error"<< std::endl;
			ret = false;
		}
	} else {
		ret = false;
	}
	
	reconfigureSimpleMove(false);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::changeCurrentFeedSpeedXYZ_MM_SEC(float value, CncSpeedMode s) {
///////////////////////////////////////////////////////////////////
	return changeCurrentFeedSpeedXYZ_MM_MIN(value * 60, s);
}
///////////////////////////////////////////////////////////////////
bool CncControl::changeCurrentFeedSpeedXYZ_MM_MIN(float value, CncSpeedMode s) {
///////////////////////////////////////////////////////////////////
	// always reset the realtime speed value, but don't
	// use MAX_FEED_SPEED_VALUE here because it is declared as MIN_LONG
	// realtimeFeedSpeed_MM_MIN = MAX_FEED_SPEED_VALUE;
	realtimeFeedSpeed_MM_MIN = THE_CONFIG->getMaxSpeedXYZ_MM_MIN();
	
	// safety checks 
	const double maxValue = THE_CONFIG->getMaxSpeedXYZ_MM_MIN();
	if ( value <= 0.0 )			value = 0.0;
	if ( value > maxValue )		value = maxValue;

	// avoid the setter below if nothing will change
	if ( configuredSpeedMode != s ) {
		configuredSpeedMode = s;
		
		displaySpeedMode(configuredSpeedMode);
		
		if ( getSerial()->knowsSpeedMode() == true ) {
			if ( processSetter(PID_SPEED_FEED_MODE, (int32_t)(configuredSpeedMode)) == false ) {
				std::cerr << "CncControl::changeCurrentFeedSpeedXYZ_MM_MIN(): processSetter(PID_SPEED_FEED_MODE) failed" << std::endl;
				return false;
			}
		}
	}
	
	// avoid the setter below if nothing will change
	if ( cnc::dblCompare(configuredFeedSpeed_MM_MIN, value) == false ) {
		configuredFeedSpeed_MM_MIN = value;
		
		displaySpeedValue(value);
		
		if ( processSetter(PID_SPEED_MM_MIN, (int32_t)(configuredFeedSpeed_MM_MIN * FLT_FACT)) == false ) {
			std::cerr << "CncControl::changeCurrentFeedSpeedXYZ_MM_MIN(): processSetter(PID_SPEED_MM_MIN) failed" << std::endl;
			return false;
		}
	}
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::changeSpeedToDefaultSpeed_MM_MIN(CncSpeedMode s) {
///////////////////////////////////////////////////////////////////
	double value = 0.0;
	
	switch( s ) {
		case CncSpeedWork: 			value = THE_CONFIG->getDefaultWorkSpeed_MM_MIN(); 	break;
		case CncSpeedRapid:			value = THE_CONFIG->getDefaultRapidSpeed_MM_MIN();	break;
		case CncSpeedMax:			value = THE_CONFIG->getMaxSpeedXYZ_MM_MIN();		break;
		case CncSpeedUserDefined:	return true;
	}
	
	return changeCurrentFeedSpeedXYZ_MM_MIN(value, s);
}
///////////////////////////////////////////////////////////////////
void CncControl::setDefaultRapidSpeed_MM_MIN(float s) { 
///////////////////////////////////////////////////////////////////
	if ( s <= 0.0)
		return;
		
	if ( s > THE_CONFIG->getMaxSpeedXYZ_MM_MIN() )
		return;
		
	defaultFeedSpeedRapid_MM_MIN = s; 
}
///////////////////////////////////////////////////////////////////
void CncControl::setDefaultWorkSpeed_MM_MIN(float s)  { 
///////////////////////////////////////////////////////////////////
	if ( s <= 0.0)
		return;
		
	if ( s > THE_CONFIG->getMaxSpeedXYZ_MM_MIN() )
		return;

	defaultFeedSpeedWork_MM_MIN  = s; 
}
///////////////////////////////////////////////////////////////////
bool CncControl::isPositionOutOfRange(const CncLongPosition& pos, bool trace) {
///////////////////////////////////////////////////////////////////
	if ( positionCheck == false )
		return false;
		
	// will only be done for emulation ports. It didn't makes sense for a cnc run
	// see below
	if ( serialPort->getPortType() != CncPORT ) {
		// in the real cnc life this is not good enough. the check has to be done by
		// the end switches at last.
		bool error = false;
		CncLongPosition::Watermarks wm;
		pos.getWatermarks(wm);
		
		if ( (wm.xMax - wm.xMin)/THE_CONFIG->getCalculationFactX() > THE_CONFIG->getMaxDimensionX() ) error = true;
		if ( (wm.yMax - wm.yMin)/THE_CONFIG->getCalculationFactY() > THE_CONFIG->getMaxDimensionY() ) error = true;
		if ( (wm.zMax - wm.zMin)/THE_CONFIG->getCalculationFactZ() > THE_CONFIG->getMaxDimensionZ() ) error = true;
	
		if ( error == true && trace == true ) {
			std::cerr << "Position out of range!" << std::endl;
			std::cerr << " Max valid X dimension: " << THE_CONFIG->getMaxDimensionX() << std::endl;
			std::cerr << " Max valid Y dimension: " << THE_CONFIG->getMaxDimensionY() << std::endl;
			std::cerr << " Max valid Z dimension: " << THE_CONFIG->getMaxDimensionZ() << std::endl;
			std::cerr << " Pos: " << pos << std::endl;
			std::cerr << " Min Watermark: " << wm.xMin << "," << wm.yMin << "," << wm.zMin << "," << std::endl;
			std::cerr << " Max Watermark: " << wm.xMax << "," << wm.yMax << "," << wm.zMax << "," << std::endl;
			std::cerr << " Calculated spread X :" <<  (wm.xMax - wm.xMin)/THE_CONFIG->getCalculationFactX() << std::endl;
			std::cerr << " Calculated spread Y :" <<  (wm.yMax - wm.yMin)/THE_CONFIG->getCalculationFactY() << std::endl;
			std::cerr << " Calculated spread Z :" <<  (wm.zMax - wm.zMin)/THE_CONFIG->getCalculationFactZ() << std::endl;
			
			return true;
		}
	}
	
	return false;
}
///////////////////////////////////////////////////////////////////
void CncControl::monitorPosition(const CncLongPosition& pos) {
///////////////////////////////////////////////////////////////////
	// motion monitor
	static CncLongPosition prevPos;
	
	if ( pos != prevPos ) {
		
		if ( THE_APP->getMotionMonitor() ) {
			THE_APP->getMotionMonitor()->appendVertex(getClientId(), getConfiguredSpeedMode(), pos);
			updatePreview3D(false);
		}
		
		prevPos.set(pos);
		
		if ( THE_CONFIG->getInterruptByPosOutOfRangeFlag() == true ) {
			if ( isPositionOutOfRange(pos, true) == true )
				interrupt("Position Out Of Range");
		} 
		else {
			if ( isPositionOutOfRange(pos, false) == true )
				positionOutOfRangeFlag = true;
		}
	}
}
///////////////////////////////////////////////////////////////////
bool CncControl::SerialMessageCallback(const ControllerMsgInfo& cmi) {
///////////////////////////////////////////////////////////////////
	wxDateTime now = wxDateTime::UNow();
	wxString msg(cmi.message.str().c_str());
	char type = (char)msg[0];
	msg = msg.SubString(1, msg.length());
	
	if ( type != 'D' )
		THE_APP->displayNotification(type, "Controller Callback", msg, (type == 'E' ? 8 : 4));
	
	switch ( type ) {
		
		case 'W':	
					cnc::msg.logWarning(now.Format("Warning Message received: %H:%M:%S.%l\n"));
					cnc::msg.logWarning(msg);
					cnc::cex1 << "Received the following CNC Controller Warning:\n" << msg;
					break;
					
		case 'E':	cnc::msg.logError(now.Format("Error Message received: %H:%M:%S.%l\n"));
					cnc::msg.logError(msg);
					std::cerr << "Received the following CNC Controller Error:\n" << msg;
					break;
					
		case 'D':	cnc::msg.logDebug(now.Format("Debug Message received: %H:%M:%S.%l\n"));
					cnc::msg.logDebug(msg);
					cnc::cex1 << "Serial Remote Debug: " << msg;
					break;

		default:	cnc::msg.logInfo(now.Format("Info Message received: %H:%M:%S.%l\n"));
					cnc::msg.logInfo(msg);
					std::cout << "Received the following CNC Controller Information:\n" << msg;
	}
	
	cnc::msg.setTextColour(wxColour(128, 128, 0));
	cnc::msg << "-------------------------------------------------------------------------------------------\n";
	cnc::msg.resetTextAttr();

	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::SerialControllerCallback(const ContollerInfo& ci) {
///////////////////////////////////////////////////////////////////
	// Event handling, enables the interrrpt functionality
	if ( THE_CONTEXT->isAllowEventHandling() )
		THE_APP->dispatchAll();
		
	if ( isInterrupted() )
		return false;
	
	switch ( ci.infoType ) {
		// --------------------------------------------------------
		case CITHeartbeat:
		{
			std::stringstream ss;
			ss << "Heartbeat received - Value: " << ci.heartbeatValue;
			
			if ( THE_APP->GetHeartbeatState() ) {
				static bool flag = false;
				if ( flag )	{ flag = false; THE_APP->GetHeartbeatState()->SetBitmap(ImageLibHeartbeat().Bitmap("BMP_HEART")); }
				else		{ flag = true;  THE_APP->GetHeartbeatState()->SetBitmap(ImageLibHeartbeat().Bitmap("BMP_HEART_PLUS")); }
				
				THE_APP->GetHeartbeatState()->GetParent()->Refresh();
				THE_APP->GetHeartbeatState()->GetParent()->Update();
			}
			
			lastCncHeartbeatValue = ci.heartbeatValue;
			
			if ( ci.limitState == true ) {
				CncInterface::ILS::States ls(ci.limitStateValue);
				displayLimitStates(ls);
				ss << " : " << ls.getValueAsString();
				ss << " (" << (int)ci.limitStateValue << ")";
			}
			
			if ( ci.supportState == true ) {
				CncInterface::ISP::States sp(ci.supportStateValue);
				displaySupportStates(sp);
				ss << " : " << sp.getValueAsString();
				ss << " (" << (int)ci.supportStateValue << ")";
			} else {
				displayUnknownSupportStates();
			}
			
			//cnc::trc.logInfoMessage(ss);
			break;
		}
		
		// --------------------------------------------------------
		case CITLimit:
		{
			//std::clog << "::L: " << ci.xLimit << ", " << ci.yLimit << ", " << ci.zLimit << std::endl;
			displayLimitStates(ci.xLimit, ci.yLimit, ci.zLimit);
			break;
		}
		
		// --------------------------------------------------------
		case CITPosition:
		{
			// update controller position
			switch ( ci.posType ) {
				case PID_X_POS: 		curCtlPos.setX(ci.xCtrlPos); 
										if ( ci.synchronizeAppPos == true ) 
											curAppPos.setX(ci.xCtrlPos);
										break;
										
				case PID_Y_POS: 		curCtlPos.setY(ci.yCtrlPos); 
										if ( ci.synchronizeAppPos == true ) 
											curAppPos.setY(ci.yCtrlPos);
										break;
				
				case PID_Z_POS: 		curCtlPos.setZ(ci.zCtrlPos); 
										if ( ci.synchronizeAppPos == true ) 
											curAppPos.setZ(ci.zCtrlPos);
										break;
				
				case PID_XYZ_POS:
				case PID_XYZ_POS_MAJOR:
				case PID_XYZ_POS_DETAIL:
				default:				curCtlPos.setXYZ(ci.xCtrlPos, ci.yCtrlPos, ci.zCtrlPos);
										realtimeFeedSpeed_MM_MIN = ci.feedSpeed;
										
										if ( false )
											std::cout << "CITPosition: speed: " << realtimeFeedSpeed_MM_MIN << std::endl;

										if ( ci.synchronizeAppPos == true )
											curAppPos.setXYZ(ci.xCtrlPos, ci.yCtrlPos, ci.zCtrlPos);
			}
			
			// display controller coordinates
			postCtlPosition(ci.posType);
			
			// motion monitor
			monitorPosition(curCtlPos);
			
			break;
		}
		
		// --------------------------------------------------------
		default:
			std::cerr << "CncControl::SerialControllerCallback:" << std::endl;
			std::cerr << " No handler defined for controller info type:" << ci.infoType << std::endl;
	}

	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::SerialExecuteControllerCallback(const ContollerExecuteInfo& cei) {
///////////////////////////////////////////////////////////////////
	
	auto checkSetterCount = [](unsigned char pid, size_t count, size_t ref) {
		bool ret = (count == ref);
		
		if ( ret == false ) {
			std::cerr << "CncControl::SerialExecuteControllerCallback(): Invalid Setter(" << ArduinoPIDs::getPIDLabel(pid) << ") value count: " 
					  << "Given: " << count << ", Reference: " << ref
					  << std::endl;
		}
		
		return ret;
	};
	
	switch ( cei.infoType ) {
		// --------------------------------------------------------
		case CEITSetter:
		{
			size_t size = cei.setterValueList.size();
			
			switch ( cei.setterPid ) {
				case PID_TOOL_SWITCH:		if ( checkSetterCount(cei.setterPid, size, 1) == false )
												return false;
												
											toolPowerState = (bool)cei.setterValueList.front();
											displayToolState(toolPowerState);
											break;
											
				case PID_SPEED_FEED_MODE:	if ( checkSetterCount(cei.setterPid, size, 1) == false )
												return false;
											
											configuredSpeedMode = (CncSpeedMode)(cei.setterValueList.front());
											displaySpeedMode(configuredSpeedMode);
											break;
											
				case PID_SPEED_MM_MIN:		if ( checkSetterCount(cei.setterPid, size, 1) == false )
												return false;
												
											displaySpeedValue(cei.setterValueList.front() / FLT_FACT);
											break;
											
				case PID_ENABLE_STEPPERS:	// nothing to do here
											break;
											
				default:					std::cerr << "CncControl::SerialExecuteControllerCallback(): Not registered Setter PID: " 
				                                      << ArduinoPIDs::getPIDLabel(cei.setterPid) 
													  << std::endl;
			}
			
			break;
		}
		
		// --------------------------------------------------------
		default:
			std::cerr << "CncControl::SerialExecuteControllerCallback:"  << std::endl;
			std::cerr << " No handler defined for controller info type:" << cei.infoType << std::endl;
	}
	return true;
}
///////////////////////////////////////////////////////////////////
void CncControl::waitActive(unsigned int millis) {
///////////////////////////////////////////////////////////////////
	THE_APP->waitActive(millis, true);
}
///////////////////////////////////////////////////////////////////
bool CncControl::SerialCallback() {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() ) {
		std::cerr << "SerialCallback: Interrupt detected"<< std::endl;
		return false;
	}

	// Event handling, enables the interrupt functionallity
	static CncNanoTimestamp tsLastDispatch = 0;
	static const CncNanoTimespan threshold = std::nano::den / 4; // 250 ms
	if ( THE_CONTEXT->isAllowEventHandling() ) {
		const CncNanoTimespan span = CncTimeFunctions::getTimeSpan(CncTimeFunctions::getNanoTimestamp(), tsLastDispatch);

		if ( span > threshold ) {
			THE_APP->dispatchAll();
			tsLastDispatch = CncTimeFunctions::getNanoTimestamp();
		}
	}
	
	// display application coordinates
	postAppPosition(PID_XYZ_POS_MAJOR);
	
	if ( CncAsyncKeyboardState::isEscapePressed() != 0 ) {
		if ( THE_CONFIG->getTheApp()->GetBtnEmergenyStop()->IsEnabled() == true ) {
			std::cerr << "SerialCallback: ESCAPE key detected" << std::endl;
			interrupt("ESCAPE detected");
		}
	}
	
	return !isInterrupted();
}
///////////////////////////////////////////////////////////////////
void CncControl::postAppPosition(unsigned char pid) {
///////////////////////////////////////////////////////////////////
	static CncLongPosition lastAppPos;
	
	if ( THE_CONTEXT->isOnlineUpdateCoordinates() ) {
		// application position
		typedef UpdateManagerThread::Event Event;
		static Event evt;
		
		// app positions are always from type major
		// so || pid == PID_XYZ_POS_MAJOR isn't necessary
		// the compairison below is necessary, because this method is also called
		// from the serialCallback(...) which not only detects pos changes
		if ( lastAppPos != curAppPos ) {
			THE_APP->umPostEvent(evt.AppPosEvent(pid, 
												 getClientId(), 
												 configuredSpeedMode, 
												 getConfiguredFeedSpeed_MM_MIN(), 
												 getRealtimeFeedSpeed_MM_MIN(), 
												 curAppPos
												)
											   );
		}
	}
	
	lastAppPos.set(curAppPos);
}
///////////////////////////////////////////////////////////////////
void CncControl::postCtlPosition(unsigned char pid) {
///////////////////////////////////////////////////////////////////
	if ( THE_CONTEXT->isOnlineUpdateCoordinates() ) {
		// application position
		typedef UpdateManagerThread::Event Event;
		static Event evt;
		
		// a position compairsion isn't necessay here because the serialControllerCallback(...)
		// call this method only on position changes
		THE_APP->umPostEvent(evt.CtlPosEvent(pid, 
											 getClientId(), 
											 configuredSpeedMode, 
											 getConfiguredFeedSpeed_MM_MIN(), 
											 getRealtimeFeedSpeed_MM_MIN(), 
											 curCtlPos
											)
										   );
	}
}
///////////////////////////////////////////////////////////////////
bool CncControl::simpleMoveXYToZeroPos(CncDimensions dim) {
///////////////////////////////////////////////////////////////////
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveXYToZeroPos(dim);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::simpleMoveXYZToZeroPos(CncDimensions dim) {
///////////////////////////////////////////////////////////////////
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveXYZToZeroPos(dim);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::simpleMoveZToZeroPos() {
///////////////////////////////////////////////////////////////////
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveZToZeroPos();
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveXYToZeroPos(CncDimensions dim) {
///////////////////////////////////////////////////////////////////
	bool ret = true;
	if ( curAppPos != zeroAppPos ) {
		int32_t moveX=0, moveY=0;
		moveX = zeroAppPos.getX() - curAppPos.getX(); 
		moveY = zeroAppPos.getY() - curAppPos.getY();
		
		if ( dim == CncDimension2D ) {
			ret = moveRelLinearStepsXY(moveX, moveY, false);
			
		} else {
			if ( moveRelLinearStepsXY(moveX, 0, false) == false ) 
				return false;
				
			if ( moveRelLinearStepsXY(0, moveY, false) == false )
				return false;
		}
	}
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveXYZToZeroPos(CncDimensions dim) {
///////////////////////////////////////////////////////////////////
	bool ret = true;
	if ( curAppPos != zeroAppPos ) {
		int32_t moveX=0, moveY=0, moveZ=0;
		moveX = zeroAppPos.getX() - curAppPos.getX(); 
		moveY = zeroAppPos.getY() - curAppPos.getY();
		moveZ = zeroAppPos.getZ() - curAppPos.getZ();
		
		if ( dim == CncDimension3D ) {
			ret = moveRelLinearStepsXYZ(moveX, moveY, moveZ, false);
			
		} else if ( dim == CncDimension2D ) {
			if ( moveRelLinearStepsXYZ(0, 0, moveZ, false) == false ) 
				return false;
				
			if ( moveRelLinearStepsXYZ(moveX, moveY, 0, false) == false ) 
				return false;
			
		} else {
			if ( moveRelLinearStepsXYZ(moveX, 0, 0, false) == false ) 
				return false;
				
			if ( moveRelLinearStepsXYZ(0, moveY, 0, false) == false ) 
				return false;
				
			if ( moveRelLinearStepsXYZ(0, 0, moveZ, false) == false ) 
				return false;
		}
	}
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveZToZeroPos() {
///////////////////////////////////////////////////////////////////
	bool ret = true;
	if ( curAppPos != zeroAppPos ) {
		int32_t moveZ=0;
		moveZ = zeroAppPos.getZ() - curAppPos.getZ();
		ret = moveRelLinearStepsXYZ(0, 0, moveZ, false);
	}
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveToStartPos() {
///////////////////////////////////////////////////////////////////
	bool ret = true;
	if ( curAppPos != startAppPos ) {
		int32_t moveX=0, moveY=0;
		moveX = startAppPos.getX() - curAppPos.getX(); 
		moveY = startAppPos.getY() - curAppPos.getY();
		moveRelLinearStepsXY(moveX, moveY, false);
	}
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveRelStepsZ(int32_t z) {
///////////////////////////////////////////////////////////////////
	if ( z == 0 )
		return true;
	// z moves are always linear, as a consequence alreadyRendered can be true
	// but to see the detail positions use false
	return serialPort->processMoveZ(z, false);
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveRelLinearStepsXY(int32_t x1, int32_t y1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	//avoid empty steps
	if ( x1 == 0 && y1 == 0 )
		return true;
	
	return serialPort->processMoveXY(x1, y1, alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveRelLinearStepsXYZ(int32_t x1, int32_t y1, int32_t z1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	//avoid empty steps
	if ( x1 == 0 && y1 == 0 && z1 == 0 )
		return true;
	
	return serialPort->processMoveXYZ(x1, y1, z1, alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveRelMetricZ(double z) {
///////////////////////////////////////////////////////////////////
	const double sZ = z * THE_CONFIG->getCalculationFactZ();
	
	return moveRelStepsZ((int32_t)round(sZ));
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveRelLinearMetricXY(double x1, double y1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	const double sX1 = x1 * THE_CONFIG->getCalculationFactX();
	const double sY1 = y1 * THE_CONFIG->getCalculationFactY();
	
	return moveRelLinearStepsXY((int32_t)round(sX1), 
	                            (int32_t)round(sY1),
	                            alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveRelLinearMetricXYZ(double x1, double y1, double z1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	const double sX1 = x1 * THE_CONFIG->getCalculationFactX();
	const double sY1 = y1 * THE_CONFIG->getCalculationFactY();
	const double sZ1 = z1 * THE_CONFIG->getCalculationFactZ();
	
	return moveRelLinearStepsXYZ((int32_t)round(sX1), 
	                             (int32_t)round(sY1),
	                             (int32_t)round(sZ1),
	                             alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveAbsMetricZ(double z) {
///////////////////////////////////////////////////////////////////
	const double sZ = z * THE_CONFIG->getCalculationFactZ();
	
	return moveRelStepsZ( (int32_t)round(sZ) - curAppPos.getZ() );
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveAbsLinearMetricXY(double x1, double y1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	const double sX1 = x1 * THE_CONFIG->getCalculationFactX();
	const double sY1 = y1 * THE_CONFIG->getCalculationFactY();
	
	return moveRelLinearStepsXY((int32_t)round(sX1) - curAppPos.getX(), 
	                            (int32_t)round(sY1) - curAppPos.getY(),
	                            alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveAbsLinearMetricXYZ(double x1, double y1, double z1, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	const double sX1 = x1 * THE_CONFIG->getCalculationFactX();
	const double sY1 = y1 * THE_CONFIG->getCalculationFactY();
	const double sZ1 = z1 * THE_CONFIG->getCalculationFactZ();
	
	return moveRelLinearStepsXYZ((int32_t)round(sX1) - curAppPos.getX(),
	                             (int32_t)round(sY1) - curAppPos.getY(),
	                             (int32_t)round(sZ1) - curAppPos.getZ(),
	                             alreadyRendered);
}
///////////////////////////////////////////////////////////////////
void CncControl::setToolState(bool defaultStyle) {
///////////////////////////////////////////////////////////////////
	if ( defaultStyle == true ) {
		toolState.setState(CncToolStateControl::red);
	} else {
		if ( toolPowerState == TOOL_STATE_ON ) 	toolState.setState(CncToolStateControl::green);
		else 									toolState.setState(CncToolStateControl::red);
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::switchToolOn() {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() )
		return;

	if ( toolPowerState == TOOL_STATE_OFF ) { 
		if ( processSetter(PID_TOOL_SWITCH, TOOL_STATE_ON) ) {
			toolPowerState = TOOL_STATE_ON;
			displayToolState(toolPowerState);
		}
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::switchToolOff(bool force) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() )
		return;

	if ( toolPowerState == TOOL_STATE_ON || force == true ) {
		if ( processSetter(PID_TOOL_SWITCH, TOOL_STATE_OFF) ) {
			toolPowerState = TOOL_STATE_OFF;
			displayToolState(toolPowerState);
		}
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::displayToolState(const bool state) {
///////////////////////////////////////////////////////////////////
	THE_APP->decorateSwitchToolOnOff(state);
	setToolState();
}
///////////////////////////////////////////////////////////////////
void CncControl::displaySpeedValue(const double value) {
///////////////////////////////////////////////////////////////////
	THE_APP->GetConfiguredFeedSpeed()->ChangeValue(wxString::Format("%3.1lf", value));
}
///////////////////////////////////////////////////////////////////
void CncControl::displaySpeedMode(const CncSpeedMode mode) {
///////////////////////////////////////////////////////////////////
	THE_APP->GetConfiguredFeedSpeedMode()->ChangeValue(wxString::Format("%c", cnc::getCncSpeedTypeAsCharacter(mode)));
}
///////////////////////////////////////////////////////////////////
bool CncControl::displayGetterList(const PidList& pidList) {
///////////////////////////////////////////////////////////////////
	if ( pidList.size() == 0 )
		return false;

	GetterListValues map;
	if ( getSerial()->processGetterList(pidList, map) == false ) {
		std::cerr << "Error while processing getter list" << std::endl;
		return false;
	}
	
	// show content:
	std::cout << "Getter List Report:" << std::endl;
	for (GetterListValues::iterator itl=map.begin(); itl!=map.end(); ++itl) {
		std::cout << ' ' << ArduinoPIDs::getPIDLabel(itl->first) << " => [";
			
		GetterValues& list = itl->second;
		for (GetterValues::iterator itp = list.begin() ; itp != list.end(); ++itp) {

			if ( itp != list.begin() )	
				std::cout << ',';

			std::cout << *itp;
		}			
		
		std::cout << "]\n";
	}

	return true;
}
///////////////////////////////////////////////////////////////////
const CncLongPosition CncControl::requestControllerPos() {
///////////////////////////////////////////////////////////////////
	static CncLongPosition controllerPos;
	std::vector<int32_t> list;
	
	if ( isConnected() == true && isInterrupted() == false )
		getSerial()->processGetter(PID_XYZ_POS, list);
		
	if ( list.size() != 3 ){
		controllerPos.setX(0);
		controllerPos.setY(0);
		controllerPos.setZ(0);
		
		if ( isConnected() == true && isInterrupted() == false ) {
			std::cerr << "CncControl::getControllerPos: Unable to evaluate controllers position:" << std::endl;
			std::cerr << " Received value count: " << list.size() << ", expected: 3" << std::endl;
		}
	} else {
		controllerPos.setX(list.at(0));
		controllerPos.setY(list.at(1));
		controllerPos.setZ(list.at(2));
	}
	
	return controllerPos;
}
///////////////////////////////////////////////////////////////////
const CncLongPosition CncControl::requestControllerLimitState() {
///////////////////////////////////////////////////////////////////
	static CncLongPosition limitStates;
	GetterValues list;
	
	if ( isInterrupted() == false )
		getSerial()->processGetter(PID_LIMIT, list);
	
	if ( list.size() != 3 ){
		if ( isInterrupted() == false ) {
			std::cerr << "CncControl::getControllerLimitState: Unable to evaluate controllers limit state:" << std::endl;
			std::cerr << " Received value count: " << list.size() << ", expected: 3" << std::endl;
		}
	} else {
		limitStates.setXYZ(list.at(0), list.at(1), list.at(2));
		return limitStates;
	}
	
	limitStates.setXYZ(0, 0, 0);
	return limitStates;
}
///////////////////////////////////////////////////////////////////
bool CncControl::validateAppAgainstCtlPosition() {
///////////////////////////////////////////////////////////////////
	CncLongPosition ctlPos = requestControllerPos();
	return ( curAppPos == ctlPos );
}
///////////////////////////////////////////////////////////////////
void CncControl::updateCncConfigTrace() {
///////////////////////////////////////////////////////////////////
	typedef UpdateManagerThread::Event Event;
	static Event evt;
	
	THE_APP->umPostEvent(evt.ConfigUpdateEvent());
}
///////////////////////////////////////////////////////////////////
bool CncControl::enableStepperMotors(bool s) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() )
		return false;
		
	if ( isConnected() == false )
		return false;
	
	bool ret = processSetter(PID_ENABLE_STEPPERS, (int32_t)s);
	if ( ret == false ) {
		
		std::cerr << "CncControl::enableStepperMotors" << std::endl;
		std::cerr << " Error while enabling stepper motors. State=" << s << std::endl;
		THE_APP->GetMiMotorEnableState()->Check(false);
		return false;
	}
	
	std::vector<int32_t> list;
	getSerial()->processGetter(PID_ENABLE_STEPPERS, list);
		
	if ( list.size() != 1 ) {
		std::cerr << "CncControl::enableStepperMotors" << std::endl;
		std::cerr << " Get Enanble State: Unable to read the serial port:" << std::endl;
		THE_APP->GetMiMotorEnableState()->Check(false);
		return false;
	}
		
	THE_APP->GetMiMotorEnableState()->Check( list.at(0) == (int32_t)ENABLE_STATE_ON );
		
	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::enableProbeMode(bool s) {
///////////////////////////////////////////////////////////////////
	if ( isInterrupted() )
		return false;
		
	if ( isConnected() == false )
		return false;
	
	bool ret = processSetter(PID_PROBE_MODE, (int32_t)s);
	if ( ret == false ) {
		std::cerr << "CncControl::enableProbeMode" << std::endl;
		std::cerr << " Error while enabling probe mode. State=" << s << std::endl;
		return false;
	}
	
	THE_APP->GetBtProbeMode()->SetValue(s);
	
	return true;
}
///////////////////////////////////////////////////////////////////
wxString& CncControl::getLimitInfoString(wxString& ret) {
///////////////////////////////////////////////////////////////////
	return limitStates.getLimitInfoString(ret);
}
///////////////////////////////////////////////////////////////////
void CncControl::evaluateLimitState() {
///////////////////////////////////////////////////////////////////
	CncLongPosition ls = requestControllerLimitState();
	CncInterface::ILS::States states(ls.getX(), ls.getY(), ls.getZ());
	displayLimitStates(states);
}
///////////////////////////////////////////////////////////////////
void CncControl::displayLimitStates(const int32_t x, const int32_t y, const int32_t z) {
///////////////////////////////////////////////////////////////////
	CncInterface::ILS::States ls(x, y, z);
	displayLimitStates(ls);
}
///////////////////////////////////////////////////////////////////
void CncControl::displayLimitStates(const CncInterface::ILS::States& ls) {
///////////////////////////////////////////////////////////////////
	limitStates.setXMinLimit(ls.xMin());
	limitStates.setXMaxLimit(ls.xMax());
	limitStates.setYMinLimit(ls.yMin());
	limitStates.setYMaxLimit(ls.yMax());
	limitStates.setZMinLimit(ls.zMin());
	limitStates.setZMaxLimit(ls.zMax());
	
	displayLimitState(THE_APP->GetXMinLimit(), limitStates.getXMinLimit());
	displayLimitState(THE_APP->GetXMaxLimit(), limitStates.getXMaxLimit());
	displayLimitState(THE_APP->GetYMinLimit(), limitStates.getYMinLimit());
	displayLimitState(THE_APP->GetYMaxLimit(), limitStates.getYMaxLimit());
	displayLimitState(THE_APP->GetZMinLimit(), limitStates.getZMinLimit());
	displayLimitState(THE_APP->GetZMaxLimit(), limitStates.getZMaxLimit());
	
	limitStates.displayLimitState();
}
///////////////////////////////////////////////////////////////////
void CncControl::displayLimitState(wxWindow* ctl, bool value) {
///////////////////////////////////////////////////////////////////
	if ( ctl != NULL ) {
		if ( value == true ) {
			ctl->SetBackgroundColour(wxColour(255,128,128));
			ctl->SetForegroundColour(*wxWHITE);
			
		} else {
			ctl->SetBackgroundColour(wxColour(181,230,29));
			ctl->SetForegroundColour(*wxBLACK);

		}
		ctl->Refresh();
		ctl->Update();
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::displaySupportStates(const CncInterface::ISP::States& sp) {
///////////////////////////////////////////////////////////////////
	if ( THE_APP != NULL ) {
		THE_APP->GetSupportButton1State()->SetBackgroundColour(sp.isSupportButton1Pressed() ? wxColour(181,230,29) : wxColour(255,128,128));
		THE_APP->GetSupportButton2State()->SetBackgroundColour(sp.isSupportButton2Pressed() ? wxColour(181,230,29) : wxColour(255,128,128));
		THE_APP->GetSupportButton3State()->SetBackgroundColour(sp.isSupportButton3Pressed() ? wxColour(181,230,29) : wxColour(255,128,128));
		
		THE_APP->GetCableConnectedState()->SetBackgroundColour(sp.isCableConnected() ? wxColour(217,217,0) : wxColour(255,128,128));
		THE_APP->GetToolPowerObserverState()->SetBackgroundColour(sp.isToolPowered() ? wxColour(217,217,0) : wxColour(255,128,128));
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::displayUnknownSupportStates() {
///////////////////////////////////////////////////////////////////
	if ( THE_APP != NULL ) {
		THE_APP->GetSupportButton1State()->SetBackgroundColour(wxColour(128,128,128));
		THE_APP->GetSupportButton2State()->SetBackgroundColour(wxColour(128,128,128));
		THE_APP->GetSupportButton3State()->SetBackgroundColour(wxColour(128,128,128));
		
		THE_APP->GetCableConnectedState()->SetBackgroundColour(wxColour(128,128,128));
		THE_APP->GetToolPowerObserverState()->SetBackgroundColour(wxColour(128,128,128));
	}
}
///////////////////////////////////////////////////////////////////
bool CncControl::meassureDimension(const char axis, wxCheckBox* min, wxCheckBox* max, double& result) {
///////////////////////////////////////////////////////////////////
	double maxSteps 	= (axis != 'Z' ? -450.0 : -100.0); // mm
	bool ret 			= false;
	long minPos 		= 0l;
	long maxPos 		= 0l;
	result 				= -DBL_MAX; // invalid Values
	
	if ( min ) min->SetValue(false);
	if ( max ) max->SetValue(false);
	
	if ( prepareSimpleMove() == true ) {
		
		// move to min position
		switch ( axis ) {
			case 'X': 	ret = moveRelLinearMetricXY(maxSteps, 0.0, true);
						if ( ret ) 
							minPos = curCtlPos.getX();
						break;
						
			case 'Y': 	ret = moveRelLinearMetricXY(0.0, maxSteps, true);
						if ( ret ) 
							minPos = curCtlPos.getY();
						break;
						
			case 'Z': 	ret = moveRelMetricZ(maxSteps);
						if ( ret ) 
							minPos = curCtlPos.getZ();
						break;
		}
		
		// move to max position
		if ( ret ) {
			if ( min ) min->SetValue(true);
			maxSteps *= -1;
			
			switch ( axis ) {
				case 'X': 	ret = moveRelLinearMetricXY(maxSteps, 0.0, true);
							if ( ret ) 
								maxPos = curCtlPos.getX();
							break;
							
				case 'Y': 	ret = moveRelLinearMetricXY(0.0, maxSteps, true);
							if ( ret ) 
								maxPos = curCtlPos.getY();
							break;
							
				case 'Z': 	ret = moveRelMetricZ(maxSteps);
							if ( ret ) 
								maxPos = curCtlPos.getZ();
							break;
			}
		}
		
		// free end position
		if ( ret ) {
			if ( max ) max->SetValue(true);
			
			switch ( axis ) {
				case 'X': 	ret = moveRelLinearMetricXY(-endSwitchStepBackMertic, 0.0, true);
							break;
							
				case 'Y': 	ret = moveRelLinearMetricXY(0.0, -endSwitchStepBackMertic, true);
							break;
							
				case 'Z': 	ret = moveRelMetricZ(-endSwitchStepBackMertic);
							break;
			}
		}
		
		// calculate result
		if ( ret ) {

			switch ( axis ) {
				case 'X': 	result = (maxPos - minPos) * THE_CONFIG->getDisplayFactX();
							break;
							
				case 'Y': 	result = (maxPos - minPos) * THE_CONFIG->getDisplayFactY();
							break;
							
				case 'Z': 	result = (maxPos - minPos) * THE_CONFIG->getDisplayFactZ();
							break;
			}
		}
	}
	
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveXToMinLimit() {
// This function move the max distiance and will be latest stopped by the end switch
// However, the PC and controller postions are not equal at the end!
// the call of reconfigureSimpleMove(true) will correct that
///////////////////////////////////////////////////////////////////
	const double distance = -THE_CONFIG->getMaxDimensionX() - getCurCtlPosMetricX();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXY(distance, 0.0, true);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelLinearMetricXY(+endSwitchStepBackMertic, 0.0, true);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveXToMaxLimit() {
// This function move the max distiance and will be latest stopped by the end switch
// However, the PC and controller postions are not equal at the end!
// the call of reconfigureSimpleMove(true) will correct that
///////////////////////////////////////////////////////////////////
	const double distance = +THE_CONFIG->getMaxDimensionX() - getCurCtlPosMetricX();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXY(distance, 0.0, true);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelLinearMetricXY(-endSwitchStepBackMertic, 0.0, true);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveYToMinLimit() {
// This function move the max distiance and will be latest stopped by the end switch
// However, the PC and controller postions are not equal at the end!
// the call of reconfigureSimpleMove(true) will correct that
///////////////////////////////////////////////////////////////////
	const double distance = -THE_CONFIG->getMaxDimensionY() - getCurCtlPosMetricY();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXY(0.0, distance, true);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelLinearMetricXY(0.0, +endSwitchStepBackMertic, true);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveYToMaxLimit() {
// This function move the max distiance and will be latest stopped by the end switch
// However, the PC and controller postions are not equal at the end!
// the call of reconfigureSimpleMove(true) will correct that
///////////////////////////////////////////////////////////////////
	const double distance = +THE_CONFIG->getMaxDimensionY() - getCurCtlPosMetricY();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXY(0.0, distance, true);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelLinearMetricXY(0.0, -endSwitchStepBackMertic, true);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveZToMinLimit() {
// This function move the max distiance and will be latest stopped by the end switch
// However, the PC and controller postions are not equal at the end!
// the call of reconfigureSimpleMove(true) will correct that
///////////////////////////////////////////////////////////////////
	const double distance = -THE_CONFIG->getMaxDimensionZ() - getCurCtlPosMetricZ();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelMetricZ(distance);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelMetricZ(+endSwitchStepBackMertic);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveZToMaxLimit() {
// This function move the max distiance and will be latest stopped by the end switch
// However, the PC and controller postions are not equal at the end!
// the call of reconfigureSimpleMove(true) will correct that
///////////////////////////////////////////////////////////////////
	const double distance = +THE_CONFIG->getMaxDimensionZ() - getCurCtlPosMetricZ();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelMetricZ(distance);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelMetricZ(-endSwitchStepBackMertic);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveXToMid() {
///////////////////////////////////////////////////////////////////
	const double distance = 5.0 + THE_CONFIG->getMaxDimensionX() - getCurCtlPosMetricX();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXY(distance, 0.0, true);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelLinearMetricXY(-THE_CONFIG->getMaxDimensionX() / 2, 0.0, true);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveYToMid() {
///////////////////////////////////////////////////////////////////
	const double distance = 5.0 + THE_CONFIG->getMaxDimensionY() - getCurCtlPosMetricY();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXY(0.0, distance, true);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelLinearMetricXY(0.0, -THE_CONFIG->getMaxDimensionY() / 2, true);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::moveZToMid() {
///////////////////////////////////////////////////////////////////
	const double distance = 5.0 + THE_CONFIG->getMaxDimensionZ() - getCurCtlPosMetricZ();
	
	bool ret = false;
	if ( prepareSimpleMove() == true ) {
		ret = moveRelMetricZ(distance);
		if ( ret == false && limitStates.hasLimit() )
			ret = moveRelMetricZ(-THE_CONFIG->getMaxDimensionZ() / 2);
	}
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualMoveFinest(StepSensitivity s,  const CncLinearDirection x, const CncLinearDirection y, const CncLinearDirection z, bool correctLimit) {
///////////////////////////////////////////////////////////////////
	CNC_PRINT_LOCATION;
	
	bool ret = false;
	
	const double xDim = 0.1 * x; // ~ 0.1 mm
	const double yDim = 0.1 * y; // ~ 0.1 mm
	const double zDim = 0.1 * z; // ~ 0.1 mm
	
	//TODO fix 10.0
	if ( changeCurrentFeedSpeedXYZ_MM_SEC(10.0) == false )
		return false;
	
	if ( prepareSimpleMove() == true ) {
		ret = moveRelLinearMetricXYZ(xDim, yDim, zDim, false);
		//WAIT
		if ( ret == false && limitStates.hasLimit() && correctLimit )
			ret = correctLimitPositions();
	}
	
	reconfigureSimpleMove(ret);
	return ret;
}
///////////////////////////////////////////////////////////////////
void CncControl::manualContinuousMoveStop() {
///////////////////////////////////////////////////////////////////
	if ( runContinuousMove == true ) {
		CNC_PRINT_LOCATION;
		
		if ( getSerial()->sendSignal(SIG_QUIT_MOVE) == false ) {
			std::cerr << "CncControl::manualContinuousMoveStop(): sendSignal(SIG_QUIT_MOVE) failed" << std::endl;
		}
	}
	
	runContinuousMove = false;
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualContinuousMoveStart(StepSensitivity s, const CncLinearDirection x, const CncLinearDirection y, const CncLinearDirection z, bool correctLimit) {
///////////////////////////////////////////////////////////////////
	const double SSF = (double)STEP_SENSITIVITY_FACTOR;
	
	// redirect on demand
	if ( s == FINEST ) 
		return manualMoveFinest(s, x, y, z, correctLimit);
	
	// Setup
	#warning
	const double xDim = 1000 * x;//s/SSF * x;
	const double yDim = 1000 * y;//s/SSF * y;
	const double zDim = 1000 * z;//s/SSF * z;
	
	bool ret = manualContinuousMoveStart_CtrlBased(xDim, yDim, zDim, correctLimit);
	
	// adjust the pc position
	if ( ret == true )
		curAppPos = curCtlPos;
		
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualContinuousMoveStart_CtrlBased(const double xDim, const double yDim, const double zDim, bool correctLimit) {
///////////////////////////////////////////////////////////////////
	if ( getSerial()->isCommandActive() == true )
		return false;
		
	// Always disable probe mode here, otherwise very long move distances appear
	const bool probeModeBefore = THE_CONTEXT->isProbeMode();
	if ( enableProbeMode(false) == false ) {
		std::cerr << " Can't disable probe mode. Manual continous move aborted" << std::endl;
		return false;
	}
	THE_CONTEXT->setProbeMode(false);
	
	// Move preparation
	initNextDuration();
	THE_CONTEXT->setAllowEventHandling(true);
	activatePositionCheck(false);
	enableStepperMotors(ENABLE_STATE_ON);
	
	double sX = xDim * THE_CONFIG->getCalculationFactX();
	double sY = yDim * THE_CONFIG->getCalculationFactY();
	double sZ = zDim * THE_CONFIG->getCalculationFactZ();
	
	int32_t values[3];
	values[0] = (int32_t)round(sX);
	values[1] = (int32_t)round(sY);
	values[2] = (int32_t)round(sZ);
	
	// move loop
	runContinuousMove = true;
	bool ret = getSerial()->processMoveUntilSignal(sizeof(values)/sizeof(int32_t), values);
	if ( ret == false ) {
		
		if ( limitStates.hasLimit() && correctLimit)
			ret = correctLimitPositions();
	}
	
	// Move touch up
	enableStepperMotors(ENABLE_STATE_OFF);
	activatePositionCheck(true);
	resetDurationCounter();
	
	// reactivate configured probe mode state
	if ( (ret = enableProbeMode(probeModeBefore)) == false ) {
		std::cerr << " Can't reactivate probe mode." << std::endl;
	}
	THE_CONTEXT->setProbeMode(probeModeBefore);
	
	return true;
}
///////////////////////////////////////////////////////////////////
bool CncControl::correctLimitPositions() {
///////////////////////////////////////////////////////////////////
	bool ret = true;
	
	if ( limitStates.hasLimit() ) {
		
		double xsbm = 0.0, ysbm = 0.0, zsbm  = 0.0;
		if (  limitStates.getXMinLimit() && !limitStates.getXMaxLimit() )	xsbm = +endSwitchStepBackMertic;
		if ( !limitStates.getXMinLimit() &&  limitStates.getXMaxLimit() )	xsbm = -endSwitchStepBackMertic;
		
		if (  limitStates.getYMinLimit() && !limitStates.getYMaxLimit() )	ysbm = +endSwitchStepBackMertic;
		if ( !limitStates.getYMinLimit() &&  limitStates.getYMaxLimit() )	ysbm = -endSwitchStepBackMertic;
		
		if (  limitStates.getZMinLimit() && !limitStates.getZMaxLimit() )	zsbm = +endSwitchStepBackMertic;
		if ( !limitStates.getZMinLimit() &&  limitStates.getZMaxLimit() )	zsbm = -endSwitchStepBackMertic;
		
		// limit correction
		if ( moveRelLinearMetricXYZ(xsbm, ysbm, zsbm, true) == false ) {
			std::cerr << "CncControl::correctLimitPositions(): Error while corrrecting limit positions!" << std::endl;
			ret = false;
		}
		
		// adjust position on demand
		if ( validateAppAgainstCtlPosition() == false )
			curAppPos = requestControllerPos();
	}
	
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualSimpleMoveMetric(double x, double y, double z, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	double sX = x * THE_CONFIG->getCalculationFactX();
	double sY = y * THE_CONFIG->getCalculationFactY();
	double sZ = z * THE_CONFIG->getCalculationFactZ();
	
	return manualSimpleMoveSteps((int32_t)round(sX), (int32_t)round(sY), (int32_t)round(sZ), alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualSimpleMoveMetric3D(double x, double y, double z, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	double sX = x * THE_CONFIG->getCalculationFactX();
	double sY = y * THE_CONFIG->getCalculationFactY();
	double sZ = z * THE_CONFIG->getCalculationFactZ();
	
	return manualSimpleMoveSteps3D((int32_t)round(sX), (int32_t)round(sY), (int32_t)round(sZ), alreadyRendered);
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualSimpleMoveSteps(int32_t x, int32_t y, int32_t z, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	bool ret = true;

	if ( x != 0 || y != 0 ) {
		ret = false;
		if ( prepareSimpleMove(false) == true ) {
			ret = moveRelLinearStepsXY(x, y, alreadyRendered );
			if ( limitStates.hasLimit() ) {
				
				if ( x != 0 ) {
					if ( limitStates.isXLimitStateValid() && limitStates.getXMinLimit() )
						ret = moveRelLinearMetricXY(+endSwitchStepBackMertic, 0.0, false);
						
					if ( limitStates.isXLimitStateValid() && limitStates.getXMaxLimit() )
						ret = moveRelLinearMetricXY(-endSwitchStepBackMertic, 0.0, false);
				}
				
				if ( y != 0 ) {
					if ( limitStates.isYLimitStateValid() && limitStates.getYMinLimit() )
						ret = moveRelLinearMetricXY(0.0, +endSwitchStepBackMertic, false);
						
					if ( limitStates.isYLimitStateValid() && limitStates.getYMaxLimit() )
						ret = moveRelLinearMetricXY(0.0, -endSwitchStepBackMertic, false);
				}
			}
				
			reconfigureSimpleMove(ret);
		}
	}
	
	if ( z != 0 ) {
		ret = false;
		if ( prepareSimpleMove(false) == true ) {
			ret = moveRelStepsZ(z);
			if ( ret && limitStates.hasLimit() ) {
				
				if ( limitStates.isZLimitStateValid() && limitStates.getZMinLimit() )
					ret = moveRelMetricZ(+endSwitchStepBackMertic);
					
				if ( limitStates.isZLimitStateValid() && limitStates.getZMaxLimit() )
					ret = moveRelMetricZ(-endSwitchStepBackMertic);
			}
				
			reconfigureSimpleMove(ret);
		}
	}
	
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::manualSimpleMoveSteps3D(int32_t x, int32_t y, int32_t z, bool alreadyRendered) {
///////////////////////////////////////////////////////////////////
	bool ret = true;
	
	if ( x != 0 || y != 0 || z != 0 ) {
		ret = false;
		if ( prepareSimpleMove(false) == true ) {
			ret = moveRelLinearStepsXYZ(x, y, z, alreadyRendered );
			
			if ( limitStates.hasLimit() ) {
				
				if ( x != 0 ) {
					if ( limitStates.isXLimitStateValid() && limitStates.getXMinLimit() )
						ret = moveRelLinearMetricXYZ(+endSwitchStepBackMertic, 0.0, 0.0, false);
						
					if ( limitStates.isXLimitStateValid() && limitStates.getXMaxLimit() )
						ret = moveRelLinearMetricXYZ(-endSwitchStepBackMertic, 0.0, 0.0, false);
				}
				
				if ( y != 0 ) {
					if ( limitStates.isYLimitStateValid() && limitStates.getYMinLimit() )
						ret = moveRelLinearMetricXYZ(0.0, +endSwitchStepBackMertic, 0.0,false);
						
					if ( limitStates.isYLimitStateValid() && limitStates.getYMaxLimit() )
						ret = moveRelLinearMetricXYZ(0.0, -endSwitchStepBackMertic, 0.0, false);
				}
				
				if ( z != 0 ) {
					if ( limitStates.isZLimitStateValid() && limitStates.getZMinLimit() )
						ret = moveRelLinearMetricXYZ(0.0, 0.0, +endSwitchStepBackMertic, false);
						
					if ( limitStates.isZLimitStateValid() && limitStates.getZMaxLimit() )
						ret = moveRelLinearMetricXYZ(0.0, 0.0, -endSwitchStepBackMertic, false);
				}
			}
			
			reconfigureSimpleMove(ret);
		}
	}
	
	return ret;
}
///////////////////////////////////////////////////////////////////
bool CncControl::prepareSimpleMove(bool enaleEventHandling) {
///////////////////////////////////////////////////////////////////
	initNextDuration();
	THE_CONTEXT->setAllowEventHandling(enaleEventHandling);
	activatePositionCheck(false);
	enableStepperMotors(ENABLE_STATE_ON);
	
	// currently no checks implemented, if checks necessary do it here
	return true;
}
///////////////////////////////////////////////////////////////////
void CncControl::reconfigureSimpleMove(bool correctPositions) {
///////////////////////////////////////////////////////////////////
	enableStepperMotors(ENABLE_STATE_OFF);
	activatePositionCheck(true);
	resetDurationCounter();
	
	if ( validateAppAgainstCtlPosition() == false && correctPositions == true ) {
		curAppPos = requestControllerPos();
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::clearControllerConfigControl() {
///////////////////////////////////////////////////////////////////
	THE_APP->GetDvListCtrlControllerConfig()->DeleteAllItems();
}
///////////////////////////////////////////////////////////////////
void CncControl::appendPidKeyValueToControllerConfig(int pid, const char* key, const char* value, const char* unit) {
///////////////////////////////////////////////////////////////////
	DcmItemList rows;

	DataControlModel::addNumKeyValueUnitRow(rows, pid, key, value, unit);
	THE_APP->GetDvListCtrlControllerConfig()->Freeze();
	
	for (DcmItemList::iterator it = rows.begin(); it != rows.end(); ++it) 
		THE_APP->GetDvListCtrlControllerConfig()->AppendItem(*it);
	
	THE_APP->GetDvListCtrlControllerConfig()->Thaw();
}
///////////////////////////////////////////////////////////////////
void CncControl::clearControllerPinControl() {
///////////////////////////////////////////////////////////////////
	THE_APP->GetDvListCtrlControllerPins()->DeleteAllItems();
}
///////////////////////////////////////////////////////////////////
void CncControl::appendNumKeyValueToControllerPinInfo(const char* desc, int pin, int type, int mode, int value) {
///////////////////////////////////////////////////////////////////
	DcmItemList rows;

	DataControlModel::addPinReportRow(rows, desc, pin, type, mode, value);
	THE_APP->GetDvListCtrlControllerPins()->Freeze();
	
	for (DcmItemList::iterator it = rows.begin(); it != rows.end(); ++it)
		THE_APP->GetDvListCtrlControllerPins()->AppendItem(*it);
	
	THE_APP->GetDvListCtrlControllerPins()->Thaw();
}
///////////////////////////////////////////////////////////////////
void CncControl::updatePreview3D(bool force) {
///////////////////////////////////////////////////////////////////
	if ( THE_APP->getMotionMonitor() == NULL )
		return;
		
	if ( force == true ) {
		THE_APP->getMotionMonitor()->Refresh();
		return;
	}
	
	// Online drawing coordinates
	if ( THE_CONTEXT->isOnlineUpdateDrawPane() ) {
		static CncMilliTimestamp tsLastUpdate = CncTimeFunctions::getMilliTimestamp();
		
		if ( (CncTimeFunctions::getMilliTimestamp() - tsLastUpdate) >= THE_CONTEXT->getUpdateInterval() ) {
			THE_APP->getMotionMonitor()->Refresh();
			tsLastUpdate = CncTimeFunctions::getMilliTimestamp();
		}
	}
}
///////////////////////////////////////////////////////////////////
void CncControl::sendIdleMessage() {
///////////////////////////////////////////////////////////////////
	if ( getSerial() == NULL )
		return;
		
	if ( getSerial()->isCommandActive() == true )
		return;
	
	getSerial()->processIdle();
}
