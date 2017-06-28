/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2014 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#include <wx/wx.h>
#include <wx/treectrl.h>
#include "compatibility.h"
#include "utility.h"
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

#ifndef _TREEPANEL_H_
#define _TREEPANEL_H_

//tree icon
#define icon_change	1
#define icon_key	"None"

class VRenderView;
class VolumeData;

//---------------------------------------

//-------------------------------------

//tree item data
class LayerInfo : public wxTreeItemData
{
public:
	LayerInfo()
	{
		type = -1;
		id = -1;
		icon = -1;
	}

	int type;	//0-root; 1-view; 
				//2-volume data; 3-mesh data;
				//5-group; 6-mesh group
				//7-volume segment; 8-segment group

	int id;		//for volume segments

	int icon;
};

class DataTreeCtrl: public wxTreeCtrl, Notifier
{
	enum
	{
		ID_TreeCtrl = wxID_HIGHEST+501,
		ID_ToggleDisp,
		ID_Rename,
		ID_Duplicate,
		ID_Save,
		ID_BakeVolume,
		ID_Isolate,
		ID_ShowAll,
		ID_ExportMetadata,
		ID_ImportMetadata,
		ID_ShowAllSegChildren,
		ID_HideAllSegChildren,
		ID_ShowAllNamedSeg,
		ID_HideAllSeg,
		ID_DeleteAllSeg,
		ID_RemoveData,
		ID_CloseView,
		ID_ManipulateData,
		ID_AddDataGroup,
		ID_AddMeshGroup,
		ID_AddSegGroup,
		ID_Expand,
		ID_Edit,
		ID_Info,
		ID_Trace,
		ID_NoiseCancelling,
		ID_Counting,
		ID_Colocalization,
		ID_Convert,
		ID_RandomizeColor
	};

public:
	DataTreeCtrl(wxWindow* frame,
		wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos=wxDefaultPosition,
		const wxSize& size=wxDefaultSize,
		long style=wxTR_HAS_BUTTONS|
		wxTR_TWIST_BUTTONS|
		wxTR_LINES_AT_ROOT|
		wxTR_NO_LINES|
		wxTR_FULL_ROW_HIGHLIGHT|
		wxTR_EDIT_LABELS);
	~DataTreeCtrl();

	//icon operations
	//change the color of the icon dual
	void ChangeIconColor(int i, wxColor c);
	void AppendIcon();
	void ClearIcons();
	int GetIconNum();

	//item operations
	//delete all
	void DeleteAll();
	void DeleteSelection();
	//traversal delete
	void TraversalDelete(wxTreeItemId item);
	//root item
	wxTreeItemId AddRootItem(const wxString &text);
	void ExpandRootItem();
	//view item
	wxTreeItemId AddViewItem(const wxString &text);
	void SetViewItemImage(const wxTreeItemId& item, int image);
	//volume data item
	wxTreeItemId AddVolItem(wxTreeItemId par_item, const wxString &text);
	wxTreeItemId AddVolItem(wxTreeItemId par_item, VolumeData* vd);
	void UpdateVolItem(wxTreeItemId item, VolumeData* vd);
	void UpdateROITreeIcons(VolumeData* vd);
	void UpdateROITreeIconColor(VolumeData* vd);
	wxTreeItemId GetNextSibling_loop(wxTreeItemId item);
	wxTreeItemId FindTreeItem(wxString name);
	wxTreeItemId FindTreeItem(wxTreeItemId par_item, const wxString& name, bool roi_tree=false);
	void SetVolItemImage(const wxTreeItemId item, int image);
	//mesh data item
	wxTreeItemId AddMeshItem(wxTreeItemId par_item, const wxString &text);
	void SetMeshItemImage(const wxTreeItemId item, int image);
	//annotation item
	wxTreeItemId AddAnnotationItem(wxTreeItemId par_item, const wxString &text);
	void SetAnnotationItemImage(const wxTreeItemId item, int image);
	//group item
	wxTreeItemId AddGroupItem(wxTreeItemId par_item, const wxString &text);
	void SetGroupItemImage(const wxTreeItemId item, int image);
	//mesh group item
	wxTreeItemId AddMGroupItem(wxTreeItemId par_item, const wxString &text);
	void SetMGroupItemImage(const wxTreeItemId item, int image);

