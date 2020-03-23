#include "stdwx.h"
#include "NeuronAnnotatorPlugin.h"
#include "NeuronAnnotatorPluginWindow.h"
#include <wx/process.h>
#include <wx/mstream.h>
#include <wx/filename.h>
#include <wx/zipstrm.h>
#include "compatibility.h"

IMPLEMENT_DYNAMIC_CLASS(NAGuiPlugin, wxObject)

NAGuiPlugin::NAGuiPlugin()
	: wxGuiPluginBase(NULL, NULL)
{
}

NAGuiPlugin::NAGuiPlugin(wxEvtHandler * handler, wxWindow * vvd)
	: wxGuiPluginBase(handler, vvd)
{
}

NAGuiPlugin::~NAGuiPlugin()
{
}

bool NAGuiPlugin::runNALoader()
{

	return runNALoader(m_id_path, m_vol_path);
}

bool NAGuiPlugin::runNALoader(wxString id_path, wxString vol_path)
{
	if (!wxFileExists(id_path) || !wxFileExists(vol_path))
		return false;
	
	VRenderFrame *vframe = (VRenderFrame *)m_vvd;
	if (!vframe) return false;

	m_lbl_reader = new V3DPBDReader();
    m_vol_reader = new H5JReader();
    
    wstring str_w = id_path.ToStdWstring();
    m_lbl_reader->SetFile(str_w);
    m_lbl_reader->Preprocess();
    int lbl_chan = m_lbl_reader->GetChanNum();
    Nrrd* lbl_nrrd = m_lbl_reader->Convert(0, 0, true);
    void* lbl_data = lbl_nrrd->data;
    
    str_w = vol_path.ToStdWstring();
    m_vol_reader->SetFile(str_w);
    m_vol_reader->Preprocess();
    int vol_chan = m_vol_reader->GetChanNum();
    Nrrd* vol_nrrd1 = m_vol_reader->Convert(0, 0, true);
    void* vol_data1 = vol_nrrd1->data;
    Nrrd* vol_nrrd2 = m_vol_reader->Convert(0, 1, true);
    void* vol_data2 = vol_nrrd1->data;
    Nrrd* vol_nrrd3 = m_vol_reader->Convert(0, 2, true);
    void* vol_data3 = vol_nrrd1->data;
    
    int nx = vol_nrrd1->axis[0].size;
    int ny = vol_nrrd1->axis[1].size;
    int nz = vol_nrrd1->axis[2].size;
    
    int i, j, k;
    map<int, BBox> seg_bbox;
    map<int, size_t> seg_size;
    //first pass: finding BBox
    for (i=0; i<nx; i++)
    {
        for (j=0; j<ny; j++)
        {
            for (k=0; k<nz; k++)
            {
                int index = nx*ny*k + nx*j + i;
                int label = 0;
                if (lbl_nrrd->type == nrrdTypeUChar)
                    label = ((unsigned char*)lbl_data)[index];
                else if (lbl_nrrd->type == nrrdTypeUShort)
                    label = ((unsigned short*)lbl_data)[index];
                if (label > 0)
                {
                    if ( seg_bbox.find(label) == seg_bbox.end() )
                    {
                        BBox bbox;
                        bbox.extend(Point(i, j, k));
                        seg_bbox[label] = bbox;
                        seg_size[label] = 1;
                    }
                    else
                    {
                        seg_bbox[label].extend(Point(i, j, k));
                        seg_size[label]++;
                    }
                }
            }
        }
    }
    
    map<int, BBox>::iterator it = seg_bbox.begin();
    m_segs.clear();
    
    while (it != seg_bbox.end())
    {
        NASegment seg;
        seg.id = it->first;
        seg.size = seg_size[seg.id];
        seg.bbox = it->second;
        
        m_segs.push_back(seg);
    }
    
    std::sort(m_segs.begin(), m_segs.end(), NAGuiPlugin::sort_data_asc);
    
    //second pass: fill holes
    unsigned char *temp = new unsigned char[nx*ny*3];
    for (int s = 0; s < m_segs.size(); s++)
    {
        BBox bbox = m_segs[s].bbox;
        for (i=int(bbox.min().x()); i<=int(bbox.max().x()); i++)
        {
            for (j=int(bbox.min().y()); j<=int(bbox.max().y()); j++)
            {
                float rmax = 0.0f;
                float gmax = 0.0f;
                float bmax = 0.0f;
                for (k=int(bbox.min().z()); k<=int(bbox.max().z()); k++)
                {
                    int index = nx*ny*k + nx*j + i;
                    float sum = 0.0f;
                    
                }
            }
        }
    }
    delete [] temp;

	m_id_path = id_path;
	m_vol_path = vol_path;

	return true;
}

