//////////////////////////////////////////////////////////////////////
// This file was auto-generated by codelite's wxCrafter Plugin
// wxCrafter project file: wxCrafterTemplateObserver.wxcp
// Do not modify this file by hand!
//////////////////////////////////////////////////////////////////////

#include "wxCrafterTemplateObserver.h"


// Declare the bitmap loading function
extern void wxC60FFInitBitmapResources();

static bool bBitmapLoaded = false;


CncTemplateObserverBase::CncTemplateObserverBase(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style)
{
    if ( !bBitmapLoaded ) {
        // We need to initialise the default bitmap handler
        wxXmlResource::Get()->AddHandler(new wxBitmapXmlHandler);
        wxC60FFInitBitmapResources();
        bBitmapLoaded = true;
    }
    
    wxFlexGridSizer* flexGridSizer6517 = new wxFlexGridSizer(2, 1, 0, 0);
    flexGridSizer6517->SetFlexibleDirection( wxBOTH );
    flexGridSizer6517->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer6517->AddGrowableCol(0);
    flexGridSizer6517->AddGrowableRow(1);
    this->SetSizer(flexGridSizer6517);
    
    wxFlexGridSizer* flexGridSizer6506 = new wxFlexGridSizer(3, 1, 0, 0);
    flexGridSizer6506->SetFlexibleDirection( wxBOTH );
    flexGridSizer6506->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer6506->AddGrowableCol(0);
    
    flexGridSizer6517->Add(flexGridSizer6506, 1, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    m_staticText6508 = new wxStaticText(this, wxID_ANY, _("Externally Template Modification Observer:"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    m_staticText6508->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    wxFont m_staticText6508Font(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_staticText6508->SetFont(m_staticText6508Font);
    
    flexGridSizer6506->Add(m_staticText6508, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    wxFlexGridSizer* flexGridSizer6550 = new wxFlexGridSizer(1, 2, 0, 0);
    flexGridSizer6550->SetFlexibleDirection( wxBOTH );
    flexGridSizer6550->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer6550->AddGrowableCol(1);
    flexGridSizer6550->AddGrowableRow(0);
    
    flexGridSizer6506->Add(flexGridSizer6550, 0, wxALL|wxEXPAND, WXC_FROM_DIP(3));
    
    wxFlexGridSizer* flexGridSizer6551 = new wxFlexGridSizer(2, 1, 0, 0);
    flexGridSizer6551->SetFlexibleDirection( wxBOTH );
    flexGridSizer6551->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    
    flexGridSizer6550->Add(flexGridSizer6551, 0, wxALL|wxEXPAND, WXC_FROM_DIP(0));
    
    m_observationMode = new wxCheckBox(this, wxID_ANY, _("Observation of externally template\nfile modifications.\n"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    m_observationMode->SetValue(false);
    wxFont m_observationModeFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_observationMode->SetFont(m_observationModeFont);
    
    flexGridSizer6551->Add(m_observationMode, 0, wxALL, WXC_FROM_DIP(1));
    
    wxFlexGridSizer* flexGridSizer6554 = new wxFlexGridSizer(1, 2, 0, 0);
    flexGridSizer6554->SetFlexibleDirection( wxBOTH );
    flexGridSizer6554->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    
    flexGridSizer6551->Add(flexGridSizer6554, 0, wxALL, WXC_FROM_DIP(0));
    
    flexGridSizer6554->Add(12, 0, 1, wxALL, WXC_FROM_DIP(0));
    
    m_staticText6553 = new wxStaticText(this, wxID_ANY, _("This will setup the source editor\nas readonly"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    wxFont m_staticText6553Font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Segoe UI"));
    m_staticText6553->SetFont(m_staticText6553Font);
    
    flexGridSizer6554->Add(m_staticText6553, 0, wxALL, WXC_FROM_DIP(5));
    
    wxArrayString m_actionSelectionArr;
    m_actionSelectionArr.Add(_("Nothing"));
    m_actionSelectionArr.Add(_("Update/Show Preview"));
    m_actionSelectionArr.Add(_("Process the template  - Auto Run"));
    m_actionSelectionArr.Add(_("Debug the template   - Auto Debug"));
    m_actionSelection = new wxRadioBox(this, wxID_ANY, _("Actions to be done .  .  ."), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), m_actionSelectionArr, 1, wxRA_SPECIFY_COLS);
    wxFont m_actionSelectionFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_actionSelection->SetFont(m_actionSelectionFont);
    m_actionSelection->SetSelection(1);
    
    flexGridSizer6550->Add(m_actionSelection, 0, wxALL|wxALIGN_RIGHT, WXC_FROM_DIP(5));
    
    wxFlexGridSizer* flexGridSizer6519 = new wxFlexGridSizer(3, 1, 0, 0);
    flexGridSizer6519->SetFlexibleDirection( wxBOTH );
    flexGridSizer6519->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer6519->AddGrowableCol(0);
    flexGridSizer6519->AddGrowableRow(1);
    
    flexGridSizer6517->Add(flexGridSizer6519, 1, wxALL|wxEXPAND, WXC_FROM_DIP(0));
    
    wxFlexGridSizer* flexGridSizer6530 = new wxFlexGridSizer(1, 3, 0, 0);
    flexGridSizer6530->SetFlexibleDirection( wxBOTH );
    flexGridSizer6530->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer6530->AddGrowableCol(1);
    flexGridSizer6530->AddGrowableRow(0);
    
    flexGridSizer6519->Add(flexGridSizer6530, 0, wxALL|wxEXPAND, WXC_FROM_DIP(0));
    
    wxFlexGridSizer* flexGridSizer6525 = new wxFlexGridSizer(1, 5, 0, 0);
    flexGridSizer6525->SetFlexibleDirection( wxBOTH );
    flexGridSizer6525->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer6525->AddGrowableCol(2);
    flexGridSizer6525->AddGrowableRow(0);
    
    flexGridSizer6530->Add(flexGridSizer6525, 1, wxALL|wxEXPAND|wxALIGN_RIGHT, WXC_FROM_DIP(1));
    
    m_staticText7386 = new wxStaticText(this, wxID_ANY, _("Template Timestamp / Last Observation:"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    wxFont m_staticText7386Font(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_staticText7386->SetFont(m_staticText7386Font);
    
    flexGridSizer6525->Add(m_staticText7386, 0, wxALL, WXC_FROM_DIP(5));
    
    m_curTemplateTimestamp = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_READONLY);
    wxFont m_curTemplateTimestampFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_curTemplateTimestamp->SetFont(m_curTemplateTimestampFont);
    #if wxVERSION_NUMBER >= 3000
    m_curTemplateTimestamp->SetHint(wxT(""));
    #endif
    
    flexGridSizer6525->Add(m_curTemplateTimestamp, 0, wxALL, WXC_FROM_DIP(1));
    
    m_staticText6527 = new wxStaticText(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    wxFont m_staticText6527Font(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_staticText6527->SetFont(m_staticText6527Font);
    
    flexGridSizer6525->Add(m_staticText6527, 0, wxALL, WXC_FROM_DIP(5));
    
    m_lastTimestamp = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_RIGHT|wxTE_READONLY);
    wxFont m_lastTimestampFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Segoe UI"));
    m_lastTimestamp->SetFont(m_lastTimestampFont);
    #if wxVERSION_NUMBER >= 3000
    m_lastTimestamp->SetHint(wxT(""));
    #endif
    
    flexGridSizer6525->Add(m_lastTimestamp, 0, wxALL|wxALIGN_RIGHT, WXC_FROM_DIP(1));
    
    m_button6532 = new wxButton(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(24,24)), 0);
    #if wxVERSION_NUMBER >= 2904
    m_button6532->SetBitmap(wxXmlResource::Get()->LoadBitmap(wxT("16-clean")), wxLEFT);
    m_button6532->SetBitmapMargins(2,2);
    #endif
    m_button6532->SetToolTip(_("Clear the observer trace"));
    
    flexGridSizer6530->Add(m_button6532, 0, wxALL|wxEXPAND|wxALIGN_RIGHT|wxALIGN_TOP, WXC_FROM_DIP(1));
    m_button6532->SetMinSize(wxSize(24,24));
    
    m_observerTrace = new wxTextCtrl(this, wxID_ANY, wxT("test text"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_RICH|wxTE_READONLY|wxTE_MULTILINE|wxHSCROLL|wxVSCROLL);
    m_observerTrace->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    wxFont m_observerTraceFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas"));
    m_observerTrace->SetFont(m_observerTraceFont);
    
    flexGridSizer6519->Add(m_observerTrace, 0, wxALL|wxEXPAND, WXC_FROM_DIP(1));
    
    wxFlexGridSizer* flexGridSizer7394 = new wxFlexGridSizer(1, 2, 0, 0);
    flexGridSizer7394->SetFlexibleDirection( wxBOTH );
    flexGridSizer7394->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizer7394->AddGrowableCol(0);
    flexGridSizer7394->AddGrowableRow(0);
    
    flexGridSizer6519->Add(flexGridSizer7394, 0, wxALL|wxEXPAND, WXC_FROM_DIP(0));
    
    m_curTemplateName = new wxTextCtrl(this, wxID_ANY, wxT("name"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_READONLY);
    m_curTemplateName->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    m_curTemplateName->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT));
    wxFont m_curTemplateNameFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas"));
    m_curTemplateName->SetFont(m_curTemplateNameFont);
    #if wxVERSION_NUMBER >= 3000
    m_curTemplateName->SetHint(wxT(""));
    #endif
    
    flexGridSizer7394->Add(m_curTemplateName, 0, wxALL|wxEXPAND, WXC_FROM_DIP(1));
    
    m_curTemplateSize = new wxTextCtrl(this, wxID_ANY, wxT("size"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(76,-1)), wxTE_READONLY);
    m_curTemplateSize->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    m_curTemplateSize->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT));
    wxFont m_curTemplateSizeFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Consolas"));
    m_curTemplateSize->SetFont(m_curTemplateSizeFont);
    #if wxVERSION_NUMBER >= 3000
    m_curTemplateSize->SetHint(wxT(""));
    #endif
    
    flexGridSizer7394->Add(m_curTemplateSize, 0, wxALL|wxEXPAND, WXC_FROM_DIP(1));
    m_curTemplateSize->SetMinSize(wxSize(76,-1));
    
    m_observeTimer = new wxTimer;
    
    SetName(wxT("CncTemplateObserverBase"));
    SetSize(500,300);
    if (GetSizer()) {
         GetSizer()->Fit(this);
    }
    // Connect events
    m_observationMode->Connect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(CncTemplateObserverBase::changeObservationMode), NULL, this);
    m_button6532->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CncTemplateObserverBase::clearTrace), NULL, this);
    m_observeTimer->Connect(wxEVT_TIMER, wxTimerEventHandler(CncTemplateObserverBase::observe), NULL, this);
    
}

CncTemplateObserverBase::~CncTemplateObserverBase()
{
    m_observationMode->Disconnect(wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(CncTemplateObserverBase::changeObservationMode), NULL, this);
    m_button6532->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CncTemplateObserverBase::clearTrace), NULL, this);
    m_observeTimer->Disconnect(wxEVT_TIMER, wxTimerEventHandler(CncTemplateObserverBase::observe), NULL, this);
    
    m_observeTimer->Stop();
    wxDELETE( m_observeTimer );

}
