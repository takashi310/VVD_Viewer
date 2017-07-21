#include "DataManager.h"
#include "teem/Nrrd/nrrd.h"
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/url.h>
#include <wx/file.h>
#include <wx/stdpaths.h>
#include "utility.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef _WIN32
#  undef min
#  undef max
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double TreeLayer::m_sw = 0.0;

TreeLayer::TreeLayer()
{
	type = -1;
	m_gamma = Color(1.0, 1.0, 1.0);
	m_brightness = Color(1.0, 1.0, 1.0);
	m_hdr = Color(0.0, 0.0, 0.0);
	m_sync_r = m_sync_g = m_sync_b = false;
}

TreeLayer::~TreeLayer()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VolumeData::VolumeData()
{
	m_reader = 0;

	m_dup = false;
	m_dup_counter = 0;

	type = 2;//volume

	//volume renderer and texture
	m_vr = 0;
	m_tex = 0;

	//current channel index
	m_chan = -1;
	m_time = 0;

	//mdoes
	m_mode = 0;
	//stream modes
	m_stream_mode = 0;

	//mask mode
	m_mask_mode = 0;
	m_use_mask_threshold = false;

	//volume properties
	m_scalar_scale = 1.0;
	m_gm_scale = 1.0;
	m_max_value = 255.0;
	//gamma
	m_gamma3d = 1.0;
	m_gm_thresh = 0.0;
	m_offset = 1.0;
	m_lo_thresh = 0.0;
	m_hi_thresh = 1.0;
	m_color = Color(1.0, 1.0, 1.0);
	SetHSV();
	m_alpha = 1.0;
	m_sample_rate = 1.0;
	m_mat_amb = 1.0;
	m_mat_diff = 1.0;
	m_mat_spec = 1.0;
	m_mat_shine = 10;
	//noise reduction
	m_noise_rd = false;
	//shading
	m_shading = false;
	//shadow
	m_shadow = false;
	m_shadow_darkness = 0.0;

	//resolution, scaling, spacing
	m_res_x = 0;	m_res_y = 0;	m_res_z = 0;
	m_sclx = 1.0;	m_scly = 1.0;	m_sclz = 1.0;
	m_spcx = 1.0;	m_spcy = 1.0;	m_spcz = 1.0;
	m_spc_from_file = false;

	//display control
	m_disp = true;
	m_draw_bounds = false;
	m_test_wiref = false;

	//colormap mode
	m_colormap_mode = 0;
	m_colormap_disp = false;
	m_colormap_low_value = 0.0;
	m_colormap_hi_value = 1.0;
	m_colormap = 0;
	m_colormap_proj = 0;
	m_id_col_disp_mode = 0;

	//blend mode
	m_blend_mode = 0;

	m_saved_mode = 0;

	m_2d_mask = 0;
	m_2d_weight1 = 0;
	m_2d_weight2 = 0;
	m_2d_dmap = 0;

	//clip distance
	m_clip_dist_x = 0;
	m_clip_dist_y = 0;
	m_clip_dist_z = 0;

	//legend
	m_legend = true;
	//interpolate
	m_interpolate = true;

	//valid brick number
	m_brick_num = 0;
}

/*
VolumeData::VolumeData(VolumeData &copy)
{
	m_reader = copy.m_reader;
	//duplication
	m_dup = true;
	copy.m_dup_counter++;
	m_dup_counter = copy.m_dup_counter;

	//layer properties
	type = 2;//volume
	SetName(copy.GetName()+wxString::Format("_%d", m_dup_counter));
	SetGamma(copy.GetGamma());
	SetBrightness(copy.GetBrightness());
	SetHdr(copy.GetHdr());
	SetSyncR(copy.GetSyncR());
	SetSyncG(copy.GetSyncG());
	SetSyncB(copy.GetSyncB());

	//path and bounds
	m_tex_path = copy.m_tex_path;
	m_bounds = copy.m_bounds;

	//volume renderer and texture
	m_vr = new VolumeRenderer(*copy.m_vr);
	m_tex = copy.m_tex;

	//current channel index
	m_chan = copy.m_chan;
	m_time = 0;

	//mdoes
	m_mode = copy.m_mode;
	//stream modes
	m_stream_mode = copy.m_stream_mode;

	//mask mode
	m_mask_mode = copy.m_mask_mode;
	m_use_mask_threshold = false;

	//volume properties
	m_scalar_scale = copy.m_scalar_scale;
	m_gm_scale = copy.m_gm_scale;
	m_max_value = copy.m_max_value;
	//gamma
	m_gamma3d = copy.m_gamma3d;
	m_gm_thresh = copy.m_gm_thresh;
	m_offset = copy.m_offset;
	m_lo_thresh = copy.m_lo_thresh;
	m_hi_thresh = copy.m_hi_thresh;
	m_color = copy.m_color;
	SetHSV();
	m_alpha = copy.m_alpha;
	m_sample_rate = copy.m_sample_rate;
	m_mat_amb = copy.m_mat_amb;
	m_mat_diff = copy.m_mat_diff;
	m_mat_spec = copy.m_mat_spec;
	m_mat_shine = copy.m_mat_shine;
	//noise reduction
	m_noise_rd = copy.m_noise_rd;
	//shading
	m_shading = copy.m_shading;
	//shadow
	m_shadow = copy.m_shadow;
	m_shadow_darkness = copy.m_shadow_darkness;

	//resolution, scaling, spacing
	m_res_x = copy.m_res_x;	m_res_y = copy.m_res_y;	m_res_z = copy.m_res_z;
	m_sclx = copy.m_sclx;	m_scly = copy.m_scly;	m_sclz = copy.m_sclz;
	m_spcx = copy.m_spcx;	m_spcy = copy.m_spcy;	m_spcz = copy.m_spcz;
	m_spc_from_file = copy.m_spc_from_file;

	//display control
	m_disp = copy.m_disp;
	m_draw_bounds = copy.m_draw_bounds;
	m_test_wiref = copy.m_test_wiref;

	//colormap mode
	m_colormap_mode = copy.m_colormap_mode;
	m_colormap_disp = copy.m_colormap_disp;
	m_colormap_low_value = copy.m_colormap_low_value;
	m_colormap_hi_value = copy.m_colormap_hi_value;
	m_colormap = copy.m_colormap;
	m_colormap_proj = copy.m_colormap_proj;
	m_id_col_disp_mode = copy.m_id_col_disp_mode;

	//blend mode
	m_blend_mode = copy.m_blend_mode;

	m_saved_mode = copy.m_saved_mode;

	m_2d_mask = 0;
	m_2d_weight1 = 0;
	m_2d_weight2 = 0;
	m_2d_dmap = 0;

	//clip distance
	m_clip_dist_x = 0;
	m_clip_dist_y = 0;
	m_clip_dist_z = 0;

	//compression
	m_compression = false;

	//skip brick
	m_skip_brick = false;

	//legend
	m_legend = true;

	//interpolate
	m_interpolate = copy.m_interpolate;

	//valid brick number
	m_brick_num = 0;

	m_annotation = copy.m_annotation;

	m_landmarks = copy.m_landmarks;
}
*/

VolumeData::~VolumeData()
{
	//m_vr�̊J�������ɂ��Ȃ���loadedbrks���̗v�f�N���A���ł��Ȃ�
	if (m_vr)
		delete m_vr;
	if (m_tex)
		delete m_tex;
}

VolumeData* VolumeData::DeepCopy(VolumeData &copy, bool use_default_settings, DataManager *d_manager)
{
	VolumeData* vd = new VolumeData();

	vd->m_reader = copy.m_reader;
	//duplication
	vd->m_dup = true;
	copy.m_dup_counter++;
	vd->m_dup_counter = copy.m_dup_counter;

	Texture *tex = copy.GetTexture();
	if (!tex)
	{
		delete(vd);
		return NULL;
	}
	bool is_brxml = tex->isBrxml();
	int time = is_brxml ? 0 : copy.GetCurTime();
	Nrrd *nv = vd->m_reader->Convert(time, copy.GetCurChannel(), true);
	if (!nv)
	{
		delete(vd);
		return NULL;
	}

	if (is_brxml)	vd->Load(nv, copy.GetName()+wxString::Format("_%d", vd->m_dup_counter), wxString(""), (BRKXMLReader*)vd->m_reader);
	else vd->Load(nv, copy.GetName()+wxString::Format("_%d", vd->m_dup_counter), wxString(""));

	if (copy.GetMask(true))
	{
		Nrrd *mask;
		mask = nrrdNew();
		nrrdCopy(mask, copy.GetMask(false));
		vd->LoadMask(mask);
	}

	if (copy.GetLabel(true))
	{
		Nrrd *label;
		label = nrrdNew();
		nrrdCopy(label, copy.GetLabel(false));
		vd->LoadLabel(label);
	}

	if (use_default_settings && d_manager)
	{
		if (is_brxml) ((BRKXMLReader*)vd->m_reader)->SetLevel(0);
		vd->SetBaseSpacings(vd->m_reader->GetXSpc(),
			vd->m_reader->GetYSpc(),
			vd->m_reader->GetZSpc());
		bool valid_spc = vd->m_reader->IsSpcInfoValid();
		vd->SetSpcFromFile(valid_spc);
		vd->SetScalarScale(vd->m_reader->GetScalarScale());
		vd->SetMaxValue(vd->m_reader->GetMaxValue());
		vd->SetCurTime(time);
		vd->SetCurChannel(copy.GetCurChannel());

		vd->SetCompression(d_manager->GetCompression());
		d_manager->SetVolumeDefault(vd);

		//get excitation wavelength
		double wavelength = vd->m_reader->GetExcitationWavelength(copy.GetCurChannel());
		if (wavelength > 0.0) {
			FLIVR::Color col = d_manager->GetWavelengthColor(wavelength);
			vd->SetColor(col);
		}
		else if (wavelength < 0.) {
			FLIVR::Color white = Color(1.0, 1.0, 1.0);
			vd->SetColor(white);
		}
		else
		{
			FLIVR::Color white = Color(1.0, 1.0, 1.0);
			FLIVR::Color red   = Color(1.0, 0.0, 0.0);
			FLIVR::Color green = Color(0.0, 1.0, 0.0);
			FLIVR::Color blue  = Color(0.0, 0.0, 1.0);
			if (vd->m_reader->GetChanNum() == 1) {
				vd->SetColor(white);
			}
			else
			{
				if (copy.GetCurChannel() == 0)
					vd->SetColor(red);
				else if (copy.GetCurChannel() == 1)
					vd->SetColor(green);
				else if (copy.GetCurChannel() == 2)
					vd->SetColor(blue);
				else
					vd->SetColor(white);
			}
		}
	}
	else
	{
		//layer properties
		vd->type = 2;//volume
		vd->SetGamma(copy.GetGamma());
		vd->SetBrightness(copy.GetBrightness());
		vd->SetHdr(copy.GetHdr());
		vd->SetSyncR(copy.GetSyncR());
		vd->SetSyncG(copy.GetSyncG());
		vd->SetSyncB(copy.GetSyncB());

		double spc[3];
		if (tex->isBrxml())
		{
			copy.GetBaseSpacings(spc[0], spc[1], spc[2]);
			vd->SetBaseSpacings(spc[0], spc[1], spc[2]);
			copy.GetSpacingScales(spc[0], spc[1], spc[2]);
			vd->SetSpacingScales(spc[0], spc[1], spc[2]);
		}
		else
		{
			copy.GetSpacings(spc[0], spc[1], spc[2]);
			vd->SetSpacings(spc[0], spc[1], spc[2]);
		}
		double scl[3];
		copy.GetScalings(scl[0], scl[1], scl[2]);
		vd->SetScalings(scl[0], scl[1], scl[2]);

		vd->SetColor(copy.GetColor());
		bool bval = copy.GetEnableAlpha();
		vd->SetEnableAlpha(copy.GetEnableAlpha());
		vd->SetShading(copy.GetShading());
		vd->SetShadow(copy.GetShadow());
		double darkness;
		copy.GetShadowParams(darkness);
		vd->SetShadowParams(darkness);
		//other settings
		double amb, diff, spec, shine;
		copy.GetMaterial(amb, diff, spec, shine);
		vd->Set3DGamma(copy.Get3DGamma());
		vd->SetBoundary(copy.GetBoundary());
		vd->SetOffset(copy.GetOffset());
		vd->SetLeftThresh(copy.GetLeftThresh());
		vd->SetRightThresh(copy.GetRightThresh());
		vd->SetAlpha(copy.GetAlpha());
		vd->SetSampleRate(copy.GetSampleRate());
		vd->SetMaterial(amb, diff, spec, shine);

		//current channel index
		vd->m_chan = copy.m_chan;
		vd->m_time = time;

		//modes
		vd->SetMode(copy.GetMode());
		//stream modes
		vd->SetStreamMode(copy.GetStreamMode());

		//volume properties
		vd->SetScalarScale(copy.GetScalarScale());
		vd->SetGMScale(copy.GetGMScale());
		vd->SetHSV();

		//noise reduction
		vd->SetNR(copy.GetNR());

		//display control
		vd->SetDisp(copy.GetDisp());
		vd->SetDrawBounds(copy.GetDrawBounds());
		vd->m_test_wiref = copy.m_test_wiref;

		//colormap mode
		vd->SetColormapMode(copy.GetColormapMode());
		vd->SetColormapDisp(copy.GetColormapDisp());
		vd->SetColormapValues(copy.m_colormap_low_value, copy.m_colormap_hi_value);
		vd->m_colormap = copy.m_colormap;
		vd->m_colormap_proj = copy.m_colormap_proj;

		//blend mode
		vd->SetBlendMode(copy.GetBlendMode());

		vd->m_2d_mask = 0;
		vd->m_2d_weight1 = 0;
		vd->m_2d_weight2 = 0;
		vd->m_2d_dmap = 0;

		//clip distance
		int dists[3];
		copy.GetClipDistance(dists[0], dists[1], dists[2]);
		vd->SetClipDistance(dists[0], dists[1], dists[2]);

		//compression
		vd->m_compression = false;

		//skip brick
		vd->m_skip_brick = false;

		//legend
		vd->m_legend = true;

		//interpolate
		vd->m_interpolate = copy.m_interpolate;

		vd->m_annotation = copy.m_annotation;

		vd->m_landmarks = copy.m_landmarks;
	}

	return vd;
}

//duplication
bool VolumeData::GetDup()
{
	return m_dup;
}

//increase duplicate counter
void VolumeData::IncDupCounter()
{
	m_dup_counter++;
}

//data related
//compression
void VolumeData::SetCompression(bool compression)
{
	m_compression = compression;
	if (m_vr)
		m_vr->set_compression(compression);
}

bool VolumeData::GetCompression()
{
	return m_compression;
}

//skip brick
void VolumeData::SetSkipBrick(bool skip)
{
	m_skip_brick = skip;
}

bool VolumeData::GetSkipBrick()
{
	return m_skip_brick;
}

int VolumeData::Load(Nrrd* data, const wxString &name, const wxString &path, BRKXMLReader *breader)
{
	if (!data || data->dim!=3)
		return 0;

	m_tex_path = path;
	m_name = name;

	if (m_tex)
	{
		delete m_tex;
		m_tex = NULL;
	}

	Nrrd *nv = data;
	Nrrd *gm = 0;
	m_res_x = nv->axis[0].size;
	m_res_y = nv->axis[1].size;
	m_res_z = nv->axis[2].size;

	BBox bounds;
	Point pmax(data->axis[0].max, data->axis[1].max, data->axis[2].max);
	Point pmin(data->axis[0].min, data->axis[1].min, data->axis[2].min);
	bounds.extend(pmin);
	bounds.extend(pmax);
	m_bounds = bounds;

	m_tex = new Texture();
	m_tex->set_use_priority(m_skip_brick);
	if(breader)
	{
		vector<Pyramid_Level> pyramid;
		vector<vector<vector<vector<FileLocInfo *>>>> fnames;
		int ftype = BRICK_FILE_TYPE_NONE;

		breader->build_pyramid(pyramid, fnames, 0, breader->GetCurChan());
		m_tex->SetCopyableLevel(breader->GetCopyableLevel());

		int lmnum = breader->GetLandmarkNum();
		for (int j = 0; j < lmnum; j++)
		{
			wstring name;
			VD_Landmark vlm;
			breader->GetLandmark(j, vlm.name, vlm.x, vlm.y, vlm.z, vlm.spcx, vlm.spcy, vlm.spcz);
			m_landmarks.push_back(vlm);
			breader->GetMetadataID(m_metadata_id);
		}
		if (!m_tex->buildPyramid(pyramid, fnames, breader->isURL())) return 0;
	}
	else if(!m_tex->build(nv, gm, 0, 256, 0, 0)) return 0;
	
	if (m_tex)
	{
		if (m_vr)
			delete m_vr;

		vector<Plane*> planelist(0);
		Plane* plane = 0;
		//x
		plane = new Plane(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
		planelist.push_back(plane);
		plane = new Plane(Point(1.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.0));
		planelist.push_back(plane);
		//y
		plane = new Plane(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0));
		planelist.push_back(plane);
		plane = new Plane(Point(0.0, 1.0, 0.0), Vector(0.0, -1.0, 0.0));
		planelist.push_back(plane);
		//z
		plane = new Plane(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0));
		planelist.push_back(plane);
		plane = new Plane(Point(0.0, 0.0, 1.0), Vector(0.0, 0.0, -1.0));
		planelist.push_back(plane);

		m_vr = new VolumeRenderer(m_tex, planelist, true);
		m_vr->set_sampling_rate(m_sample_rate);
		m_vr->set_material(m_mat_amb, m_mat_diff, m_mat_spec, m_mat_shine);
		m_vr->set_shading(true);
		m_vr->set_scalar_scale(m_scalar_scale);
		m_vr->set_gm_scale(m_scalar_scale);

		SetMode(m_mode);

		if (breader)
		{
			wstring tree = breader->GetROITree();
			if (!tree.empty())
			{
				ImportROITree(tree);
				SetColormapMode(3);//ID color mode
				SelectAllNamedROI();
			}
		}
	}

	return 1;
}

int VolumeData::Replace(Nrrd* data, bool del_tex)
{
	if (!data || data->dim!=3)
		return 0;

	if (del_tex)
	{
		Nrrd *nv = data;
		Nrrd *gm = 0;
		m_res_x = nv->axis[0].size;
		m_res_y = nv->axis[1].size;
		m_res_z = nv->axis[2].size;

		if (m_tex)
			delete m_tex;
		m_tex = new Texture();
		m_tex->set_use_priority(m_skip_brick);
		m_tex->build(nv, gm, 0, m_max_value, 0, 0);
	}
	else
	{
		//set new
		m_tex->set_nrrd(data, 0);
	}

	//clear pool
	if (m_vr)
		m_vr->set_texture(m_tex);
	else
		return 0;

	return 1;
}

int VolumeData::Replace(VolumeData* data)
{
	if (!data ||
		m_res_x!=data->m_res_x ||
		m_res_y!=data->m_res_y ||
		m_res_z!=data->m_res_z)
		return 0;

	double spcx = 1.0, spcy = 1.0, spcz = 1.0;

	if (m_tex && m_vr)
	{
		m_tex->get_spacings(spcx, spcy, spcz);
		m_vr->clear_tex_current();
		delete m_tex;
		m_vr->reset_texture();
	}
	m_tex = data->GetTexture();
	data->SetTexture();
	SetScalarScale(data->GetScalarScale());
	SetGMScale(data->GetGMScale());
	SetMaxValue(data->GetMaxValue());
	if (m_vr)
		m_vr->set_texture(m_tex);
	else
		return 0;
	return 1;
}

