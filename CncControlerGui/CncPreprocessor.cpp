#include "GlobalFunctions.h"
#include "GlobalStrings.h"
#include "CncConfig.h"
#include "CncContext.h"
#include "CncMoveSequenceListCtrl.h"
#include "CncPathListEntryListCtrl.h"
#include "CncPreprocessor.h"

extern GlobalConstStringDatabase globalStrings;

//////////////////////////////////////////////////////////////////
CncPreprocessor::CncPreprocessor(wxWindow* parent)
: CncPreprocessorBase(parent)
, pathListEntries(NULL)
//////////////////////////////////////////////////////////////////
{
	// path list entries control
	pathListEntries = new CncPathListEntryListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL); 
	GblFunc::replaceControl(m_pathListEntriesPlaceholder, pathListEntries);
	
	// move sequences control
	moveSequences = new CncMoveSequenceListCtrl(this, wxLC_HRULES | wxLC_VRULES | wxLC_SINGLE_SEL);
	GblFunc::replaceControl(m_moveSequencesPlaceholder, moveSequences);

	const wxFont font = GBL_CONTEXT->outboundListBookFont;
	m_listbookPreProcessor->GetListView()->SetFont(font);
}
//////////////////////////////////////////////////////////////////
CncPreprocessor::~CncPreprocessor() {
//////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::clearAll() {
//////////////////////////////////////////////////////////////////
	clearPathListEntries();
	clearMoveSequences();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::clearPathListEntries() {
//////////////////////////////////////////////////////////////////
	pathListEntries->clear();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::clearMoveSequences() {
//////////////////////////////////////////////////////////////////
	moveSequences->clear();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::freeze() {
//////////////////////////////////////////////////////////////////
	pathListEntries->freeze();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::thaw() {
//////////////////////////////////////////////////////////////////
	pathListEntries->thaw();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::enablePathListEntries(bool state) {
//////////////////////////////////////////////////////////////////
	wxBitmap bmpOn  = ImageLib16().Bitmap("BMP_CONNECTED");
	wxBitmap bmpOff = ImageLib16().Bitmap("BMP_DISCONNECTED");
	
	state == true ? m_btConnectPathListEntries->SetBitmap(bmpOn) 			: m_btConnectPathListEntries->SetBitmap(bmpOff);
	state == true ? m_btConnectPathListEntries->SetToolTip("Disable List")	: m_btConnectPathListEntries->SetToolTip("Enable List");
	
	m_btConnectPathListEntries->SetValue(state);
	
	m_btConnectPathListEntries->Refresh();
	m_btConnectPathListEntries->Update();
	
	if ( state == false )
		clearPathListEntries();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::enableMoveSequences(bool state) {
//////////////////////////////////////////////////////////////////
	wxBitmap bmpOn  = ImageLib16().Bitmap("BMP_CONNECTED");
	wxBitmap bmpOff = ImageLib16().Bitmap("BMP_DISCONNECTED");
	
	state == true ? m_btConnectMoveSequences->SetBitmap(bmpOn) 				: m_btConnectMoveSequences->SetBitmap(bmpOff);
	state == true ? m_btConnectMoveSequences->SetToolTip("Disable List")	: m_btConnectMoveSequences->SetToolTip("Enable List");
	
	m_btConnectMoveSequences->SetValue(state);
	
	m_btConnectMoveSequences->Refresh();
	m_btConnectMoveSequences->Update();
	
	if ( state == false )
		clearMoveSequences();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::addMoveSequenceStart(const CncMoveSequence& seq) {
//////////////////////////////////////////////////////////////////
	if ( m_btConnectMoveSequences->GetValue() == false ) 
		return;

	CncColumContainer cc(CncMoveSequenceListCtrl::TOTAL_COL_COUNT);
	static const wxString fmt(globalStrings.moveSeqRefFormat);

	cc.updateItem(CncMoveSequenceListCtrl::COL_TYPE, wxString::Format("%d",   1));
	cc.updateItem(CncMoveSequenceListCtrl::COL_REF,  wxString::Format("%lld", seq.getReference()));

	moveSequences->appendItem(cc);
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::addMoveSequence(const CncMoveSequence& seq) {
//////////////////////////////////////////////////////////////////
	if ( m_btConnectMoveSequences->GetValue() == false ) 
		return;
	
	// over all entries (SequencePoints)
	moveSequences->freeze();
	for ( auto it = seq.const_begin(); it != seq.const_end(); ++it) {

		CncColumContainer cc(CncMoveSequenceListCtrl::TOTAL_COL_COUNT);
		static const wxString fmt(globalStrings.moveSeqRefFormat);

		cc.updateItem(CncMoveSequenceListCtrl::COL_TYPE, 			wxString::Format("%d", 		3));
		cc.updateItem(CncMoveSequenceListCtrl::COL_REF, 			wxString::Format("%lld", 	seq.getReference()));

		const CncMoveSequence::SequencePoint sp = *it;
		cc.updateItem(CncMoveSequenceListCtrl::COL_CLD_ID,			wxString::Format(fmt, 		sp.clientID));

		cc.updateItem(CncMoveSequenceListCtrl::COL_DISTANCE_X,		wxString::Format("%10ld",	(long)sp.x));
		cc.updateItem(CncMoveSequenceListCtrl::COL_DISTANCE_Y,		wxString::Format("%10ld", 	(long)sp.y));
		cc.updateItem(CncMoveSequenceListCtrl::COL_DISTANCE_Z,		wxString::Format("%10ld", 	(long)sp.z));

		moveSequences->appendItem(cc);
	}
	moveSequences->thaw();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::addPathListEntry(const CncPathListEntry& cpe) {
//////////////////////////////////////////////////////////////////
	if ( m_btConnectPathListEntries->GetValue() == false )														return;
	if ( cpe.isNothingChange() 					== true && m_btToogleFormat->GetValue()             == true )	return;
	if ( cpe.isClientIdChange() 				== true && m_btConsiderClientIdChanges->GetValue() 	== false )	return;
	if ( cpe.isSpeedChange() 					== true && m_btConsiderFeedSpeedChanges->GetValue() == false )	return;
	if ( cpe.isPositionChange() 				== true && m_btConsiderPositionChanges->GetValue() 	== false )	return;
	
	CncColumContainer cc(CncPathListEntryListCtrl::TOTAL_COL_COUNT);
	static const wxString fmt(globalStrings.pathListRefFormat);
	
	const bool displaySpeed    = ( cpe.isSpeedChange()    == true || cpe.isNothingChange() == true );
	const bool displayPosition = ( cpe.isPositionChange() == true || cpe.isNothingChange() == true );

	cc.updateItem(CncPathListEntryListCtrl::COL_TYPE, 				wxString::Format("%d", 			cpe.type));
	cc.updateItem(CncPathListEntryListCtrl::COL_REF, 				wxString::Format("%lld", 		cpe.pathListReference));
	cc.updateItem(CncPathListEntryListCtrl::COL_CLD_ID,				wxString::Format(fmt, 			cpe.clientId));
	
	if ( displaySpeed == true ) {
		cc.updateItem(CncPathListEntryListCtrl::COL_F, 				wxString::Format("%4.1lf %c", 	cpe.feedSpeed_MM_MIN, cnc::getCncSpeedTypeAsCharacter(cpe.feedSpeedMode)));
	}
	
	if ( displayPosition == true ) {
		cc.updateItem(CncPathListEntryListCtrl::COL_DISTANCE_X, 	wxString::Format("%10.3lf", 	cpe.entryDistance.getX()));
		cc.updateItem(CncPathListEntryListCtrl::COL_DISTANCE_Y, 	wxString::Format("%10.3lf", 	cpe.entryDistance.getY()));
		cc.updateItem(CncPathListEntryListCtrl::COL_DISTANCE_Z, 	wxString::Format("%10.3lf", 	cpe.entryDistance.getZ()));
		cc.updateItem(CncPathListEntryListCtrl::COL_TARGET_X, 		wxString::Format("%10.3lf", 	cpe.entryTarget.getX()));
		cc.updateItem(CncPathListEntryListCtrl::COL_TARGET_Y, 		wxString::Format("%10.3lf", 	cpe.entryTarget.getY()));
		cc.updateItem(CncPathListEntryListCtrl::COL_TARGET_Z, 		wxString::Format("%10.3lf", 	cpe.entryTarget.getZ()));
		cc.updateItem(CncPathListEntryListCtrl::COL_TOTAL_DISTANCE, wxString::Format("%10.3lf", 	cpe.totalDistance));
	}
	
	pathListEntries->appendItem(cc);
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::clearMoveSequences(wxCommandEvent& event) {
//////////////////////////////////////////////////////////////////
	clearMoveSequences();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::clearPathListEntries(wxCommandEvent& event) {
//////////////////////////////////////////////////////////////////
	clearPathListEntries();
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::connectMoveSequences(wxCommandEvent& event) {
//////////////////////////////////////////////////////////////////
	enableMoveSequences(m_btConnectMoveSequences->GetValue());
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::connectPathListEntries(wxCommandEvent& event) {
//////////////////////////////////////////////////////////////////
	enablePathListEntries(m_btConnectPathListEntries->GetValue());
}
//////////////////////////////////////////////////////////////////
void CncPreprocessor::selectClientId(long id, ListType lt) {
//////////////////////////////////////////////////////////////////
	switch ( lt ) {
		case LT_PATH_LIST: 		pathListEntries->searchReferenceById(id); 	break;
		case LT_MOVE_SEQUENCE:	moveSequences->searchReferenceById(id); 	break;
	}
}
