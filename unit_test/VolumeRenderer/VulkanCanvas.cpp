#include "VulkanCanvas.h"
#include "VulkanException.h"
#include "wxVulkanTutorialApp.h"
#include <vulkan/vulkan.h>
#include <fstream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>

#include "compatibility.h"

#pragma comment(lib, "vulkan-1.lib")

/*

Enable DISCRIPTOR_UPDATE_AFTER_BIND

instance extension	VkInstanceCreateInfo.ppEnabledExtensionNames	"VK_KHR_get_physical_device_properties2"
device extension 	VkDeviceCreateInfo.ppEnabledExtensionNames	"VK_EXT_descriptor_indexing", "VK_KHR_maintenance3"

--------------------
***VkDevice***

VkPhysicalDeviceDescriptorIndexingFeaturesEXT ind = {};
ind.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
ind.pNext = nullptr;
ind.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
ind.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

VkDeviceCreateInfo createInfo;
createInfo.pNext = &ind;
--------------------

--------------------
***DiscriptorSetLayout***

std::array<VkDescriptorBindingFlagsEXT, 2> bflags = { 0, VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT};
VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bext;
bext.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
bext.pNext = nullptr;
bext.bindingCount = 3;
bext.pBindingFlags = bflags.data();

VkDescriptorSetLayoutCreateInfo layoutInfo;
layoutInfo.pNext = &bext;
--------------------

--------------------
***DiscriptorPool***

VkDescriptorPoolCreateInfo poolInfo = {};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = poolSizes.size();
poolInfo.pPoolSizes = poolSizes.data();
poolInfo.maxSets = 1;

poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
--------------------

***Texture Compression***
VkPhysicalDeviceFeatures deviceFeatures = {};
deviceFeatures.textureCompressionBC = VK_TRUE;

*/

/*

Enable PUSH_DISCRIPTOR_SET

instance extension	VkInstanceCreateInfo.ppEnabledExtensionNames	"VK_KHR_get_physical_device_properties2"
device extension 	VkDeviceCreateInfo.ppEnabledExtensionNames	"VK_KHR_push_descriptor", "VK_KHR_maintenance3"

PFN_vkCmdPushDescriptorSetKHR p_vkCmdPushDescriptorSetKHR;
p_vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(m_logicalDevice, "vkCmdPushDescriptorSetKHR");

--------------------
***DiscriptorSetLayout***
VkDescriptorSetLayoutCreateInfo layoutInfo;
layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
--------------------

do not use vkAllocateDescriptorSets, vkCmdBindDescriptorSets, vkUpdateDescriptorSets

you don't need descriptor pool!

*/

long long milliseconds_now() {
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount64();
	}
}