//volume data
void VolumeData::AddEmptyData(int bits,
	int nx, int ny, int nz,
	double spcx, double spcy, double spcz)
{
	if (bits!=8 && bits!=16)
		return;

	if (m_vr)
		delete m_vr;
	if (m_tex)
		delete m_tex;

	Nrrd *nv = nrrdNew();
	if (bits == 8)
	{
		unsigned long long mem_size = (unsigned long long)nx*
			(unsigned long long)ny*(unsigned long long)nz;
		uint8 *val8 = new (std::nothrow) uint8[mem_size];
		if (!val8)
		{
			wxMessageBox("Not enough memory. Please save project and restart.");
			return;
		}
		memset((void*)val8, 0, sizeof(uint8)*nx*ny*nz);
		nrrdWrap(nv, val8, nrrdTypeUChar, 3, (size_t)nx, (size_t)ny, (size_t)nz);
	}
	else if (bits == 16)
	{
		unsigned long long mem_size = (unsigned long long)nx*
			(unsigned long long)ny*(unsigned long long)nz;
		uint16 *val16 = new (std::nothrow) uint16[mem_size];
		if (!val16)
		{
			wxMessageBox("Not enough memory. Please save project and restart.");
			return;
		}
		memset((void*)val16, 0, sizeof(uint16)*nx*ny*nz);
		nrrdWrap(nv, val16, nrrdTypeUShort, 3, (size_t)nx, (size_t)ny, (size_t)nz);
	}
	nrrdAxisInfoSet(nv, nrrdAxisInfoSpacing, spcx, spcy, spcz);
	nrrdAxisInfoSet(nv, nrrdAxisInfoMax, spcx*nx, spcy*ny, spcz*nz);
	nrrdAxisInfoSet(nv, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
	nrrdAxisInfoSet(nv, nrrdAxisInfoSize, (size_t)nx, (size_t)ny, (size_t)nz);

	//resolution
	m_res_x = nv->axis[0].size;
	m_res_y = nv->axis[1].size;
	m_res_z = nv->axis[2].size;

	//bounding box
	BBox bounds;
	Point pmax(nv->axis[0].max, nv->axis[1].max, nv->axis[2].max);
	Point pmin(nv->axis[0].min, nv->axis[1].min, nv->axis[2].min);
	bounds.extend(pmin);
	bounds.extend(pmax);
	m_bounds = bounds;

	//create texture
	m_tex = new Texture();
	m_tex->set_use_priority(false);
	m_tex->build(nv, 0, 0, 256, 0, 0);
	m_tex->set_spacings(spcx, spcy, spcz);

	//clipping planes
	vector<Plane*> planelist(0);
	Plane* plane = 0;
	//x
	plane = new Plane(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
	planelist.push_back(plane);
	plane = new Plane(Point(1.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.0));
	planelist.push_back(plane);
	//y
	plane = new Plane(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0));
	planelist.push_back(plane);
	plane = new Plane(Point(0.0, 1.0, 0.0), Vector(0.0, -1.0, 0.0));
	planelist.push_back(plane);
	//z
	plane = new Plane(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0));
	planelist.push_back(plane);
	plane = new Plane(Point(0.0, 0.0, 1.0), Vector(0.0, 0.0, -1.0));
	planelist.push_back(plane);

	//create volume renderer
	m_vr = new VolumeRenderer(m_tex, planelist, true);
	m_vr->set_sampling_rate(m_sample_rate);
	m_vr->set_material(m_mat_amb, m_mat_diff, m_mat_spec, m_mat_shine);
	m_vr->set_shading(true);
	m_vr->set_scalar_scale(m_scalar_scale);
	m_vr->set_gm_scale(m_scalar_scale);

	SetMode(m_mode);
}

//volume mask
void VolumeData::LoadMask(Nrrd* mask)
{
	if (!mask || !m_tex || !m_vr)
		return;

	//prepare the texture bricks for the mask
	m_tex->add_empty_mask();
	m_tex->set_nrrd(mask, m_tex->nmask());
}

void VolumeData::DeleteMask()
{
	if (!m_tex || !m_vr)
		return;

	m_tex->delete_mask();
}

void VolumeData::AddEmptyMask()
{
	if (!m_tex || !m_vr)
		return;

	//prepare the texture bricks for the mask
	if (m_tex->add_empty_mask())
	{
		//add the nrrd data for mask
		Nrrd *nrrd_mask = nrrdNew();
		unsigned long long mem_size = (unsigned long long)m_res_x*
			(unsigned long long)m_res_y*(unsigned long long)m_res_z;
		uint8 *val8 = new (std::nothrow) uint8[mem_size];
		if (!val8)
		{
			wxMessageBox("Not enough memory. Please save project and restart.");
			return;
		}
		double spcx, spcy, spcz;
		m_tex->get_spacings(spcx, spcy, spcz);
		memset((void*)val8, 0, mem_size*sizeof(uint8));
		nrrdWrap(nrrd_mask, val8, nrrdTypeUChar, 3, (size_t)m_res_x, (size_t)m_res_y, (size_t)m_res_z);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoSize, (size_t)m_res_x, (size_t)m_res_y, (size_t)m_res_z);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoSpacing, spcx, spcy, spcz);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoMax, spcx*m_res_x, spcy*m_res_y, spcz*m_res_z);

		m_tex->set_nrrd(nrrd_mask, m_tex->nmask());
	}
}

//volume label
void VolumeData::LoadLabel(Nrrd* label)
{
	if (!m_tex || !m_vr)
		return;

	m_tex->add_empty_label();
	m_tex->set_nrrd(label, m_tex->nlabel());
}

void VolumeData::DeleteLabel()
{
	if (!m_tex || !m_vr)
		return;

	m_tex->delete_label();
}

void VolumeData::SetOrderedID(unsigned int* val)
{
	for (int i=0; i<m_res_x; i++)
		for (int j=0; j<m_res_y; j++)
			for (int k=0; k<m_res_z; k++)
			{
				unsigned int index = m_res_y*m_res_z*i + m_res_z*j + k;
				val[index] = index+1;
			}
}

void VolumeData::SetReverseID(unsigned int* val)
{
	for (int i=0; i<m_res_x; i++)
		for (int j=0; j<m_res_y; j++)
			for (int k=0; k<m_res_z; k++)
			{
				unsigned int index = m_res_y*m_res_z*i + m_res_z*j + k;
				val[index] = m_res_x*m_res_y*m_res_z - index;
			}
}

void VolumeData::SetShuffledID(unsigned int* val)
{
	unsigned int x, y, z;
	unsigned int res;
	unsigned int len = 0;
	unsigned int r = Max(m_res_x, Max(m_res_y, m_res_z));
	while (r > 0)
	{
		r /= 2;
		len++;
	}
	for (int i=0; i<m_res_x; i++)
		for (int j=0; j<m_res_y; j++)
			for (int k=0; k<m_res_z; k++)
			{
				x = reverse_bit(i, len);
				y = reverse_bit(j, len);
				z = reverse_bit(k, len);
				res = 0;
				for (unsigned int ii=0; ii<len; ii++)
				{
					res |= (1<<ii & x)<<(2*ii);
					res |= (1<<ii & y)<<(2*ii+1);
					res |= (1<<ii & z)<<(2*ii+2);
				}
				unsigned int index = m_res_x*m_res_y*k + m_res_x*j + i;
				val[index] = m_res_x*m_res_y*m_res_z - res;
			}
}

