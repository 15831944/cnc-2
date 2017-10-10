#ifndef CNC_CONTROL_H
#define CNC_CONTROL_H

#include <vector>
#include <time.h>
#include <assert.h>
#include <wx/string.h>
#include <wx/geometry.h>
#include <wx/menuitem.h>
#include <wx/checkbox.h>
#include <wx/dragimag.h>
#include <wx/panel.h>
#include <wx/dcclient.h>
#include <wx/pen.h>
#include "CommandTemplates.h"
#include "CncToolStateControl.h"
#include "CncCommon.h"
#include "PenHandler.h"
#include "GuiControlSetup.h"
#include "CncConfig.h"
#include "SerialPort.h"
#include "CncLimitStates.h"
#include "CncPosition.h"

const double endSwitchStepBackMertic = 2.5;

///////////////////////////////////////////////////////////////////
class CncControl {
	public:
		enum DimensionMode {DM_2D, DM_3D};
		
	private:
		wxString stringTemplate;
		
		///////////////////////////////////////////////////////////////////
		struct SetterTuple {
			unsigned char 	id;
			int32_t			value;
			
			SetterTuple(unsigned char i, int32_t v)
			: id(i)
			, value(v)
			{}
		};
		
	protected:
		CncPortType portType;
		// internal port object
		Serial* serialPort;
		// handels the cnc configuration
		CncConfig* cncConfig;
		// Defines the absolute zero pos as a reference pos 
		CncLongPosition zeroPos;
		// Defines the start postion of an object in relation to zeroPos 
		CncLongPosition startPos;
		// Defines the current pos
		CncLongPosition curPos;
		// Stores the lateset requested control positions
		CncLongPosition curCtlPos;
		// Stores the lateset requested control positions
		CncLongPosition controllerPos;
		// Render mode
		CncRenderMode renderMode;
		// Duration counter
		unsigned int durationCounter;
		// Interrupt stae
		bool interruptState;
		// power state
		bool powerOn;
		// flag for updating tools
		bool toolUpdateState;
		// Artificially Step Delay
		unsigned int stepDelay;
		// output controls
		GuiControlSetup* guiCtlSetup;
		#define GET_GUI_CTL(ctl)           (guiCtlSetup->ctl)
		#define IS_GUI_CTL_VALID(ctl)      (guiCtlSetup != NULL && guiCtlSetup->ctl != NULL)
		#define IS_GUI_CTL_NOT_VALID(ctl)  (guiCtlSetup == NULL || guiCtlSetup->ctl == NULL)
		// Tool state handling
		CncToolStateControl toolState;
		//measurements variables
		struct timeb endTime, startTime;
		// command counter
		int32_t commandCounter;
		// Flag to indicatate if a positions check aplies
		bool positionCheck;
		// Pen handler
		PenHandler penHandler;
		// margin of the draw pane
		unsigned int drawPaneMargin;
		// determine the speed mode 
		DimensionMode speedMonitorMode;
		//sets a value into the given text control
		void setValue(wxTextCtrl *ctl, int32_t val);
		void setValue(wxTextCtrl *ctl, double val);
		void setValue(wxTextCtrl *ctl, const char* t);

		// Tool management
		void setToolState(bool defaultStyle = false);
		
		// Limit management
		CncLimitStates limitStates;
		void displayLimitState(wxStaticText* ctl, bool value);
		void evaluateLimitState(long x, long y, long z);
		
		//simple move
		bool prepareSimpleMove(bool enaleEventHandling = true);
		void reconfigureSimpleMove(bool correctPositions);
		
		// secial convesion to transfer a double as long
		long convertDoubleToCtrlLong(unsigned char 	id, double d);
		
	public:
		