VulkanCanvas::VulkanCanvas(wxWindow *pParent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name)
    : wxWindow(pParent, id, pos, size, style, name)
{
	m_tex_mat = glm::mat4();

	m_init_view = false;
	m_persp = false;
	m_aov = 15.0;
	m_scale_factor = 1.0;

	m_distance = 10.0;
	m_init_dist = 10.0;
	m_transx = m_transy = m_transz = 0.0;
	m_ctrx = m_ctry = m_ctrz = 0.0;
	m_rotx = m_roty = m_rotz = 0.0;
	m_up = FLIVR::Vector(0.0, 1.0, 0.0);
	m_head = FLIVR::Vector(0.0, 0.0, -1.0);

	m_obj_transx = m_obj_transy = m_obj_transz = 0.0;
	m_obj_ctrx = m_obj_ctry = m_obj_ctrz = 0.0;
	m_obj_rotx = m_obj_roty = m_obj_rotz = 0.0;

	m_radius = 5.0;


	m_vulkan = make_shared<VVulkan>();
	m_vulkan->initVulkan();
#if defined(_WIN32)
	m_vulkan->setWindow((HWND)GetHWND(), GetModuleHandle(NULL));
#elif (defined(__WXMAC__))
	m_vulkan->setWindow(GetHandle());
#endif
	m_vulkan->prepare();

	m_v2drender = make_shared<Vulkan2dRender>(m_vulkan);
	FLIVR::TextureRenderer::set_vulkan(m_vulkan, m_v2drender);
	
	FLIVR::TextureRenderer::set_mem_swap(false);
	
	/*FLIVR::TextureRenderer::set_mem_swap(true);
	FLIVR::TextureRenderer::set_large_data_size(1.0);
	FLIVR::TextureRenderer::set_force_brick_size(180);
	FLIVR::TextureRenderer::set_up_time(100);*/

	FLIVR::TextureRenderer::set_update_order(1);

	FLIVR::VolumeRenderer::init();

	LoadVolume();
	InitView(INIT_BOUNDS | INIT_CENTER | INIT_TRANSL | INIT_OBJ_TRANSL | INIT_ROTATE);

	uint64_t st_time, ed_time;
	wxString dbgstr;

	////////////////////
	st_time = milliseconds_now();

	DrawMask();

	ed_time = milliseconds_now();
	dbgstr = wxString::Format("draw_mask time: %lld\n", ed_time-st_time);
	OutputDebugStringA(dbgstr.ToStdString().c_str());
	
	//////////////////////
	//m_vulkan->vulkanDevice->clear_tex_pool();
	//st_time = milliseconds_now();

	//DrawMask();

	//ed_time = milliseconds_now();
	//dbgstr = wxString::Format("draw_mask time: %lld\n", ed_time - st_time);
	//OutputDebugStringA(dbgstr.ToStdString().c_str());

	////////////////////////
	//st_time = milliseconds_now();

	//DrawMask();

	//ed_time = milliseconds_now();
	//dbgstr = wxString::Format("draw_mask time: %lld\n", ed_time - st_time);
	//OutputDebugStringA(dbgstr.ToStdString().c_str());

	///////////////////////
	//st_time = milliseconds_now();
	//
	//m_vr->return_mask();

	//ed_time = milliseconds_now();
	//dbgstr = wxString::Format("return_mask time: %lld\n", ed_time - st_time);
	//OutputDebugStringA(dbgstr.ToStdString().c_str());

	///////////////////////
	//vector<FLIVR::TextureBrick*>* bricks = m_vol->get_bricks();
	//int c = m_vol->nmask();
	//for (unsigned int i = 0; i < bricks->size(); i++)
	//{
	//	FLIVR::TextureBrick* b = (*bricks)[i];
	//	b->set_dirty(c, true);
	//}

	//st_time = milliseconds_now();

	//m_vr->return_mask();

	//ed_time = milliseconds_now();
	//dbgstr = wxString::Format("return_mask time: %lld\n", ed_time - st_time);
	//OutputDebugStringA(dbgstr.ToStdString().c_str());

	////////////////////
	//NRRDWriter writer;
	//writer.SetData( m_vol->get_nrrd(m_vol->nmask()) );
	//writer.Save(L"mask.nrrd", 0);

    m_startTime = std::chrono::high_resolution_clock::now();
    m_timer = std::make_unique<wxTimer>(this, TIMERNUMBER);
    m_timer->Start(INTERVAL);

	Bind(wxEVT_PAINT, &VulkanCanvas::OnPaint, this);
	Bind(wxEVT_SIZE, &VulkanCanvas::OnResize, this);
	Bind(wxEVT_TIMER, &VulkanCanvas::OnTimer, this);
	Bind(wxEVT_ERASE_BACKGROUND, &VulkanCanvas::OnErase, this);
}


VulkanCanvas::~VulkanCanvas() noexcept
{
    m_timer->Stop();

	m_paint.reset();

	m_vr.reset();
	m_vol.reset();

	FLIVR::VolumeRenderer::finalize();
	FLIVR::TextureRenderer::finalize_vulkan();

	m_v2drender.reset();
	m_vulkan.reset();
}