void VolumeData::AddEmptyLabel(int mode)
{
	if (!m_tex || !m_vr)
		return;

	Nrrd *nrrd_label = 0;
	unsigned int *val32 = 0;
	//prepare the texture bricks for the labeling mask
	if (m_tex->add_empty_label())
	{
		//add the nrrd data for the labeling mask
		nrrd_label = nrrdNew();
		unsigned long long mem_size = (unsigned long long)m_res_x*
			(unsigned long long)m_res_y*(unsigned long long)m_res_z;
		val32 = new (std::nothrow) unsigned int[mem_size];
		if (!val32)
		{
			wxMessageBox("Not enough memory. Please save project and restart.");
			return;
		}

		double spcx, spcy, spcz;
		m_tex->get_spacings(spcx, spcy, spcz);
		nrrdWrap(nrrd_label, val32, nrrdTypeUInt, 3, (size_t)m_res_x, (size_t)m_res_y, (size_t)m_res_z);
		nrrdAxisInfoSet(nrrd_label, nrrdAxisInfoSpacing, spcx, spcy, spcz);
		nrrdAxisInfoSet(nrrd_label, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
		nrrdAxisInfoSet(nrrd_label, nrrdAxisInfoMax, spcx*m_res_x, spcy*m_res_y, spcz*m_res_z);
		nrrdAxisInfoSet(nrrd_label, nrrdAxisInfoSize, (size_t)m_res_x, (size_t)m_res_y, (size_t)m_res_z);

		m_tex->set_nrrd(nrrd_label, m_tex->nlabel());
	}
	else
	{
		nrrd_label = m_tex->get_nrrd(m_tex->nlabel());
		val32 = (unsigned int*)nrrd_label->data;
	}

	//apply values
	switch (mode)
	{
	case 0://zeros
		memset(val32, 0, sizeof(unsigned int)*(unsigned long long)m_res_x*
			(unsigned long long)m_res_y*(unsigned long long)m_res_z);
		break;
	case 1://ordered
		SetOrderedID(val32);
		break;
	case 2://shuffled
		SetShuffledID(val32);
		break;
	}

}

bool VolumeData::SearchLabel(unsigned int label)
{
	if (!m_tex)
		return false;

	Nrrd* nrrd_label = m_tex->get_nrrd(m_tex->nlabel());
	if (!nrrd_label)
		return false;
	unsigned int* data_label = (unsigned int*)(nrrd_label->data);
	if (!data_label)
		return false;

	unsigned long long for_size = (unsigned long long)m_res_x *
		(unsigned long long)m_res_y * (unsigned long long)m_res_z;
	for (unsigned long long index = 0; index < for_size; ++index)
		if (data_label[index] == label)
			return true;
	return false;
}

Nrrd* VolumeData::GetVolume(bool ret)
{
	if (m_vr && m_tex)
	{
		if (ret) m_vr->return_volume();
		return m_tex->get_nrrd(0);
	}

	return 0;
}

Nrrd* VolumeData::GetMask(bool ret)
{
	if (m_vr && m_tex && m_tex->nmask()!=-1)
	{
		if (ret) m_vr->return_mask();
		return m_tex->get_nrrd(m_tex->nmask());
	}

	return 0;
}

Nrrd* VolumeData::GetLabel(bool ret)
{
	if (m_vr && m_tex && m_tex->nlabel() != -1)
	{
		if (ret) m_vr->return_label();
		return m_tex->get_nrrd(m_tex->nlabel());
	}

	return 0;
}

double VolumeData::GetOriginalValue(int i, int j, int k, bool normalize)
{
	Nrrd* data = m_tex->get_nrrd(0);
	if (!data) return 0.0;

	int bits = data->type;
	int64_t nx = (int64_t)(data->axis[0].size);
	int64_t ny = (int64_t)(data->axis[1].size);
	int64_t nz = (int64_t)(data->axis[2].size);

	if (i<0 || i>=nx || j<0 || j>=ny || k<0 || k>=nz)
		return 0.0;
	uint64_t ii = i, jj = j, kk = k;

	if (!m_tex->isBrxml())
	{
		if (!data->data) return 0.0;
		if (bits == nrrdTypeUChar)
		{
			uint64_t index = (nx)*(ny)*(kk) + (nx)*(jj) + (ii);
			uint8 old_value = ((uint8*)(data->data))[index];
			return normalize ? double(old_value)/255.0 : double(old_value);
		}
		else if (bits == nrrdTypeUShort)
		{
			uint64_t index = (nx)*(ny)*(kk) + (nx)*(jj) + (ii);
			uint16 old_value = ((uint16*)(data->data))[index];
			return normalize ? double(old_value)*m_scalar_scale/65535.0 : double(old_value);
		}
	}
	else
	{
		double rval = 0.0;
		rval = m_tex->get_brick_original_value(ii, jj, kk, normalize);
		if (bits == nrrdTypeUShort && normalize)
			rval *= m_scalar_scale;
		return rval;
	}

	return 0.0;
}

double VolumeData::GetTransferedValue(int i, int j, int k)
{
	Nrrd* data = m_tex->get_nrrd(0);
	if (!data) return 0.0;

	int bits = data->type;
	int64_t nx = (int64_t)(data->axis[0].size);
	int64_t ny = (int64_t)(data->axis[1].size);
	int64_t nz = (int64_t)(data->axis[2].size);
	if (i<0 || i>=nx || j<0 || j>=ny || k<0 || k>=nz)
		return 0.0;
	int64_t ii = i, jj = j, kk = k;

	double v1, v2, v3, v4, v5, v6;
	if (bits == nrrdTypeUChar)
	{
		double old_value;
		if (!m_tex->isBrxml())
		{
			if (!data->data) return 0.0;
			uint64_t index = nx*ny*kk + nx*jj + ii;
			old_value = (double)((uint8*)(data->data))[index];
		}
		else
			old_value = m_tex->get_brick_original_value(ii, jj, kk, false);

		double gm = 0.0;
		double new_value = old_value/255.0;
		if (m_vr->get_inversion())
			new_value = 1.0-new_value;
		if (i>0 && i<nx-1 &&
			j>0 && j<ny-1 &&
			k>0 && k<nz-1)
		{
			if (!m_tex->isBrxml())
			{
				v1 = ((uint8*)(data->data))[nx*ny*kk + nx*jj + ii-1];
				v2 = ((uint8*)(data->data))[nx*ny*kk + nx*jj + ii+1];
				v3 = ((uint8*)(data->data))[nx*ny*kk + nx*(jj-1) + ii];
				v4 = ((uint8*)(data->data))[nx*ny*kk + nx*(jj+1) + ii];
				v5 = ((uint8*)(data->data))[nx*ny*(kk-1) + nx*jj + ii];
				v6 = ((uint8*)(data->data))[nx*ny*(kk+1) + nx*jj + ii];
			}
			else
			{
				v1 = m_tex->get_brick_original_value(ii-1, jj, kk, false);
				v2 = m_tex->get_brick_original_value(ii+1, jj, kk, false);
				v3 = m_tex->get_brick_original_value(ii, jj-1, kk, false);
				v4 = m_tex->get_brick_original_value(ii, jj+1, kk, false);
				v5 = m_tex->get_brick_original_value(ii, jj, kk-1, false);
				v6 = m_tex->get_brick_original_value(ii, jj, kk+1, false);
			}
			double normal_x, normal_y, normal_z;
			normal_x = (v2 - v1) / 255.0;
			normal_y = (v4 - v3) / 255.0;
			normal_z = (v6 - v5) / 255.0;
			gm = sqrt(normal_x*normal_x + normal_y*normal_y + normal_z*normal_z)*0.53;
		}
		if (new_value<m_lo_thresh-m_sw ||
			new_value>m_hi_thresh+m_sw ||
			gm<m_gm_thresh)
			new_value = 0.0;
		else
		{
			double gamma = 1.0 / m_gamma3d;
			new_value = (new_value<m_lo_thresh?
				(m_sw-m_lo_thresh+new_value)/m_sw:
			(new_value>m_hi_thresh?
				(m_sw-new_value+m_hi_thresh)/m_sw:1.0))
				*new_value;
			new_value = pow(Clamp(new_value/m_offset,
				gamma<1.0?-(gamma-1.0)*0.00001:0.0,
				gamma>1.0?0.9999:1.0), gamma);
			new_value *= m_alpha;
		}
		return new_value;
	}
	else if (bits == nrrdTypeUShort)
	{
		uint64_t index = nx*ny*kk + nx*jj + ii;
		double old_value;
		if (!m_tex->isBrxml())
		{
			if (!data->data) return 0.0;
			uint64_t index = nx*ny*kk + nx*jj + ii;
			old_value = (double)((uint16*)(data->data))[index];
		}
		else
			old_value = m_tex->get_brick_original_value(ii, jj, kk, false);

		double gm = 0.0;
		double new_value = double(old_value)*m_scalar_scale/65535.0;
		if (m_vr->get_inversion())
			new_value = 1.0-new_value;
		if (ii>0 && ii<nx-1 &&
			jj>0 && jj<ny-1 &&
			kk>0 && kk<nz-1)
		{
			if (!m_tex->isBrxml())
			{
				v1 = ((uint16*)(data->data))[nx*ny*kk + nx*jj + ii-1];
				v2 = ((uint16*)(data->data))[nx*ny*kk + nx*jj + ii+1];
				v3 = ((uint16*)(data->data))[nx*ny*kk + nx*(jj-1) + ii];
				v4 = ((uint16*)(data->data))[nx*ny*kk + nx*(jj+1) + ii];
				v5 = ((uint16*)(data->data))[nx*ny*(kk-1) + nx*jj + ii];
				v6 = ((uint16*)(data->data))[nx*ny*(kk+1) + nx*jj + ii];
			}
			else
			{
				v1 = m_tex->get_brick_original_value(ii-1, jj, kk, false);
				v2 = m_tex->get_brick_original_value(ii+1, jj, kk, false);
				v3 = m_tex->get_brick_original_value(ii, jj-1, kk, false);
				v4 = m_tex->get_brick_original_value(ii, jj+1, kk, false);
				v5 = m_tex->get_brick_original_value(ii, jj, kk-1, false);
				v6 = m_tex->get_brick_original_value(ii, jj, kk+1, false);
			}
			double normal_x, normal_y, normal_z;
			normal_x = (v2 - v1)*m_scalar_scale/65535.0;
			normal_y = (v4 - v3)*m_scalar_scale/65535.0;
			normal_z = (v6 - v5)*m_scalar_scale/65535.0;
			gm = sqrt(normal_x*normal_x + normal_y*normal_y + normal_z*normal_z)*0.53;
		}
		if (new_value<m_lo_thresh-m_sw ||
			new_value>m_hi_thresh+m_sw ||
			gm<m_gm_thresh)
			new_value = 0.0;
		else
		{
			double gamma = 1.0 / m_gamma3d;
			new_value = (new_value<m_lo_thresh?
				(m_sw-m_lo_thresh+new_value)/m_sw:
			(new_value>m_hi_thresh?
				(m_sw-new_value+m_hi_thresh)/m_sw:1.0))
				*new_value;
			new_value = pow(Clamp(new_value/m_offset,
				gamma<1.0?-(gamma-1.0)*0.00001:0.0,
				gamma>1.0?0.9999:1.0), gamma);
			new_value *= m_alpha;
		}
		return new_value;
	}

	return 0.0;
}

//save
void VolumeData::Save(wxString &filename, int mode, bool bake, bool compress, bool save_msk, bool save_label)
{
	if (m_vr && m_tex)
	{
		Nrrd* data = 0;

		BaseWriter *writer = 0;
		switch (mode)
		{
		case 0://multi-page tiff
			writer = new TIFWriter();
			break;
		case 1://single-page tiff sequence
			writer = new TIFWriter();
			break;
		case 2://nrrd
			writer = new NRRDWriter();
			break;
		}

		double spcx, spcy, spcz;
		GetSpacings(spcx, spcy, spcz);

		//save data
		data = m_tex->get_nrrd(0);
		if (data)
		{
			if (bake)
			{
				wxProgressDialog *prg_diag = new wxProgressDialog(
					"FluoRender: Baking volume data...",
					"Baking volume data. Please wait.",
					100, 0, wxPD_SMOOTH|wxPD_ELAPSED_TIME|wxPD_AUTO_HIDE);

				//process the data
				int bits = data->type==nrrdTypeUShort?16:8;
				int nx = int(data->axis[0].size);
				int ny = int(data->axis[1].size);
				int nz = int(data->axis[2].size);

				//clipping planes
				vector<Plane*> *planes = m_vr->get_planes();

				Nrrd* baked_data = nrrdNew();
				if (bits == 8)
				{
					unsigned long long mem_size = (unsigned long long)nx*
						(unsigned long long)ny*(unsigned long long)nz;
					uint8 *val8 = new (std::nothrow) uint8[mem_size];
					if (!val8)
					{
						wxMessageBox("Not enough memory. Please save project and restart.");
						return;
					}
					//transfer function
					for (int i=0; i<nx; i++)
					{
						prg_diag->Update(95*(i+1)/nx);
						for (int j=0; j<ny; j++)
							for (int k=0; k<nz; k++)
							{
								int index = nx*ny*k + nx*j + i;
								bool clipped = false;
								Point p(double(i) / double(nx),
									double(j) / double(ny),
									double(k) / double(nz));
								for (int pi = 0; pi < 6; ++pi)
								{
									if ((*planes)[pi] &&
										(*planes)[pi]->eval_point(p) < 0.0)
									{
										val8[index] = 0;
										clipped = true;
									}
								}
								if (!clipped)
								{
									double new_value = GetTransferedValue(i, j, k);
									val8[index] = uint8(new_value*255.0);
								}
							}
					}
					nrrdWrap(baked_data, val8, nrrdTypeUChar, 3, (size_t)nx, (size_t)ny, (size_t)nz);
				}
				else if (bits == 16)
				{
					unsigned long long mem_size = (unsigned long long)nx*
						(unsigned long long)ny*(unsigned long long)nz;
					uint16 *val16 = new (std::nothrow) uint16[mem_size];
					if (!val16)
					{
						wxMessageBox("Not enough memory. Please save project and restart.");
						return;
					}
					//transfer function
					for (int i=0; i<nx; i++)
					{
						prg_diag->Update(95*(i+1)/nx);
						for (int j=0; j<ny; j++)
							for (int k=0; k<nz; k++)
							{
								int index = nx*ny*k + nx*j + i;
								bool clipped = false;
								Point p(double(i) / double(nx),
									double(j) / double(ny),
									double(k) / double(nz));
								for (int pi = 0; pi < 6; ++pi)
								{
									if ((*planes)[pi] &&
										(*planes)[pi]->eval_point(p) < 0.0)
									{
										val16[index] = 0;
										clipped = true;
									}
								}
								if (!clipped)
								{
									double new_value = GetTransferedValue(i, j, k);
									val16[index] = uint16(new_value*65535.0);
								}
							}
					}
					nrrdWrap(baked_data, val16, nrrdTypeUShort, 3, (size_t)nx, (size_t)ny, (size_t)nz);
				}
				nrrdAxisInfoSet(baked_data, nrrdAxisInfoSpacing, spcx, spcy, spcz);
				nrrdAxisInfoSet(baked_data, nrrdAxisInfoMax, spcx*nx, spcy*ny, spcz*nz);
				nrrdAxisInfoSet(baked_data, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
				nrrdAxisInfoSet(baked_data, nrrdAxisInfoSize, (size_t)nx, (size_t)ny, (size_t)nz);

				writer->SetData(baked_data);
				writer->SetSpacings(spcx, spcy, spcz);
				writer->SetCompression(compress);
				writer->Save(filename.ToStdWstring(), mode);

				//free memory
				delete []baked_data->data;
				nrrdNix(baked_data);

				prg_diag->Update(100);
				delete prg_diag;
			}
			else
			{
				writer->SetData(data);
				writer->SetSpacings(spcx, spcy, spcz);
				writer->SetCompression(compress);
				writer->Save(filename.ToStdWstring(), mode);
			}
		}
		delete writer;

		//save mask
		if (m_tex->nmask() != -1 && save_msk)
		{
			m_vr->return_mask();
			data = m_tex->get_nrrd(m_tex->nmask());
			if (data)
			{
				MSKWriter msk_writer;
				msk_writer.SetData(data);
				msk_writer.SetSpacings(spcx, spcy, spcz);
				msk_writer.Save(filename.ToStdWstring(), 0);
			}
		}

		//save label
		if (m_tex->nlabel() != -1 && save_label)
		{
			data = m_tex->get_nrrd(m_tex->nlabel());
			if (data)
			{
				MSKWriter msk_writer;
				msk_writer.SetData(data);
				msk_writer.SetSpacings(spcx, spcy, spcz);
				msk_writer.Save(filename.ToStdWstring(), 1);
			}
		}

		m_tex_path = filename;
	}
}

//bounding box
BBox VolumeData::GetBounds()
{
	return m_bounds;
}

//path
void VolumeData::SetPath(wxString path)
{
	m_tex_path = path;
}

wxString VolumeData::GetPath()
{
	return m_tex_path;
}

//multi-channel
void VolumeData::SetCurChannel(int chan)
{
	m_chan = chan;
}

int VolumeData::GetCurChannel()
{
	return m_chan;
}

//time sequence
void VolumeData::SetCurTime(int time)
{
	m_time = time;
}

int VolumeData::GetCurTime()
{
	return m_time;
}

//MIP & normal modes
void VolumeData::SetMode(int mode)
{
	if (!m_vr)
		return;

	m_saved_mode = m_mode;
	m_mode = mode;

	switch (mode)
	{
	case 0://normal
		m_vr->set_mode(TextureRenderer::MODE_OVER);

		m_vr->set_color(m_color);
		m_vr->set_alpha(m_alpha);
		m_vr->set_lo_thresh(m_lo_thresh);
		m_vr->set_hi_thresh(m_hi_thresh);
		m_vr->set_gm_thresh(m_gm_thresh);
		m_vr->set_gamma3d(m_gamma3d);
		m_vr->set_offset(m_offset);
		break;
	case 1://MIP
		m_vr->set_mode(TextureRenderer::MODE_MIP);
		{
			double h, s, v;
			GetHSV(h, s, v);
			HSVColor hsv(h, s, 1.0);
			Color rgb = Color(hsv);
			m_vr->set_color(rgb);
		}
		m_vr->set_alpha(1.0);
		m_vr->set_lo_thresh(0.0);
		m_vr->set_hi_thresh(1.0);
		m_vr->set_gm_thresh(0.0);
		m_vr->set_gamma3d(m_gamma3d);
		m_vr->set_offset(m_offset);
		break;
	case 2://white shading
		m_vr->set_mode(TextureRenderer::MODE_OVER);
		m_vr->set_colormap_mode(0);

		m_vr->set_color(Color(1.0, 1.0, 1.0));
		m_vr->set_alpha(1.0);
		m_vr->set_lo_thresh(m_lo_thresh);
		m_vr->set_hi_thresh(m_hi_thresh);
		m_vr->set_gm_thresh(m_gm_thresh);
		m_vr->set_gamma3d(m_gamma3d);
		m_vr->set_offset(m_offset);
		break;
	case 3://white mip
		m_vr->set_mode(TextureRenderer::MODE_MIP);
		m_vr->set_colormap_mode(0);

		m_vr->set_color(Color(1.0, 1.0, 1.0));
		m_vr->set_alpha(1.0);
		m_vr->set_lo_thresh(0.0);
		m_vr->set_hi_thresh(1.0);
		m_vr->set_gm_thresh(0.0);
		m_vr->set_gamma3d(1.0);
		m_vr->set_offset(1.0);
		break;
	}
}

int VolumeData::GetMode()
{
	return m_mode;
}

void VolumeData::RestoreMode()
{
	SetMode(m_saved_mode);
}

//inversion
void VolumeData::SetInvert(bool mode)
{
	if (m_vr)
		m_vr->set_inversion(mode);
}

bool VolumeData::GetInvert()
{
	if (m_vr)
		return m_vr->get_inversion();
	else
		return false;
}

//mask mode
void VolumeData::SetMaskMode(int mode)
{
	m_mask_mode = mode;
	if (m_vr)
		m_vr->set_ml_mode(mode);
}

int VolumeData::GetMaskMode()
{
	return m_mask_mode;
}

bool VolumeData::isActiveMask()
{
	if (m_tex->nmask()==-1) return false;

	bool mask;
	switch (m_mask_mode)
	{
	case 0:
		mask = false;
		break;
	case 1:
		mask = true;
		break;
	case 2:
		mask = true;
		break;
	case 3:
		mask = false;
		break;
	case 4:
		mask = true;
		break;
	}

	return mask;
}

bool VolumeData::isActiveLabel()
{
	if (m_tex->nlabel()==-1) return false;

	bool label;
	switch (m_mask_mode)
	{
	case 0:
		label = false;
		break;
	case 1:
		label = false;
		break;
	case 2:
		label = false;
		break;
	case 3:
		label = true;
		break;
	case 4:
		label = true;
		break;
	}

	return label;
}

//noise reduction
void VolumeData::SetNR(bool val)
{
	m_noise_rd = val;
	if (m_vr)
		m_vr->SetNoiseRed(val);
}

bool VolumeData::GetNR()
{
	return m_noise_rd;
}

//volumerenderer
VolumeRenderer *VolumeData::GetVR()
{
	return m_vr;
}

//texture
Texture* VolumeData::GetTexture()
{
	return m_tex;
}

void VolumeData::SetTexture()
{
	if (m_vr)
		m_vr->reset_texture();
	m_tex = 0;
}

void VolumeData::SetMatrices(glm::mat4 &mv_mat,
	glm::mat4 &proj_mat, glm::mat4 &tex_mat)
{
	glm::mat4 scale_mv = glm::scale(mv_mat, glm::vec3(m_sclx, m_scly, m_sclz));
	if (m_vr)
		m_vr->set_matrices(scale_mv, proj_mat, tex_mat);
}

//draw volume
void VolumeData::Draw(bool ortho, bool interactive, double zoom, double sampling_frq_fac)
{
	if (m_vr)
	{
		m_vr->draw(m_test_wiref, interactive, ortho, zoom, m_stream_mode, sampling_frq_fac);
	}
	if (m_draw_bounds)
		DrawBounds();
}

void VolumeData::DrawBounds()
{
}

//draw mask (create the mask)
//type: 0-initial; 1-diffusion-based growing
//paint_mode: 1-select; 2-append; 3-erase; 4-diffuse; 5-flood; 6-clear, 7-all
//			  11-posterize
//hr_mode (hidden removal): 0-none; 1-ortho; 2-persp
void VolumeData::DrawMask(int type, int paint_mode, int hr_mode,
						  double ini_thresh, double gm_falloff, double scl_falloff, double scl_translate,
						  double w2d, double bins, bool ortho)
{
	if (m_vr)
	{
		m_vr->set_2d_mask(m_2d_mask);
		m_vr->set_2d_weight(m_2d_weight1, m_2d_weight2);
		m_vr->draw_mask(type, paint_mode, hr_mode, ini_thresh, gm_falloff, scl_falloff, scl_translate, w2d, bins, ortho, false);
	}
}

void VolumeData::DrawMaskDSLT(int type, int paint_mode, int hr_mode,
							  double ini_thresh, double gm_falloff, double scl_falloff, double scl_translate,
							  double w2d, double bins, int dslt_r, int dslt_q, double dslt_c, bool ortho)
{
	if (m_vr)
	{
		m_vr->set_2d_mask(m_2d_mask);
		m_vr->set_2d_weight(m_2d_weight1, m_2d_weight2);
		m_vr->draw_mask_dslt(type, paint_mode, hr_mode, ini_thresh, gm_falloff, scl_falloff, scl_translate, w2d, bins, ortho, false, dslt_r, dslt_q, dslt_c*GetMaxValue());
	}
}

//draw label (create the label)
//type: 0-initialize; 1-maximum intensity filtering
//mode: 0-normal; 1-posterized; 2-copy values; 3-poster, copy
void VolumeData::DrawLabel(int type, int mode, double thresh, double gm_falloff)
{
	if (m_vr)
		m_vr->draw_label(type, mode, thresh, gm_falloff);
}

//calculation
void VolumeData::Calculate(int type, VolumeData *vd_a, VolumeData *vd_b)
{
	if (m_vr)
	{
		if (type==6 || type==7)
			m_vr->set_hi_thresh(vd_a->GetRightThresh());
		m_vr->calculate(type, vd_a?vd_a->GetVR():0, vd_b?vd_b->GetVR():0);
		m_vr->return_volume();
		if (m_tex && m_tex->get_nrrd(0))
		{
			Nrrd* nrrd_data = m_tex->get_nrrd(0);
			uint8 *val8nr = (uint8*)nrrd_data->data;
			int max_val = 255;
			int bytes = 1;
			if (nrrd_data->type == nrrdTypeUShort) bytes = 2;
			int mem_size = (unsigned long long)m_res_x * (unsigned long long)m_res_y * (unsigned long long)m_res_z * bytes;
			if (nrrd_data->type == nrrdTypeUChar)
				max_val = *std::max_element(val8nr, val8nr+mem_size);
			else if (nrrd_data->type == nrrdTypeUShort)
				max_val = *std::max_element((uint16*)val8nr, (uint16*)val8nr+mem_size/2);
			SetMaxValue(max_val);
		}
	}
}

//set 2d mask for segmentation
void VolumeData::Set2dMask(GLuint mask)
{
	m_2d_mask = mask;
}

//set 2d weight map for segmentation
void VolumeData::Set2DWeight(GLuint weight1, GLuint weight2)
{
	m_2d_weight1 = weight1;
	m_2d_weight2 = weight2;
}

//set 2d depth map for rendering shadows
void VolumeData::Set2dDmap(GLuint dmap)
{
	m_2d_dmap = dmap;
	if (m_vr)
		m_vr->set_2d_dmap(m_2d_dmap);
}

//properties
//transfer function
void VolumeData::Set3DGamma(double dVal)
{
	m_gamma3d = dVal;
	if (m_vr)
		m_vr->set_gamma3d(m_gamma3d);
}

double VolumeData::Get3DGamma()
{
	return m_gamma3d;
}

void VolumeData::SetBoundary(double dVal)
{
	m_gm_thresh = dVal;
	if (m_vr)
		m_vr->set_gm_thresh(m_gm_thresh);
}

double VolumeData::GetBoundary()
{
	return m_gm_thresh;
}

void VolumeData::SetOffset(double dVal)
{
	m_offset = dVal;
	if (m_vr)
		m_vr->set_offset(m_offset);
}

double VolumeData::GetOffset()
{
	return m_offset;
}

void VolumeData::SetLeftThresh(double dVal)
{
	m_lo_thresh = dVal;
	if (m_vr)
		m_vr->set_lo_thresh(m_lo_thresh);
}

double VolumeData::GetLeftThresh()
{
	return m_lo_thresh;
}

void VolumeData::SetRightThresh(double dVal)
{
	m_hi_thresh = dVal;
	if (m_vr)
		m_vr->set_hi_thresh(m_hi_thresh);
}

double VolumeData::GetRightThresh()
{
	return m_hi_thresh;
}

void VolumeData::SetColor(const Color &color, bool update_hsv)
{
	m_color = color;
	if (update_hsv)
		SetHSV();
	if (m_vr)
		m_vr->set_color(color);
}

Color VolumeData::GetColor()
{
	return m_color;
}

void VolumeData::SetMaskColor(Color &color, bool set)
{
	if (m_vr)
		m_vr->set_mask_color(color, set);
}

Color VolumeData::GetMaskColor()
{
	Color result;
	if (m_vr)
		result = m_vr->get_mask_color();
	return result;
}

bool VolumeData::GetMaskColorSet()
{
	if (m_vr)
		return m_vr->get_mask_color_set();
	else
		return false;
}

void VolumeData::ResetMaskColorSet()
{
	if (m_vr)
		m_vr->reset_mask_color_set();
}

Color VolumeData::SetLuminance(double dVal)
{
	double h, s, v;
	GetHSV(h, s, v);
	HSVColor hsv(h, s, dVal);
	m_color = Color(hsv);
	if (m_vr)
		m_vr->set_color(m_color);
	return m_color;
}

double VolumeData::GetLuminance()
{
	HSVColor hsv(m_color);
	return hsv.val();
}

void VolumeData::SetAlpha(double alpha)
{
	m_alpha = alpha;
	if (m_vr)
		m_vr->set_alpha(m_alpha);
}

double VolumeData::GetAlpha()
{
	return m_alpha;
}

void VolumeData::SetEnableAlpha(bool mode)
{
	if (m_vr)
	{
		m_vr->set_solid(!mode);
		if (mode)
			m_vr->set_alpha(m_alpha);
		else
			m_vr->set_alpha(1.0);
	}
}

bool VolumeData::GetEnableAlpha()
{
	if (m_vr)
		return !m_vr->get_solid();
	else
		return true;
}

void VolumeData::SetHSV(double hue, double sat, double val)
{
	if (hue < 0 || sat < 0 || val < 0)
	{
		m_hsv = HSVColor(m_color);
	}
	else
	{
		m_hsv = HSVColor(hue, sat, val);
	}
}

void VolumeData::GetHSV(double &hue, double &sat, double &val)
{
	hue = m_hsv.hue();
	sat = m_hsv.sat();
	val = m_hsv.val();
}

//mask threshold
void VolumeData::SetMaskThreshold(double thresh)
{
	if (m_use_mask_threshold && m_vr)
		m_vr->set_mask_thresh(thresh);
}

void VolumeData::SetUseMaskThreshold(bool mode)
{
	m_use_mask_threshold = mode;
	if (m_vr && !m_use_mask_threshold)
		m_vr->set_mask_thresh(0.0);
}

//shading
void VolumeData::SetShading(bool bVal)
{
	m_shading = bVal;
	if (m_vr)
		m_vr->set_shading(bVal);
}

bool VolumeData::GetShading()
{
	return m_shading;
}

//shadow
void VolumeData::SetShadow(bool bVal)
{
	m_shadow = bVal;
}

bool VolumeData::GetShadow()
{
	return m_shadow;
}

void VolumeData::SetShadowParams(double val)
{
	m_shadow_darkness = val;
}

void VolumeData::GetShadowParams(double &val)
{
	val = m_shadow_darkness;
}

void VolumeData::SetMaterial(double amb, double diff, double spec, double shine)
{
	m_mat_amb = amb;
	m_mat_diff = diff;
	m_mat_spec = spec;
	m_mat_shine = shine;
	if (m_vr)
		m_vr->set_material(m_mat_amb, m_mat_diff, m_mat_spec, m_mat_shine);
}

void VolumeData::GetMaterial(double &amb, double &diff, double &spec, double &shine)
{
	amb = m_mat_amb;
	diff = m_mat_diff;
	spec = m_mat_spec;
	shine = m_mat_shine;
}

void VolumeData::SetLowShading(double dVal)
{
	double amb, diff, spec, shine;
	GetMaterial(amb, diff, spec, shine);
	SetMaterial(dVal, diff, spec, shine);
}

void VolumeData::SetHiShading(double dVal)
{
	double amb, diff, spec, shine;
	GetMaterial(amb, diff, spec, shine);
	SetMaterial(amb, diff, spec, dVal);
}

//sample rate
void VolumeData::SetSampleRate(double rate)
{
	m_sample_rate = rate;
	if (m_vr)
		m_vr->set_sampling_rate(m_sample_rate);
}

double VolumeData::GetSampleRate()
{
	return m_sample_rate;
}

//colormap mode
void VolumeData::SetColormapMode(int mode)
{
	m_colormap_mode = mode;
	if (m_vr)
	{
		m_vr->set_colormap_mode(m_colormap_mode);
		m_vr->set_color(m_color);
	}
}

int VolumeData::GetColormapMode()
{
	return m_colormap_mode;
}

void VolumeData::SetColormapDisp(bool disp)
{
	m_colormap_disp = disp;
}

bool VolumeData::GetColormapDisp()
{
	return m_colormap_disp;
}

void VolumeData::SetColormapValues(double low, double high)
{
	m_colormap_low_value = low;
	m_colormap_hi_value = high;
	if (m_vr)
		m_vr->set_colormap_values(
		m_colormap_low_value, m_colormap_hi_value);
}

void VolumeData::GetColormapValues(double &low, double &high)
{
	low = m_colormap_low_value;
	high = m_colormap_hi_value;
}

void VolumeData::SetColormap(int value)
{
	m_colormap = value;
	if (m_vr)
		m_vr->set_colormap(m_colormap);
}

void VolumeData::SetColormapProj(int value)
{
	m_colormap_proj = value;
	if (m_vr)
		m_vr->set_colormap_proj(m_colormap_proj);
}

int VolumeData::GetColormap()
{
	return m_colormap;
}

int VolumeData::GetColormapProj()
{
	return m_colormap_proj;
}

//resolution  scaling and spacing
void VolumeData::GetResolution(int &res_x, int &res_y, int &res_z)
{
	res_x = m_res_x;
	res_y = m_res_y;
	res_z = m_res_z;
}

void VolumeData::SetScalings(double sclx, double scly, double sclz)
{
	m_sclx = sclx; m_scly = scly; m_sclz = sclz;
}

void VolumeData::GetScalings(double &sclx, double &scly, double &sclz)
{
	sclx = m_sclx; scly = m_scly; sclz = m_sclz;
}

//brxml�̂Ƃ��͍ŉ��w��spacing���ݒ肷���i���̊K�w�͎����I�Ɍv�Z�j
void VolumeData::SetSpacings(double spcx, double spcy, double spcz)
{
	if (GetTexture())
	{
		GetTexture()->set_spacings(spcx, spcy, spcz);
		m_bounds.reset();
		GetTexture()->get_bounds(m_bounds);
	}
}

void VolumeData::GetSpacings(double &spcx, double &spcy, double & spcz, int lv)
{
	if (GetTexture())
		GetTexture()->get_spacings(spcx, spcy, spcz, lv);
}

void VolumeData::SetBaseSpacings(double spcx, double spcy, double spcz)
{
	if (GetTexture())
	{
		GetTexture()->set_base_spacings(spcx, spcy, spcz);
		m_bounds.reset();
		GetTexture()->get_bounds(m_bounds);
	}
}

void VolumeData::GetBaseSpacings(double &spcx, double &spcy, double & spcz)
{
	if (GetTexture())
		GetTexture()->get_base_spacings(spcx, spcy, spcz);
}

void VolumeData::SetSpacingScales(double s_spcx, double s_spcy, double s_spcz)
{
	if (GetTexture())
	{
		GetTexture()->set_spacing_scales(s_spcx, s_spcy, s_spcz);
		m_bounds.reset();
		GetTexture()->get_bounds(m_bounds);
	}
}

void VolumeData::GetSpacingScales(double &s_spcx, double &s_spcy, double &s_spcz)
{
	if (GetTexture())
		GetTexture()->get_spacing_scales(s_spcx, s_spcy, s_spcz);
}

void VolumeData::SetLevel(int lv)
{
	if (GetTexture() && isBrxml())
	{
		GetTexture()->setLevel(lv);
		m_bounds.reset();
		GetTexture()->get_bounds(m_bounds);
	}
}

int VolumeData::GetLevel()
{
	if (GetTexture() && isBrxml())
		return GetTexture()->GetCurLevel();
	else
		return -1;
}

int VolumeData::GetLevelNum()
{
	if (GetTexture() && isBrxml())
		return GetTexture()->GetLevelNum();
	else
		return -1;
}

void VolumeData::GetFileSpacings(double &spcx, double &spcy, double &spcz)
{
	spcx = m_spcx; spcy = m_spcy; spcz = m_spcz;
}

//display controls
void VolumeData::SetDisp(bool disp)
{
	m_disp = disp;
	GetTexture()->set_sort_bricks();
	if (!disp)
	{
		GetMask(true);
		GetVR()->clear_tex_current();
	}
}

bool VolumeData::GetDisp()
{
	return m_disp;
}

void VolumeData::ToggleDisp()
{
	m_disp = !m_disp;
	GetTexture()->set_sort_bricks();
	if (!m_disp)
	{
		GetMask(true);
		GetVR()->clear_tex_current();
	}
}

//bounding box
void VolumeData::SetDrawBounds(bool draw)
{
	m_draw_bounds = draw;
}

bool VolumeData::GetDrawBounds()
{
	return m_draw_bounds;
}

void VolumeData::ToggleDrawBounds()
{
	m_draw_bounds = !m_draw_bounds;
}

//wireframe
void VolumeData::SetWireframe(bool val)
{
	m_test_wiref = val;
}

//blend mode
void VolumeData::SetBlendMode(int mode)
{
	m_blend_mode = mode;
}

int VolumeData::GetBlendMode()
{
	return m_blend_mode;
}

//clip distance
void VolumeData::SetClipDistance(int distx, int disty, int distz)
{
	m_clip_dist_x = distx;
	m_clip_dist_y = disty;
	m_clip_dist_z = distz;
}

void VolumeData::GetClipDistance(int &distx, int &disty, int &distz)
{
	distx = m_clip_dist_x;
	disty = m_clip_dist_y;
	distz = m_clip_dist_z;
}

//randomize color
void VolumeData::RandomizeColor()
{
	double hue = (double)rand()/(RAND_MAX) * 360.0;
	Color color(HSVColor(hue, 1.0, 1.0));
	SetColor(color);
}

//shown in legend
void VolumeData::SetLegend(bool val)
{
	m_legend = val;
}

bool VolumeData::GetLegend()
{
	return m_legend;
}

//interpolate
void VolumeData::SetInterpolate(bool val)
{
	if (m_vr)
		m_vr->set_interpolate(val);
	m_interpolate = val;
}

bool VolumeData::GetInterpolate()
{
	return m_interpolate;
}

void VolumeData::SetFog(bool use_fog,
	double fog_intensity, double fog_start, double fog_end)
{
	if (m_vr)
		m_vr->set_fog(use_fog, fog_intensity, fog_start, fog_end);
}

VolumeData* VolumeData::CopyLevel(int lv)
{
	if(!m_tex->isBrxml()) return NULL;

	VolumeData* vd = new VolumeData();

	Nrrd *src_nv = m_tex->loadData(lv);
	if (!src_nv) return NULL;
	vd->Load(src_nv, GetName() + wxT("_Copy_Lv") + wxString::Format("%d", lv), wxString(""));

	vd->m_dup = true;
	vd->m_dup_counter = m_dup_counter;
	
	Texture *tex = vd->GetTexture();
	if (!tex) return NULL;
	Nrrd *nv = tex->get_nrrd(0);
	if (!nv) return NULL;

	vector<Plane*> *planes = GetVR() ? GetVR()->get_planes() : 0;
	if (planes && vd->GetVR())
		vd->GetVR()->set_planes(planes);

	double spc[3];
	GetSpacings(spc[0], spc[1], spc[2], lv);
	vd->SetSpacings(spc[0], spc[1], spc[2]);
	double scl[3];
	GetScalings(scl[0], scl[1], scl[2]);
	vd->SetScalings(scl[0], scl[1], scl[2]);

	vd->SetColor(GetColor());
	bool bval = GetEnableAlpha();
	vd->SetEnableAlpha(GetEnableAlpha());
	vd->SetShading(GetShading());
	vd->SetShadow(GetShadow());
	double darkness;
	GetShadowParams(darkness);
	vd->SetShadowParams(darkness);
	//other settings
	double amb, diff, spec, shine;
	GetMaterial(amb, diff, spec, shine);
	vd->Set3DGamma(Get3DGamma());
	vd->SetBoundary(GetBoundary());
	vd->SetOffset(GetOffset());
	vd->SetLeftThresh(GetLeftThresh());
	vd->SetRightThresh(GetRightThresh());
	vd->SetAlpha(GetAlpha());
	vd->SetSampleRate(GetSampleRate());
	vd->SetMaterial(amb, diff, spec, shine);

	//layer properties
	vd->SetGamma(GetGamma());
	vd->SetBrightness(GetBrightness());
	vd->SetHdr(GetHdr());
	vd->SetSyncR(GetSyncR());
	vd->SetSyncG(GetSyncG());
	vd->SetSyncB(GetSyncB());

	//current channel index
	vd->m_chan = 0;
	vd->m_time = 0;

	//modes
	vd->SetMode(GetMode());
	//stream modes
	vd->SetStreamMode(GetStreamMode());
	
	//volume properties
	vd->SetScalarScale(GetScalarScale());
	vd->SetGMScale(GetGMScale());
	vd->SetHSV();

	//noise reduction
	vd->SetNR(GetNR());
	
	//display control
	vd->SetDisp(GetDisp());
	vd->SetDrawBounds(GetDrawBounds());
	vd->m_test_wiref = m_test_wiref;

	//colormap mode
	vd->SetColormapMode(GetColormapMode());
	vd->SetColormapDisp(GetColormapDisp());
	vd->SetColormapValues(m_colormap_low_value, m_colormap_hi_value);
	
	//blend mode
	vd->SetBlendMode(GetBlendMode());

	vd->m_2d_mask = 0;
	vd->m_2d_weight1 = 0;
	vd->m_2d_weight2 = 0;
	vd->m_2d_dmap = 0;

	//clip distance
	int dists[3];
	GetClipDistance(dists[0], dists[1], dists[2]);
	vd->SetClipDistance(dists[0], dists[1], dists[2]);
	
	//compression
	vd->m_compression = false;

	//skip brick
	vd->m_skip_brick = false;

	//legend
	vd->m_legend = true;

	vd->m_annotation = m_annotation;

	vd->m_landmarks = m_landmarks;

	return vd;
}

bool VolumeData::isBrxml()
{
	if (!m_tex) return false;

	return m_tex->isBrxml();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MeshData::MeshData() :
m_data(0),
	m_mr(0),
	m_center(0.0, 0.0, 0.0),
	m_disp(true),
	m_draw_bounds(false),
	m_light(true),
	m_mat_amb(0.3, 0.3, 0.3),
	m_mat_diff(1.0, 0.0, 0.0),
	m_mat_spec(0.2, 0.2, 0.2),
	m_mat_shine(30.0),
	m_mat_alpha(1.0),
	m_shadow(true),
	m_shadow_darkness(0.6),
	m_enable_limit(false),
	m_limit(50)
{
	type = 3;//mesh

	m_trans[0] = 0.0;
	m_trans[1] = 0.0;
	m_trans[2] = 0.0;
	m_rot[0] = 0.0;
	m_rot[1] = 0.0;
	m_rot[2] = 0.0;
	m_scale[0] = 1.0;
	m_scale[1] = 1.0;
	m_scale[2] = 1.0;

	double hue, sat, val;
	hue = double(rand()%360);
	sat = 1.0;
	val = 1.0;
	Color color(HSVColor(hue, sat, val));
	m_mat_diff = color;

	m_legend = true;

	m_swc = false;
	m_r_scale = 1.0;
	m_def_r = 0.25;
	m_subdiv = 1;
	m_swc_reader = NULL;
}

MeshData::~MeshData()
{
	if (m_mr)
		delete m_mr;
	if (m_data)
		glmDelete(m_data);
	if (m_swc_reader)
		delete m_swc_reader;
}

int MeshData::Load(GLMmodel* mesh)
{
	if (!mesh) return 0;

	m_data_path = "";
	m_name = "New Mesh";

	if (m_data)
		delete m_data;
	m_data = mesh;

	if (!m_data->normals)
	{
		if (!m_data->facetnorms)
			glmFacetNormals(m_data);
		glmVertexNormals(m_data, 89.0);
	}

	if (!m_data->materials)
	{
		m_data->materials = new GLMmaterial;
		m_data->nummaterials = 1;
	}

	/* set the default material */
	m_data->materials[0].name = NULL;
	m_data->materials[0].ambient[0] = m_mat_amb.r();
	m_data->materials[0].ambient[1] = m_mat_amb.g();
	m_data->materials[0].ambient[2] = m_mat_amb.b();
	m_data->materials[0].ambient[3] = m_mat_alpha;
	m_data->materials[0].diffuse[0] = m_mat_diff.r();
	m_data->materials[0].diffuse[1] = m_mat_diff.g();
	m_data->materials[0].diffuse[2] = m_mat_diff.b();
	m_data->materials[0].diffuse[3] = m_mat_alpha;
	m_data->materials[0].specular[0] = m_mat_spec.r();
	m_data->materials[0].specular[1] = m_mat_spec.g();
	m_data->materials[0].specular[2] = m_mat_spec.b();
	m_data->materials[0].specular[3] = m_mat_alpha;
	m_data->materials[0].shininess = m_mat_shine;
	m_data->materials[0].emmissive[0] = 0.0;
	m_data->materials[0].emmissive[1] = 0.0;
	m_data->materials[0].emmissive[2] = 0.0;
	m_data->materials[0].emmissive[3] = 0.0;
	m_data->materials[0].havetexture = GL_FALSE;
	m_data->materials[0].textureID = 0;

	//bounds
	GLfloat fbounds[6];
	glmBoundingBox(m_data, fbounds);
	BBox bounds;
	Point pmin(fbounds[0], fbounds[2], fbounds[4]);
	Point pmax(fbounds[1], fbounds[3], fbounds[5]);
	bounds.extend(pmin);
	bounds.extend(pmax);
	m_bounds = bounds;
	m_center = Point((m_bounds.min().x()+m_bounds.max().x())*0.5,
		(m_bounds.min().y()+m_bounds.max().y())*0.5,
		(m_bounds.min().z()+m_bounds.max().z())*0.5);

	if (m_mr)
		delete m_mr;
	m_mr = new FLIVR::MeshRenderer(m_data);

	return 1;
}

int MeshData::Load(wxString &filename)
{
	m_data_path = filename;
	m_name = m_data_path.Mid(m_data_path.Find(GETSLASH(), true)+1);
	wxString suffix = m_data_path.Mid(m_data_path.Find('.', true)).MakeLower();

	if (m_data)
		glmDelete(m_data);

	if (suffix == ".obj")
	{
		string str_fn = filename.ToStdString();
		bool no_fail = true;
		m_data = glmReadOBJ(str_fn.c_str(),&no_fail);
		while (!no_fail) {
			wxMessageDialog *dial = new wxMessageDialog(NULL, 
				wxT("A part of the OBJ file failed to load. Would you like to try re-loading?"), 
				wxT("OBJ Load Failure"), 
				wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
			if (dial->ShowModal() == wxID_YES) {
				if (m_data)
					delete m_data;
				m_data = glmReadOBJ(str_fn.c_str(),&no_fail);
			} else break;
		}
	}
	if (suffix == ".swc")
	{
		if (!m_swc_reader) m_swc_reader = new SWCReader();
		wstring wstr = m_data_path.ToStdWstring();
		m_swc_reader->SetFile(wstr);
		m_swc_reader->Preprocess();
		m_data = m_swc_reader->GenerateSolidModel(m_def_r, m_r_scale, m_subdiv);
		m_swc = true;
	}

	if (!m_data)
		return 0;

	if (!m_data->normals && m_data->numtriangles)
	{
		if (!m_data->facetnorms)
			glmFacetNormals(m_data);
		glmVertexNormals(m_data, 89.0);
	}

	if (!m_data->materials)
	{
		m_data->materials = new GLMmaterial;
		m_data->nummaterials = 1;
	}

	/* set the default material */
	m_data->materials[0].name = NULL;
	m_data->materials[0].ambient[0] = m_mat_amb.r();
	m_data->materials[0].ambient[1] = m_mat_amb.g();
	m_data->materials[0].ambient[2] = m_mat_amb.b();
	m_data->materials[0].ambient[3] = m_mat_alpha;
	m_data->materials[0].diffuse[0] = m_mat_diff.r();
	m_data->materials[0].diffuse[1] = m_mat_diff.g();
	m_data->materials[0].diffuse[2] = m_mat_diff.b();
	m_data->materials[0].diffuse[3] = m_mat_alpha;
	m_data->materials[0].specular[0] = m_mat_spec.r();
	m_data->materials[0].specular[1] = m_mat_spec.g();
	m_data->materials[0].specular[2] = m_mat_spec.b();
	m_data->materials[0].specular[3] = m_mat_alpha;
	m_data->materials[0].shininess = m_mat_shine;
	m_data->materials[0].emmissive[0] = 0.0;
	m_data->materials[0].emmissive[1] = 0.0;
	m_data->materials[0].emmissive[2] = 0.0;
	m_data->materials[0].emmissive[3] = 0.0;
	m_data->materials[0].havetexture = GL_FALSE;
	m_data->materials[0].textureID = 0;

	//bounds
	GLfloat fbounds[6];
	glmBoundingBox(m_data, fbounds);
	BBox bounds;
	Point pmin(fbounds[0], fbounds[2], fbounds[4]);
	Point pmax(fbounds[1], fbounds[3], fbounds[5]);
	bounds.extend(pmin);
	bounds.extend(pmax);
	m_bounds = bounds;
	m_center = Point((m_bounds.min().x()+m_bounds.max().x())*0.5,
		(m_bounds.min().y()+m_bounds.max().y())*0.5,
		(m_bounds.min().z()+m_bounds.max().z())*0.5);

	if (m_mr)
		delete m_mr;
	m_mr = new FLIVR::MeshRenderer(m_data);

	return 1;
}

MeshData* MeshData::DeepCopy(MeshData &copy, bool use_default_settings, DataManager *d_manager)
{
	if (!d_manager || !copy.m_data || !copy.m_mr) return NULL;

	MeshData* md = new MeshData();
	md->m_data_path = copy.m_data_path;
	md->m_name = copy.m_name;
	
	GLMmodel* mi = copy.m_data;
	GLMmodel* model = (GLMmodel*)malloc(sizeof(GLMmodel));
	model = (GLMmodel*)malloc(sizeof(GLMmodel));
	model->pathname    = STRDUP(mi->pathname);
	model->mtllibname  = STRDUP(mi->mtllibname);
	model->numvertices = mi->numvertices;
	model->vertices =  NULL;
	if (model->numvertices)
	{
		model->vertices = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (model->numvertices + 1));
		memcpy(model->vertices, mi->vertices, sizeof(GLfloat) * 3 * (model->numvertices + 1));
	}
	model->numnormals = mi->numnormals;
	model->normals = NULL;
	if (model->numnormals)
	{
		model->normals = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (model->numnormals + 1));
		memcpy(model->normals, mi->normals, sizeof(GLfloat) * 3 * (model->numnormals + 1));
	}
	model->numtexcoords = mi->numtexcoords;
	model->texcoords = NULL;
	if (model->numtexcoords)
	{
		model->texcoords = (GLfloat*)malloc(sizeof(GLfloat) * 2 * (model->numtexcoords + 1));
		memcpy(model->texcoords, mi->texcoords, sizeof(GLfloat) * 2 * (model->numtexcoords + 1));
	}
	model->numfacetnorms = mi->numfacetnorms;
	model->facetnorms = NULL;
	if (model->numfacetnorms)
	{
		model->facetnorms = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (model->numfacetnorms + 1));
		memcpy(model->facetnorms, mi->facetnorms, sizeof(GLfloat) * 3 * (model->numfacetnorms + 1));
	}
	model->numtriangles = mi->numtriangles;
	model->triangles = NULL;
	if (model->numtriangles)
	{
		model->triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) * model->numtriangles);
		memcpy(model->triangles, mi->triangles, sizeof(GLMtriangle) * model->numtriangles);
	}
	model->numlines = mi->numlines;
	model->lines = NULL;
	if (model->numlines)
	{
		model->lines = (GLMline*)malloc(sizeof(GLMline) * model->numlines);
		memcpy(model->lines, mi->lines, sizeof(GLMline) * model->numlines);
	}
	model->nummaterials = mi->nummaterials;
	model->materials = NULL;
	if (model->nummaterials)
	{
		model->materials = (GLMmaterial*)malloc(sizeof(GLMmaterial) * model->nummaterials);
		for (int i = 0; i < model->nummaterials; i++)
		{
			model->materials[i].name = STRDUP(mi->materials[i].name);
			model->materials[i].shininess = mi->materials[i].shininess;
			model->materials[i].diffuse[0] = mi->materials[i].diffuse[0];
			model->materials[i].diffuse[1] = mi->materials[i].diffuse[1];
			model->materials[i].diffuse[2] = mi->materials[i].diffuse[2];
			model->materials[i].diffuse[3] = mi->materials[i].diffuse[3];
			model->materials[i].ambient[0] = mi->materials[i].ambient[0];
			model->materials[i].ambient[1] = mi->materials[i].ambient[1];
			model->materials[i].ambient[2] = mi->materials[i].ambient[2];
			model->materials[i].ambient[3] = mi->materials[i].ambient[3];
			model->materials[i].specular[0] = mi->materials[i].specular[0];
			model->materials[i].specular[1] = mi->materials[i].specular[1];
			model->materials[i].specular[2] = mi->materials[i].specular[2];
			model->materials[i].specular[3] = mi->materials[i].specular[3];
			model->materials[i].emmissive[0] = mi->materials[i].emmissive[0];
			model->materials[i].emmissive[1] = mi->materials[i].emmissive[1];
			model->materials[i].emmissive[2] = mi->materials[i].emmissive[2];
			model->materials[i].emmissive[3] = mi->materials[i].emmissive[3];
			model->materials[i].havetexture = mi->materials[i].havetexture;
			model->materials[i].textureID = mi->materials[i].textureID;
		}
	}
	model->numgroups = mi->numgroups;
	GLMgroup* group = mi->groups;
	GLMgroup* og = NULL;
	GLMgroup* tmpg = NULL;
	model->groups = NULL;
	while(group)
	{
		og = (GLMgroup*)malloc(sizeof(GLMgroup));
		og->name = STRDUP(group->name);
		og->material = group->material;
		og->numtriangles = group->numtriangles;
		og->next = NULL;
		if (group->numtriangles)
		{
			og->triangles = (GLuint*)malloc(sizeof(GLuint) * group->numtriangles);
			memcpy(og->triangles, group->triangles, sizeof(GLuint) * group->numtriangles);
		}
		if (!model->groups) 
		{
			model->groups = og;
			tmpg = og;
		}
		else 
		{
			tmpg->next = og;
			tmpg = tmpg->next;
		}
		group = group->next;
	}
	model->position[0]   = mi->position[0];
	model->position[1]   = mi->position[1];
	model->position[2]   = mi->position[2];
	model->hastexture = mi->hastexture;
	md->m_data = model;

	md->m_swc = copy.m_swc;
	if (copy.m_swc)
	{
		md->m_swc_reader = new SWCReader();
		SWCReader::DeepCopy(copy.m_swc_reader, md->m_swc_reader);
	}

	if (use_default_settings)
	{
		/* set the default material */
		md->m_data->materials[0].name = NULL;
		md->m_data->materials[0].ambient[0] = md->m_mat_amb.r();
		md->m_data->materials[0].ambient[1] = md->m_mat_amb.g();
		md->m_data->materials[0].ambient[2] = md->m_mat_amb.b();
		md->m_data->materials[0].ambient[3] = md->m_mat_alpha;
		md->m_data->materials[0].diffuse[0] = md->m_mat_diff.r();
		md->m_data->materials[0].diffuse[1] = md->m_mat_diff.g();
		md->m_data->materials[0].diffuse[2] = md->m_mat_diff.b();
		md->m_data->materials[0].diffuse[3] = md->m_mat_alpha;
		md->m_data->materials[0].specular[0] = md->m_mat_spec.r();
		md->m_data->materials[0].specular[1] = md->m_mat_spec.g();
		md->m_data->materials[0].specular[2] = md->m_mat_spec.b();
		md->m_data->materials[0].specular[3] = md->m_mat_alpha;
		md->m_data->materials[0].shininess = md->m_mat_shine;
		md->m_data->materials[0].emmissive[0] = 0.0;
		md->m_data->materials[0].emmissive[1] = 0.0;
		md->m_data->materials[0].emmissive[2] = 0.0;
		md->m_data->materials[0].emmissive[3] = 0.0;
		md->m_data->materials[0].havetexture = GL_FALSE;
		md->m_data->materials[0].textureID = 0;

		//bounds
		GLfloat fbounds[6];
		glmBoundingBox(md->m_data, fbounds);
		BBox bounds;
		Point pmin(fbounds[0], fbounds[2], fbounds[4]);
		Point pmax(fbounds[1], fbounds[3], fbounds[5]);
		bounds.extend(pmin);
		bounds.extend(pmax);
		md->m_bounds = bounds;
		md->m_center = Point((md->m_bounds.min().x()+md->m_bounds.max().x())*0.5,
			(md->m_bounds.min().y()+md->m_bounds.max().y())*0.5,
			(md->m_bounds.min().z()+md->m_bounds.max().z())*0.5);
	}
	else
	{
		md->m_mat_alpha = copy.m_mat_alpha;
		md->m_mat_amb = copy.m_mat_amb;
		md->m_mat_diff = copy.m_mat_diff;
		md->m_mat_spec = copy.m_mat_spec;
		md->m_mat_shine = copy.m_mat_shine;

		md->m_bounds = copy.m_bounds;
		md->m_center = copy.m_center;

		md->m_disp = copy.m_disp;
		md->m_draw_bounds = copy.m_draw_bounds;
		md->m_light = copy.m_light;
		md->m_fog = copy.m_fog;
		md->m_shadow = copy.m_shadow;
		md->m_shadow_darkness = copy.m_shadow_darkness;
		md->m_enable_limit = copy.m_enable_limit;
		md->m_limit = copy.m_limit;
		md->m_legend = copy.m_legend;
		md->m_r_scale = copy.m_r_scale;
		md->m_def_r = copy.m_def_r;
		md->m_subdiv = copy.m_subdiv;

		memcpy(md->m_trans, copy.m_trans, sizeof(double)*3);
		memcpy(md->m_rot, copy.m_rot, sizeof(double)*3);
		memcpy(md->m_scale, copy.m_scale, sizeof(double)*3);

		md->m_info = copy.m_info;

		md->m_gamma = copy.m_gamma;
		md->m_brightness = copy.m_brightness;
		md->m_hdr = copy.m_hdr;
		md->m_sync_r = copy.m_sync_r;
		md->m_sync_g = copy.m_sync_g;
		md->m_sync_b = copy.m_sync_b;
	}
	
	md->m_mr = new FLIVR::MeshRenderer(md->m_data);
	md->m_mr->set_alpha(copy.m_mr->get_alpha());
	md->m_mr->set_depth_peel(copy.m_mr->get_depth_peel());
	md->m_mr->set_lighting(copy.m_mr->get_lighting());
	md->m_mr->set_limit(copy.m_mr->get_limit());
	
	return md;
}