	void UpdateSelection();
	wxString GetCurrentSel();
	int GetCurrentSelType();
	int TraversalSelect(wxTreeItemId item, wxString name);
	void Select(wxString view, wxString name);
	void SelectROI(VolumeData* vd, int id);

	//brush commands (from the panel)
	void BrushClear();
	void BrushCreate();
	void BrushCreateInv();

	VRenderView* GetCurrentView();

	void SetFix(bool fix) { m_fixed = fix; }
	bool isFixed() { return m_fixed; }

	void SaveExpState();
	void SaveExpState(wxTreeItemId node, const wxString& prefix=wxT(""));
	void LoadExpState();
	void LoadExpState(wxTreeItemId node, const wxString& prefix=wxT(""), bool expand_newitem=true);
	std::string ExportExpState();
	void ImportExpState(const std::string &state);

	void TraversalExpand(wxTreeItemId item);
	wxTreeItemId GetParentVolItem(wxTreeItemId item);
	void ExpandDataTreeItem(wxString name, bool expand_children=false);
	void CollapseDataTreeItem(wxString name, bool collapse_children=false);

	friend class TreePanel;

private:
	wxWindow* m_frame;

	//drag
	wxTreeItemId m_drag_item;
	wxTreeItemId m_drag_nb_item;
	int m_insert_mode;
	//fix current selection
	bool m_fixed;
	//remember the pos
	int m_scroll_pos;

	static bool m_md_save_indv;

	std::unordered_map<wxString, bool> m_exp_state;

private:

	static wxWindow* CreateExtraControl(wxWindow* parent);
	void OnCh1Check(wxCommandEvent &event);

	//change the color of just one icon of the dual,
	//either enable(type=0), or disable(type=1)
	void ChangeIconColor(int which, wxColor c, int type);

	void UpdateROITreeIcons(wxTreeItemId par_item, VolumeData* vd);
	void UpdateROITreeIconColor(wxTreeItemId par_item, VolumeData* vd);
	void BuildROITree(wxTreeItemId par_item, const boost::property_tree::wptree& tree, VolumeData *vd);

	void OnContextMenu(wxContextMenuEvent &event );

	void OnToggleDisp(wxCommandEvent& event);
	void OnDuplicate(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnBakeVolume(wxCommandEvent& event);
	void OnRenameMenu(wxCommandEvent& event);
	void OnIsolate(wxCommandEvent& event);
	void OnShowAll(wxCommandEvent& event);
	void OnExportMetadata(wxCommandEvent& event);
	void OnImportMetadata(wxCommandEvent& event);
	void OnShowAllSegChildren(wxCommandEvent& event);
	void OnHideAllSegChildren(wxCommandEvent& event);
	void OnShowAllNamedSeg(wxCommandEvent& event);
	void OnHideAllSeg(wxCommandEvent& event);
	void OnDeleteAllSeg(wxCommandEvent& event);
	void OnRemoveData(wxCommandEvent& event);
	void OnCloseView(wxCommandEvent& event);
	void OnManipulateData(wxCommandEvent& event);
	void OnAddDataGroup(wxCommandEvent& event);
	void OnAddMeshGroup(wxCommandEvent& event);
	void OnAddSegGroup(wxCommandEvent& event);
	void OnExpand(wxCommandEvent& event);
	void OnEdit(wxCommandEvent& event);
	void OnInfo(wxCommandEvent& event);
	void OnTrace(wxCommandEvent& event);
	void OnNoiseCancelling(wxCommandEvent& event);
	void OnCounting(wxCommandEvent& event);
	void OnColocalization(wxCommandEvent& event);
	void OnConvert(wxCommandEvent& event);
	void OnRandomizeColor(wxCommandEvent& event);

	void OnSelChanged(wxTreeEvent& event);
	void OnSelChanging(wxTreeEvent& event);
	void OnDeleting(wxTreeEvent& event);
	void OnAct(wxTreeEvent &event);
	void OnBeginDrag(wxTreeEvent& event);
	void OnEndDrag(wxTreeEvent& event);
	void OnRename(wxTreeEvent& event);
	void OnRenamed(wxTreeEvent& event);

	void OnDragging(wxMouseEvent& event);

	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);