void VulkanCanvas::LoadVolume()
{
	Nrrd* data = nullptr;
	std:string fname = ".\\1gb.nrrd";

	m_reader.SetFile(fname);
	m_reader.Preprocess();
	data = m_reader.Convert(0, 0, true);
	if (!data)
		throw std::runtime_error("could not open 1gb.nrrd");

	m_vol = make_unique<FLIVR::Texture>();
	m_vol->set_use_priority(false);
	if (!m_vol->build(data, nullptr, 0, 256, 0, 0))
		throw std::runtime_error("Texture::build failed");
	m_vol->set_base_spacings(m_reader.GetXSpc(), m_reader.GetYSpc(), m_reader.GetZSpc());

	vector<FLIVR::Plane*> planelist(0);
	FLIVR::Plane* plane = 0;
	//x
	plane = new FLIVR::Plane(FLIVR::Point(0.0, 0.0, 0.0), FLIVR::Vector(1.0, 0.0, 0.0));
	planelist.push_back(plane);
	plane = new FLIVR::Plane(FLIVR::Point(1.0, 0.0, 0.0), FLIVR::Vector(-1.0, 0.0, 0.0));
	planelist.push_back(plane);
	//y
	plane = new FLIVR::Plane(FLIVR::Point(0.0, 0.0, 0.0), FLIVR::Vector(0.0, 1.0, 0.0));
	planelist.push_back(plane);
	plane = new FLIVR::Plane(FLIVR::Point(0.0, 1.0, 0.0), FLIVR::Vector(0.0, -1.0, 0.0));
	planelist.push_back(plane);
	//z
	plane = new FLIVR::Plane(FLIVR::Point(0.0, 0.0, 0.0), FLIVR::Vector(0.0, 0.0, 1.0));
	planelist.push_back(plane);
	plane = new FLIVR::Plane(FLIVR::Point(0.0, 0.0, 1.0), FLIVR::Vector(0.0, 0.0, -1.0));
	planelist.push_back(plane);

	m_vr = std::make_unique<FLIVR::VolumeRenderer>(m_vol.get(), planelist, true);
	m_vr->set_sampling_rate(0.1);
	m_vr->set_shading(true);
	m_vr->set_mode(FLIVR::TextureRenderer::MODE_OVER);

	m_vr->set_gamma3d(0.6);

	//mask
	size_t res_x = m_vol->nx();
	size_t res_y = m_vol->ny();
	size_t res_z = m_vol->nz();

	//prepare the texture bricks for the mask
	if (m_vol->add_empty_mask())
	{
		//add the nrrd data for mask
		Nrrd* nrrd_mask = nrrdNew();
		size_t mem_size = res_x * res_y * res_z;
		uint8* val8 = new (std::nothrow) uint8[mem_size];
		if (!val8)
		{
			wxMessageBox("Not enough memory. Please save project and restart.");
			return;
		}
		double spcx, spcy, spcz;
		m_vol->get_spacings(spcx, spcy, spcz);
		memset((void*)val8, 0, mem_size * sizeof(uint8));
		nrrdWrap(nrrd_mask, val8, nrrdTypeUChar, 3, res_x, res_y, res_z);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoSize, res_x, res_y, res_z);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoSpacing, spcx, spcy, spcz);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
		nrrdAxisInfoSet(nrrd_mask, nrrdAxisInfoMax, spcx * res_x, spcy * res_y, spcz * res_z);

		m_vol->set_nrrd(nrrd_mask, m_vol->nmask());
	}

	//mask paint tex
	int pw = 256;
	int ph = 256;
	unsigned char *paint = new unsigned char[(size_t)pw * ph];
	for (int y = 0; y < ph; y++)
	{
			for (int x = 0; x < pw; x++)
			{
				if (y >= ph / 4 && y <= ph / 4 * 3 &&
					x >= pw / 4 && x <= pw / 4 * 3)
					paint[y * pw + x] = 255;
				else
					paint[y * pw + x] = 0;
			}
	}

	m_paint = std::make_shared<vks::VTexture>();
	m_paint = m_vulkan->vulkanDevice->GenTexture2D(VK_FORMAT_R8_UNORM, VK_FILTER_NEAREST, 256, 256);
	m_vulkan->vulkanDevice->UploadTexture(m_paint, paint);

	m_vr->set_2d_mask(m_paint);

	delete[] paint;
}

void VulkanCanvas::DrawMask()
{
	m_scale_factor = 1.3;
	m_aov = 30.0;
	m_persp = true;

	wxSize view_size = GetSize();

	HandleProjection(view_size.GetWidth(), view_size.GetHeight());
	//Transformation
	HandleCamera();
	glm::mat4 mv_temp = m_mv_mat;
	//translate object
	m_mv_mat = glm::translate(m_mv_mat, glm::vec3(m_obj_transx, m_obj_transy, m_obj_transz));
	//rotate object
	m_mv_mat = glm::rotate(m_mv_mat, float(m_obj_rotx), glm::vec3(1.0, 0.0, 0.0));
	m_mv_mat = glm::rotate(m_mv_mat, float(m_obj_roty), glm::vec3(0.0, 1.0, 0.0));
	m_mv_mat = glm::rotate(m_mv_mat, float(m_obj_rotz), glm::vec3(0.0, 0.0, 1.0));
	//center object
	m_mv_mat = glm::translate(m_mv_mat, glm::vec3(-m_obj_ctrx, -m_obj_ctry, -m_obj_ctrz));

	m_vr->set_matrices(m_mv_mat, m_proj_mat, m_tex_mat);

	m_vr->draw_mask(0, 2, 0, 0.215, 1.0, 0.0, 0.215, 0.0, 0.0, false, false);
}