void MeshData::Save(wxString& filename)
{
	if (m_data)
	{
		char* str = new char[filename.length()+1];
		for (int i=0; i<(int)filename.length(); i++)
			str[i] = (char)filename[i];
		str[filename.length()] = 0;
		glmWriteOBJ(m_data, str, GLM_SMOOTH);
		delete []str;
		m_data_path = filename;
	}
}

bool MeshData::UpdateModelSWC()
{
	if (!m_swc || !m_swc_reader)
		return false;

	if (m_data)
		glmDelete(m_data);

	m_data = m_swc_reader->GenerateSolidModel(m_def_r, m_r_scale, m_subdiv);

	if (!m_data)
		return false;

	if (!m_data->normals && m_data->numtriangles)
	{
		if (!m_data->facetnorms)
			glmFacetNormals(m_data);
		glmVertexNormals(m_data, 89.0);
	}

	if (!m_data->materials)
	{
		m_data->materials = new GLMmaterial;
		m_data->nummaterials = 1;
	}

	/* set the default material */
	m_data->materials[0].name = NULL;
	m_data->materials[0].ambient[0] = m_mat_amb.r();
	m_data->materials[0].ambient[1] = m_mat_amb.g();
	m_data->materials[0].ambient[2] = m_mat_amb.b();
	m_data->materials[0].ambient[3] = m_mat_alpha;
	m_data->materials[0].diffuse[0] = m_mat_diff.r();
	m_data->materials[0].diffuse[1] = m_mat_diff.g();
	m_data->materials[0].diffuse[2] = m_mat_diff.b();
	m_data->materials[0].diffuse[3] = m_mat_alpha;
	m_data->materials[0].specular[0] = m_mat_spec.r();
	m_data->materials[0].specular[1] = m_mat_spec.g();
	m_data->materials[0].specular[2] = m_mat_spec.b();
	m_data->materials[0].specular[3] = m_mat_alpha;
	m_data->materials[0].shininess = m_mat_shine;
	m_data->materials[0].emmissive[0] = 0.0;
	m_data->materials[0].emmissive[1] = 0.0;
	m_data->materials[0].emmissive[2] = 0.0;
	m_data->materials[0].emmissive[3] = 0.0;
	m_data->materials[0].havetexture = GL_FALSE;
	m_data->materials[0].textureID = 0;

	//bounds
	GLfloat fbounds[6];
	glmBoundingBox(m_data, fbounds);
	BBox bounds;
	Point pmin(fbounds[0], fbounds[2], fbounds[4]);
	Point pmax(fbounds[1], fbounds[3], fbounds[5]);
	bounds.extend(pmin);
	bounds.extend(pmax);
	m_bounds = bounds;
	m_center = Point((m_bounds.min().x()+m_bounds.max().x())*0.5,
		(m_bounds.min().y()+m_bounds.max().y())*0.5,
		(m_bounds.min().z()+m_bounds.max().z())*0.5);

	if (m_mr)
		delete m_mr;
	m_mr = new FLIVR::MeshRenderer(m_data);
	m_mr->set_alpha(m_mat_alpha);

	return true;
}