bool NAGuiPlugin::runNALoaderRemote(wxString url, wxString usr, wxString pwd, wxString outdir, wxString ofname)
{
	if (outdir.IsEmpty() || ofname.IsEmpty())
		return false;
	
	VRenderFrame *vframe = (VRenderFrame *)m_vvd;
	if (!vframe) return false;

	VolumeData *vd = vframe->GetCurSelVol();
	if (!vd || !vd->GetTexture() || !vd->GetVR()) return false;
	
	wxString tempvdpath = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH + "vvdnbtmpv.nrrd";
	wxString maskfname = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH + "vvdnbtmpv.msk";

	//save mask
	if (vd->GetTexture()->nmask() != -1)
	{
		vd->GetVR()->return_mask();
		Nrrd *data = vd->GetTexture()->get_nrrd(vd->GetTexture()->nmask());
		double spcx, spcy, spcz;
		vd->GetSpacings(spcx, spcy, spcz);

		if (data)
		{
			MSKWriter msk_writer;
			msk_writer.SetData(data);
			msk_writer.SetSpacings(spcx, spcy, spcz);
			msk_writer.Save(maskfname.ToStdWstring(), 0);
			wxRenameFile(maskfname, tempvdpath, true);
		}
	}
	else
		vd->Save(tempvdpath, 2, false, true, true, false); 

	wxString inurl = url + "opt/NBLAST/input/"; // "sftp://10.36.13.16:22//opt/NBLAST/";
	int len = 16;
	char s[17];
	const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	s[len] = 0;
	wxString randname(s);
	wxString upvolname = randname + ".nrrd";
	vframe->UploadFileRemote(inurl, upvolname, tempvdpath, usr, pwd);

	return true;
}

bool NAGuiPlugin::OnRun(wxString options)
{

	return true;
}

bool NAGuiPlugin::LoadSWC(wxString name, wxString swc_zip_path)
{
	wxFFileInputStream fis(swc_zip_path);
	if (!fis.IsOk()) return false;

	wxConvAuto conv;
	wxZipInputStream zis(fis, conv);
	if (!zis.IsOk()) return false;

	VRenderFrame *vframe = (VRenderFrame *)m_vvd;
	if (!vframe) return false;

	wxZipEntry *entry;
	wxString ename = name;

	if (ename.AfterLast(L'.') != wxT("swc"))
		ename += wxT(".swc");

	size_t esize = 0;
	while (entry = zis.GetNextEntry())
	{
		if (ename == entry->GetName())
		{
			esize = entry->GetSize();
			break;
		}
	}

	if (esize <= 0) return false;

	char *buff = new char[esize];
	zis.Read(buff, esize);

	wxString tempswcpath = wxStandardPaths::Get().GetTempDir() + wxFILE_SEP_PATH + ename;

	wxFile tmpswc(tempswcpath, wxFile::write);

	if (tmpswc.IsOpened())
	{
		tmpswc.Write(buff, esize);
		tmpswc.Close();
		wxArrayString arr;
		arr.Add(tempswcpath);
		vframe->LoadMeshes(arr);
		wxRemoveFile(tempswcpath);
	}

	delete [] buff;

	return true;
}