void VulkanCanvas::InitView(unsigned int type)
{
	int i;

	if (type & INIT_BOUNDS)
	{
		m_bounds.reset();

		FLIVR::BBox vb;
		m_vol->get_bounds(vb);
		m_bounds.extend(vb);
		
		if (m_bounds.valid())
		{
			FLIVR::Vector diag = m_bounds.diagonal();
			m_radius = sqrt(diag.x() * diag.x() + diag.y() * diag.y()) / 2.0;
			m_near_clip = m_radius / 1000.0;
			m_far_clip = m_radius * 100.0;
		}
	}

	if (type & INIT_CENTER)
	{
		if (m_bounds.valid())
		{
			m_obj_ctrx = (m_bounds.min().x() + m_bounds.max().x()) / 2.0;
			m_obj_ctry = (m_bounds.min().y() + m_bounds.max().y()) / 2.0;
			m_obj_ctrz = (m_bounds.min().z() + m_bounds.max().z()) / 2.0;
		}
	}

	if (type & INIT_TRANSL/*||!m_init_view*/)
	{
		m_distance = m_radius / tan(d2r(m_aov / 2.0));
		m_init_dist = m_distance;
		m_transx = 0.0;
		m_transy = 0.0;
		m_transz = m_distance;
		m_scale_factor = 1.0;
	}

	if (type & INIT_OBJ_TRANSL)
	{
		m_obj_transx = 0.0;
		m_obj_transy = 0.0;
		m_obj_transz = 0.0;
	}

	if (type & INIT_ROTATE || !m_init_view)
	{
		m_q = FLIVR::Quaternion(0, 0, 0, 1);
		m_q.ToEuler(m_rotx, m_roty, m_rotz);
	}

	m_init_view = true;

}

void VulkanCanvas::HandleProjection(int nx, int ny)
{
	double aspect = (double)nx / (double)ny;
	m_distance = m_radius / tan(d2r(m_aov / 2.0)) / m_scale_factor;
	if (aspect > 1.0)
	{
		m_ortho_left = -m_radius * aspect / m_scale_factor;
		m_ortho_right = -m_ortho_left;
		m_ortho_bottom = -m_radius / m_scale_factor;
		m_ortho_top = -m_ortho_bottom;
	}
	else
	{
		m_ortho_left = -m_radius / m_scale_factor;
		m_ortho_right = -m_ortho_left;
		m_ortho_bottom = -m_radius / aspect / m_scale_factor;
		m_ortho_top = -m_ortho_bottom;
	}
	if (m_persp)
	{
		m_proj_mat = glm::perspective(glm::radians(m_aov), aspect, m_near_clip, m_far_clip);
	}
	else
	{
		m_proj_mat = glm::ortho(m_ortho_left, m_ortho_right, m_ortho_bottom, m_ortho_top,
			-m_far_clip / 100.0, m_far_clip);
	}
}

void VulkanCanvas::HandleCamera()
{
	FLIVR::Vector pos(m_transx, m_transy, m_transz);
	pos.normalize();
	pos *= m_distance;
	m_transx = pos.x();
	m_transy = pos.y();
	m_transz = pos.z();

	m_mv_mat = glm::lookAt(glm::vec3(m_transx, m_transy, m_transz),
							glm::vec3(0.0), glm::vec3(m_up.x(), m_up.y(), m_up.z()));
}