//MR
MeshRenderer* MeshData::GetMR()
{
	return m_mr;
}

void MeshData::SetMatrices(glm::mat4 &mv_mat, glm::mat4 &proj_mat)
{
	if (m_mr)
	{
		glm::mat4 mv_temp;
		mv_temp = glm::translate(
			mv_mat, glm::vec3(
		m_trans[0]+m_center.x(),
		m_trans[1]+m_center.y(),
		m_trans[2]+m_center.z()));
		mv_temp = glm::rotate(
			mv_temp, float(m_rot[0]),
			glm::vec3(1.0, 0.0, 0.0));
		mv_temp = glm::rotate(
			mv_temp, float(m_rot[1]),
			glm::vec3(0.0, 1.0, 0.0));
		mv_temp = glm::rotate(
			mv_temp, float(m_rot[2]),
			glm::vec3(0.0, 0.0, 1.0));
		mv_temp = glm::scale(mv_temp,
			glm::vec3(float(m_scale[0]), float(m_scale[1]), float(m_scale[2])));
		mv_temp = glm::translate(mv_temp,
			glm::vec3(-m_center.x(), -m_center.y(), -m_center.z()));

		m_mr->SetMatrices(mv_temp, proj_mat);
	}
}

void MeshData::Draw(int peel)
{
	if (!m_mr)
		return;

	glDisable(GL_CULL_FACE);
	m_mr->set_depth_peel(peel);
	m_mr->draw();
	if (m_draw_bounds)
		DrawBounds();
	glEnable(GL_CULL_FACE);
}

void MeshData::DrawBounds()
{
	if (!m_mr)
		return;

	m_mr->draw_wireframe();
}

void MeshData::DrawInt(unsigned int name)
{
	if (!m_mr)
		return;

	m_mr->draw_integer(name);
}

//lighting
void MeshData::SetLighting(bool bVal)
{
	m_light = bVal;
	if (m_mr) m_mr->set_lighting(m_light);
}

bool MeshData::GetLighting()
{
	return m_light;
}

//fog
void MeshData::SetFog(bool bVal,
	double fog_intensity, double fog_start, double fog_end)
{
	m_fog = bVal;
	if (m_mr) m_mr->set_fog(m_fog, fog_intensity, fog_start, fog_end);
}

bool MeshData::GetFog()
{
	return m_fog;
}

void MeshData::SetMaterial(Color& amb, Color& diff, Color& spec, 
	double shine, double alpha)
{
	m_mat_amb = amb;
	m_mat_diff = diff;
	m_mat_spec = spec;
	m_mat_shine = shine;
	m_mat_alpha = alpha;

	if (m_data && m_data->materials)
	{
		for (int i=0; i<(int)m_data->nummaterials; i++)
		{
			if (i==0)
			{
				m_data->materials[i].ambient[0] = m_mat_amb.r();
				m_data->materials[i].ambient[1] = m_mat_amb.g();
				m_data->materials[i].ambient[2] = m_mat_amb.b();
				m_data->materials[i].diffuse[0] = m_mat_diff.r();
				m_data->materials[i].diffuse[1] = m_mat_diff.g();
				m_data->materials[i].diffuse[2] = m_mat_diff.b();
				m_data->materials[i].specular[0] = m_mat_spec.r();
				m_data->materials[i].specular[1] = m_mat_spec.g();
				m_data->materials[i].specular[2] = m_mat_spec.b();
				m_data->materials[i].shininess = m_mat_shine;
			}
			m_data->materials[i].specular[3] = m_mat_alpha;
			m_data->materials[i].ambient[3] = m_mat_alpha;
			m_data->materials[i].diffuse[3] = m_mat_alpha;
		}
	}
}

void MeshData::SetColor(Color &color, int type)
{
	switch (type)
	{
	case MESH_COLOR_AMB:
		m_mat_amb = color;
		if (m_data && m_data->materials)
		{
			m_data->materials[0].ambient[0] = m_mat_amb.r();
			m_data->materials[0].ambient[1] = m_mat_amb.g();
			m_data->materials[0].ambient[2] = m_mat_amb.b();
		}
		break;
	case MESH_COLOR_DIFF:
		m_mat_diff = color;
		if (m_data && m_data->materials)
		{
			m_data->materials[0].diffuse[0] = m_mat_diff.r();
			m_data->materials[0].diffuse[1] = m_mat_diff.g();
			m_data->materials[0].diffuse[2] = m_mat_diff.b();
		}
		break;
	case MESH_COLOR_SPEC:
		m_mat_spec = color;
		if (m_data && m_data->materials)
		{
			m_data->materials[0].specular[0] = m_mat_spec.r();
			m_data->materials[0].specular[1] = m_mat_spec.g();
			m_data->materials[0].specular[2] = m_mat_spec.b();
		}
		break;
	}
}

void MeshData::SetFloat(double &value, int type)
{
	switch (type)
	{
	case MESH_FLOAT_SHN:
		m_mat_shine = value;
		if (m_data && m_data->materials)
		{
			m_data->materials[0].shininess = m_mat_shine;
		}
		break;
	case MESH_FLOAT_ALPHA:
		m_mat_alpha = value;
		if (m_data && m_data->materials)
		{
			for (int i=0; i<(int)m_data->nummaterials; i++)
			{
				m_data->materials[i].ambient[3] = m_mat_alpha;
				m_data->materials[i].diffuse[3] = m_mat_alpha;
				m_data->materials[i].specular[3] = m_mat_alpha;
			}
		}
		if (m_mr) m_mr->set_alpha(value);
		break;
	}

}

void MeshData::GetMaterial(Color& amb, Color& diff, Color& spec,
	double& shine, double& alpha)
{
	amb = m_mat_amb;
	diff = m_mat_diff;
	spec = m_mat_spec;
	shine = m_mat_shine;
	alpha = m_mat_alpha;
}

bool MeshData::IsTransp()
{
	if (m_mat_alpha>=1.0)
		return false;
	else
		return true;
}

//shadow
void MeshData::SetShadow(bool bVal)
{
	m_shadow= bVal;
}

bool MeshData::GetShadow()
{
	return m_shadow;
}

void MeshData::SetShadowParams(double val)
{
	m_shadow_darkness = val;
}

void MeshData::GetShadowParams(double &val)
{
	val = m_shadow_darkness;
}

wxString MeshData::GetPath()
{
	return m_data_path;
}

BBox MeshData::GetBounds()
{
	BBox bounds;
	Point p[8];
	p[0] = Point(m_bounds.min().x(), m_bounds.min().y(), m_bounds.min().z());
	p[1] = Point(m_bounds.min().x(), m_bounds.min().y(), m_bounds.max().z());
	p[2] = Point(m_bounds.min().x(), m_bounds.max().y(), m_bounds.min().z());
	p[3] = Point(m_bounds.min().x(), m_bounds.max().y(), m_bounds.max().z());
	p[4] = Point(m_bounds.max().x(), m_bounds.min().y(), m_bounds.min().z());
	p[5] = Point(m_bounds.max().x(), m_bounds.min().y(), m_bounds.max().z());
	p[6] = Point(m_bounds.max().x(), m_bounds.max().y(), m_bounds.min().z());
	p[7] = Point(m_bounds.max().x(), m_bounds.max().y(), m_bounds.max().z());
	double s, c;
	Point temp;
	for (int i=0 ; i<8 ; i++)
	{
		p[i] = Point(p[i].x()*m_scale[0], p[i].y()*m_scale[1], p[i].z()*m_scale[2]);
		s = sin(d2r(m_rot[2]));
		c = cos(d2r(m_rot[2]));
		temp = Point(c*p[i].x()-s*p[i].y(), s*p[i].x()+c*p[i].y(), p[i].z());
		p[i] = temp;
		s = sin(d2r(m_rot[1]));
		c = cos(d2r(m_rot[1]));
		temp = Point(c*p[i].x()+s*p[i].z(), p[i].y(), -s*p[i].x()+c*p[i].z());
		p[i] = temp;
		s = sin(d2r(m_rot[0]));
		c = cos(d2r(m_rot[0]));
		temp = Point(p[i].x(), c*p[i].y()-s*p[i].z(), s*p[i].y()+c*p[i].z());
		p[i] = Point(temp.x()+m_trans[0], temp.y()+m_trans[1], temp.z()+m_trans[2]);
		bounds.extend(p[i]);
	}
	return bounds;
}

GLMmodel* MeshData::GetMesh()
{
	return m_data;
}

void MeshData::SetDisp(bool disp)
{
	m_disp = disp;
}

void MeshData::ToggleDisp()
{
	m_disp = !m_disp;
}

bool MeshData::GetDisp()
{
	return m_disp;
}

void MeshData::SetDrawBounds(bool draw)
{
	m_draw_bounds = draw;
}

void MeshData::ToggleDrawBounds()
{
	m_draw_bounds = !m_draw_bounds;
}

bool MeshData::GetDrawBounds()
{
	return m_draw_bounds;
}

void MeshData::SetTranslation(double x, double y, double z)
{
	m_trans[0] = x;
	m_trans[1] = y;
	m_trans[2] = z;
}

void MeshData::GetTranslation(double &x, double &y, double &z)
{
	x = m_trans[0];
	y = m_trans[1];
	z = m_trans[2];
}

void MeshData::SetRotation(double x, double y, double z)
{
	m_rot[0] = x;
	m_rot[1] = y;
	m_rot[2] = z;
}

void MeshData::GetRotation(double &x, double &y, double &z)
{
	x = m_rot[0];
	y = m_rot[1];
	z = m_rot[2];
}

void MeshData::SetScaling(double x, double y, double z)
{
	m_scale[0] = x;
	m_scale[1] = y;
	m_scale[2] = z;
}

void MeshData::GetScaling(double &x, double &y, double &z)
{
	x = m_scale[0];
	y = m_scale[1];
	z = m_scale[2];
}

//randomize color
void MeshData::RandomizeColor()
{
	double hue = (double)rand()/(RAND_MAX) * 360.0;
	Color color(HSVColor(hue, 1.0, 1.0));
	SetColor(color, MESH_COLOR_DIFF);
    Color amb = color * 0.3;
	SetColor(amb, MESH_COLOR_AMB);
}

//shown in legend
void MeshData::SetLegend(bool val)
{
	m_legend = val;
}

bool MeshData::GetLegend()
{
	return m_legend;
}

//size limiter
void MeshData::SetLimit(bool bVal)
{
	m_enable_limit = bVal;
	if (m_enable_limit)
		m_mr->set_limit(m_limit);
	else
		m_mr->set_limit(-1);
//	m_mr->update();
}

bool MeshData::GetLimit()
{
	return m_enable_limit;
}

void MeshData::SetLimitNumer(int val)
{
	m_limit = val;
	if (m_enable_limit)
	{
		m_mr->set_limit(val);
//		m_mr->update();
	}
}

