#pragma once
#define GLM_FORCE_RADIANS
#include "wx/wxprec.h"
#include <VVulkan.h>
#include <VolumeRenderer.h>
#include <base_reader.h>
#include <nrrd_reader.h>
#include <nrrd_writer.h>
#include <string>
#include <set>
#include <array>
#include <memory>
#include <chrono>

#define INIT_BOUNDS  1
#define INIT_CENTER  2
#define INIT_TRANSL  4
#define INIT_ROTATE  8
#define INIT_OBJ_TRANSL  16

class VulkanCanvas :
    public wxWindow
{
public:
    VulkanCanvas(wxWindow *pParent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = "VulkanCanvasName");

    virtual ~VulkanCanvas() noexcept;

	void stop() { if (m_timer) m_timer->Stop(); }

private:

	void InitView(unsigned int type);
	void HandleProjection(int nx, int ny);
	void HandleCamera();
	double CalcCameraDistance();
	void DrawMask();
    
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnResize(wxSizeEvent& event);
    virtual void OnTimer(wxTimerEvent& event);
	virtual void OnErase(wxEraseEvent& event);
    void OnPaintException(const std::string& msg);

    static const int INTERVAL = 1000 / 90;
    static const int TIMERNUMBER = 3;
    std::unique_ptr<wxTimer> m_timer;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;

	std::shared_ptr<VVulkan> m_vulkan;
	std::shared_ptr<Vulkan2dRender> m_v2drender;

	std::unique_ptr<FLIVR::VolumeRenderer> m_vr;
	std::unique_ptr<FLIVR::Texture> m_vol;
	NRRDReader m_reader;
	NRRDWriter m_writer;

	std::shared_ptr<vks::VTexture> m_paint;

	bool m_init_view;
	glm::mat4 m_mv_mat, m_proj_mat, m_tex_mat;
	bool m_persp;
	double m_scale_factor;
	double m_radius;
	double m_aov;
	double m_ortho_left, m_ortho_right, m_ortho_bottom, m_ortho_top;
	double m_near_clip, m_far_clip;
	
	double m_distance, m_init_dist;
	double m_ctrx, m_ctry, m_ctrz;
	double m_transx, m_transy, m_transz;
	double m_rotx, m_roty, m_rotz;
	FLIVR::Vector m_up, m_head;
	FLIVR::Quaternion m_q;

	double m_obj_ctrx, m_obj_ctry, m_obj_ctrz;
	double m_obj_transx, m_obj_transy, m_obj_transz;
	double m_obj_rotx, m_obj_roty, m_obj_rotz;
	FLIVR::BBox m_bounds;

	void LoadVolume();

};
