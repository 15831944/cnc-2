#include <iostream>
#include "SvgEditPopup.h"
#include "CncSourceEditor.h"

///////////////////////////////////////////////////////////////////
CncSourceEditor::CncSourceEditor(wxWindow *parent) 
: CncBaseEditor(parent)
///////////////////////////////////////////////////////////////////
{
	initialize();
}
///////////////////////////////////////////////////////////////////
CncSourceEditor::~CncSourceEditor() {
///////////////////////////////////////////////////////////////////
}
///////////////////////////////////////////////////////////////////
void CncSourceEditor::initialize() {
///////////////////////////////////////////////////////////////////
	svgPopupMenu = SvgEditPopup::createMenu(this, svgPopupMenu, true);
}