int MeshData::GetLimitNumber()
{
	return m_limit;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AText::AText()
{
}

AText::AText(const string &str, const Point &pos)
{
	m_txt = str;
	m_pos = pos;
}

AText::~AText()
{
}

string AText::GetText()
{
	return m_txt;
}

Point AText::GetPos()
{
	return m_pos;
}

void AText::SetText(string str)
{
	m_txt = str;
}

void AText::SetPos(Point pos)
{
	m_pos = pos;
}

void AText::SetInfo(string str)
{
	m_info = str;
}

int Annotations::m_num = 0;

Annotations::Annotations()
{
	type = 4;//annotations
	m_num++;
	m_name = wxString::Format("Antn_%d", m_num);
	m_tform = 0;
	m_vd = 0;
	m_disp = true;
	m_memo_ro = false;
}

Annotations::~Annotations()
{
	Clear();
}

int Annotations::GetTextNum()
{
	return (int)m_alist.size();
}

string Annotations::GetTextText(int index)
{
	if (index>=0 && index<(int)m_alist.size())
	{
		AText* atext = m_alist[index];
		if (atext)
			return atext->m_txt;
	}
	return "";
}

Point Annotations::GetTextPos(int index)
{
	if (index>=0 && index<(int)m_alist.size())
	{
		AText* atext = m_alist[index];
		if (atext)
			return atext->m_pos;
	}
	return Point(Vector(0.0));
}

Point Annotations::GetTextTransformedPos(int index)
{
	if (index>=0 && index<(int)m_alist.size())
	{
		AText* atext = m_alist[index];
		if (atext && m_tform)
			return m_tform->transform(atext->m_pos);
	}
	return Point(Vector(0.0));
}

string Annotations::GetTextInfo(int index)
{
	if (index>=0 && index<(int)m_alist.size())
	{
		AText* atext = m_alist[index];
		if(atext)
			return atext->m_info;
	}
	return "";
}

void Annotations::AddText(std::string str, Point pos, std::string info)
{
	AText* atext = new AText(str, pos);
	atext->SetInfo(info);
	m_alist.push_back(atext);
}

void Annotations::SetTransform(Transform *tform)
{
	m_tform = tform;
}

void Annotations::SetVolume(VolumeData *vd)
{
	m_vd = vd;
	if (m_vd)
		m_name += "_FROM_" + m_vd->GetName();
}

VolumeData* Annotations::GetVolume()
{
	return m_vd;
}

void Annotations::Clear()
{
	for (int i=0; i<(int)m_alist.size(); i++)
	{
		AText* atext = m_alist[i];
		if (atext)
			delete atext;
	}
	m_alist.clear();
}

//memo
void Annotations::SetMemo(string &memo)
{
	m_memo = memo;
}

string& Annotations::GetMemo()
{
	return m_memo;
}

void Annotations::SetMemoRO(bool ro)
{
	m_memo_ro = ro;
}

bool Annotations::GetMemoRO()
{
	return m_memo_ro;
}

//save/load
wxString Annotations::GetPath()
{
	return m_data_path;
}

int Annotations::Load(wxString &filename, DataManager* mgr)
{
	wxFileInputStream fis(filename);
	if (!fis.Ok())
		return 0;

	wxTextInputStream tis(fis);
	wxString str;

	while (!fis.Eof())
	{
		wxString sline = tis.ReadLine();

		if (sline.SubString(0, 5) == "Name: ")
		{
			m_name = sline.SubString(6, sline.Length()-1);
		}
		else if (sline.SubString(0, 8) == "Display: ")
		{
			str = sline.SubString(9, 9);
			if (str == "0")
				m_disp = false;
			else
				m_disp = true;
		}
		else if (sline.SubString(0, 4) == "Memo:")
		{
			str = tis.ReadLine();
			while (str.SubString(0, 12) != "Memo Update: " &&
				!fis.Eof())
			{
				m_memo += str + "\n";
				str = tis.ReadLine();
			}
			if (str.SubString(13, 13) == "0")
				m_memo_ro = false;
			else
				m_memo_ro = true;
		}
		else if (sline.SubString(0, 7) == "Volume: ")
		{
			str = sline.SubString(8, sline.Length()-1);
			VolumeData* vd = mgr->GetVolumeData(str);
			if (vd)
			{
				m_vd = vd;
				m_tform = vd->GetTexture()->transform();
			}
		}
		else if (sline.SubString(0, 9) == "Transform:")
		{
			str = tis.ReadLine();
			str = tis.ReadLine();
			str = tis.ReadLine();
			str = tis.ReadLine();
		}
		else if (sline.SubString(0, 10) == "Components:")
		{
			str = tis.ReadLine();
			int tab_counter = 0;
			for (int i=0; i<(int)str.Length(); i++)
			{
				if (str[i] == '\t')
					tab_counter++;
				if (tab_counter == 4)
				{
					m_info_meaning = str.SubString(i+1, str.Length()-1);
					break;
				}
			}

			str = tis.ReadLine();
			while (!fis.Eof())
			{
				if (AText* atext = GetAText(str))
					m_alist.push_back(atext);
				str = tis.ReadLine();
			}
		}
	}

	m_data_path = filename;
	return 1;
}

void Annotations::Save(wxString &filename)
{
	wxFileOutputStream fos(filename);
	if (!fos.Ok())
		return;

	wxTextOutputStream tos(fos);

	int resx = 1;
	int resy = 1;
	int resz = 1;
	if (m_vd)
		m_vd->GetResolution(resx, resy, resz);

	tos << "Name: " << m_name << "\n";
	tos << "Display: " << m_disp << "\n";
	tos << "Memo:\n" << m_memo << "\n";
	tos << "Memo Update: " << m_memo_ro << "\n";
	if (m_vd)
	{
		tos << "Volume: " << m_vd->GetName() << "\n";
		tos << "Voxel size (X Y Z):\n";
		double spcx, spcy, spcz;
		m_vd->GetSpacings(spcx, spcy, spcz);
		tos << spcx << "\t" << spcy << "\t" << spcz << "\n";
	}


	tos << "\nComponents:\n";
	tos << "ID\tX\tY\tZ\t" << m_info_meaning << "\n\n";
	for (int i=0; i<(int)m_alist.size(); i++)
	{
		AText* atext = m_alist[i];
		if (atext)
		{
			tos << atext->m_txt << "\t";
			tos << int(atext->m_pos.x()*resx+1.0) << "\t";
			tos << int(atext->m_pos.y()*resy+1.0) << "\t";
			tos << int(atext->m_pos.z()*resz+1.0) << "\t";
			tos << atext->m_info << "\n";
		}
	}

	m_data_path = filename;
}

wxString Annotations::GetInfoMeaning()
{
	return m_info_meaning;
}

void Annotations::SetInfoMeaning(wxString &str)
{
	m_info_meaning = str;
}

//test if point is inside the clipping planes
bool Annotations::InsideClippingPlanes(Point &pos)
{
	if (!m_vd)
		return true;

	vector<Plane*> *planes = m_vd->GetVR()->get_planes();
	if (!planes)
		return true;
	if (planes->size() != 6)
		return true;

	Plane* plane = 0;
	for (int i=0; i<6; i++)
	{
		plane = (*planes)[i];
		if (!plane)
			continue;
		if (plane->eval_point(pos) < 0)
			return false;
	}

	return true;
}

AText* Annotations::GetAText(wxString str)
{
	AText *atext = 0;
	wxString sID;
	wxString sX;
	wxString sY;
	wxString sZ;
	wxString sInfo;
	int tab_counter = 0;

	for (int i=0; i<(int)str.Length(); i++)
	{
		if (str[i] == '\t')
			tab_counter++;
		else
		{
			if (tab_counter == 0)
				sID += str[i];
			else if (tab_counter == 1)
				sX += str[i];
			else if (tab_counter == 2)
				sY += str[i];
			else if (tab_counter == 3)
				sZ += str[i];
			else if (tab_counter == 4)
			{
				sInfo = str.SubString(i, str.Length()-1);
				break;
			}
		}
	}
	if (tab_counter == 4)
	{
		double x, y, z;
		sX.ToDouble(&x);
		sY.ToDouble(&y);
		sZ.ToDouble(&z);
		int resx = 1;
		int resy = 1;
		int resz = 1;
		if (m_vd)
			m_vd->GetResolution(resx, resy, resz);
		x /= resx?resx:1;
		y /= resy?resy:1;
		z /= resz?resz:1;
		Point pos(x, y, z);
		atext = new AText(sID.ToStdString(), pos);
		atext->SetInfo(sInfo.ToStdString());
	}

	return atext;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t callbackWriteAnnotation(char *ptr, size_t size, size_t nmemb, string *stream)
{
	int data_len = size * nmemb;
	stream->append(ptr, data_len);
	return data_len;
}

RulerBalloon::RulerBalloon(bool visibility, Point point, wxArrayString annotations)
	: m_visibility(visibility), m_point(point), m_annotations(annotations), m_annotationdb(vector<AnnotationDB>())
{
	//m_curl = curl_easy_init();
}

RulerBalloon::~RulerBalloon()
{
	extern CURLM *_g_curlm;

	for(int i = 0; i < m_curl.size(); i++)
	{
		if(m_curl[i])
		{
			curl_multi_remove_handle(_g_curlm, m_curl[i]);
			curl_easy_cleanup(m_curl[i]);
		}
	}
}

void RulerBalloon::SetAnnotationsFromDatabase(vector<AnnotationDB> ann, Point new_p, double spcx, double spcy, double spcz)
{
	if(ann == m_annotationdb && m_point.x() == new_p.x() && m_point.y() == new_p.y() && m_point.z() == new_p.z()) return;

	m_point = new_p;
	m_annotationdb = ann;
	
	if(m_curl.size() < m_annotationdb.size()) m_curl.resize(m_annotationdb.size(), NULL);
	if(m_bufs.size() < m_annotationdb.size()) m_bufs.resize(m_annotationdb.size(), string());

	for(int i = 0; i < m_annotationdb.size(); i++){
		wxString url = m_annotationdb[i].url;

		url << wxT("?annotations=");
		for(int j = 0; j < m_annotationdb[i].contents.size(); j++)
		{
			if(j != 0) url << wxT("+");
			url << m_annotationdb[i].contents[j];
		}
		url << wxT("&coordinate=");
		url << (int)(m_point.x()/spcx) << "+" << (int)(m_point.y()/spcy) << "+" << (int)(m_point.z()/spcz);
		/*
		stringstream coord;
		coord << (int)(m_point.x()/spcx) << "+" << (int)(m_point.y()/spcy) << "+" << (int)(m_point.z()/spcz);
		m_buf = coord.str() + " ";
		*/
		extern CURLM *_g_curlm;

		if(!m_curl[i]) m_curl[i] = curl_easy_init();

		curl_multi_remove_handle(_g_curlm, m_curl[i]);

		if (m_curl[i] == NULL) {
			cerr << "curl_easy_init() failed" << endl;
			return;
		}
		curl_easy_reset(m_curl[i]);
		curl_easy_setopt(m_curl[i], CURLOPT_URL, url.ToStdString().c_str());
		curl_easy_setopt(m_curl[i], CURLOPT_TIMEOUT, 10L);
		curl_easy_setopt(m_curl[i], CURLOPT_USERAGENT, "libcurl-agent/1.0");
		curl_easy_setopt(m_curl[i], CURLOPT_WRITEFUNCTION, callbackWriteAnnotation);
		m_annotations.Clear();
		m_bufs[i] = "";
		curl_easy_setopt(m_curl[i], CURLOPT_WRITEDATA, &m_bufs[i]);
		curl_easy_setopt(m_curl[i], CURLOPT_SSL_VERIFYPEER, 0);

		curl_multi_add_handle(_g_curlm, m_curl[i]);
		int handle_count;
		curl_multi_perform(_g_curlm, &handle_count);

	}

	//curl_easy_perform(m_curl);

/*
	char a[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	m_annotations.Clear();
	int lines = 1 + rand() % 5;
	for(int i = 0; i < lines; i++)
	{
		stringstream ss;
		int char_num = rand() % 20;
		for(int j = 0; j < char_num; j++)
		{
			ss << a[rand() % 62];
		}
		m_annotations.Add(wxString(ss.str()));
	}
*/
}

wxArrayString RulerBalloon::GetAnnotations()
{
    wxArrayString annotations;
    
    int stline = 0;
    for(int a = 0; a < m_bufs.size(); a++)
    {
        stringstream ss(m_bufs[a]);
        string temp, elem;
        int i = 0;
        while (getline(ss, temp))
        {
            std::stringstream ls(temp);
            int j = stline;
            while (getline(ls, elem, '\t'))
            {
                if(i == 0) annotations.Add(wxString(elem) + wxT(": "));
                else if (j < annotations.size()) annotations.Item(j) += wxT(" ") + elem;
                j++;
            }
            i++;
        }
        stline = annotations.size();
    }
/*
    char a[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    annotations.Clear();
    int lines = 1 + rand() % 5;
    for(int i = 0; i < lines; i++)
    {
        stringstream ss;
        int char_num = rand() % 20;
        for(int j = 0; j < char_num; j++)
        {
            ss << a[rand() % 62];
        }
        annotations.Add(wxString(ss.str()));
    }
*/    
    return annotations;
}

void RulerBalloon::DrawBalloon(int nx, int ny, Point orig)
{
	m_annotations.Clear();

	int stline = 0;
	for(int a = 0; a < m_bufs.size(); a++)
	{
		stringstream ss(m_bufs[a]);
		string temp, elem;
		int i = 0;
		while (getline(ss, temp))
		{
			std::stringstream ls(temp);
			int j = stline;
			while (getline(ls, elem, '\t'))
			{
				if(i == 0) m_annotations.Add(wxString(elem) + wxT(": "));
				else if (j < m_annotations.size()) m_annotations.Item(j) += wxT(" ") + elem;
				j++;
			}
			i++;
		}
		stline = m_annotations.size();
	}

	double xpx = 2.0/nx;
	double ypx = 2.0/ny;
	double asp = (double)nx / (double)ny;;
	double margin_x = 5;
	double margin_top = 3;
	double margin_bottom = 3;
	double line_spc = 2;
	int lnum = m_annotations.size();
	const BitmapFontType ftype = BITMAP_FONT_TYPE_HELVETICA_12;

	if(lnum <= 0) return;

	double maxlw, lw, lh;

	lh = fontHeight(ftype);
	margin_bottom += lh/2;//�e�L�X�g��Y���W��baseline�Ɉ��v���邽��
	maxlw = 0.0;

	for(int i = 0; i < lnum; i++)
	{
		lw = renderTextLen(ftype, m_annotations.Item(i).To8BitData().data());
		if(lw > maxlw) maxlw = lw;
		renderText(orig.x()+1.0+margin_x*xpx, 1.05+(lh*(i+1)+line_spc*i+margin_top)*ypx-orig.y(),
				ftype, m_annotations.Item(i).To8BitData().data());
	}

	double bw = (margin_x*2 + maxlw)*xpx;
	double bh = (margin_top + margin_bottom + lh*lnum + line_spc*(lnum-1))*ypx;

	glBegin(GL_LINE_LOOP);
	glVertex2d(orig.x()+1.0, 1.0-orig.y());
	glVertex2d(orig.x()+1.0+0.03/asp, 1.05-orig.y());
	glVertex2d(orig.x()+1.0+bw, 1.05-orig.y());
	glVertex2d(orig.x()+1.0+bw, 1.05+bh-orig.y());
	glVertex2d(orig.x()+1.0, 1.05+bh-orig.y());
	glEnd();
}

int Ruler::m_num = 0;
Ruler::Ruler()
{
	type = 7;//ruler
	m_num++;
	m_name = wxString::Format("%d", m_num);
	m_disp = true;
	m_tform = 0;
	m_ruler_type = 0;
	m_finished = false;
	m_use_color = false;

	//time-dependent
	m_time_dep = false;
	m_time = 0;

	m_name_disp = wxString::Format("Ruler %d", m_num);
	m_desc = wxString();

}

Ruler::~Ruler()
{
}

//data
int Ruler::GetNumPoint()
{
	return (int)m_ruler.size();
}

Point *Ruler::GetPoint(int index)
{
	if (index>=0 && (size_t)index<m_ruler.size())
		return &(m_ruler[index]);
	else
		return 0;
}

int Ruler::GetRulerType()
{
	return m_ruler_type;
}

void Ruler::SetRulerType(int type)
{
	m_ruler_type = type;
}

bool Ruler::GetFinished()
{
	return m_finished;
}

void Ruler::SetFinished()
{
	m_finished = true;
}

double Ruler::GetLength()
{
	double length = 0.0;
	Point p1, p2;

	for (unsigned int i=1; i<m_ruler.size(); ++i)
	{
		p1 = m_ruler[i-1];
		p2 = m_ruler[i];
		length += (p2-p1).length();
	}

	return length;
}

double Ruler::GetLengthObject(double spcx, double spcy, double spcz)
{
	double length = 0.0;
	Point p1, p2;

	for (unsigned int i=1; i<m_ruler.size(); ++i)
	{
		p1 = m_ruler[i-1];
		p2 = m_ruler[i];
		p1 = Point(p1.x()/spcx, p1.y()/spcy, p1.z()/spcz);
		p2 = Point(p2.x()/spcx, p2.y()/spcy, p2.z()/spcz);
		length += (p2-p1).length();
	}

	return length;
}

double Ruler::GetAngle()
{
	double angle = 0.0;

	if (m_ruler_type == 0 ||
		m_ruler_type == 3)
	{
		if (m_ruler.size() >= 2)
		{
			Vector v = m_ruler[1] - m_ruler[0];
			v.normalize();
			angle = atan2(-v.y(), (v.x()>0.0?1.0:-1.0)*sqrt(v.x()*v.x() + v.z()*v.z()));
			angle = r2d(angle);
			angle = angle<0.0?angle+180.0:angle;
		}
	}
	else if (m_ruler_type == 4)
	{
		if (m_ruler.size() >=3)
		{
			Vector v1, v2;
			v1 = m_ruler[0] - m_ruler[1];
			v1.normalize();
			v2 = m_ruler[2] - m_ruler[1];
			v2.normalize();
			angle = acos(Dot(v1, v2));
			angle = r2d(angle);
		}
	}

	return angle;
}

void Ruler::Scale(double spcx, double spcy, double spcz)
{
	for (size_t i = 0; i < m_ruler.size(); ++i)
		m_ruler[i].scale(spcx, spcy, spcz);
}

bool Ruler::AddPoint(const Point &point)
{
	if (m_ruler_type == 2 &&
		m_ruler.size() == 1)
		return false;
	else if ((m_ruler_type == 0 ||
		m_ruler_type == 3) &&
		m_ruler.size() == 2)
		return false;
	else if (m_ruler_type == 4 &&
		m_ruler.size() == 3)
		return false;
	else
	{
		m_ruler.push_back(point);
		m_balloons.push_back(RulerBalloon(false, point, wxArrayString()));
		if (m_ruler_type == 2 &&
			m_ruler.size() == 1)
			m_finished = true;
		else if ((m_ruler_type == 0 ||
			m_ruler_type == 3) &&
			m_ruler.size() == 2)
			m_finished = true;
		else if (m_ruler_type == 4 &&
			m_ruler.size() == 3)
			m_finished = true;
		return true;
	}
}

void Ruler::SetTransform(Transform *tform)
{
	m_tform = tform;
}

void Ruler::Clear()
{
	m_ruler.clear();
}

wxArrayString Ruler::GetAnnotations(int index, vector<AnnotationDB> annotationdb, double spcx, double spcy, double spcz)
{
    if (index < 0 || (size_t)index >= m_ruler.size())
        return wxArrayString();
    
    m_balloons[index].SetAnnotationsFromDatabase(annotationdb, m_ruler[index], spcx, spcy, spcz);
    return m_balloons[index].GetAnnotations();
}

void Ruler::Draw(bool persp, int nx, int ny, glm::mat4 mv_mat, glm::mat4 proj_mat, vector<AnnotationDB> annotationdb, double spcx, double spcy, double spcz)
{
	double asp = (double)nx / (double)ny;
	Transform mv;
	Transform p;
	mv.set(glm::value_ptr(mv_mat));
	p.set(glm::value_ptr(proj_mat));

	beginRenderText(2, 2, true);//glOrtho�ō��オ(0,0)�E����(2,2)�ɂȂ�
	for (int i=0; i<(int)m_ruler.size(); i++)
	{
		Point p1, p2;
		p2 = m_ruler[i];
		//if (m_tform)
		//	p2 = m_tform->transform(p2);
		p2 = mv.transform(p2);
		p2 = p.transform(p2);
		if ((persp && (p2.z()<=0.0 || p2.z()>=1.0)) ||
			(!persp && (p2.z()>=0.0 || p2.z()<=-1.0)))
			continue;
		glBegin(GL_LINE_LOOP);
			glVertex2d(p2.x()+1.0-0.01/asp, 0.99-p2.y());
			glVertex2d(p2.x()+1.0+0.01/asp, 0.99-p2.y());
			glVertex2d(p2.x()+1.0+0.01/asp, 1.01-p2.y());
			glVertex2d(p2.x()+1.0-0.01/asp, 1.01-p2.y());
		glEnd();
		if(m_balloons[i].GetVisibility())
		{
			m_balloons[i].SetAnnotationsFromDatabase(annotationdb, Point(m_ruler[i]), spcx, spcy, spcz);
			m_balloons[i].DrawBalloon(nx, ny, p2);
		}
		if (i+1 == m_ruler.size())
			renderText(p2.x()+1.02, 1.01-p2.y(),
			BITMAP_FONT_TYPE_HELVETICA_12,
			m_name_disp.To8BitData().data());
		if (m_ruler_type==2 && i==0)
		{
			glBegin(GL_POINTS);
				glVertex2d(p2.x()+1.0, 1.0-p2.y());
			glEnd();
		}
		if (i > 0)
		{
			p1 = m_ruler[i-1];
			if (m_tform)
				p1 = m_tform->transform(p1);
			p1 = mv.transform(p1);
			p1 = p.transform(p1);
			if ((persp && (p1.z()<=0.0 || p1.z()>=1.0)) ||
				(!persp && (p1.z()>=0.0 || p1.z()<=-1.0)))
				continue;
			glBegin(GL_LINES);
				glVertex2d(p1.x()+1.0, 1.0-p1.y());
				glVertex2d(p2.x()+1.0, 1.0-p2.y());
			glEnd();
		}
	}
	endRenderText();
}

wxString Ruler::GetDelInfoValues(wxString del)
{
	wxString output;

	for (size_t i=0; i<m_info_values.length(); i++)
	{
		if (m_info_values[i] == '\t')
			output += del;
		else
			output += m_info_values[i];
	}

	return output;
}

void Ruler::SaveProfile(wxString &filename)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int TraceGroup::m_num = 0;
TraceGroup::TraceGroup()
{
	type = 8;//traces
	m_num++;
	m_name = wxString::Format("Traces %d", m_num);
	m_cur_time = -1;
	m_prv_time = -1;
	m_ghost_num = 10;
	m_draw_tail = true;
	m_draw_lead = false;
	m_cell_size = 20;
}

TraceGroup::~TraceGroup()
{
}

void TraceGroup::SetCurTime(int time)
{
	m_cur_time = time;
}

int TraceGroup::GetCurTime()
{
	return m_cur_time;
}
void TraceGroup::SetPrvTime(int time)
{
	m_prv_time = time;
}

int TraceGroup::GetPrvTime()
{
	return m_prv_time;
}

//get information
void TraceGroup::GetLinkLists(size_t frame,
	FL::VertexList &in_orphan_list,
	FL::VertexList &out_orphan_list,
	FL::VertexList &in_multi_list,
	FL::VertexList &out_multi_list)
{
	if (in_orphan_list.size())
		in_orphan_list.clear();
	if (out_orphan_list.size())
		out_orphan_list.clear();
	if (in_multi_list.size())
		in_multi_list.clear();
	if (out_multi_list.size())
		out_multi_list.clear();

	FL::TrackMapProcessor tm_processor;
	tm_processor.SetSizeThresh(m_cell_size);
	tm_processor.GetLinkLists(m_track_map, frame,
		in_orphan_list, out_orphan_list,
		in_multi_list, out_multi_list);
}

void TraceGroup::ClearCellList()
{
	m_cell_list.clear();
}

//cur_sel_list: ids from previous time point
//m_prv_time: previous time value
//m_id_map: ids of current time point that are linked to previous
//m_cur_time: current time value
//time values are check with frame ids in the frame list
void TraceGroup::UpdateCellList(FL::CellList &cur_sel_list)
{
	ClearCellList();
	FL::CellListIter cell_iter;

	//why does not the time change?
	//because I just want to find out the current selection
	if (m_prv_time == m_cur_time)
	{
		//copy cur_sel_list to m_cell_list
		for (cell_iter = cur_sel_list.begin();
			cell_iter != cur_sel_list.end();
			++cell_iter)
		{
			if (cell_iter->second->GetSizeUi() >
				(unsigned int)m_cell_size)
				m_cell_list.insert(pair<unsigned int, FL::pCell>
					(cell_iter->second->Id(), cell_iter->second));
		}
		return;
	}

	//get mapped cells
	//cur_sel_list -> m_cell_list
	FL::TrackMapProcessor tm_processor;
	tm_processor.GetMappedCells(m_track_map,
		cur_sel_list, m_cell_list,
		(unsigned int)m_prv_time,
		(unsigned int)m_cur_time);
}

FL::CellList &TraceGroup::GetCellList()
{
	return m_cell_list;
}

bool TraceGroup::FindCell(unsigned int id)
{
	return m_cell_list.find(id) != m_cell_list.end();
}

//modifications
bool TraceGroup::AddCell(FL::pCell &cell, size_t frame)
{
	FL::TrackMapProcessor tm_processor;
	FL::CellListIter iter;
	return tm_processor.AddCell(m_track_map, cell, frame, iter);
}

bool TraceGroup::LinkCells(FL::CellList &list1, FL::CellList &list2,
	size_t frame1, size_t frame2, bool exclusive)
{
	FL::TrackMapProcessor tm_processor;
	return tm_processor.LinkCells(m_track_map,
		list1, list2, frame1, frame2, exclusive);
}

bool TraceGroup::IsolateCells(FL::CellList &list, size_t frame)
{
	FL::TrackMapProcessor tm_processor;
	return tm_processor.IsolateCells(m_track_map,
		list, frame);
}

bool TraceGroup::UnlinkCells(FL::CellList &list1, FL::CellList &list2,
	size_t frame1, size_t frame2)
{
	FL::TrackMapProcessor tm_processor;
	return tm_processor.UnlinkCells(m_track_map,
		list1, list2, frame1, frame2);
}

bool TraceGroup::CombineCells(FL::pCell &cell, FL::CellList &list,
	size_t frame)
{
	FL::TrackMapProcessor tm_processor;
	return tm_processor.CombineCells(m_track_map,
		cell, list, frame);
}

bool TraceGroup::DivideCells(FL::CellList &list, size_t frame)
{
	FL::TrackMapProcessor tm_processor;
	return tm_processor.DivideCells(m_track_map, list, frame);
}

bool TraceGroup::ReplaceCellID(unsigned int old_id, unsigned int new_id, size_t frame)
{
	FL::TrackMapProcessor tm_processor;
	return tm_processor.ReplaceCellID(m_track_map, old_id, new_id, frame);
}

bool TraceGroup::GetMappedRulers(FL::RulerList &rulers)
{
	size_t frame_num = m_track_map.GetFrameNum();
	if (m_ghost_num <= 0 ||
		m_cur_time < 0 ||
		m_cur_time >= frame_num)
		return false;

	//estimate verts size
	size_t remain_num = frame_num - m_cur_time - 1;
	size_t ghost_lead, ghost_tail;
	ghost_lead = m_draw_lead ?
		(remain_num>m_ghost_num ?
			m_ghost_num : remain_num) : 0;
	ghost_tail = m_draw_tail ?
		(m_cur_time >= m_ghost_num ?
			m_ghost_num : m_cur_time) : 0;

	FL::CellList temp_sel_list1, temp_sel_list2;
	FL::TrackMapProcessor tm_processor;

	if (m_draw_lead)
	{
		temp_sel_list1 = m_cell_list;
		for (size_t i = m_cur_time;
		i < m_cur_time + ghost_lead; ++i)
		{
			tm_processor.GetMappedRulers(m_track_map,
				temp_sel_list1, temp_sel_list2,
				rulers, i, i + 1);
			//swap
			temp_sel_list1 = temp_sel_list2;
			temp_sel_list2.clear();
		}
	}

	//clear ruler id
	for (FL::RulerListIter iter = rulers.begin();
	iter != rulers.end(); ++iter)
		(*iter)->Id(0);

	if (m_draw_tail)
	{
		temp_sel_list1 = m_cell_list;
		for (size_t i = m_cur_time;
		i > m_cur_time - ghost_tail; --i)
		{
			tm_processor.GetMappedRulers(
				m_track_map, temp_sel_list1, temp_sel_list2,
				rulers, i, i - 1);
			//sawp
			temp_sel_list1 = temp_sel_list2;
			temp_sel_list2.clear();
		}
	}

	return true;
}

bool TraceGroup::Load(wxString &filename)
{
	m_data_path = filename;
	FL::TrackMapProcessor tm_processor;
    std::string str = ws2s(m_data_path.ToStdWstring());
	return tm_processor.Import(m_track_map, str);
}

bool TraceGroup::Save(wxString &filename)
{
	m_data_path = filename;
	FL::TrackMapProcessor tm_processor;
    
    std::string str = ws2s(m_data_path.ToStdWstring());
	return tm_processor.Export(m_track_map, str);
}

unsigned int TraceGroup::Draw(vector<float> &verts)
{
	unsigned int result = 0;
	size_t frame_num = m_track_map.GetFrameNum();
	if (m_ghost_num <= 0 ||
		m_cur_time < 0 ||
		m_cur_time >= frame_num)
		return result;

	//estimate verts size
	size_t remain_num = frame_num - m_cur_time - 1;
	size_t ghost_lead, ghost_tail;
	ghost_lead = m_draw_lead ?
		(remain_num>m_ghost_num ?
		m_ghost_num : remain_num) : 0;
	ghost_tail = m_draw_tail ?
		(m_cur_time>=m_ghost_num ?
		m_ghost_num : m_cur_time) : 0;
	verts.reserve((ghost_lead + ghost_tail) *
		m_cell_list.size() * 3 * 6 * 3);//1.5 branches each

	FL::CellList temp_sel_list1, temp_sel_list2;
	FL::TrackMapProcessor tm_processor;

	if (m_draw_lead)
	{
		temp_sel_list1 = m_cell_list;
		for (size_t i = m_cur_time;
			i < m_cur_time + ghost_lead; ++i)
		{
			result += tm_processor.GetMappedEdges(m_track_map,
				temp_sel_list1, temp_sel_list2,
				verts, i, i + 1);
			//swap
			temp_sel_list1 = temp_sel_list2;
			temp_sel_list2.clear();
		}
	}

	if (m_draw_tail)
	{
		temp_sel_list1 = m_cell_list;
		for (size_t i = m_cur_time;
			i > m_cur_time - ghost_tail; --i)
		{
			result += tm_processor.GetMappedEdges(
				m_track_map, temp_sel_list1, temp_sel_list2,
				verts, i, i - 1);
			//sawp
			temp_sel_list1 = temp_sel_list2;
			temp_sel_list2.clear();
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int DataGroup::m_num = 0;
DataGroup::DataGroup()
{
	type = 5;//group
	m_num++;
	m_name = wxString::Format("Group %d", m_num);
	m_disp = true;
	m_sync_volume_prop = false;
	m_sync_volume_spc = false;
}

DataGroup::~DataGroup()
{
}

int DataGroup::GetBlendMode()
{
	if (!m_vd_list.empty())
		return m_vd_list[0]->GetBlendMode();
	else
		return 0;
}

//set gamma to all
void DataGroup::SetGammaAll(Color &gamma)
{
	SetGamma(gamma);
	for (int i=0; i<(int)m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd)
			vd->SetGamma(gamma);
	}
}

//set brightness to all
void DataGroup::SetBrightnessAll(Color &brightness)
{
	SetBrightness(brightness);
	for (int i=0; i<(int)m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd)
			vd->SetBrightness(brightness);
	}
}

//set Hdr to all
void DataGroup::SetHdrAll(Color &hdr)
{
	SetHdr(hdr);
	for (int i=0; i<(int)m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd)
			vd->SetHdr(hdr);
	}
}

//set sync red to all
void DataGroup::SetSyncRAll(bool sync_r)
{
	SetSyncR(sync_r);
	for (int i=0; i<(int)m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd)
			vd->SetSyncR(sync_r);
	}
}

//set sync green to all
void DataGroup::SetSyncGAll(bool sync_g)
{
	SetSyncG(sync_g);
	for (int i=0; i<(int)m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd)
			vd->SetSyncG(sync_g);
	}
}