		CncControl(CncPortType pt);
		virtual ~CncControl();
		//Get the current port type
		CncPortType getPortType(void) { return portType; }
		// Connection to portName
		bool connect(const char * portName);
		// Check the connection
		bool isConnected();
		//Make Serial available
		Serial* getSerial() { return serialPort; }
		//Make cnc config available
		CncConfig* getCncConfig() { return cncConfig; }
		void updateCncConfig(CncConfig& cc);
		// wrapper
		bool processCommand(const char* cmd, std::ostream& txtCtl);
		// wrapper
		bool processCommand(const unsigned char c, std::ostream& txtCtl);
		// wrapper
		bool processMoveXYZ(int32_t x1, int32_t y1, int32_t z1, bool alreadyRendered);
		// Zero positioning
		void setZeroPosX();
		void setZeroPosY();
		void setZeroPosZ();
		void setZeroPos();
		// Start positioning
		void setStartPos();
		// Move from current position
		bool moveRelStepsZ(int32_t z);
		bool moveRelLinearStepsXY(int32_t x1, int32_t y1, bool alreadyRendered);
		bool moveRelLinearStepsXYZ(int32_t x1, int32_t y1, int32_t z1, bool alreadyRendered);
		
		bool moveRelMetricZ(double z);
		bool moveRelLinearMetricXY(double x1, double y1, bool alreadyRendered);
		bool moveRelLinearMetricXYZ(double x1, double y1, double z1, bool alreadyRendered);
		
		bool moveAbsMetricZ(double z);
		bool moveAbsLinearMetricXY(double x1, double y1, bool alreadyRendered);
		bool moveAbsLinearMetricXYZ(double x1, double y1, double z1, bool alreadyRendered);
		
		// Callback from Serial
		bool SerialCallback(int32_t cmdCount);
		// Callback from Serial with controller content
		bool SerialControllerCallback(const ContollerInfo& ci);
		// Callback fromS Serial with a controller message
		bool SerialMessageCallback(const ControllerMsgInfo& cmi);
		// wait an dispath events
		void waitActive(unsigned int milliseconds, bool once = true);
		//interrupt the processing
		void interrupt();
		void resetInterrupt();
		bool isInterrupted();
		bool reset();
		bool resetWatermarks();
		// Setup the cnc control
		void setup(bool reset = true);
		// Measurements funtions
		void logProcessingStart();
		void logProcessingCurrent();
		void logProcessingEnd(bool valuesOnly = false);
		// Sets the output controls for cooridinate infos
		void setGuiControls(GuiControlSetup* guiCtlSetup);
		//handle draw control
		unsigned int getDrawPaneMargin() { return drawPaneMargin; }
		void clearDrawControl();
		void updateDrawControl();
		// Duration management
		unsigned int getDurationCount();
		unsigned int getDurationCounter();
		void resetDurationCounter();
		void initNextDuration();
		bool hasNextDuration();
		bool isFirstDuration();
		bool isLastDuration();
		// Z axis management
		bool moveZToTop();
		bool moveZToBottom();
		
		// Tool management
		void switchToolOn();
		void switchToolOff(bool force = false);
		bool getToolState() { return powerOn; }	
		// Updates the config trace control
		void updateCncConfigTrace();
		// validating PC pos due to the given reference coordinates
		bool validateCurrentPostion();
		// returns the correponding pc postions
		const CncLongPosition getStartPos() { return startPos; }
		const CncLongPosition getCurPos()   { return curPos; }
		const CncDoublePosition getStartPosMetric();
		const CncDoublePosition getCurPosMetric();
		const CncLongPosition getMinPositions();
		const CncDoublePosition getMinPositionsMetric();
		const CncLongPosition getMaxPositions();
		const CncDoublePosition getMaxPositionsMetric();
		const CncLongPosition::Watermarks getWaterMarks();
		const CncDoublePosition::Watermarks getWaterMarksMetric();
		// query the current error count
		const int32_t getControllerErrorCount();
		// query the current controller position
		const CncLongPosition getControllerPos();
		// query the current controller limit state
		const CncLongPosition getControllerLimitState();
		const CncLimitStates& getLimitState() { return limitStates; }
		// validates pc and controller positions
		bool validatePositions();
		void forceDisplayPositions();
		// processing the given setter values
		bool processSetter(unsigned char id, int32_t value);
		bool processSetterList(std::vector<SetterTuple>& setup);
		// Change the current work speed parameter
		void changeWorkSpeedXY(CncSpeed s);
		void changeWorkSpeedZ(CncSpeed s);
		// Sets a flag that the postions x/y min/max should be checked within the Serial callback
		void activatePositionCheck(bool a) { positionCheck = a; }
		// Sets the enable pin HIGH (s == false) or LOW ( s == true)
		void enableStepperMotors(bool s);
		//Limit management
		wxString& getLimitInfoString(wxString& ret);
		void evaluateLimitState();
		bool isALimitSwitchActive() { return limitStates.hasLimit(); }
		
