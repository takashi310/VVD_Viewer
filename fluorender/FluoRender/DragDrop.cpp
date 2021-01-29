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
#include "DragDrop.h"
#include "VRenderFrame.h"
#include "VRenderView.h"
#include "wx/regex.h"

DnDFile::DnDFile(wxWindow *frame, wxWindow *view)
: m_frame(frame),
m_view(view)
{
}

DnDFile::~DnDFile()
{
}

wxDragResult DnDFile::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
	return wxDragCopy;
}

bool DnDFile::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString &filenames)
{
	if (filenames.Count())
	{
		VRenderFrame* vr_frame = (VRenderFrame*)m_frame;
		if (vr_frame)
		{
            wxArrayString flatfns;
            for (auto fn : filenames)
            {
                wxDir dir(fn);
                if (wxDirExists(fn) && dir.IsOpened())
                {
                    wxRegEx scdir_pattern("^s[0-9]+$");
                    wxString n;
                    
                    bool bOk = dir.GetFirst(&n);
                    while(bOk)
                    {
                        wxString fullpath = dir.GetNameWithSep() + n;
                        if (wxDirExists(fullpath))
                        {
                            if (scdir_pattern.Matches(n))
                            {
                                if (wxFileExists(fullpath + wxFILE_SEP_PATH + "attributes.json"))
                                {
                                    flatfns.Add(fn + ".n5fs_ch");
                                    break;
                                }
                            }
                        }
                        else if (wxFileExists(fullpath))
                            flatfns.Add(fullpath);
                    }
                }
                else
                    flatfns.Add(fn);
            }
            
            if (flatfns.IsEmpty())
                return false;
            
			wxString filename = flatfns[0];
			wxString suffix = filename.Mid(filename.Find('.', true)).MakeLower();

			if (suffix == ".vrp")
			{
				if (m_view)
				{
					wxMessageBox("For project files, drag and drop to regions other than render views.");
					return false;
				}
				vr_frame->OpenProject(filename);
			}
			else if (suffix == ".nrrd" ||
				suffix == ".tif" ||
				suffix == ".tiff" ||
				suffix == ".oib" ||
				suffix == ".oif" ||
				suffix == ".lsm" ||
				suffix == ".xml" ||
				suffix == ".vvd" ||
				suffix == ".h5j" ||
				suffix == ".v3dpbd" ||
				suffix == ".zip" ||
				suffix == ".idi" ||
                suffix == ".n5" ||
                suffix == ".json" ||
                suffix == ".n5fs_ch")
			{
				vr_frame->LoadVolumes(flatfns, (VRenderView*)m_view);
			}
			else if (suffix == ".obj" || suffix == ".swc")
			{
				vr_frame->LoadMeshes(flatfns, (VRenderView*)m_view);
			}
		}
	}

	return true;
}