//set sync blue to all
void DataGroup::SetSyncBAll(bool sync_b)
{
	SetSyncB(sync_b);
	for (int i=0; i<(int)m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd)
			vd->SetSyncB(sync_b);
	}
}

void DataGroup::ResetSync()
{
	int i;
	int cnt = 0;
	bool r_v = false;
	bool g_v = false;
	bool b_v = false;

	for (i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
		{
			Color c = vd->GetColor();
			bool r, g, b;
			r = g = b = false;
			cnt = 0;
			if (c.r()>0) {cnt++; r=true;}
			if (c.g()>0) {cnt++; g=true;}
			if (c.b()>0) {cnt++; b=true;}

			if (cnt > 1)
			{
				r_v = r_v||r;
				g_v = g_v||g;
				b_v = b_v||b;
			}
		}
	}

	SetSyncRAll(r_v);
	SetSyncGAll(g_v);
	SetSyncBAll(b_v);
}

//volume properties
void DataGroup::SetEnableAlpha(bool mode)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetEnableAlpha(mode);
	}
}

void DataGroup::SetAlpha(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetAlpha(dVal);
	}
}

void DataGroup::SetSampleRate(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetSampleRate(dVal);
	}
}

void DataGroup::SetBoundary(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetBoundary(dVal);
	}
}

void DataGroup::Set3DGamma(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->Set3DGamma(dVal);
	}
}

void DataGroup::SetOffset(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetOffset(dVal);
	}
}

void DataGroup::SetLeftThresh(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetLeftThresh(dVal);
	}
}

void DataGroup::SetRightThresh(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetRightThresh(dVal);
	}
}

void DataGroup::SetLowShading(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetLowShading(dVal);
	}
}

void DataGroup::SetHiShading(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetHiShading(dVal);
	}
}

void DataGroup::SetLuminance(double dVal)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetLuminance(dVal);
	}
}

void DataGroup::SetColormapMode(int mode)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetColormapMode(mode);
	}
}

void DataGroup::SetColormapDisp(bool disp)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetColormapDisp(disp);
	}
}

void DataGroup::SetColormapValues(double low, double high)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
		{
			double l, h;
			vd->GetColormapValues(l, h);
			vd->SetColormapValues(low<0?l:low, high<0?h:high);
		}
	}
}

void DataGroup::SetColormap(int value)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetColormap(value);
	}
}

void DataGroup::SetColormapProj(int value)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetColormapProj(value);
	}
}

void DataGroup::SetShading(bool shading)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetShading(shading);
	}
}

void DataGroup::SetShadow(bool shadow)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetShadow(shadow);
	}
}

void DataGroup::SetShadowParams(double val)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetShadowParams(val);
	}
}

void DataGroup::SetMode(int mode)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetMode(mode);
	}
}

void DataGroup::SetNR(bool val)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetNR(val);
	}
}
//inversion
void DataGroup::SetInterpolate(bool mode)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetInterpolate(mode);
	}
}

//inversion
void DataGroup::SetInvert(bool mode)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetInvert(mode);
	}
}

//blend mode
void DataGroup::SetBlendMode(int mode)
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
			vd->SetBlendMode(mode);
	}
}