double VulkanCanvas::CalcCameraDistance()
{
	glm::mat4 mv_temp = m_mv_mat;
	//translate object
	mv_temp = glm::translate(mv_temp, glm::vec3(m_obj_transx, m_obj_transy, m_obj_transz));
	//rotate object
	mv_temp = glm::rotate(mv_temp, float(m_obj_rotx), glm::vec3(1.0, 0.0, 0.0));
	mv_temp = glm::rotate(mv_temp, float(m_obj_roty), glm::vec3(0.0, 1.0, 0.0));
	mv_temp = glm::rotate(mv_temp, float(m_obj_rotz), glm::vec3(0.0, 0.0, 1.0));
	//center object
	mv_temp = glm::translate(mv_temp, glm::vec3(-m_obj_ctrx, -m_obj_ctry, -m_obj_ctrz));

	double mvmat[16] =
	{ mv_temp[0][0], mv_temp[0][1], mv_temp[0][2], mv_temp[0][3],
	  mv_temp[1][0], mv_temp[1][1], mv_temp[1][2], mv_temp[1][3],
	  mv_temp[2][0], mv_temp[2][1], mv_temp[2][2], mv_temp[2][3],
	  mv_temp[3][0], mv_temp[3][1], mv_temp[3][2], mv_temp[3][3] };


	FLIVR::Transform mv;
	mv.set_trans(mvmat);

	vector<FLIVR::Vector> points;
	points.reserve(8);

	points.push_back(FLIVR::Vector(m_bounds.min().x(), m_bounds.min().y(), m_bounds.min().z()));
	points.push_back(FLIVR::Vector(m_bounds.max().x(), m_bounds.min().y(), m_bounds.min().z()));
	points.push_back(FLIVR::Vector(m_bounds.min().x(), m_bounds.max().y(), m_bounds.min().z()));
	points.push_back(FLIVR::Vector(m_bounds.max().x(), m_bounds.max().y(), m_bounds.min().z()));
	points.push_back(FLIVR::Vector(m_bounds.min().x(), m_bounds.min().y(), m_bounds.max().z()));
	points.push_back(FLIVR::Vector(m_bounds.max().x(), m_bounds.min().y(), m_bounds.max().z()));
	points.push_back(FLIVR::Vector(m_bounds.min().x(), m_bounds.max().y(), m_bounds.max().z()));
	points.push_back(FLIVR::Vector(m_bounds.max().x(), m_bounds.max().y(), m_bounds.max().z()));

	static int edges[12 * 2] = { 0, 1,  1, 3,  3, 2,  2, 0,
							  0, 4,  1, 5,  3, 7,  2, 6,
							  4, 5,  5, 7,  7, 6,  6, 4 };

	static int planes[3 * 6] = { 0, 1, 2,  0, 1, 4,  1, 3, 5,  3, 2, 7,  2, 0, 6,  4, 5, 6 };

	for (int i = 0; i < 8; i++)
		points[i] = mv.project(points[i].asPoint()).asVector();

	double dmin = DBL_MAX;

	//point
	for (int i = 0; i < 8; i++)
	{
		double d = points[i].length2();
		if (d < dmin) dmin = d;
	}

	//edge
	for (int i = 0; i < 12; i++)
	{
		FLIVR::Vector p = points[edges[i * 2]];
		FLIVR::Vector q = points[edges[i * 2 + 1]];

		if (Dot(p, p - q) <= 0 || Dot(q, q - p) <= 0) continue;

		FLIVR::Vector e = q - p;
		e.safe_normalize();
		double d = Dot(p, e);
		d = p.length2() - d * d;

		if (d < dmin) dmin = d;
	}

	//planes
	for (int i = 0; i < 6; i++)
	{
		FLIVR::Vector o = points[planes[i * 3]];
		FLIVR::Vector p = points[planes[i * 3 + 1]] - points[planes[i * 3]];
		FLIVR::Vector q = points[planes[i * 3 + 2]] - points[planes[i * 3]];
		double plen = p.length();
		double qlen = q.length();
		p.safe_normalize();
		q.safe_normalize();
		FLIVR::Vector c = Cross(p, q);
		c.safe_normalize();

		double d = Dot(points[planes[i * 3 + 1]], c);
		FLIVR::Vector v = c * d - o;

		double s = Dot(v, p) / plen;
		double t = Dot(v, q) / qlen;

		d = d * d;

		if (s > 0 && s < 1 && t > 0 && t < 1 && d < dmin)
			dmin = d;
	}

	//inside?
	bool inside = false;
	//double padding = 0.0;
	{
		FLIVR::Vector ex = points[1] - points[0];
		FLIVR::Vector ey = points[2] - points[0];
		FLIVR::Vector ez = points[4] - points[0];
		double xlen = ex.length();
		double ylen = ey.length();
		double zlen = ez.length();
		ex.safe_normalize();
		ey.safe_normalize();
		ez.safe_normalize();

		FLIVR::Vector v = FLIVR::Vector(0) - points[0];

		double x = Dot(v, ex) / xlen;
		double y = Dot(v, ey) / ylen;
		double z = Dot(v, ez) / zlen;
		if (x > 0 && x < 1 && y > 0 && y < 1 && z > 0 && z < 1)
			inside = true;

		//padding = Min(Min(xlen, ylen), zlen) / 2.0;
	}

	dmin = inside ? 0 : sqrt(dmin);

	return dmin;
}