bool NAGuiPlugin::LoadFiles(wxString path)
{
	VRenderFrame *vframe = (VRenderFrame *)m_vvd;
	if (!vframe) return false;

	wxArrayString arr;
	arr.Add(path);
	vframe->StartupLoad(arr);

	return true;
}

wxString NAGuiPlugin::GetName() const
{
	return _("NBLAST Search");
}

wxString NAGuiPlugin::GetId() const
{
	return wxT("{EEFDF66-5FBB-4719-AF17-76C1C82D3FE1}");
}

wxWindow * NAGuiPlugin::CreatePanel(wxWindow * parent)
{
	return new NAGuiPluginWindow(this, parent);
}

void NAGuiPlugin::OnInit()
{
	LoadConfigFile();
}

void NAGuiPlugin::OnDestroy()
{

}

void NAGuiPlugin::OnTimer(wxTimerEvent& event)
{

}

void NAGuiPlugin::LoadConfigFile()
{
/*
	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(), NULL);
#ifdef _WIN32
	wxString dft = expath + "\\NA_plugin_settings.dft";
	if (!wxFileExists(dft))
		dft = wxStandardPaths::Get().GetUserConfigDir() + "\\NA_plugin_settings.dft";
#else
	wxString dft = expath + "/../Resources/NA_plugin_settings.dft";
#endif
	if (wxFileExists(dft))
	{
		wxFileInputStream is(dft);
		if (is.IsOk())
		{
			wxFileConfig fconfig(is);
			wxString str;
			if (fconfig.Read("R_path", &str))
				m_R_path = str;
			if (fconfig.Read("neuronlib_path", &str))
				m_nlib_path = str;
			if (fconfig.Read("output_dir", &str))
				m_out_dir = str;
			
			if (fconfig.Read("rnum", &str))
				m_rnum = str;
			else
				m_rnum = "10";

			bool bval = false;
			if (fconfig.Read("export_swc", &bval))
				m_exp_swc = bval;
			if (fconfig.Read("export_swcprev", &bval))
				m_exp_swcprev = bval;
			if (fconfig.Read("export_mip", &bval))
				m_exp_mip = bval;
			if (fconfig.Read("export_vol", &bval))
				m_exp_vol = bval;
			if (fconfig.Read("prefix_score", &bval))
				m_pfx_score = bval;
			if (fconfig.Read("prefix_database", &bval))
				m_pfx_db = bval;
			
			if (fconfig.Read("scoring_method", &str))
				m_scmtd = str;
		}
	}
 */
}

void NAGuiPlugin::SaveConfigFile()
{
/*
	wxFileConfig fconfig("NA plugin default settings");

	fconfig.Write("R_path", m_R_path);
	fconfig.Write("neuronlib_path", m_nlib_path);
	fconfig.Write("output_dir", m_out_dir);
	fconfig.Write("rnum", m_rnum);
	fconfig.Write("export_swc", m_exp_swc);
	fconfig.Write("export_swcprev", m_exp_swcprev);
	fconfig.Write("export_mip", m_exp_mip);
	fconfig.Write("export_vol", m_exp_vol);
	fconfig.Write("prefix_score", m_pfx_score);
	fconfig.Write("prefix_database", m_pfx_db);
	fconfig.Write("scoring_method", m_scmtd);

	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(),NULL);
#ifdef _WIN32
	wxString dft = expath + "\\NBLAST_plugin_settings.dft";
	wxString dft2 = wxStandardPaths::Get().GetUserConfigDir() + "\\NBLAST_plugin_settings.dft";
	if (!wxFileExists(dft) && wxFileExists(dft2))
		dft = dft2;
#else
	wxString dft = expath + "/../Resources/NBLAST_plugin_settings.dft";
#endif
	wxFileOutputStream os(dft);
	fconfig.Save(os);
 */
}