//randomize color
void DataGroup::RandomizeColor()
{
	for (int i=0; i<GetVolumeNum(); i++)
	{
		VolumeData* vd = GetVolumeData(i);
		if (vd)
		{
			double hue = (double)rand()/(RAND_MAX) * 360.0;
			Color color(HSVColor(hue, 1.0, 1.0));
			vd->SetColor(color);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MeshGroup::m_num = 0;
MeshGroup::MeshGroup()
{
	type = 6;//mesh group
	m_num++;
	m_name = wxString::Format("MGroup %d", m_num);
	m_disp = true;
	m_sync_mesh_prop = false;
}

MeshGroup::~MeshGroup()
{
}

//randomize color
void MeshGroup::RandomizeColor()
{
	for (int i=0; i<GetMeshNum(); i++)
	{
		MeshData* md = GetMeshData(i);
		if (md)
		{
			double hue = (double)rand()/(RAND_MAX) * 360.0;
			Color color(HSVColor(hue, 1.0, 1.0));
			md->SetColor(color, MESH_COLOR_DIFF);
            Color amb = color * 0.3;
			md->SetColor(amb, MESH_COLOR_AMB);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DataManager::DataManager() :
m_vol_exb(0.0),
	m_vol_gam(1.0),
	m_vol_of1(1.0),
	m_vol_of2(1.0),
	m_vol_lth(0.0),
	m_vol_hth(1.0),
	m_vol_lsh(0.5),
	m_vol_hsh(10.0),
	m_vol_alf(0.5),
	m_vol_spr(1.5),
	m_vol_xsp(1.0),
	m_vol_ysp(1.0),
	m_vol_zsp(2.5),
	m_vol_lum(1.0),
	m_vol_cmp(0),
	m_vol_lcm(0.0),
	m_vol_hcm(1.0),
	m_vol_eap(true),
	m_vol_esh(true),
	m_vol_interp(true),
	m_vol_inv(false),
	m_vol_mip(false),
	m_vol_nrd(false),
	m_vol_shw(false),
	m_vol_swi(0.0),
	m_vol_test_wiref(false),
	m_use_defaults(true),
	m_override_vox(true)
{
	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(),NULL);
#ifdef _WIN32
	wxString dft = expath + "\\default_volume_settings.dft";
	if (!wxFileExists(dft))
		dft = wxStandardPaths::Get().GetUserConfigDir() + "\\default_volume_settings.dft";
#else
	wxString dft = expath + "/../Resources/default_volume_settings.dft";
#endif
	wxFileInputStream is(dft);
	if (!is.IsOk())
		return;
	wxFileConfig fconfig(is);

	double val;
	int ival;
	if (fconfig.Read("extract_boundary", &val))
		m_vol_exb = val;
	if (fconfig.Read("gamma", &val))
		m_vol_gam = val;
	if (fconfig.Read("low_offset", &val))
		m_vol_of1 = val;
	if (fconfig.Read("high_offset", &val))
		m_vol_of2 = val;
	if (fconfig.Read("low_thresholding", &val))
		m_vol_lth = val;
	if (fconfig.Read("high_thresholding", &val))
		m_vol_hth = val;
	if (fconfig.Read("low_shading", &val))
		m_vol_lsh = val;
	if (fconfig.Read("high_shading", &val))
		m_vol_hsh = val;
	if (fconfig.Read("alpha", &val))
		m_vol_alf = val;
	if (fconfig.Read("sample_rate", &val))
		m_vol_spr = val;
	if (fconfig.Read("x_spacing", &val))
		m_vol_xsp = val;
	if (fconfig.Read("y_spacing", &val))
		m_vol_ysp = val;
	if (fconfig.Read("z_spacing", &val))
		m_vol_zsp = val;
	if (fconfig.Read("luminance", &val))
		m_vol_lum = val;
	if (fconfig.Read("colormap", &ival))
		m_vol_cmp = ival;
	if (fconfig.Read("colormap_low", &val))
		m_vol_lcm = val;
	if (fconfig.Read("colormap_hi", &val))
		m_vol_hcm = val;

	bool bval;
	if (fconfig.Read("enable_alpha", &bval))
		m_vol_eap = bval;
	if (fconfig.Read("enable_shading", &bval))
		m_vol_esh = bval;
	if (fconfig.Read("enable_interp", &bval))
		m_vol_interp = bval;
	if (fconfig.Read("enable_inv", &bval))
		m_vol_inv = bval;
	if (fconfig.Read("enable_mip", &bval))
		m_vol_mip = bval;
	if (fconfig.Read("noise_rd", &bval))
		m_vol_nrd = bval;

	//shadow
	if (fconfig.Read("enable_shadow", &bval))
		m_vol_shw = bval;
	if (fconfig.Read("shadow_intensity", &val))
		m_vol_swi = val;

	//wavelength to color table
	m_vol_wav[0] = Color(1.0, 1.0, 1.0);
	m_vol_wav[1] = Color(1.0, 1.0, 1.0);
	m_vol_wav[2] = Color(1.0, 1.0, 1.0);
	m_vol_wav[3] = Color(1.0, 1.0, 1.0);

	//slice sequence
	m_sliceSequence = false;
	m_timeSequence = false;
	//compression
	m_compression = false;
	//skip brick
	m_skip_brick = false;
	//time sequence identifier
	m_timeId = "_T";
	//load mask
	m_load_mask = false;
}

DataManager::~DataManager()
{
	for (int i=0 ; i<(int)m_vd_list.size() ; i++)
		if (m_vd_list[i])
			delete m_vd_list[i];
	for (int i=0 ; i<(int)m_md_list.size() ; i++)
		if (m_md_list[i])
			delete m_md_list[i];
	for (int i=0; i<(int)m_reader_list.size(); i++)
		if (m_reader_list[i])
			delete m_reader_list[i];
	for (int i=0; i<(int)m_annotation_list.size(); i++)
		if (m_annotation_list[i])
			delete m_annotation_list[i];
}

void DataManager::ClearAll()
{
	for (int i=0 ; i<(int)m_vd_list.size() ; i++)
		if (m_vd_list[i])
			delete m_vd_list[i];
	for (int i=0 ; i<(int)m_md_list.size() ; i++)
		if (m_md_list[i])
			delete m_md_list[i];
	for (int i=0; i<(int)m_reader_list.size(); i++)
		if (m_reader_list[i])
			delete m_reader_list[i];
	for (int i=0; i<(int)m_annotation_list.size(); i++)
		if (m_annotation_list[i])
			delete m_annotation_list[i];
	m_vd_list.clear();
	m_md_list.clear();
	m_reader_list.clear();
	m_annotation_list.clear();
}

void DataManager::SetVolumeDefault(VolumeData* vd)
{
	if (m_use_defaults)
	{
		vd->SetWireframe(m_vol_test_wiref);
		vd->Set3DGamma(m_vol_gam);
		vd->SetBoundary(m_vol_exb);
		vd->SetOffset(m_vol_of1);
		vd->SetLeftThresh(m_vol_lth);
		vd->SetRightThresh(m_vol_hth);
		vd->SetAlpha(m_vol_alf);
		double amb, diff, spec, shine;
		vd->GetMaterial(amb, diff, spec, shine);
		vd->SetMaterial(m_vol_lsh, diff, spec, m_vol_hsh);
		vd->SetColormap(m_vol_cmp);
		vd->SetColormapValues(m_vol_lcm, m_vol_hcm);

		int xres, yres, zres;
		vd->GetResolution(xres, yres, zres);
		if (zres == 1) vd->SetSampleRate(1.0);
		else vd->SetSampleRate(m_vol_spr);
		if (!vd->GetSpcFromFile())
		{
			double zspcfac = (double)Max(xres,yres)/256.0;
			if (zspcfac < 1.0) zspcfac = 1.0;
			if (zres == 1) vd->SetSpacings(m_vol_xsp, m_vol_ysp, m_vol_xsp*zspcfac);
			else vd->SetSpacings(m_vol_xsp, m_vol_ysp, m_vol_zsp);
		}

		vd->SetEnableAlpha(m_vol_eap);
		int resx, resy, resz;
		vd->GetResolution(resx, resy, resz);
		if (resz > 1)
			vd->SetShading(m_vol_esh);
		else
			vd->SetShading(false);
		vd->SetMode(m_vol_mip?1:0);
		vd->SetNR(m_vol_nrd);
		//inversion
		vd->SetInterpolate(m_vol_interp);
		//inversion
		vd->SetInvert(m_vol_inv);

		//shadow
		vd->SetShadow(m_vol_shw);
		vd->SetShadowParams(m_vol_swi);
	}
}

//set project path
//when data and project are moved, use project file's path
//if data's directory doesn't exist
void DataManager::SetProjectPath(wxString path)
{
	m_prj_path.Clear();
	int sep = path.Find(GETSLASH(), true);
	if (sep != wxNOT_FOUND)
		m_prj_path = path.Left(sep);
}

wxString DataManager::SearchProjectPath(wxString &filename)
{
	int i;

	wxString pathname = filename;

	if (m_prj_path == "")
		return "";
	wxString search_str;
	for (i=pathname.Length()-1; i>=0; i--)
	{
		search_str.Prepend(pathname[i]);
		if (pathname[i]=='\\' || pathname[i]=='/')
		{
			wxString name_temp = m_prj_path + search_str;
			if (wxFileExists(name_temp))
				return name_temp;
		}
	}
	return "";
}

bool DataManager::DownloadToCurrentDir(wxString &filename)
{
	wxString pathname = filename;
	wxURL url(pathname);
	if (!url.IsOk())
		return false;
	url.GetProtocol().SetTimeout(10);
	wxString suffix = pathname.Mid(pathname.Find('.', true)).MakeLower();
	if(url.GetError() != wxURL_NOERR) return false;
	wxInputStream *in = url.GetInputStream();
	if(!in || !in->IsOk()) return false;
#define DOWNLOAD_BUFSIZE 8192
	unsigned char buffer[DOWNLOAD_BUFSIZE];
	size_t count = -1;
	wxMemoryBuffer mem_buf;
	while(!in->Eof() && count != 0)
	{
		in->Read(buffer, DOWNLOAD_BUFSIZE-1);
		count = in->LastRead();
		if(count > 0) mem_buf.AppendData(buffer, count);
	}

#ifdef _WIN32
	wxString tmpfname = wxStandardPaths::Get().GetTempDir() + "\\" + pathname.Mid(pathname.Find(wxT('/'), true)).Mid(1);
#else
    wxString tmpfname = wxStandardPaths::Get().GetTempDir() + "/" + pathname.Mid(pathname.Find(wxT('/'), true)).Mid(1);
#endif

	wxFile of(tmpfname, wxFile::write);
	of.Write(mem_buf.GetData(), mem_buf.GetDataLen());
	of.Close();

	filename = tmpfname;

	return true;
}

int DataManager::LoadVolumeData(wxString &filename, int type, int ch_num, int t_num)
{
	wxString pathname = filename;
	bool isURL = false;
	bool downloaded = false;
	wxString downloaded_filepath;
	bool downloaded_metadata = false;
	wxString downloaded_metadatafilepath;
	
	if (!wxFileExists(pathname))
	{
		pathname = SearchProjectPath(filename);
		if (!wxFileExists(pathname))
		{
			pathname = filename;
			if (!DownloadToCurrentDir(pathname)) return 0;
			//wxString tmpfname = pathname.Mid(pathname.Find(wxT('/'), true)).Mid(1);
			//pathname = tmpfname;
			
			downloaded = true;
			downloaded_filepath = pathname;

			if(type == LOAD_TYPE_BRKXML)
			{
				wxString metadatapath = filename;
				metadatapath = metadatapath.Mid(0, metadatapath.Find(wxT('/'), true)+1) + wxT("_metadata.xml");
				//std::remove("_metadata.xml");
				if (DownloadToCurrentDir(metadatapath))
				{
					downloaded_metadata = true;
					downloaded_metadatafilepath = metadatapath;
				}
				isURL = true;
			}
		}
	}

	int i;
	int result = 0;
	BaseReader* reader = 0;

	for (i=0; i<(int)m_reader_list.size(); i++)
	{
		wstring wstr = pathname.ToStdWstring();
		if (m_reader_list[i]->Match(wstr))
		{
			reader = m_reader_list[i];
			break;
		}
	}

	if (reader)
	{
		bool preprocess = false;
		if (reader->GetSliceSeq() != m_sliceSequence)
		{
			reader->SetSliceSeq(m_sliceSequence);
			preprocess = true;
		}
		if (reader->GetSliceSeq() != m_timeSequence)
		{
			reader->SetTimeSeq(m_timeSequence);
			preprocess = true;
		}
		if (reader->GetTimeId() != m_timeId.ToStdWstring())
		{
			wstring str_w = m_timeId.ToStdWstring();
			reader->SetTimeId(str_w);
			preprocess = true;
		}
		if (preprocess)
			reader->Preprocess();
	}
	else
	{
		//RGB tiff
		if (type == LOAD_TYPE_TIFF)
			reader = new TIFReader();
		else if (type == LOAD_TYPE_NRRD)
			reader = new NRRDReader();
		else if (type == LOAD_TYPE_OIB)
			reader = new OIBReader();
		else if (type == LOAD_TYPE_OIF)
			reader = new OIFReader();
		else if (type == LOAD_TYPE_LSM)
			reader = new LSMReader();
		else if (type == LOAD_TYPE_PVXML)
			reader = new PVXMLReader();
		else if (type == LOAD_TYPE_BRKXML)
			reader = new BRKXMLReader();

		m_reader_list.push_back(reader);
		wstring str_w = pathname.ToStdWstring();
		reader->SetFile(str_w);
		reader->SetSliceSeq(m_sliceSequence);
		reader->SetTimeSeq(m_timeSequence);
		str_w = m_timeId.ToStdWstring();
		reader->SetTimeId(str_w);

		if(isURL && type == LOAD_TYPE_BRKXML)
		{
			str_w = filename.ToStdWstring();
#ifdef _WIN32
			wchar_t slash = L'\\';
#else
			wchar_t slash = L'/';
#endif
			wstring dir_name = str_w.substr(0, str_w.find_last_of(slash)+1);
			((BRKXMLReader *)reader)->SetDir(str_w);
		}

		reader->Preprocess();

		if (type == LOAD_TYPE_BRKXML && !((BRKXMLReader *)reader)->GetExMetadataURL().empty())
		{
			wxString metadatapath = ((BRKXMLReader *)reader)->GetExMetadataURL();
			if (DownloadToCurrentDir(metadatapath))
			{
				downloaded_metadata = true;
				downloaded_metadatafilepath = metadatapath;
				((BRKXMLReader *)reader)->loadMetadata(metadatapath.ToStdWstring());
			}
		}
	}

	//align data for compression if vtc is not supported
	if (!GLEW_NV_texture_compression_vtc && m_compression)
	{
		reader->SetResize(1);
		reader->SetAlignment(4);
	}

	int chan = reader->GetChanNum();
	for (i=(ch_num>=0?ch_num:0);
		i<(ch_num>=0?ch_num+1:chan); i++)
	{
		VolumeData *found_vd = NULL;
		VolumeData *vd;
		for (int j = 0; j < m_vd_list.size(); j++)
			if (m_vd_list[j] && m_vd_list[j]->GetReader() == reader && !m_vd_list[j]->GetDup() &&
				m_vd_list[j]->GetCurChannel() == i)
				found_vd = m_vd_list[j];
		if (found_vd)
		{
			vd = VolumeData::DeepCopy(*found_vd, true, this);
			AddVolumeData(vd);
			result++;
		}
		else
		{
			VolumeData *vd = new VolumeData();
			vd->SetSkipBrick(m_skip_brick);
			Nrrd* data = reader->Convert(t_num>=0?t_num:reader->GetCurTime(), i, true);
			if (!data)
				continue;

			wxString name;
			if (type != LOAD_TYPE_BRKXML)
			{
				name = wxString(reader->GetDataName());
				if (chan > 1)
					name += wxString::Format("_Ch%d", i+1);
			}
			else 
			{
				BRKXMLReader* breader = (BRKXMLReader*)reader;
				name = reader->GetDataName();
				name = name.Mid(0, name.find_last_of(wxT('.')));
				if(ch_num > 1) name = wxT("_Ch") + wxString::Format("%i", i);
				pathname = filename;
				breader->SetCurChan(i);
				breader->SetCurTime(0);
			}

			bool valid_spc = reader->IsSpcInfoValid();
			if (vd && vd->Load(data, name, pathname, (type == LOAD_TYPE_BRKXML) ? (BRKXMLReader*)reader : NULL))
			{
				if (m_load_mask)
				{
					//mask
					MSKReader msk_reader;
					std::wstring str = reader->GetPathName();
					msk_reader.SetFile(str);
					Nrrd* mask = msk_reader.Convert(t_num>=0?t_num:reader->GetCurTime(), i, true);
					if (mask)
						vd->LoadMask(mask);
					//label mask
					LBLReader lbl_reader;
					str = reader->GetPathName();
					lbl_reader.SetFile(str);

					Nrrd* label = lbl_reader.Convert(t_num>=0?t_num:reader->GetCurTime(), i, true);
					if (label)
						vd->LoadLabel(label);
				}
				if (type == LOAD_TYPE_BRKXML) ((BRKXMLReader*)reader)->SetLevel(0);
				//for 2D data
				int xres, yres, zres;
				vd->GetResolution(xres, yres, zres);
				double zspcfac = (double)Max(xres,yres)/256.0;
				if (zspcfac < 1.0) zspcfac = 1.0;
				if (zres == 1) vd->SetBaseSpacings(reader->GetXSpc(), reader->GetYSpc(), reader->GetXSpc()*zspcfac);
				else vd->SetBaseSpacings(reader->GetXSpc(), reader->GetYSpc(), reader->GetZSpc());
				vd->SetSpcFromFile(valid_spc);
				vd->SetScalarScale(reader->GetScalarScale());
				vd->SetMaxValue(reader->GetMaxValue());
				vd->SetCurTime(reader->GetCurTime());
				vd->SetCurChannel(i);
				//++
				result++;
			}
			else
			{
				delete vd;
				continue;
			}
			vd->SetReader(reader);
			vd->SetCompression(m_compression);
			AddVolumeData(vd);

			SetVolumeDefault(vd);

			//get excitation wavelength
			double wavelength = reader->GetExcitationWavelength(i);
			if (wavelength > 0.0) {
				FLIVR::Color col = GetWavelengthColor(wavelength);
				vd->SetColor(col);
			}
			else if (wavelength < 0.) {
				FLIVR::Color white = Color(1.0, 1.0, 1.0);
				vd->SetColor(white);
			}
			else
			{
				FLIVR::Color white = Color(1.0, 1.0, 1.0);
				FLIVR::Color red   = Color(1.0, 0.0, 0.0);
				FLIVR::Color green = Color(0.0, 1.0, 0.0);
				FLIVR::Color blue  = Color(0.0, 0.0, 1.0);
				if (chan == 1) {
					vd->SetColor(white);
				}
				else
				{
					if (i == 0)
						vd->SetColor(red);
					else if (i == 1)
						vd->SetColor(green);
					else if (i == 2)
						vd->SetColor(blue);
					else
						vd->SetColor(white);
				}
			}
		}

	}

	if (downloaded) wxRemoveFile(downloaded_filepath);
	if (downloaded_metadata) wxRemoveFile(downloaded_metadatafilepath);

	return result;
}

int DataManager::LoadMeshData(wxString &filename)
{
	wxString pathname = filename;
	if (!wxFileExists(pathname))
	{
		pathname = SearchProjectPath(filename);
		if (!wxFileExists(pathname))
			return 0;
	}

	MeshData *md = new MeshData();
	md->Load(pathname);

	wxString name = md->GetName();
	wxString new_name = name;
	int i;
	for (i=1; CheckNames(new_name, DATA_MESH); i++)
		new_name = name+wxString::Format("_%d", i);
	if (i>1)
		md->SetName(new_name);
	m_md_list.push_back(md);

	return 1;
}

int DataManager::LoadMeshData(GLMmodel* mesh)
{
	if (!mesh) return 0;

	MeshData *md = new MeshData();
	md->Load(mesh);

	wxString name = md->GetName();
	wxString new_name = name;
	int i;
	for (i=1; CheckNames(new_name, DATA_MESH); i++)
		new_name = name+wxString::Format("_%d", i);
	if (i>1)
		md->SetName(new_name);
	m_md_list.push_back(md);

	return 1;
}

VolumeData* DataManager::GetVolumeData(int index)
{
	if (index>=0 && index<(int)m_vd_list.size())
		return m_vd_list[index];
	else
		return 0;
}

MeshData* DataManager::GetMeshData(int index)
{
	if (index>=0 && index<(int)m_md_list.size())
		return m_md_list[index];
	else
		return 0;
}

VolumeData* DataManager::GetVolumeData(const wxString &name)
{
	for (int i=0 ; i<(int)m_vd_list.size() ; i++)
	{
		if (name == m_vd_list[i]->GetName())
		{
			return m_vd_list[i];
		}
	}
	return 0;
}

MeshData* DataManager::GetMeshData(wxString &name)
{
	for (int i=0 ; i<(int)m_md_list.size() ; i++)
	{
		if (name == m_md_list[i]->GetName())
		{
			return m_md_list[i];
		}
	}
	return 0;
}

int DataManager::GetVolumeIndex(wxString &name)
{
	for (int i=0 ; i<(int)m_vd_list.size() ; i++)
	{
		if (!m_vd_list[i])
			continue;
		if (name == m_vd_list[i]->GetName())
		{
			return i;
		}
	}
	return -1;
}

int DataManager::GetMeshIndex(wxString &name)
{
	for (int i=0 ; i<(int)m_md_list.size() ; i++)
	{
		if (name == m_md_list[i]->GetName())
		{
			return i;
		}
	}
	return -1;
}

void DataManager::ReplaceVolumeData(int index, VolumeData *vd)
{
	if (index < 0 || index >= m_vd_list.size())
		return;

	VolumeData* data = m_vd_list[index];
	if (data)
	{
		delete data;
		data = 0;
	}
	m_vd_list[index] = vd;
}

void DataManager::RemoveVolumeData(int index)
{
	if (index < 0 || index >= m_vd_list.size())
		return;

	VolumeData* data = m_vd_list[index];
	if (data)
	{
		m_vd_list.erase(m_vd_list.begin()+index);
		delete data;
		data = 0;
	}	
}

void DataManager::RemoveVolumeData(const wxString &name)
{
	for (int i=0 ; i<(int)m_vd_list.size() ; i++)
	{
		if (name == m_vd_list[i]->GetName())
		{
			RemoveVolumeData(i);
		}
	}
}

void DataManager::RemoveVolumeDataset(BaseReader *reader, int channel)
{
	
	for (int i = m_vd_list.size()-1; i >= 0; i--)
	{
		VolumeData* data = m_vd_list[i];
		if (data && data->GetReader() == reader && data->GetCurChannel() == channel)
		{
			m_vd_list.erase(m_vd_list.begin()+i);
			delete data;
			data = 0;
		}
	}
}

void DataManager::RemoveMeshData(int index)
{
	MeshData* data = m_md_list[index];
	if (data)
	{
		m_md_list.erase(m_md_list.begin()+index);
		delete data;
		data = 0;
	}
}

void DataManager::RemoveMeshData(const wxString &name)
{
	for (int i=0 ; i<(int)m_md_list.size() ; i++)
	{
		if (name == m_md_list[i]->GetName())
		{
			RemoveMeshData(i);
		}
	}
}

int DataManager::GetVolumeNum()
{
	return m_vd_list.size();
}

int DataManager::GetMeshNum()
{
	return m_md_list.size();
}

void DataManager::AddVolumeData(VolumeData* vd)
{
	if (!vd)
		return;

	wxString name = vd->GetName();
	wxString new_name = name;

	int i;
	for (i=1; CheckNames(new_name, DATA_VOLUME); i++)
		new_name = name+wxString::Format("_%d", i);

	if (i>1)
		vd->SetName(new_name);
	
	if (m_override_vox)
	{
		if (m_vd_list.size() > 0)
		{
			double spcx, spcy, spcz;
			m_vd_list[0]->GetBaseSpacings(spcx, spcy, spcz);
			vd->SetBaseSpacings(spcx, spcy, spcz);
			vd->SetSpcFromFile(true);
		}
	}
	
	m_vd_list.push_back(vd);
}

void DataManager::AddMeshData(MeshData *md)
{
	wxString name = md->GetName();
	wxString new_name = name;
	int i;
	for (i=1; CheckNames(new_name, DATA_MESH); i++)
		new_name = name+wxString::Format("_%d", i);
	if (i>1)
		md->SetName(new_name);
	m_md_list.push_back(md);

	return;
}

VolumeData* DataManager::DuplicateVolumeData(VolumeData* vd, bool use_default_settings)
{
	VolumeData* vd_new = 0;

	if (vd)
	{
		vd_new = VolumeData::DeepCopy(*vd, use_default_settings, this);
		if (vd_new) AddVolumeData(vd_new);
	}

	return vd_new;
}

MeshData* DataManager::DuplicateMeshData(MeshData* md, bool use_default_settings)
{
	MeshData* md_new = 0;

	if (md)
	{
		md_new = MeshData::DeepCopy(*md, use_default_settings, this);
		if (md_new) AddMeshData(md_new);
	}

	return md_new;
}

int DataManager::LoadAnnotations(wxString &filename)
{
	wxString pathname = filename;
	if (!wxFileExists(pathname))
	{
		pathname = SearchProjectPath(filename);
		if (!wxFileExists(pathname))
			return 0;
	}

	Annotations* ann = new Annotations();
	ann->Load(pathname, this);

	wxString name = ann->GetName();
	wxString new_name = name;
	int i;
	for (i=1; CheckNames(new_name, DATA_ANNOTATIONS); i++)
		new_name = name+wxString::Format("_%d", i);
	if (i>1)
		ann->SetName(new_name);
	m_annotation_list.push_back(ann);

	return 1;
}

void DataManager::AddAnnotations(Annotations* ann)
{
	if (!ann)
		return;

	wxString name = ann->GetName();
	wxString new_name = name;

	int i;
	for (i=1; CheckNames(new_name, DATA_ANNOTATIONS); i++)
		new_name = name+wxString::Format("_%d", i);

	if (i>1)
		ann->SetName(new_name);

	m_annotation_list.push_back(ann);
}

void DataManager::RemoveAnnotations(int index)
{
	Annotations* ann = m_annotation_list[index];
	if (ann)
	{
		m_annotation_list.erase(m_annotation_list.begin()+index);
		delete ann;
		ann = 0;
	}
}

void DataManager::RemoveAnnotations(const wxString &name)
{
	for (int i=0 ; i<(int)m_annotation_list.size() ; i++)
	{
		if (name == m_annotation_list[i]->GetName())
		{
			RemoveAnnotations(i);
		}
	}
}

int DataManager::GetAnnotationNum()
{
	return m_annotation_list.size();
}

Annotations* DataManager::GetAnnotations(int index)
{
	if (index>=0 && index<(int)m_annotation_list.size())
		return m_annotation_list[index];
	else
		return 0;
}

Annotations* DataManager::GetAnnotations(wxString &name)
{
	for (int i=0; i<(int)m_annotation_list.size(); i++)
	{
		if (name == m_annotation_list[i]->GetName())
			return m_annotation_list[i];
	}
	return 0;
}

int DataManager::GetAnnotationIndex(wxString &name)
{
	for (int i=0; i<(int)m_annotation_list.size(); i++)
	{
		if (!m_annotation_list[i])
			continue;
		if (name == m_annotation_list[i]->GetName())
			return i;
	}
	return -1;
}

wxString DataManager::CheckNewName(const wxString &name, int type)
{
	wxString result = name;

	int i;
	for (i=1; CheckNames(result, type); i++)
		result = name+wxString::Format("_%d", i);

	return result;
}

bool DataManager::CheckNames(const wxString &str, int type)
{
	bool result = false;

	switch(type)
	{
	case DATA_VOLUME:
		for (unsigned int i=0; i<m_vd_list.size(); i++)
		{
			VolumeData* vd = m_vd_list[i];
			if (vd && vd->GetName()==str)
			{
				result = true;
				break;
			}
		}
		break;

	case DATA_MESH:
		for (unsigned int i=0; i<m_md_list.size(); i++)
		{
			MeshData* md = m_md_list[i];
			if (md && md->GetName()==str)
			{
				result = true;
				break;
			}
		}
		break;

	case DATA_ANNOTATIONS:
		for (unsigned int i=0; i<m_annotation_list.size(); i++)
		{
			Annotations* ann = m_annotation_list[i];
			if (ann && ann->GetName()==str)
			{
				result = true;
				break;
			}
		}
		break;
	}

	return result;
}

bool DataManager::CheckNames(const wxString &str)
{
	bool result = false;
	for (unsigned int i=0; i<m_vd_list.size(); i++)
	{
		VolumeData* vd = m_vd_list[i];
		if (vd && vd->GetName()==str)
		{
			result = true;
			break;
		}
	}
	if (!result)
	{
		for (unsigned int i=0; i<m_md_list.size(); i++)
		{
			MeshData* md = m_md_list[i];
			if (md && md->GetName()==str)
			{
				result = true;
				break;
			}
		}
	}
	if (!result)
	{
		for (unsigned int i=0; i<m_annotation_list.size(); i++)
		{
			Annotations* ann = m_annotation_list[i];
			if (ann && ann->GetName()==str)
			{
				result = true;
				break;
			}
		}
	}
	return result;
}

void DataManager::SetWavelengthColor(int c1, int c2, int c3, int c4)
{
	switch (c1)
	{
	case 1:
		m_vol_wav[0] = Color(1.0, 0.0, 0.0);
		break;
	case 2:
		m_vol_wav[0] = Color(0.0, 1.0, 0.0);
		break;
	case 3:
		m_vol_wav[0] = Color(0.0, 0.0, 1.0);
		break;
	case 4:
		m_vol_wav[0] = Color(1.0, 0.0, 1.0);
		break;
	case 5:
		m_vol_wav[0] = Color(1.0, 1.0, 1.0);
		break;
	default:
		m_vol_wav[0] = Color(1.0, 1.0, 1.0);
		break;
	}
	switch (c2)
	{
	case 1:
		m_vol_wav[1] = Color(1.0, 0.0, 0.0);
		break;
	case 2:
		m_vol_wav[1] = Color(0.0, 1.0, 0.0);
		break;
	case 3:
		m_vol_wav[1] = Color(0.0, 0.0, 1.0);
		break;
	case 4:
		m_vol_wav[1] = Color(1.0, 0.0, 1.0);
		break;
	case 5:
		m_vol_wav[1] = Color(1.0, 1.0, 1.0);
		break;
	default:
		m_vol_wav[1] = Color(1.0, 1.0, 1.0);
		break;
	}
	switch (c3)
	{
	case 1:
		m_vol_wav[2] = Color(1.0, 0.0, 0.0);
		break;
	case 2:
		m_vol_wav[2] = Color(0.0, 1.0, 0.0);
		break;
	case 3:
		m_vol_wav[2] = Color(0.0, 0.0, 1.0);
		break;
	case 4:
		m_vol_wav[2] = Color(1.0, 0.0, 1.0);
		break;
	case 5:
		m_vol_wav[2] = Color(1.0, 1.0, 1.0);
		break;
	default:
		m_vol_wav[2] = Color(1.0, 1.0, 1.0);
		break;
	}
	switch (c4)
	{
	case 1:
		m_vol_wav[3] = Color(1.0, 0.0, 0.0);
		break;
	case 2:
		m_vol_wav[3] = Color(0.0, 1.0, 0.0);
		break;
	case 3:
		m_vol_wav[3] = Color(0.0, 0.0, 1.0);
		break;
	case 4:
		m_vol_wav[3] = Color(1.0, 0.0, 1.0);
		break;
	case 5:
		m_vol_wav[3] = Color(1.0, 1.0, 1.0);
		break;
	default:
		m_vol_wav[0] = Color(1.0, 1.0, 1.0);
		break;
	}
}

Color DataManager::GetWavelengthColor(double wavelength)
{
	if (wavelength < 340.0)
		return Color(1.0, 1.0, 1.0);
	else if (wavelength < 440.0)
		return m_vol_wav[0];
	else if (wavelength < 500.0)
		return m_vol_wav[1];
	else if (wavelength < 600.0)
		return m_vol_wav[2];
	else if (wavelength < 750.0)
		return m_vol_wav[3];
	else
		return Color(1.0, 1.0, 1.0);
}