void VulkanCanvas::OnPaint(wxPaintEvent& event)
{
	m_scale_factor = 1.3;
	m_aov = 30.0;
	m_persp = true;

	wxPaintDC dc(this);

	wxSize size = GetSize();

	HandleProjection(size.GetWidth(), size.GetHeight());
	//Transformation
	HandleCamera();
	glm::mat4 mv_temp = m_mv_mat;
	//translate object
	m_mv_mat = glm::translate(m_mv_mat, glm::vec3(m_obj_transx, m_obj_transy, m_obj_transz));
	//rotate object
	m_mv_mat = glm::rotate(m_mv_mat, float(m_obj_rotx), glm::vec3(1.0, 0.0, 0.0));
	m_mv_mat = glm::rotate(m_mv_mat, float(m_obj_roty), glm::vec3(0.0, 1.0, 0.0));
	m_mv_mat = glm::rotate(m_mv_mat, float(m_obj_rotz), glm::vec3(0.0, 0.0, 1.0));
	//center object
	m_mv_mat = glm::translate(m_mv_mat, glm::vec3(-m_obj_ctrx, -m_obj_ctry, -m_obj_ctrz));

	double sampling_frq_fac = 2 / min(m_ortho_right - m_ortho_left, m_ortho_top - m_ortho_bottom);

	m_vol->set_sort_bricks();

	if (FLIVR::TextureRenderer::get_mem_swap())
	{
		vector<FLIVR::TextureBrick*>* bricks = m_vol->get_bricks();
		for (int j = 0; j < bricks->size(); j++)
		{
			(*bricks)[j]->set_drawn(false);
		}
		FLIVR::TextureRenderer::set_update_loop();
		FLIVR::TextureRenderer::set_total_brick_num(m_vol->get_brick_num());
		FLIVR::TextureRenderer::reset_done_current_chan();

		FLIVR::TextureRenderer::set_st_time(GET_TICK_COUNT());
	}

    try {
		m_vulkan->prepareFrame();
		
		m_vr->set_matrices(m_mv_mat, m_proj_mat, m_tex_mat);

		m_vr->set_ml_mode(1);
		m_vr->draw(m_vulkan->frameBuffers[m_vulkan->currentBuffer], true, false, false, !m_persp, m_scale_factor, 0, sampling_frq_fac);

		m_vulkan->submitFrame();
    }
    catch (const VulkanException& ve) {
        std::string status = ve.GetStatus();
        std::stringstream ss;
        ss << ve.what() << "\n" << status;
        CallAfter(&VulkanCanvas::OnPaintException, ss.str());
    }
    catch (const std::exception& err) {
        std::stringstream ss;
        ss << "Error encountered trying to create the Vulkan canvas:\n";
        ss << err.what();
        CallAfter(&VulkanCanvas::OnPaintException, ss.str());
    }

	m_mv_mat = mv_temp;
}

void VulkanCanvas::OnResize(wxSizeEvent& event)
{
    wxSize size = GetSize();
    if (size.GetWidth() == 0 || size.GetHeight() == 0) {
        return;
    }
	m_vulkan->setSize(size.GetWidth(), size.GetHeight());
    wxRect refreshRect(size);
    RefreshRect(refreshRect, false);
}

void VulkanCanvas::OnTimer(wxTimerEvent&)
{
	double d_pi = glm::pi<double>();
	m_obj_roty += d_pi / 90.0;
	m_obj_roty -= 2.0 * d_pi * (int)( m_obj_roty / (2 * d_pi) );

	Refresh();
}

void VulkanCanvas::OnPaintException(const std::string& msg)
{
    wxMessageBox(msg, "Vulkan Error");
    wxTheApp->ExitMainLoop();
}

void VulkanCanvas::OnErase(wxEraseEvent& event)
{

}