	DECLARE_EVENT_TABLE()
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TreePanel : public wxPanel, Observer
{
public:
	enum
	{
		ID_ToggleView = wxID_HIGHEST+551,
		ID_Save,
		ID_BakeVolume,
		ID_RemoveData,
		ID_BrushAppend,
		ID_BrushDesel,
		ID_BrushDiffuse,
		ID_BrushErase,
		ID_BrushClear,
		ID_BrushCreate
	};

	TreePanel(wxWindow* frame,
		wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = "TreePanel");
	~TreePanel();

	DataTreeCtrl* GetTreeCtrl();

	//icon operations
	void ChangeIconColor(int i, wxColor c);
	void AppendIcon();
	void ClearIcons();
	int GetIconNum();

	//item operations
	void SelectItem(wxTreeItemId item);
	void Expand(wxTreeItemId item);
	void ExpandAll();
	void DeleteAll();
	void TraversalDelete(wxTreeItemId item);
	wxTreeItemId AddRootItem(const wxString &text);
	void ExpandRootItem();
	wxTreeItemId AddViewItem(const wxString &text);
	void SetViewItemImage(const wxTreeItemId& item, int image);
	wxTreeItemId AddVolItem(wxTreeItemId par_item, const wxString &text);
	wxTreeItemId AddVolItem(wxTreeItemId par_item, VolumeData* vd);
	void UpdateROITreeIcons(VolumeData* vd);
	void UpdateROITreeIconColor(VolumeData* vd);
	void UpdateVolItem(wxTreeItemId item, VolumeData* vd);
	wxTreeItemId FindTreeItem(wxTreeItemId par_item, wxString name);
	wxTreeItemId FindTreeItem(wxString name);
	void SetVolItemImage(const wxTreeItemId item, int image);
	wxTreeItemId AddMeshItem(wxTreeItemId par_item, const wxString &text);
	void SetMeshItemImage(const wxTreeItemId item, int image);
	wxTreeItemId AddAnnotationItem(wxTreeItemId par_item, const wxString &text);
	void SetAnnotationItemImage(const wxTreeItemId item, int image);
	wxTreeItemId AddGroupItem(wxTreeItemId par_item, const wxString &text);
	void SetGroupItemImage(const wxTreeItemId item, int image);
	wxTreeItemId AddMGroupItem(wxTreeItemId par_item, const wxString &text);
	void SetMGroupItemImage(const wxTreeItemId item, int image);

	//seelction
	void UpdateSelection();
	wxString GetCurrentSel();
	void Select(wxString view, wxString name);
	void SelectROI(VolumeData* vd, int id);

	//set the brush icon down
	void SelectBrush(int id);
	int GetBrushSelected();
	//control from outside
	void BrushAppend();
	void BrushDiffuse();
	void BrushDesel();
	void BrushClear();
	void BrushErase();
	void BrushCreate();
	void BrushSolid(bool state);

	VRenderView* GetCurrentView();

	void SetFix(bool fix) { if (m_datatree) m_datatree->SetFix(fix); }
	bool isFixed() { return m_datatree ? m_datatree->isFixed() : false; }

	void SaveExpState();
	void LoadExpState();
	std::string ExportExpState();
	void ImportExpState(const std::string &state);

	wxTreeItemId GetParentVolItem(wxTreeItemId item);
	wxTreeItemId GetNextSibling_loop(wxTreeItemId item);
	void ExpandDataTreeItem(wxString name, bool expand_children=false);
	void CollapseDataTreeItem(wxString name, bool collapse_children=false);

	void doAction(ActionInfo *info);

private:
	wxWindow* m_frame;
	DataTreeCtrl* m_datatree;
	wxToolBar *m_toolbar;

	void OnSave(wxCommandEvent& event);
	void OnBakeVolume(wxCommandEvent& event);
	void OnToggleView(wxCommandEvent& event);
	void OnRemoveData(wxCommandEvent& event);
	//brush commands
	void OnBrushAppend(wxCommandEvent& event);
	void OnBrushDesel(wxCommandEvent& event);
	void OnBrushDiffuse(wxCommandEvent& event);
	void OnBrushErase(wxCommandEvent& event);
	void OnBrushClear(wxCommandEvent& event);
	void OnBrushCreate(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

#endif//_TREEPANEL_H_
