#ifndef GameControllerThread_H
#define GameControllerThread_H

#include <wx/thread.h>
#include "GamepadEvent.h"
#include "CncGamePad.h"

////////////////////////////////////////////////////////////////////////////////////
class MainFrame;

typedef void (wxEvtHandler::*GamepadEventFunction)(GamepadEvent&);
#define GamepadEventHandler(func) wxEVENT_HANDLER_CAST(GamepadEventFunction, func)

class GamepadThread : public wxThread {
	
	protected:
		MainFrame* pHandler;
		bool exit;
		
		virtual ExitCode Entry();
		void evaluateNotifications(CncGamepad& gamepad, GamepadEvent& state);
		
	public:
		GamepadThread(MainFrame *handler);
		virtual ~GamepadThread();
		
		// thread interface
		void stop();
};

#endif