		bool moveXToMinLimit();
		bool moveXToMaxLimit();
		bool moveYToMinLimit();
		bool moveYToMaxLimit();
		bool moveZToMinLimit();
		bool moveZToMaxLimit();
		bool moveXToMid();
		bool moveYToMid();
		bool moveZToMid();
		
		bool manualSimpleMoveSteps(int32_t x, int32_t y, int32_t z, bool alreadyRendered = false);
		bool manualSimpleMoveSteps3D(int32_t x, int32_t y, int32_t z, bool alreadyRendered = false);
		bool manualSimpleMoveMetric(double x, double y, double z, bool alreadyRendered = false);
		bool manualSimpleMoveMetric3D(double x, double y, double z, bool alreadyRendered = false);
		
		// Moves curPos to zeroPos
		bool moveXYToZeroPos(CncDimensions dim = CncDimension1D);
		bool moveXYZToZeroPos(CncDimensions dim = CncDimension1D);
		bool moveZToZeroPos();
		
		// Moves curPos to startPos
		bool moveToStartPos();
		
		// Moves curPos to zeroPos
		bool simpleMoveXYToZeroPos(CncDimensions dim);
		bool simpleMoveXYZToZeroPos(CncDimensions dim);
		bool simpleMoveZToZeroPos();
		
		// contorler configuration output
		bool hasControllerConfigControl();
		void clearControllerConfigControl();
		void appendPidKeyValueToControllerConfig(int pid, const char* key, const char* value);
		
		// controller error output
		bool hasControllerErrorControl();
		void clearControllerErrorControl();
		void appendNumKeyValueToControllerErrorInfo(int num, int code, const char* key, const char* value);
		
		// controller pin report
		bool hasControllerPinControl();
		void clearControllerPinControl();
		void appendNumKeyValueToControllerErrorInfo(const char* desc, int pin, int type, int mode, int value);
		
		void setUpdateToolControlsState(bool state) { toolUpdateState = state; }
		
		void setStepDelay(unsigned int d) { stepDelay = d; }
		unsigned int getStepDelay() { return stepDelay; }
		
		//meassurments
		bool meassureDimension(const char axis, wxCheckBox* min, wxCheckBox* max, double& result);
		bool meassureXDimension(wxCheckBox* min, wxCheckBox* max, double& result) { return meassureDimension('X', min, max, result); }
		bool meassureYDimension(wxCheckBox* min, wxCheckBox* max, double& result) { return meassureDimension('Y', min, max, result); }
		bool meassureZDimension(wxCheckBox* min, wxCheckBox* max, double& result) { return meassureDimension('Z', min, max, result); }
		
		// 3D control
		void updatePreview3D(bool force=false);
		
		// idle handling
		void sendIdleMessage();
		
		// Speed monitor handling
		void setSpeedControlMode(const DimensionMode& spm) { speedMonitorMode = spm; }
		const DimensionMode& getSpeedControlMode() const   { return speedMonitorMode; }
		
};

#endif