//  
//  For more information, please see: http://software.sci.utah.edu
//  
//  The MIT License
//  
//  Copyright (c) 2004 Scientific Computing and Imaging Institute,
//  University of Utah.
//  
//  
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  

#include <GL/glew.h>
#include <FLIVR/texturebrick.h>
#include <FLIVR/TextureRenderer.h>
#include <FLIVR/Color.h>
#include <FLIVR/Utils.h>
#include <FLIVR/ShaderProgram.h>
#include <FLIVR/VolShader.h>
#include <FLIVR/SegShader.h>
#include <FLIVR/VolCalShader.h>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <FLIVR/palettes.h>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include "compatibility.h"
#include <time.h>
#include <wx/utils.h>
//#include <iomanip>

using boost::property_tree::wptree;
using namespace std;

namespace FLIVR
{
	bool TextureRenderer::mem_swap_ = true;
	bool TextureRenderer::use_mem_limit_ = false;
	double TextureRenderer::mem_limit_ = 0.0;
	double TextureRenderer::available_mem_ = 0.0;
	double TextureRenderer::mainmem_buf_size_ = 0.0;
	double TextureRenderer::available_mainmem_buf_size_ = 0.0;
	double TextureRenderer::large_data_size_ = 0.0;
	int TextureRenderer::force_brick_size_ = 0;
	vector<TexParam> TextureRenderer::tex_pool_;
	bool TextureRenderer::start_update_loop_ = false;
	bool TextureRenderer::done_update_loop_ = true;
	bool TextureRenderer::done_current_chan_ = true;
	int TextureRenderer::total_brick_num_ = 0;
	int TextureRenderer::cur_brick_num_ = 0;
	int TextureRenderer::cur_chan_brick_num_ = 0;
	int TextureRenderer::cur_tid_offset_multi_ = 0;
	bool TextureRenderer::clear_chan_buffer_ = true;
	bool TextureRenderer::save_final_buffer_ = true;
	unsigned long TextureRenderer::st_time_ = 0;
	unsigned long TextureRenderer::up_time_ = 100;
	unsigned long TextureRenderer::cor_up_time_ = 100;
	unsigned long TextureRenderer::consumed_time_ = 0;
	bool TextureRenderer::interactive_ = false;
	int TextureRenderer::finished_bricks_ = 0;
	BrickQueue TextureRenderer::brick_queue_(15);
	int TextureRenderer::quota_bricks_ = 0;
	Point TextureRenderer::quota_center_;
	int TextureRenderer::update_order_ = 0;
	bool TextureRenderer::load_on_main_thread_ = false;
	bool TextureRenderer::clear_pool_ = false;

	vector<TextureRenderer::LoadedBrick> TextureRenderer::loadedbrks;
	int TextureRenderer::del_id = 0;

	TextureRenderer::TextureRenderer(Texture* tex)
		:
		tex_(tex),
		mode_(MODE_NONE),
		sampling_rate_(1.0),
		num_slices_(0),
		irate_(0.5),
		imode_(false),
		blend_framebuffer_resize_(false),
		blend_framebuffer_(0),
		blend_tex_id_(0),
		label_tex_id_(0),
		palette_tex_id_(0),
		base_palette_tex_id_(0),
		desel_palette_mode_(0),
		desel_col_fac_(0.1),
		edit_sel_id_(-1),
		filter_buffer_resize_(false),
		filter_buffer_(0),
		filter_tex_id_(0),
		fbo_mask_(0),
		fbo_label_(0),
		tex_2d_mask_(0),
		tex_2d_weight1_(0),
		tex_2d_weight2_(0),
		tex_2d_dmap_(0),
		blend_num_bits_(32)
	{
		init_palette();
/*
		roi_tree_.add(L"-3", L"G3");
		roi_tree_.add(L"-3.-2", L"G2");
		roi_tree_.add(L"-3.-2.71", L"TEST71");
		roi_tree_.add(L"-3.-2.25", L"TEST25");
		roi_tree_.add(L"-4", L"G4");
		roi_tree_.add(L"-4.49", L"TEST49");
*/
/*		int segid = 1;
		for (int i = 2; i <= 86; i++)
		{
			if (i != 21 &&
				i != 41 &&
				i != 42 &&
				i != 43 &&
				i != 44 &&
				i != 45 &&
				i != 46 &&
				i != 47 &&
				i != 48 &&
				i != 68 )
			{
				wstringstream wsskey, wssname;
				wsskey << i;
				wssname << L"Segment" << segid;
				roi_tree_.add(wsskey.str(), wssname.str());
				segid++;
			}
		}
*/
		select_all_roi_tree();
	}

	TextureRenderer::TextureRenderer(const TextureRenderer& copy)
		:
		tex_(copy.tex_),
		mode_(copy.mode_),
		sampling_rate_(copy.sampling_rate_),
		num_slices_(0),
		irate_(copy.irate_),
		imode_(copy.imode_),
		blend_framebuffer_resize_(false),
		blend_framebuffer_(0),
		blend_tex_id_(0),
		label_tex_id_(0),
		palette_tex_id_(0),
		base_palette_tex_id_(0),
		desel_palette_mode_(copy.desel_palette_mode_),
		desel_col_fac_(copy.desel_col_fac_),
		sel_ids_(copy.sel_ids_),
		edit_sel_id_(copy.edit_sel_id_),
		roi_tree_(copy.roi_tree_),
		filter_buffer_resize_(false),
		filter_buffer_(0),
		filter_tex_id_(0),
		fbo_mask_(0),
		fbo_label_(0),
		tex_2d_mask_(0),
		tex_2d_weight1_(0),
		tex_2d_weight2_(0),
		tex_2d_dmap_(0),
		blend_num_bits_(copy.blend_num_bits_)
	{
		memcpy(palette_, copy.palette_, sizeof(unsigned char)*PALETTE_SIZE*PALETTE_ELEM_COMP); 
		memcpy(base_palette_, copy.base_palette_, sizeof(unsigned char)*PALETTE_SIZE*PALETTE_ELEM_COMP); 
		if (!glIsTexture(palette_tex_id_))
			glGenTextures(1, &palette_tex_id_);
		glBindTexture(GL_TEXTURE_2D, palette_tex_id_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, PALETTE_W, PALETTE_H, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, (void *)palette_);//GL_RGBA16F
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!glIsTexture(base_palette_tex_id_))
			glGenTextures(1, &base_palette_tex_id_);
		glBindTexture(GL_TEXTURE_2D, base_palette_tex_id_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, PALETTE_W, PALETTE_H, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, (void *)base_palette_);//GL_RGBA16F
		glBindTexture(GL_TEXTURE_2D, 0);

		update_palette_tex();
	}

	TextureRenderer::~TextureRenderer()
	{
		clear_brick_buf();
		//clear_tex_pool();
		clear_tex_current();
		if (glIsFramebuffer(blend_framebuffer_))
			glDeleteFramebuffers(1, &blend_framebuffer_);
		if (glIsFramebuffer(fbo_label_))
			glDeleteFramebuffers(1, &fbo_label_);
		if (glIsFramebuffer(fbo_mask_))
			glDeleteFramebuffers(1, &fbo_mask_);
		if (glIsTexture(blend_tex_id_))
			glDeleteTextures(1, &blend_tex_id_);
		if (glIsTexture(label_tex_id_))
			glDeleteTextures(1, &label_tex_id_);
		if (glIsTexture(palette_tex_id_))
			glDeleteTextures(1, &palette_tex_id_);
		if (glIsTexture(base_palette_tex_id_))
			glDeleteTextures(1, &base_palette_tex_id_);
		if (glIsFramebuffer(filter_buffer_))
			glDeleteFramebuffers(1, &filter_buffer_);
		if (glIsTexture(filter_tex_id_))
			glDeleteTextures(1, &filter_tex_id_);

		if (glIsBuffer(m_slices_vbo))
			glDeleteBuffers(1, &m_slices_vbo);
		if (glIsBuffer(m_slices_ibo))
			glDeleteBuffers(1, &m_slices_ibo);
		if (glIsVertexArray(m_slices_vao))
			glDeleteVertexArrays(1, &m_slices_vao);
		if (glIsBuffer(m_quad_vbo))
			glDeleteBuffers(1, &m_quad_vbo);
		if (glIsVertexArray(m_quad_vao))
			glDeleteVertexArrays(1, &m_quad_vao);
	}

	void TextureRenderer::init_palette()
	{
/*
		float hue;
		unsigned char p2, p3;
		unsigned char c[4];
		
		for (int i = 0; i < PALETTE_SIZE; i++)
		{
			hue = float(rand() % 360)/60.0;
			p2 = (unsigned char)((1.0 - hue + floor(hue))*255);
			p3 = (unsigned char)((hue - floor(hue))*255);
			if (hue < 1.0)
				{c[0] = 255; c[1] = p3; c[2] = 0; c[3] = 255;}
			else if (hue < 2.0)
				{c[0] = p2; c[1] = 255; c[2] = 0; c[3] = 255;}
			else if (hue < 3.0)
				{c[0] = 0; c[1] = 255; c[2] = p3; c[3] = 255;}
			else if (hue < 4.0)
				{c[0] = 0; c[1] = p2; c[2] = 255; c[3] = 255;}
			else if (hue < 5.0)
				{c[0] = p3; c[1] = 0; c[2] = 255; c[3] = 255;}
			else
				{c[0] = 255; c[1] = 0; c[2] = p2; c[3] = 255;}
			
			for (int j = 0; j < PALETTE_ELEM_COMP; j++)
			{
				if (j < 4) palette_[i*PALETTE_ELEM_COMP+j] = c[j];
				else palette_[i*PALETTE_ELEM_COMP+j] = 0;
			}
		}
*/
		memcpy(palette_, (const void *)palettes::palette_random_256_256_4, sizeof(unsigned char)*PALETTE_SIZE*PALETTE_ELEM_COMP);
		memcpy(base_palette_, (const void *)palettes::palette_random_256_256_4, sizeof(unsigned char)*PALETTE_SIZE*PALETTE_ELEM_COMP); 

		if (!glIsTexture(palette_tex_id_))
			glGenTextures(1, &palette_tex_id_);
		glBindTexture(GL_TEXTURE_2D, palette_tex_id_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, PALETTE_W, PALETTE_H, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, (void *)palette_);//GL_RGBA16F
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!glIsTexture(base_palette_tex_id_))
			glGenTextures(1, &base_palette_tex_id_);
		glBindTexture(GL_TEXTURE_2D, base_palette_tex_id_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, PALETTE_W, PALETTE_H, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, (void *)base_palette_);//GL_RGBA16F
		glBindTexture(GL_TEXTURE_2D, 0);
/*
		ofstream ofs("random_palette.txt");
		ofs << "const unsigned char palette_random[] = {\n";
		for (int k = 0; k < PALETTE_SIZE*PALETTE_ELEM_COMP; k++)
		{
			ofs << " 0x"  << std::hex << std::setw(2) << std::setfill('0') << (int)palette_[k];
			if (k != PALETTE_SIZE*PALETTE_ELEM_COMP-1) ofs << ",";
			if ((k % 8) == 7) ofs << "\n";
		}
		ofs << "};";
		ofs.close();
*/
	}

	void TextureRenderer::update_palette_tex()
	{
		if (glIsTexture(palette_tex_id_))
		{
			glBindTexture(GL_TEXTURE_2D, palette_tex_id_);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PALETTE_W, PALETTE_H, GL_RGBA, GL_UNSIGNED_BYTE, (void *)palette_);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	boost::optional<wstring> TextureRenderer::get_roi_path(int id)
	{
		return get_roi_path(id, roi_tree_, L"");
	}
	boost::optional<wstring> TextureRenderer::get_roi_path(int id, const wptree& tree, const wstring &parent)
	{
		for (wptree::const_iterator child = tree.begin(); child != tree.end(); ++child)
		{
			wstring c_path = (parent.empty() ? L"" : parent + L".") + child->first;
			try
			{
				int val = boost::lexical_cast<int>(child->first);
				if (val == id)
					return c_path;
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::get_roi_path(int id, const wptree& tree): bad_lexical_cast" << endl;
			}
			if (auto rval = get_roi_path(id, child->second, c_path))
				return rval;
		}

		return boost::none;
	}

	boost::optional<wstring> TextureRenderer::get_roi_path(wstring name)
	{
		return get_roi_path(name, roi_tree_, L"");
	}
	boost::optional<wstring> TextureRenderer::get_roi_path(wstring name, const wptree& tree, const wstring& parent)
	{
		for (wptree::const_iterator child = tree.begin(); child != tree.end(); ++child)
		{
			wstring c_path = (parent.empty() ? L"" : parent + L".") + child->first;
			if (const auto val = tree.get_optional<wstring>(child->first))
			{
				if (val == name)
					return c_path;
			}
			if (auto rval = get_roi_path(name, child->second, c_path))
				return rval;
		}

		return boost::none;
	}

	void TextureRenderer::set_roi_name(wstring name, int id, wstring parent_name)
	{
		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if (edid == -1)
			return;

		if (!name.empty())
		{
			if(auto path = get_roi_path(edid))
				roi_tree_.put(*path, name);
			else
			{
				wstring prefix(L"");
				if(auto path = get_roi_path(parent_name))
					prefix = *path + L".";
				roi_tree_.add(prefix + boost::lexical_cast<wstring>(edid), name);
			}
		}
		else
		{
			if(auto path = get_roi_path(edid))
			{
				auto pos = path->find_last_of(L'.');
				wstring strid(L""), prefix(L"");
				if (pos != wstring::npos && pos+1 < path->length())
				{
					strid = path->substr(pos+1);
					prefix = path->substr(0, pos);
				}
				else
					strid = *path;
				roi_tree_.get_child(prefix).erase(strid);
			}
		}
	}

	void TextureRenderer::set_roi_name(wstring name, int id, int parent_id)
	{
		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if (edid == -1)
			return;

		if (!name.empty())
		{
			if(auto path = get_roi_path(edid))
				roi_tree_.put(*path, name);
			else
			{
				wstring prefix(L"");
				if(auto path = get_roi_path(parent_id))
					prefix = *path + L".";
				roi_tree_.add(prefix + boost::lexical_cast<wstring>(edid), name);
			}
		}
		else
		{
			if(auto path = get_roi_path(edid))
			{
				auto pos = path->find_last_of(L'.');
				wstring strid(L""), prefix(L"");
				if (pos != wstring::npos && pos+1 < path->length())
				{
					strid = path->substr(pos+1);
					prefix = path->substr(0, pos);
				}
				else
					strid = *path;
				roi_tree_.get_child(prefix).erase(strid);
			}
		}
	}

	int TextureRenderer::get_available_group_id()
	{
		int id = -2;
		while (1)
		{
			if (!get_roi_path(id))
				break;
			id--;
		}
		return id;
	}

	int TextureRenderer::add_roi_group_node(int parent_id, wstring name)
	{
		int gid = get_available_group_id();
		wstring prefix(L"");

		if (name.empty())
		{
			wstringstream wss;
			wss << L"Seg Group " << (gid*-1)-1;
			name = wss.str();
		}

		if(auto path = get_roi_path(parent_id))
			prefix = *path + L".";

		roi_tree_.add(prefix + boost::lexical_cast<wstring>(gid), name);

		return gid;
	}

	int TextureRenderer::add_roi_group_node(wstring parent_name, wstring name)
	{
		int pid = parent_name.empty() ? -1 : get_roi_id(parent_name);
		
		return add_roi_group_node(pid, name);
	}

	//return the parent if there is no sibling.
	int TextureRenderer::get_next_sibling_roi(int id)
	{
		if(auto path = get_roi_path(id))
		{
			auto pos = path->find_last_of(L'.');
			wstring prefix(L"");
			if (pos != wstring::npos && pos+1 < path->length())
				prefix = path->substr(0, pos);
			
			auto subtree = roi_tree_.get_child_optional(prefix);
			if (subtree)
			{
				bool found_myself = false;
				int prev_id = -1;
				int temp_id = -1;
				for (wptree::const_iterator child = subtree->begin(); child != subtree->end(); ++child)
				{
					wstring strid = child->first;
					int cid = -1;
					try
					{
						cid = boost::lexical_cast<int>(strid);
					}
					catch (boost::bad_lexical_cast e)
					{
						cerr << "TextureRenderer::get_next_child_roi(int id): bad_lexical_cast" << endl;
					}

					if (found_myself)
						return cid;

					if (id == cid)
					{
						found_myself = true;
						prev_id = temp_id;
					}
					temp_id = cid;
				}

				if (found_myself)
				{
					if (prev_id != -1)
						return prev_id;
					else if (!prefix.empty())
					{
						auto pos = prefix.find_last_of(L'.');
						wstring parent_strid(L"");
						if (pos != wstring::npos && pos+1 < prefix.length())
							parent_strid = prefix.substr(pos+1);
						else
							parent_strid = prefix;

						int pid = -1;
						try
						{
							pid = boost::lexical_cast<int>(parent_strid);
						}
						catch (boost::bad_lexical_cast e)
						{
							cerr << "TextureRenderer::get_next_child_roi(int id): bad_lexical_cast 2" << endl;
						}

						return pid;
					}
				}
			}
		}

		return -1;
	}

	//insert_mode: 0-before dst; 1-after dst; 2-into group
	void TextureRenderer::move_roi_node(int src_id, int dst_id, int insert_mode)
	{
		auto path = get_roi_path(src_id);
		if (!path || src_id == dst_id) return;
		
		wptree subtree = roi_tree_.get_child(*path);
		erase_node(src_id);

		if (dst_id != -1)
		{
			if (insert_mode == 0 || insert_mode == 1)
				insert_roi_node(roi_tree_, dst_id, subtree, src_id, insert_mode);
			else if (insert_mode == 2 && dst_id < -1) 
			{
				if (auto dst_par_path = get_roi_path(dst_id))
				{
					try
					{
						wstring dst_path = *dst_par_path + L"." + boost::lexical_cast<wstring>(src_id);
						roi_tree_.put_child(dst_path, subtree);
					}
					catch (boost::bad_lexical_cast e)
					{
						cerr << "TextureRenderer::move_roi_node(int src_id, int dst_id): bad_lexical_cast" << endl;
					}
				}
			}
		}
		else
		{
			try
			{
				roi_tree_.put_child(boost::lexical_cast<wstring>(src_id), subtree);
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::move_roi_node(int src_id, int dst_id): bad_lexical_cast 2" << endl;
			}
		}
	}
	
	//insert_mode: 0-before dst; 1-after dst;
	bool TextureRenderer::insert_roi_node(wptree& tree, int dst_id, const wptree& node, int id, int insert_mode)
	{
		for (wptree::iterator child = tree.begin(); child != tree.end(); ++child)
		{
			wstring strid = child->first;
			int cid = -1;
			try
			{
				cid = boost::lexical_cast<int>(strid);
				if (dst_id == cid)
				{
					wptree::value_type v(boost::lexical_cast<wstring>(id), node);
					if (insert_mode == 1) ++child;
					tree.insert(child, v); 
					return true;
				}
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::insert_roi_node(wptree& tree, int dst_id, const wptree& node, int id): bad_lexical_cast" << endl;
			}
			
			if (insert_roi_node(child->second, dst_id, node, id, insert_mode))
				return true;
		}

		return false;
	}

	void TextureRenderer::erase_node(int id)
	{
		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if(auto path = get_roi_path(edid))
		{
			auto pos = path->find_last_of(L'.');
			wstring strid(L""), parent(L"");
			if (pos != wstring::npos && pos+1 < path->length())
			{
				strid = path->substr(pos+1);
				parent = path->substr(0, pos);
			}
			else
				strid = *path;
			roi_tree_.get_child(parent).erase(strid);
		}
	}

	void TextureRenderer::erase_node(wstring name)
	{
		int edid = get_roi_id(name);
		
		if (edid != -1) erase_node(edid);
	}

	wstring TextureRenderer::get_roi_name(int id)
	{
		wstring rval;

		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if (edid != -1)
		{
			if(auto path = get_roi_path(edid))
				rval = roi_tree_.get<wstring>(*path);
		}

		return rval;
	}

	int TextureRenderer::get_roi_id(wstring name)
	{
		int rval = -1;

		if (auto path = get_roi_path(name))
		{
			auto pos = path->find_last_of(L'.');
			wstring strid;
			if (pos != wstring::npos && pos+1 < path->length())
				strid = path->substr(pos+1);
			else
				strid = *path;
			try
			{
				rval = boost::lexical_cast<int>(strid);
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::get_roi_id(wstring name): bad_lexical_cast" << endl;
			}
		}

		return rval;
	}

	void TextureRenderer::set_roi_select(wstring name, bool select, bool traverse)
	{
		if (auto path = get_roi_path(name))
		{
			auto pos = path->find_last_of(L'.');
			wstring strid;
			if (pos != wstring::npos && pos+1 < path->length())
				strid = path->substr(pos+1);
			else
				strid = *path;
			try
			{
				int id = boost::lexical_cast<int>(strid);
				if (id != -1)
				{
					if (select)
						sel_ids_.insert(id);
					else
					{
						auto ite = sel_ids_.find(id);
						if (ite != sel_ids_.end())
							sel_ids_.erase(ite);
					}
				}
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::set_roi_select(wstring name, bool select): bad_lexical_cast" << endl;
			}

			if (traverse)
			{
				if (auto subtree = roi_tree_.get_child_optional(*path))
					set_roi_select_r(*subtree, select);
			}

			update_palette(desel_palette_mode_, desel_col_fac_);
		}
	}

	void TextureRenderer::set_roi_select_children(wstring name, bool select, bool traverse)
	{
		if (name.empty())
		{
			set_roi_select_r(roi_tree_, select, traverse);
		}
		else if (auto path = get_roi_path(name))
		{
			if (auto subtree = roi_tree_.get_child_optional(*path))
				set_roi_select_r(*subtree, select, traverse);
		}
		
		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::set_roi_select_r(const boost::property_tree::wptree& tree, bool select, bool recursive)
	{
		for (wptree::const_iterator child = tree.begin(); child != tree.end(); ++child)
		{
			wstring strid = child->first;
			int id = -1;
			try
			{
				id = boost::lexical_cast<int>(strid);
				if (id != -1)
				{
					if (select)
						sel_ids_.insert(id);
					else
					{
						auto ite = sel_ids_.find(id);
						if (ite != sel_ids_.end())
							sel_ids_.erase(ite);
					}
				}
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::toggle_roi_select(boost::property_tree::wptree& tree, bool select): bad_lexical_cast" << endl;
			}

			if (recursive) set_roi_select_r(child->second, select, recursive);
		}
	}

	void TextureRenderer::update_sel_segs()
	{
		sel_segs_.clear();
		update_sel_segs(roi_tree_);

		//add unnamed visible segments
		for(auto ite = sel_ids_.begin(); ite != sel_ids_.end(); ++ite)
		{
			if(!get_roi_path(*ite) && (*ite >= 0))
				sel_segs_.insert(*ite);
		}
	}

	void TextureRenderer::update_sel_segs(const wptree& tree)
	{
		for (wptree::const_iterator child = tree.begin(); child != tree.end(); ++child)
		{
			wstring strid = child->first;
			int id = -1;
			try
			{
				id = boost::lexical_cast<int>(strid);
				if (id != -1)
				{
					auto ite = sel_ids_.find(id);
					if (ite != sel_ids_.end())
					{
						if (*ite >= 0)
							sel_segs_.insert(*ite);
						update_sel_segs(child->second);
					}
				}
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::update_sel_segs(const wptree& tree): bad_lexical_cast" << endl;
			}
		}
	}

	void TextureRenderer::set_id_color(unsigned char r, unsigned char g, unsigned char b, bool update, int id)
	{
		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if (edid < 0 || edid >= PALETTE_SIZE)
			return;
		
		base_palette_[edid*PALETTE_ELEM_COMP+0] = r;
		base_palette_[edid*PALETTE_ELEM_COMP+1] = g;
		base_palette_[edid*PALETTE_ELEM_COMP+2] = b;

		if (update)
			update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::get_id_color(unsigned char &r, unsigned char &g, unsigned char &b, int id)
	{
		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if (edid < 0 || edid >= PALETTE_SIZE)
			return;
		
		r = base_palette_[edid*PALETTE_ELEM_COMP+0];
		g = base_palette_[edid*PALETTE_ELEM_COMP+1];
		b = base_palette_[edid*PALETTE_ELEM_COMP+2];
	}

	void TextureRenderer::get_rendered_id_color(unsigned char &r, unsigned char &g, unsigned char &b, int id)
	{
		update_palette(desel_palette_mode_, desel_col_fac_);

		int edid = (id == -1) ? edit_sel_id_ : id;
		
		if (edid < 0 || edid >= PALETTE_SIZE)
			return;
		
		if (sel_ids_.empty() && roi_tree_.empty())
		{
			r = base_palette_[edid*PALETTE_ELEM_COMP+0];
			g = base_palette_[edid*PALETTE_ELEM_COMP+1];
			b = base_palette_[edid*PALETTE_ELEM_COMP+2];
		}
		else
		{
			r = palette_[edid*PALETTE_ELEM_COMP+0];
			g = palette_[edid*PALETTE_ELEM_COMP+1];
			b = palette_[edid*PALETTE_ELEM_COMP+2];
		}
	}

	void TextureRenderer::set_desel_palette_mode_dark(float fac)
	{
		for (int i = 0; i < PALETTE_SIZE; i++)
		{
			for (int j = 0; j < 3; j++)
				palette_[i*PALETTE_ELEM_COMP+j] = (unsigned char)(base_palette_[i*PALETTE_ELEM_COMP+j]*fac);
			//palette_[i*PALETTE_ELEM_COMP+3] = base_palette_[i*PALETTE_ELEM_COMP+3]*fac;
		}

		for(auto ite = sel_segs_.begin(); ite != sel_segs_.end(); ++ite)
			for (int j = 0; j < PALETTE_ELEM_COMP; j++)
				if ((*ite) >= 0) palette_[(*ite)*PALETTE_ELEM_COMP + j] = base_palette_[(*ite)*PALETTE_ELEM_COMP + j];
/*
		for (int i = 0; i < 256*64; i++)
			for (int j = 0; j < PALETTE_ELEM_COMP; j++)
				palette_[i*PALETTE_ELEM_COMP+j] = base_palette_[i*PALETTE_ELEM_COMP+j];
*/
		update_palette_tex();
	}

	void TextureRenderer::set_desel_palette_mode_gray(float fac)
	{
		for (int i = 1; i < PALETTE_SIZE; i++)
		{
			for (int j = 0; j < 3; j++)
				palette_[i*PALETTE_ELEM_COMP+j] = (unsigned char)(128.0*fac);
			//palette_[i*PALETTE_ELEM_COMP+3] = base_palette_[i*PALETTE_ELEM_COMP+3]*fac;
		}

		palette_[0] = 0; palette_[1] = 0; palette_[2] = 0;

		for(auto ite = sel_segs_.begin(); ite != sel_segs_.end(); ++ite)
			for (int j = 0; j < PALETTE_ELEM_COMP; j++)
				if ((*ite) >= 0) palette_[(*ite)*PALETTE_ELEM_COMP + j] = base_palette_[(*ite)*PALETTE_ELEM_COMP + j];

		update_palette_tex();
	}

	void TextureRenderer::set_desel_palette_mode_invisible()
	{
		for (int i = 0; i < PALETTE_SIZE; i++)
			for (int j = 0; j < 4; j++)
				palette_[i*PALETTE_ELEM_COMP+j] = 0;

		for(auto ite = sel_segs_.begin(); ite != sel_segs_.end(); ++ite)
			for (int j = 0; j < PALETTE_ELEM_COMP; j++)
				if ((*ite) >= 0) palette_[(*ite)*PALETTE_ELEM_COMP + j] = base_palette_[(*ite)*PALETTE_ELEM_COMP + j];

		update_palette_tex();
	}

	void TextureRenderer::update_palette(int mode, float fac)
	{
		update_sel_segs();

		switch(mode)
		{
		case 0:
			set_desel_palette_mode_dark(fac);
			break;
		case 1:
			set_desel_palette_mode_gray(fac);
			break;
		case 2:
			set_desel_palette_mode_invisible();
			break;
		}

		desel_palette_mode_ = mode;
		desel_col_fac_ = fac;
	}

	GLuint TextureRenderer::get_palette()
	{
		update_sel_segs();

		if (sel_ids_.empty() && roi_tree_.empty()) return base_palette_tex_id_;
		else return palette_tex_id_;
	}

	bool TextureRenderer::is_sel_id(int id)
	{
		auto ite = sel_ids_.find(id);
		
		if (ite != sel_ids_.end())
			return true;
		else
			return false;
	}

	void TextureRenderer::add_sel_id(int id)
	{
		if (id == -1)
			return;

		sel_ids_.insert(id);
		edit_sel_id_ = id;

		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::del_sel_id(int id)
	{
		if (sel_ids_.empty()) return;

		auto ite = sel_ids_.find(id);
		if (ite != sel_ids_.end())
		{
			sel_ids_.erase(ite);
			if (edit_sel_id_ == id) edit_sel_id_ = -1;
		}

		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::set_edit_sel_id(int id)
	{
		edit_sel_id_ = id;
	}

	void TextureRenderer::clear_sel_ids()
	{
		if (!sel_ids_.empty()) sel_ids_.clear();
		
		edit_sel_id_ = -1;

		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::clear_sel_ids_roi_only()
	{
		auto ite = sel_ids_.begin();
		while (ite != sel_ids_.end())
		{
			if ((*ite) >= 0)
				ite = sel_ids_.erase(ite);
			else
				++ite;
		}
		
		if (edit_sel_id_ >= 0) edit_sel_id_ = -1;

		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::clear_roi()
	{
		if (!roi_tree_.empty()) roi_tree_.clear();

		clear_sel_ids();
	}

	wstring TextureRenderer::export_roi_tree()
	{
		wstring str;

		export_roi_tree_r(str, roi_tree_, wstring(L""));

		return str;
	}

	void TextureRenderer::export_roi_tree_r(wstring &buf, const wptree& tree, const wstring& parent)
	{
		for (wptree::const_iterator child = tree.begin(); child != tree.end(); ++child)
		{
			wstring c_path = (parent.empty() ? L"" : parent + L".") + child->first;
			wstring strid = child->first;
			int id = -1;
			try
			{
				id = boost::lexical_cast<int>(strid);
				if (id != -1)
				{
					if (const auto val = tree.get_optional<wstring>(child->first))
					{
						buf += c_path + L"\n";	//path
						buf += *val + L"\n";	//name
						
						//id and color
						wstringstream wss;
						unsigned char r=0, g=0, b=0;
						get_id_color(r, g, b, id);
						wss << id << L" " << (int)r << L" " << (int)g << L" " << (int)b << L"\n";
						buf += wss.str();
					}
				}
			}
			catch (boost::bad_lexical_cast e)
			{
				cerr << "TextureRenderer::update_sel_segs(const wptree& tree): bad_lexical_cast" << endl;
			}
			export_roi_tree_r(buf, child->second, c_path);
		}
	}

	string TextureRenderer::exprot_selected_roi_ids()
	{
		stringstream ss;
		for(auto ite = sel_ids_.begin(); ite != sel_ids_.end(); ++ite)
			ss << *ite << " ";

		return ss.str();
	}

	void TextureRenderer::import_roi_tree_xml(const wstring &filepath)
	{
		tinyxml2::XMLDocument doc; 
		if (doc.LoadFile(ws2s(filepath).c_str()) != 0){
			return;
		}

		tinyxml2::XMLElement *root = doc.RootElement();
		if (!root || strcmp(root->Name(), "Metadata"))
			return;

		init_palette();
		roi_tree_.clear();

		tinyxml2::XMLElement *child = root->FirstChildElement();
		while (child)
		{
			if (child->Name())
			{
				if (strcmp(child->Name(), "ROI_Tree") == 0)
				{
					int gid = -2;
					import_roi_tree_xml_r(child, roi_tree_, wstring(L""), gid);
				}
			}
			child = child->NextSiblingElement();
		}
		
		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::import_roi_tree_xml_r(tinyxml2::XMLElement *lvNode, const wptree& tree, const wstring& parent, int& gid)
	{
		tinyxml2::XMLElement *child = lvNode->FirstChildElement();
		while (child)
		{
			if (child->Name() && child->Attribute("name"))
			{
				try
				{
					wstring name = s2ws(child->Attribute("name"));
					if (strcmp(child->Name(), "Group") == 0)
					{
						wstringstream wss;
						wss << (parent.empty() ? L"" : parent + L".") << gid;
						wstring c_path = wss.str();

						roi_tree_.add(c_path, name);
						import_roi_tree_xml_r(child, tree, c_path, --gid);
					}
					if (strcmp(child->Name(), "ROI") == 0 && child->Attribute("id"))
					{
						string strid = child->Attribute("id");
						int id = boost::lexical_cast<int>(strid);
						if (id >= 0 && id < PALETTE_SIZE && child->Attribute("r") && child->Attribute("g") && child->Attribute("b"))
						{
							wstring c_path = (parent.empty() ? L"" : parent + L".") + s2ws(strid);
							string strR = child->Attribute("r");
							string strG = child->Attribute("g");
							string strB = child->Attribute("b");
							int r = boost::lexical_cast<int>(strR);
							int g = boost::lexical_cast<int>(strG);
							int b = boost::lexical_cast<int>(strB);
							roi_tree_.add(c_path, name);
							set_id_color(r, g, b, false, id);
						}
					}
				}
				catch (boost::bad_lexical_cast e)
				{
					cerr << "TextureRenderer::import_roi_tree_xml_r(XMLElement *lvNode, const wptree& tree, const wstring& parent): bad_lexical_cast" << endl;
				}
			}
			child = child->NextSiblingElement();
		}
	}

	void TextureRenderer::import_roi_tree(const wstring &tree)
	{
		wistringstream wiss(tree);
		
		init_palette();

		while (1)
		{
			wstring path, name, strcol;
			
			if (!getline(wiss, path))
				break;
			if (!getline(wiss, name))
				break;
			if (!getline(wiss, strcol))
				break;
			
			vector<int> col;
			int ival;
			std::wistringstream ls(strcol);
			while (ls >> ival)
				col.push_back(ival);
			
			if (col.size() < 4)
				break;

			roi_tree_.add(path, name);
			set_id_color((unsigned char)col[1], (unsigned char)col[2], (unsigned char)col[3], false, col[0]); 
		}

		update_palette(desel_palette_mode_, desel_col_fac_);
	}

	void TextureRenderer::import_selected_ids(const string &sel_ids_str)
	{
		istringstream iss(sel_ids_str);

		clear_sel_ids();

		int id;
		while (iss >> id)
		{
			if (id != 0 && id != -1)
				add_sel_id(id);
		}

		update_palette(desel_palette_mode_, desel_col_fac_);
	}


	//set the texture for rendering
	void TextureRenderer::set_texture(Texture* tex)
	{
		if (tex_ != tex) 
		{
			// new texture, flag existing tex id's for deletion.
			//clear_pool_ = true;
			clear_tex_current();
			tex->clear_undos();
			tex_ = tex;
		}
	}

	void TextureRenderer::reset_texture()
	{
		tex_ = 0;
	}

	//set blending bits. b>8 means 32bit blending
	void TextureRenderer::set_blend_num_bits(int b)
	{
		blend_num_bits_ = b;
	}

	// Pool is static, however it is cleared each time
	// when a texture is deleted
	void TextureRenderer::clear_tex_pool() 
	{
		for(unsigned int i = 0; i < tex_pool_.size(); i++)
		{
			// delete tex object.
			if(glIsTexture(tex_pool_[i].id))
			{
				glDeleteTextures(1, (GLuint*)&tex_pool_[i].id);
				tex_pool_[i].id = 0;
			}
		}
		tex_pool_.clear();
		clear_pool_ = false;
		available_mem_ = mem_limit_;
	}

	void TextureRenderer::clear_tex_current()
	{
		if (!tex_)
			return;
		vector<TextureBrick*>* bricks = tex_->get_bricks();
		TextureBrick* brick = 0;
		double est_avlb_mem = available_mem_;
		for (int i = tex_pool_.size() - 1; i >= 0; --i)
		{
			for (size_t j = 0; j < bricks->size(); ++j)
			{
				brick = (*bricks)[j];
				if (tex_pool_[i].brick == brick)
				{
					if (tex_pool_[i].comp >= 0 && tex_pool_[i].comp < TEXTURE_MAX_COMPONENTS && brick->nb(tex_pool_[i].comp) > 0)
						est_avlb_mem += brick->nx()*brick->ny()*brick->nz()*brick->nb(tex_pool_[i].comp)/1.04e6;
					glDeleteTextures(1, (GLuint*)&tex_pool_[i].id);
					tex_pool_.erase(tex_pool_.begin() + i);
					break;
				}
			}
		}
		if (use_mem_limit_)
			available_mem_ = est_avlb_mem;
	}

	//resize the fbo texture
	void TextureRenderer::resize()
	{
		blend_framebuffer_resize_ = true;
		filter_buffer_resize_ = true;
	}

	//set the 2d texture mask for segmentation
	void TextureRenderer::set_2d_mask(GLuint id)
	{
		tex_2d_mask_ = id;
	}

	//set 2d weight map for segmentation
	void TextureRenderer::set_2d_weight(GLuint weight1, GLuint weight2)
	{
		tex_2d_weight1_ = weight1;
		tex_2d_weight2_ = weight2;
	}

	//set the 2d texture depth map for rendering shadows
	void TextureRenderer::set_2d_dmap(GLuint id)
	{
		tex_2d_dmap_ = id;
	}

	//timer
	unsigned long TextureRenderer::get_up_time()
	{
		if (interactive_)
			return cor_up_time_;
		else
			return up_time_;
	}

	//set corrected up time according to mouse speed
	void TextureRenderer::set_cor_up_time(int speed)
	{
		//cor_up_time_ = speed;
		if (speed < 10) speed = 10;
		cor_up_time_ = (unsigned long)(log10(1000.0/speed)*up_time_);
	}

	//number of bricks rendered before time is up
	void TextureRenderer::reset_finished_bricks()
	{
		if (finished_bricks_>0 && consumed_time_>0)
		{
			brick_queue_.Push(int(double(finished_bricks_)*double(up_time_)/double(consumed_time_)));
		}
		finished_bricks_ = 0;
	}

	//get the maximum finished bricks in queue
	int TextureRenderer::get_finished_bricks_max()
	{
		int max = 0;
		int temp;
		for (int i=0; i<brick_queue_.GetLimit(); i++)
		{
			temp = brick_queue_.Get(i);
			max = temp>max?temp:max;
		}
		return max;
	}

	//estimate next brick number
	int TextureRenderer::get_est_bricks(int mode)
	{
		double result = 0.0;
		if (mode == 0)
		{
			//mean
			double sum = 0.0;
			for (int i=0; i<brick_queue_.GetLimit(); i++)
				sum += brick_queue_.Get(i);
			result = sum / brick_queue_.GetLimit();
		}
		else if (mode == 1)
		{
			//trend (weighted average)
			double sum = 0.0;
			double weights = 0.0;
			double w;
			for (int i=0; i<brick_queue_.GetLimit(); i++)
			{
				w = (i+1) * (i+1);
				sum += brick_queue_.Get(i) * w;
				weights += w;
			}
			result = sum / weights;
		}
		else if (mode == 2)
		{
			//linear regression
			double sum_xy = 0.0;
			double sum_x = 0.0;
			double sum_y = 0.0;
			double sum_x2 = 0.0;
			double x, y;
			double n = brick_queue_.GetLimit();
			for (int i=0; i<brick_queue_.GetLimit(); i++)
			{
				x = i;
				y = brick_queue_.Get(i);
				sum_xy += x * y;
				sum_x += x;
				sum_y += y;
				sum_x2 += x * x;
			}
			double beta = (sum_xy/n - sum_x*sum_y/n/n)/(sum_x2/n - sum_x*sum_x/n/n);
			result = Max(sum_y/n - beta*sum_x/n + beta*n, 1.0);
		}
		else if (mode == 3)
		{
			//most recently
			result = brick_queue_.GetLast();
		}
		else if (mode ==4)
		{
			//median
			int n0 = 0;
			double sum = 0.0;
			int n = brick_queue_.GetLimit();
			int *sorted_queue = new int[n];
			memset(sorted_queue, 0, n*sizeof(int));
			for (int i=0; i<n; i++)
			{
				sorted_queue[i] = brick_queue_.Get(i);
				if (sorted_queue[i] == 0)
					n0++;
				else
					sum += sorted_queue[i];
				for (int j=i; j>0; j--)
				{
					if (sorted_queue[j] < sorted_queue[j-1])
					{
						sorted_queue[j] = sorted_queue[j]+sorted_queue[j-1];
						sorted_queue[j-1] = sorted_queue[j]-sorted_queue[j-1];
						sorted_queue[j] = sorted_queue[j]-sorted_queue[j-1];
					}
					else
						break;
				}
			}
			if (n0 == 0)
				result = sorted_queue[n/2];
			else if (n0 < n)
				result = sum / (n-n0);
			else
				result = 0.0;
			delete []sorted_queue;
		}

		if (interactive_)
			return Max(1, int(cor_up_time_*result/up_time_));
		else
			return int(result);
	}

	Ray TextureRenderer::compute_view()
	{
		Transform *field_trans = tex_->transform();
		double mvmat[16] =
			{m_mv_mat[0][0], m_mv_mat[0][1], m_mv_mat[0][2], m_mv_mat[0][3],
			 m_mv_mat[1][0], m_mv_mat[1][1], m_mv_mat[1][2], m_mv_mat[1][3],
			 m_mv_mat[2][0], m_mv_mat[2][1], m_mv_mat[2][2], m_mv_mat[2][3],
			 m_mv_mat[3][0], m_mv_mat[3][1], m_mv_mat[3][2], m_mv_mat[3][3]};

		// index space view direction
		Vector v = field_trans->project(Vector(-mvmat[2], -mvmat[6], -mvmat[10]));
		v.safe_normalize();
		Transform mv;
		mv.set_trans(mvmat);
		Point p = field_trans->unproject(mv.unproject(Point(0,0,0)));
		return Ray(p, v);
	}

	Ray TextureRenderer::compute_snapview(double snap)
	{
		Transform *field_trans = tex_->transform();
		double mvmat[16] =
			{m_mv_mat[0][0], m_mv_mat[0][1], m_mv_mat[0][2], m_mv_mat[0][3],
			 m_mv_mat[1][0], m_mv_mat[1][1], m_mv_mat[1][2], m_mv_mat[1][3],
			 m_mv_mat[2][0], m_mv_mat[2][1], m_mv_mat[2][2], m_mv_mat[2][3],
			 m_mv_mat[3][0], m_mv_mat[3][1], m_mv_mat[3][2], m_mv_mat[3][3]};
		
		//snap
		Vector vd;
		if (snap>0.0 && snap<0.5)
		{
			double vdx = -mvmat[2];
			double vdy = -mvmat[6];
			double vdz = -mvmat[10];
			double vdx_abs = fabs(vdx);
			double vdy_abs = fabs(vdy);
			double vdz_abs = fabs(vdz);
			//if (vdx_abs < snap) vdx = 0.0;
			//if (vdy_abs < snap) vdy = 0.0;
			//if (vdz_abs < snap) vdz = 0.0;
			if (vdx_abs<snap-0.1) vdx = 0.0;
			else if (vdx_abs<snap) vdx = (vdx_abs-snap+0.1)*snap*10.0*vdx/vdx_abs;
			if (vdy_abs<snap-0.1) vdy = 0.0;
			else if (vdy_abs<snap) vdy = (vdy_abs-snap+0.1)*snap*10.0*vdy/vdy_abs;
			if (vdz_abs<snap-0.1) vdz = 0.0;
			else if (vdz_abs<snap) vdz = (vdz_abs-snap+0.1)*snap*10.0*vdz/vdz_abs;
			vd = Vector(vdx, vdy, vdz);
			vd.safe_normalize();
		}
		else
			vd = Vector(-mvmat[2], -mvmat[6], -mvmat[10]);

		// index space view direction
		Vector v = field_trans->project(vd);
		v.safe_normalize();
		Transform mv;
		mv.set_trans(mvmat);
		Point p = field_trans->unproject(mv.unproject(Point(0,0,0)));
		return Ray(p, v);
	}

	double TextureRenderer::compute_rate_scale(Vector v)
	{
		//Transform *field_trans = tex_->transform();
		double spcx, spcy, spcz;
		tex_->get_spacings(spcx, spcy, spcz);
		double z_factor = spcz/Max(spcx, spcy);
		Vector n(double(tex_->nx())/double(tex_->nz()), 
			double(tex_->ny())/double(tex_->nz()), 
			z_factor>1.0&&z_factor<100.0?sqrt(z_factor):1.0);

		double e = 0.0001;
		double a, b, c;
		double result;
		double v_len2 = v.length2();
		if (fabs(v.x())>=e && fabs(v.y())>=e && fabs(v.z())>=e)
		{
			a = v_len2 / v.x();
			b = v_len2 / v.y();
			c = v_len2 / v.z();
			result = n.x()*n.y()*n.z()*sqrt(a*a*b*b+b*b*c*c+c*c*a*a)/
				sqrt(n.x()*n.x()*a*a*n.y()*n.y()*b*b+
					 n.x()*n.x()*a*a*n.z()*n.z()*c*c+
					 n.y()*n.y()*b*b*n.z()*n.z()*c*c);
		}
		else if (fabs(v.x())<e && fabs(v.y())>=e && fabs(v.z())>=e)
		{
			b = v_len2 / v.y();
			c = v_len2 / v.z();
			result = n.y()*n.z()*sqrt(b*b+c*c)/
				sqrt(n.y()*n.y()*b*b+n.z()*n.z()*c*c);
		}
		else if (fabs(v.y())<e && fabs(v.x())>=e && fabs(v.z())>=e)
		{
			a = v_len2 / v.x();
			c = v_len2 / v.z();
			result = n.x()*n.z()*sqrt(a*a+c*c)/
				sqrt(n.x()*n.x()*a*a+n.z()*n.z()*c*c);
		}
		else if (fabs(v.z())<e && fabs(v.x())>=e && fabs(v.y())>=e)
		{
			a = v_len2 / v.x();
			b = v_len2 / v.y();
			result = n.x()*n.y()*sqrt(a*a+b*b)/
				sqrt(n.x()*n.x()*a*a+n.y()*n.y()*b*b);
		}
		else if (fabs(v.x())>=e && fabs(v.y())<e && fabs(v.z())<e)
		{
			result = n.x();
		}
		else if (fabs(v.y())>=e && fabs(v.x())<e && fabs(v.z())<e)
		{
			result = n.y();
		}
		else if (fabs(v.z())>=e && fabs(v.x())<e && fabs(v.y())<e)
		{
			result = n.z();
		}
		else
			result = 1.0;

		return result;

	}

	bool TextureRenderer::test_against_view(const BBox &bbox, bool persp)
	{
		memcpy(mvmat_, glm::value_ptr(m_mv_mat2), 16*sizeof(float));
		memcpy(prmat_, glm::value_ptr(m_proj_mat), 16*sizeof(float));

		Transform mv;
		Transform pr;
		mv.set_trans(mvmat_);
		pr.set_trans(prmat_);

		if (persp)
		{
			const Point p0_cam(0.0, 0.0, 0.0);
			Point p0, p0_obj;
			pr.unproject(p0_cam, p0);
			mv.unproject(p0, p0_obj);
			if (bbox.inside(p0_obj))
				return true;
		}

		bool overx = true;
		bool overy = true;
		bool overz = true;
		bool underx = true;
		bool undery = true;
		bool underz = true;
		for (int i = 0; i < 8; i++)
		{
			const Point pold((i & 1) ? bbox.min().x() : bbox.max().x(),
				(i&2)?bbox.min().y():bbox.max().y(),
				(i&4)?bbox.min().z():bbox.max().z());
			const Point p = pr.project(mv.project(pold));
			overx = overx && (p.x() > 1.0);
			overy = overy && (p.y() > 1.0);
			overz = overz && (p.z() > 1.0);
			underx = underx && (p.x() < -1.0);
			undery = undery && (p.y() < -1.0);
			underz = underz && (p.z() < -1.0);
		}

		return !(overx || overy || overz || underx || undery || underz);
	}

	
	GLint TextureRenderer::load_brick(int unit, int c,
		vector<TextureBrick*> *bricks, int bindex,
		GLint filter, bool compression, int mode, bool set_drawn)
	{
		GLint result = -1;

		if (clear_pool_) clear_tex_pool();
		TextureBrick* brick = (*bricks)[bindex];
		int idx;

		if (c < 0 || c >= TEXTURE_MAX_COMPONENTS)
			return 0;
		if (brick->ntype(c) != TextureBrick::TYPE_INT)
			return 0;

		glActiveTexture(GL_TEXTURE0+unit);

		int nb = brick->nb(c);
		int nx = brick->nx();
		int ny = brick->ny();
		int nz = brick->nz();
		GLenum textype = brick->tex_type(c);

		//! Try to find the existing texture in tex_pool_, for this brick.
		idx = -1;
		for(unsigned int i = 0; i < tex_pool_.size() && idx < 0; i++)
		{
			if(tex_pool_[i].id != 0
				&& tex_pool_[i].brick == brick
				&& tex_pool_[i].comp == c
				&& nx == tex_pool_[i].nx
				&& ny == tex_pool_[i].ny
				&& nz == tex_pool_[i].nz
				&& nb == tex_pool_[i].nb
				&& textype == tex_pool_[i].textype
				&& glIsTexture(tex_pool_[i].id))
			{
				//found!
				idx = i;
			}
		}

		if(idx != -1) 
		{
			//! The texture object was located, bind it.
			// bind texture object
			glBindTexture(GL_TEXTURE_3D, tex_pool_[idx].id);
			result = tex_pool_[idx].id;
			// set interpolation method
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);
		} 
		else //idx == -1
		{
			//see if it needs to free some memory
			if (mem_swap_)
				check_swap_memory(brick, c);

			// allocate new object
			unsigned int tex_id;
			glGenTextures(1, (GLuint*)&tex_id);

			// create new entry
			tex_pool_.push_back(TexParam(c, nx, ny, nz, nb, textype, tex_id));
			idx = int(tex_pool_.size())-1;

			tex_pool_[idx].brick = brick;
			tex_pool_[idx].comp = c;
			// bind texture object
			glBindTexture(GL_TEXTURE_3D, tex_pool_[idx].id);
			result = tex_pool_[idx].id;
			// set border behavior
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			// set interpolation method
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);

			// download texture data
#ifdef _WIN32
			if(tex_->isBrxml())
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, nx);
				glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, ny);
			}
			else
			{
				glPixelStorei(GL_UNPACK_ROW_LENGTH, brick->sx());
				glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, brick->sy());
			}
#else
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
#endif
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			if (ShaderProgram::shaders_supported())
			{
				GLenum format;
				GLint internal_format;
				if (nb < 3)
				{
					if (compression && GLEW_ARB_texture_compression_rgtc &&
						brick->ntype(c)==TextureBrick::TYPE_INT && (brick->tex_type(c)==GL_BYTE||brick->tex_type(c)==GL_UNSIGNED_BYTE))
						internal_format = GL_COMPRESSED_RED;
					else
						internal_format = (brick->tex_type(c)==GL_SHORT||
							brick->tex_type(c)==GL_UNSIGNED_SHORT)?
							GL_R16:GL_R8;
					format = GL_RED;
				}
				else
				{
					if (compression && GLEW_ARB_texture_compression_rgtc &&
						brick->ntype(c)==TextureBrick::TYPE_INT && (brick->tex_type(c)==GL_BYTE||brick->tex_type(c)==GL_UNSIGNED_BYTE))
						internal_format = GL_COMPRESSED_RED;
					else
						internal_format = (brick->tex_type(c)==GL_SHORT||
							brick->tex_type(c)==GL_UNSIGNED_SHORT)?
							GL_RGBA16UI:GL_RGBA8UI;
					format = GL_RGBA;
				}

				if (glTexImage3D)
				{
					if(tex_->isBrxml())
					{
						if (load_on_main_thread_)
						{
							bool brkerror = false;
							bool lb_swapped = false;
							if (brick->isLoaded())
							{
								if (brick->get_id_in_loadedbrks() >= 0 && brick->get_id_in_loadedbrks() < loadedbrks.size())
								{
									loadedbrks[brick->get_id_in_loadedbrks()].swapped = true;
									lb_swapped = true;
								}
								else
									brkerror = true;
							}
							else if(mainmem_buf_size_ >= 1.0)
							{
								double bsize = brick->nx()*brick->ny()*brick->nz()*brick->nb(c)/1.04e6;
								if(available_mainmem_buf_size_ - bsize < 0.0)
								{
									double free_mem_size = 0.0;
									while (free_mem_size < bsize && del_id < loadedbrks.size() )
									{
										TextureBrick* b = loadedbrks[del_id].brk;
										if(!loadedbrks[del_id].swapped && b->isLoaded()){
											b->freeBrkData();
											free_mem_size += b->nx() * b->ny() * b->nz() * b->nb(0) / 1.04e6;
										}
										del_id++;
									}
									available_mainmem_buf_size_ += free_mem_size;
								}
							}

							FileLocInfo *finfo = tex_->GetFileName(brick->getID());
							void *texdata = brick->tex_data_brk(c, finfo);
							if (texdata)
							{
								glTexImage3D(GL_TEXTURE_3D, 0, internal_format, nx, ny, nz, 0, format, brick->tex_type(c), 0);
								glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format, brick->tex_type(c), texdata);
							}
							else 
							{
								glDeleteTextures(1, (GLuint*)&tex_pool_[idx].id);
								tex_pool_.erase(tex_pool_.begin()+idx);
								brkerror = true;
								result = -1;
							}

							if (mainmem_buf_size_ == 0.0) brick->freeBrkData();
							else 
							{
								if (brkerror)
								{
									brick->freeBrkData();

									double new_mem = brick->nx()*brick->ny()*brick->nz()*brick->nb(c)/1.04e6;
									available_mainmem_buf_size_ += new_mem;
								}
								else
								{
									if(!lb_swapped)
									{
										double new_mem = brick->nx()*brick->ny()*brick->nz()*brick->nb(c)/1.04e6;
										available_mainmem_buf_size_ -= new_mem;
									}

									LoadedBrick lb;
									lb.swapped = false;
									lb.size = brick->nx()*brick->ny()*brick->nz()*brick->nb(c)/1.04e6;
									lb.brk = brick;
									lb.brk->set_id_in_loadedbrks(loadedbrks.size());
									loadedbrks.push_back(lb);

								}

							}
						}
						else 
						{
							if (brick->isLoaded())
							{
								bool brkerror = false;
								void *texdata = brick->tex_data_brk(c, NULL);
								if (texdata)
								{
									glTexImage3D(GL_TEXTURE_3D, 0, internal_format, nx, ny, nz, 0, format, brick->tex_type(c), 0);
									glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format, brick->tex_type(c), texdata);
								}
								else 
								{
									glDeleteTextures(1, (GLuint*)&tex_pool_[idx].id);
									tex_pool_.erase(tex_pool_.begin()+idx);
									brkerror = true;
									result = -1;
								}
							}
							else if (!interactive_)
							{
								uint32_t rn_time;
								unsigned long elapsed;
								long t;
								do {
									rn_time = GET_TICK_COUNT();
									elapsed = rn_time - st_time_;
									t = up_time_ - elapsed;
									if (t > 0) wxMilliSleep(t);
								} while (elapsed <= up_time_);
								
								if (brick->isLoaded())
								{
									bool brkerror = false;
									void *texdata = brick->tex_data_brk(c, NULL);
									if (texdata)
									{
										glTexImage3D(GL_TEXTURE_3D, 0, internal_format, nx, ny, nz, 0, format, brick->tex_type(c), 0);
										glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format, brick->tex_type(c), texdata);
									}
									else 
									{
										glDeleteTextures(1, (GLuint*)&tex_pool_[idx].id);
										tex_pool_.erase(tex_pool_.begin()+idx);
										brkerror = true;
										result = -1;
									}
								}
								else
								{
									glDeleteTextures(1, (GLuint*)&tex_pool_[idx].id);
									tex_pool_.erase(tex_pool_.begin()+idx);
									result = -1;
								}
							}
							else
							{
								glDeleteTextures(1, (GLuint*)&tex_pool_[idx].id);
								tex_pool_.erase(tex_pool_.begin()+idx);
								result = -1;
							}
						}
					}
					else
					{
						glTexImage3D(GL_TEXTURE_3D, 0, internal_format, nx, ny, nz, 0, format,
							brick->tex_type(c), 0);
#ifdef _WIN32
						glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
							brick->tex_type(c), brick->tex_data(c));
#else
//						if (bricks->size() > 1)
						{
							unsigned long long mem_size = (unsigned long long)nx*
								(unsigned long long)ny*(unsigned long long)nz*nb;
							unsigned char* temp = new unsigned char[mem_size];
							unsigned char* tempp = temp;
							unsigned char* tp = (unsigned char*)(brick->tex_data(c));
							unsigned char* tp2;
							for (unsigned int k = 0; k < nz; ++k)
							{
								tp2 = tp;
								for (unsigned int j = 0; j < ny; ++j)
								{
									memcpy(tempp, tp2, nx*nb);
									tempp += nx*nb;
									tp2 += brick->sx()*nb;
								}
								tp += brick->sx()*brick->sy()*nb;
							}
							glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
								brick->tex_type(c), (GLvoid*)temp);
							delete[]temp;
						}
//						else
//							glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
//							brick->tex_type(c), brick->tex_data(c));
#endif
					}

					if (mem_swap_ && result >= 0)
					{
						double new_mem = brick->nx()*brick->ny()*brick->nz()*brick->nb(c)/1.04e6;
						available_mem_ -= new_mem;
					}

				}
			}

#ifdef _WIN32
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
#endif
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}

		if (mem_swap_ &&
			start_update_loop_ &&
			!done_update_loop_ &&
			set_drawn && result >= 0)
		{
			if (!brick->drawn(mode))
			{
				brick->set_drawn(mode, true);
				cur_brick_num_++;
				cur_chan_brick_num_++;
			}
		}

		glActiveTexture(GL_TEXTURE0);

		return result;
	}

	//search for or create the mask texture in the texture pool
	GLint TextureRenderer::load_brick_mask(vector<TextureBrick*> *bricks, int bindex, GLint filter, bool compression, int unit)
	{
		GLint result = -1;

		TextureBrick* brick = (*bricks)[bindex];
		int c = brick->nmask();

		glActiveTexture(GL_TEXTURE0+(unit>0?unit:c));

		int nb = brick->nb(c);
		int nx = brick->nx();
		int ny = brick->ny();
		int nz = brick->nz();
		GLenum textype = brick->tex_type(c);

		//! Try to find the existing texture in tex_pool_, for this brick.
		int idx = -1;
		for(unsigned int i = 0; i < tex_pool_.size() && idx < 0; i++)
		{
			if(tex_pool_[i].id != 0
				&& tex_pool_[i].brick == brick
				&& tex_pool_[i].comp == c
				&& nx == tex_pool_[i].nx
				&& ny == tex_pool_[i].ny
				&& nz == tex_pool_[i].nz
				&& nb == tex_pool_[i].nb
				&& textype == tex_pool_[i].textype
				&& glIsTexture(tex_pool_[i].id))
			{
				//found!
				idx = i;
			}
		}

		if(idx != -1) 
		{
			//! The texture object was located, bind it.
			// bind texture object
			glBindTexture(GL_TEXTURE_3D, tex_pool_[idx].id);
			result = tex_pool_[idx].id;
			// set interpolation method
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);
		} 
		else //idx == -1
		{
			////! try again to find the matching texture object
			unsigned int tex_id;
			glGenTextures(1, (GLuint*)&tex_id);

			tex_pool_.push_back(TexParam(c, nx, ny, nz, nb, textype, tex_id));
			idx = int(tex_pool_.size())-1;

			tex_pool_[idx].brick = brick;
			tex_pool_[idx].comp = c;
			// bind texture object
			glBindTexture(GL_TEXTURE_3D, tex_pool_[idx].id);
			result = tex_pool_[idx].id;

			// set border behavior
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			// set interpolation method
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filter);

			// download texture data
#ifdef _WIN32
			glPixelStorei(GL_UNPACK_ROW_LENGTH, brick->sx());
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, brick->sy());
#else
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
#endif
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			if (ShaderProgram::shaders_supported())
			{
				GLint internal_format = GL_R8;
				GLenum format = (nb == 1 ? GL_RED : GL_RGBA);
				if (glTexImage3D)
				{
					glTexImage3D(GL_TEXTURE_3D, 0, internal_format, nx, ny, nz, 0, format,
					brick->tex_type(c), 0);
#ifdef _WIN32
					glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
					brick->tex_type(c), brick->tex_data(c));
#else
//					if (bricks->size() > 1)
					{
						unsigned long long mem_size = (unsigned long long)nx*
							(unsigned long long)ny*(unsigned long long)nz*nb;
						unsigned char* temp = new unsigned char[mem_size];
						unsigned char* tempp = temp;
						unsigned char* tp = (unsigned char*)(brick->tex_data(c));
						unsigned char* tp2;
						for (unsigned int k = 0; k < nz; ++k)
						{
							tp2 = tp;
							for (unsigned int j = 0; j < ny; ++j)
							{
								memcpy(tempp, tp2, nx*nb);
								tempp += nx*nb;
								tp2 += brick->sx()*nb;
				}
							tp += brick->sx()*brick->sy()*nb;
						}
						glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
							brick->tex_type(c), (GLvoid*)temp);
						delete[]temp;
					}
//					else
//						glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
//							brick->tex_type(c), brick->tex_data(c));
#endif
			}
			}

#ifdef _WIN32
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
#endif
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}

		if (mem_swap_ &&
			start_update_loop_ &&
			!done_update_loop_)
		{
			if (!brick->drawn(TEXTURE_RENDER_MODE_MASK))
			{
				brick->set_drawn(TEXTURE_RENDER_MODE_MASK, true);
				cur_brick_num_++;
				cur_chan_brick_num_++;
			}
		}
		
		glActiveTexture(GL_TEXTURE0);
		
		return result;
	}

	//search for or create the label texture in the texture pool
	GLint TextureRenderer::load_brick_label(vector<TextureBrick*> *bricks, int bindex)
	{
		GLint result = -1;

		TextureBrick* brick = (*bricks)[bindex];
		int c = brick->nlabel();

		glActiveTexture(GL_TEXTURE0+c);

		int nb = brick->nb(c);
		int nx = brick->nx();
		int ny = brick->ny();
		int nz = brick->nz();
		GLenum textype = brick->tex_type(c);
		
		//! Try to find the existing texture in tex_pool_, for this brick.
		int idx = -1;
		for(unsigned int i = 0; i < tex_pool_.size() && idx < 0; i++)
		{
			if(tex_pool_[i].id != 0
				&& tex_pool_[i].brick == brick
				&& tex_pool_[i].comp == c
				&& nx == tex_pool_[i].nx
				&& ny == tex_pool_[i].ny
				&& nz == tex_pool_[i].nz
				&& nb == tex_pool_[i].nb
				&& textype == tex_pool_[i].textype
				&& glIsTexture(tex_pool_[i].id))
			{
				//found!
				idx = i;
			}
		}

		if(idx != -1) 
		{
			//! The texture object was located, bind it.
			// bind texture object
			glBindTexture(GL_TEXTURE_3D, tex_pool_[idx].id);
			result = tex_pool_[idx].id;
			// set interpolation method
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		} 
		else //idx == -1
		{
			unsigned int tex_id;
			glGenTextures(1, (GLuint*)&tex_id);
			tex_pool_.push_back(TexParam(c, nx, ny, nz, nb, textype, tex_id));
			idx = int(tex_pool_.size())-1;

			tex_pool_[idx].brick = brick;
			tex_pool_[idx].comp = c;
			// bind texture object
			glBindTexture(GL_TEXTURE_3D, tex_pool_[idx].id);
			result = tex_pool_[idx].id;

			// set border behavior
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			// set interpolation method
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// download texture data
#ifdef _WIN32
			glPixelStorei(GL_UNPACK_ROW_LENGTH, brick->sx());
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, brick->sy());
#else
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
#endif
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			if (ShaderProgram::shaders_supported())
			{
				GLenum format = GL_RED_INTEGER;
				GLint internal_format = GL_R32UI;
				if (glTexImage3D)
				{
					glTexImage3D(GL_TEXTURE_3D, 0, internal_format, nx, ny, nz, 0, format,
						brick->tex_type(c), NULL);
#ifdef _WIN32
					glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
					brick->tex_type(c), brick->tex_data(c));
#else
//					if (bricks->size() > 1)
					{
						unsigned long long mem_size = (unsigned long long)nx*
							(unsigned long long)ny*(unsigned long long)nz*nb;
						unsigned char* temp = new unsigned char[mem_size];
						unsigned char* tempp = temp;
						unsigned char* tp = (unsigned char*)(brick->tex_data(c));
						unsigned char* tp2;
						for (unsigned int k = 0; k < nz; ++k)
						{
							tp2 = tp;
							for (unsigned int j = 0; j < ny; ++j)
							{
								memcpy(tempp, tp2, nx*nb);
								tempp += nx*nb;
								tp2 += brick->sx()*nb;
							}
							tp += brick->sx()*brick->sy()*nb;
						}
						glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
							brick->tex_type(c), (GLvoid*)temp);
						delete[]temp;
					}
//					else
//						glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, format,
//					brick->tex_type(c), brick->tex_data(c));
#endif
				}
			}

#ifdef _WIN32
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
#endif
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		}

		if (mem_swap_ &&
			start_update_loop_ &&
			!done_update_loop_)
		{
			if (!brick->drawn(TEXTURE_RENDER_MODE_LABEL))
			{
				brick->set_drawn(TEXTURE_RENDER_MODE_LABEL, true);
				cur_brick_num_++;
				cur_chan_brick_num_++;
			}
		}

		glActiveTexture(GL_TEXTURE0);

		return result;
	}

	bool TextureRenderer::brick_sort(const BrickDist& bd1, const BrickDist& bd2)
	{
		return bd1.dist > bd2.dist;
	}

	void TextureRenderer::check_swap_memory(TextureBrick* brick, int c)
	{
		unsigned int i;
		double new_mem = brick->nx()*brick->ny()*brick->nz()*brick->nb(c)/1.04e6;

		if (use_mem_limit_)
		{
			if (available_mem_ >= new_mem)
				return;
		}
		else
		{
			GLenum error = glGetError();
			GLint mem_info[4] = {0, 0, 0, 0};
			glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, mem_info);
			error = glGetError();
			if (error == GL_INVALID_ENUM)
			{
				glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, mem_info);
				error = glGetError();
				if (error == GL_INVALID_ENUM)
					return;
			}

			//available memory size in MB
			available_mem_ = mem_info[0]/1024.0;
			if (available_mem_ >= new_mem)
				return;
		}

		vector<BrickDist> bd_list;
		BrickDist bd;
		//generate a list of bricks and their distances to the new brick
		for (i=0; i<tex_pool_.size(); i++)
		{
			bd.index = i;
			bd.brick = tex_pool_[i].brick;
			//calculate the distance
			bd.dist = brick->bbox().distance(bd.brick->bbox());
			bd_list.push_back(bd);
		}

		//release bricks far away
		double est_avlb_mem = available_mem_;
		if (bd_list.size() > 0)
		{
			//sort from farthest to closest
			std::sort(bd_list.begin(), bd_list.end(), TextureRenderer::brick_sort);

			vector<BrickDist> bd_undisp;
			vector<BrickDist> bd_saved;
			vector<BrickDist> bd_others;
			for (i=0; i<bd_list.size(); i++)
			{
				TextureBrick* b = bd_list[i].brick;
				if (!b->get_disp())
					bd_undisp.push_back(bd_list[i]);
				else if (b->is_tex_deletion_prevented())
					bd_saved.push_back(bd_list[i]);
				else
					bd_others.push_back(bd_list[i]);
			}


			//remove
			for (i=0; i<bd_undisp.size(); i++)
			{
				TextureBrick* btemp = bd_undisp[i].brick;
				tex_pool_[bd_undisp[i].index].delayed_del = true;
				double released_mem = btemp->nx()*btemp->ny()*btemp->nz()*btemp->nb(c)/1.04e6;
				est_avlb_mem += released_mem;
				if (est_avlb_mem >= new_mem)
					break;
			}
			if (est_avlb_mem < new_mem)
			{
				for (i=0; i<bd_others.size(); i++)
				{
					TextureBrick* btemp = bd_others[i].brick;
					tex_pool_[bd_others[i].index].delayed_del = true;
					double released_mem = btemp->nx()*btemp->ny()*btemp->nz()*btemp->nb(c)/1.04e6;
					est_avlb_mem += released_mem;
					if (est_avlb_mem >= new_mem)
						break;
				}
			}
			if (est_avlb_mem < new_mem)
			{
				for (i=0; i<bd_saved.size(); i++)
				{
					TextureBrick* btemp = bd_saved[i].brick;
					tex_pool_[bd_saved[i].index].delayed_del = true;
					double released_mem = btemp->nx()*btemp->ny()*btemp->nz()*btemp->nb(c)/1.04e6;
					est_avlb_mem += released_mem;
					if (est_avlb_mem >= new_mem)
						break;
				}
			}

			//delete from pool
			for (int j=int(tex_pool_.size()-1); j>=0; j--)
			{
				if (tex_pool_[j].delayed_del &&
					glIsTexture(tex_pool_[j].id))
				{
					glDeleteTextures(1, (GLuint*)&tex_pool_[j].id);
					tex_pool_.erase(tex_pool_.begin()+j);
				}
			}

			if (use_mem_limit_)
				available_mem_ = est_avlb_mem;
		}
	}

	void TextureRenderer::draw_slices(double d)
	{
		glBegin(GL_QUADS);
			glMultiTexCoord3d(GL_TEXTURE0, 0.0, 0.0, d); glVertex2d(-1.0, -1.0);
			glMultiTexCoord3d(GL_TEXTURE0, 1.0, 0.0, d); glVertex2d(1.0, -1.0);
			glMultiTexCoord3d(GL_TEXTURE0, 1.0, 1.0, d); glVertex2d(1.0, 1.0);
			glMultiTexCoord3d(GL_TEXTURE0, 0.0, 1.0, d); glVertex2d(-1.0, 1.0);
		glEnd();
	}

	void TextureRenderer::draw_view_quad(double d)
	{
		glEnable(GL_TEXTURE_2D);
		float points[] = {
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, float(d),
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f, float(d),
			-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, float(d),
			1.0f, 1.0f, 0.0f, 1.0f, 1.0f, float(d)};

		if (!glIsBuffer(m_quad_vbo))
			glGenBuffers(1, &m_quad_vbo);
		if (!glIsVertexArray(m_quad_vao))
			glGenVertexArrays(1, &m_quad_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*24, points, GL_STREAM_DRAW);

		glBindVertexArray(m_quad_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)12);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);
		glDisable(GL_TEXTURE_2D);
	}

	void TextureRenderer::draw_polygons(vector<double>& vertex, 
		vector<double>& texcoord,
		vector<int>& poly, 
		bool fog,
		ShaderProgram* shader)
	{
		double mvmat[16];
		if(fog)
		{
			glGetDoublev(GL_MODELVIEW_MATRIX, mvmat);
		}

		for(unsigned int i=0, k=0; i<poly.size(); i++)
		{
			glBegin(GL_POLYGON);
			{
				for(int j=0; j<poly[i]; j++) 
				{
					double* t = &texcoord[(k+j)*3];
					double* v = &vertex[(k+j)*3];
					if (glMultiTexCoord3d) 
					{
						glMultiTexCoord3d(GL_TEXTURE0, t[0], t[1], t[2]);
						if(fog) 
						{
							double vz = mvmat[2]*v[0] + mvmat[6]*v[1] + mvmat[10]*v[2] + mvmat[14];
							glMultiTexCoord3d(GL_TEXTURE1, -vz, 0.0, 0.0);
						}
					}
					glVertex3d(v[0], v[1], v[2]);
				}
			}
			glEnd();

			glFlush();

			k += poly[i];
		}
	}

	void TextureRenderer::draw_polygons_wireframe(vector<double>& vertex,
		vector<double>& texcoord,
		vector<int>& poly,
		bool fog)
	{
		for(unsigned int i=0, k=0; i<poly.size(); i++)
		{
			glBegin(GL_LINE_LOOP);
			{
				for(int j=0; j<poly[i]; j++)
				{
					double* v = &vertex[(k+j)*3];
					glVertex3d(v[0], v[1], v[2]);
				}
			}
			glEnd();
			k += poly[i];
		}
	}

	void TextureRenderer::draw_polygons(vector<float>& vertex,
		vector<uint32_t>& triangle_verts)
	{
		if (vertex.empty() || triangle_verts.empty())
			return;

		if (!glIsBuffer(m_slices_vbo))
			glGenBuffers(1, &m_slices_vbo);
		if (!glIsBuffer(m_slices_ibo))
			glGenBuffers(1, &m_slices_ibo);
		if (!glIsVertexArray(m_slices_vao))
			glGenVertexArrays(1, &m_slices_vao);

		//link to the new data
		glBindBuffer(GL_ARRAY_BUFFER, m_slices_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertex.size(), &vertex[0], GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_slices_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*triangle_verts.size(), 
			&triangle_verts[0], GL_STREAM_DRAW);
		
		glBindVertexArray(m_slices_vao);
		//now draw it
		glBindBuffer(GL_ARRAY_BUFFER, m_slices_vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)12);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_slices_ibo);
		glDrawElements(GL_TRIANGLES, triangle_verts.size(), GL_UNSIGNED_INT, 0);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void TextureRenderer::draw_polygons(vector<float>& vertex,
		vector<uint32_t>& triangle_verts, vector<uint32_t>& size)
	{
        if (vertex.empty() || triangle_verts.empty())
            return;

		if (!glIsBuffer(m_slices_vbo))
			glGenBuffers(1, &m_slices_vbo);
		if (!glIsBuffer(m_slices_ibo))
			glGenBuffers(1, &m_slices_ibo);
		if (!glIsVertexArray(m_slices_vao))
			glGenVertexArrays(1, &m_slices_vao);
        
        //link to the new data
        glBindBuffer(GL_ARRAY_BUFFER, m_slices_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertex.size(), &vertex[0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_slices_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*triangle_verts.size(),
                     &triangle_verts[0], GL_DYNAMIC_DRAW);
        
        glBindVertexArray(m_slices_vao);
        //now draw it
        glBindBuffer(GL_ARRAY_BUFFER, m_slices_vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)12);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_slices_ibo);
        int slicen = size.size();
        uint32_t *t = &triangle_verts[0];
        for (int s = 0; s < slicen; s++)
        {
            unsigned int tsize = (size[s]-2)*3;
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*tsize,
                         t, GL_DYNAMIC_DRAW);
            glDrawElements(GL_TRIANGLES, tsize, GL_UNSIGNED_INT, 0);
            glFlush();
            t += tsize;
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        //unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
	}

	void TextureRenderer::draw_polygons_wireframe(vector<float>& vertex,
		vector<uint32_t>& index, vector<uint32_t>& size)
	{
		if (vertex.empty() || index.empty())
			return;

		if (!glIsBuffer(m_slices_vbo))
			glGenBuffers(1, &m_slices_vbo);
		if (!glIsBuffer(m_slices_ibo))
			glGenBuffers(1, &m_slices_ibo);
		if (!glIsVertexArray(m_slices_vao))
			glGenVertexArrays(1, &m_slices_vao);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glBindBuffer(GL_ARRAY_BUFFER, m_slices_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertex.size(), &vertex[0], GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_slices_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*index.size(), 
			&index[0], GL_STREAM_DRAW);

		glBindVertexArray(m_slices_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_slices_vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)12);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_slices_ibo);
		unsigned int idx_num;
		for (unsigned int i=0, k=0; i<size.size(); ++i)
		{
			idx_num = (size[i]-2)*3;
			glDrawElements(GL_TRIANGLES, idx_num, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*> (k));
			k += idx_num*4;
		}
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//bind 2d mask for segmentation
	void TextureRenderer::bind_2d_mask()
	{
		if (ShaderProgram::shaders_supported() && glActiveTexture)
		{
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, tex_2d_mask_);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	//bind 2d weight map for segmentation
	void TextureRenderer::bind_2d_weight()
	{
		if (ShaderProgram::shaders_supported() && glActiveTexture)
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, tex_2d_weight1_);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, tex_2d_weight2_);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	//bind 2d depth map for rendering shadows
	void TextureRenderer::bind_2d_dmap()
	{
		if (ShaderProgram::shaders_supported() && glActiveTexture)
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, tex_2d_dmap_);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	//release texture
	void TextureRenderer::release_texture(int unit, GLenum taget)
	{
		if (ShaderProgram::shaders_supported() && glActiveTexture)
		{
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(taget, 0);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	//Texture‚ÖˆÚ“®—\’è
	void TextureRenderer::rearrangeLoadedBrkVec()
	{
		if (loadedbrks.empty()) return;
		vector<LoadedBrick>::iterator ite = loadedbrks.begin();
		while (ite != loadedbrks.end())
		{
			if (!ite->brk->isLoaded())
			{
				ite->brk->set_id_in_loadedbrks(-1);
				ite = loadedbrks.erase(ite);
			}
			else if(ite->swapped)
			{
				ite = loadedbrks.erase(ite);
			}
			else ite++;
		}
		for (int i = 0; i < loadedbrks.size(); i++) loadedbrks[i].brk->set_id_in_loadedbrks(i);
		del_id = 0;
	}

	//Texture‚ÖˆÚ“®—\’è
	void TextureRenderer::clear_brick_buf()
	{
		int cur_lv = tex_->GetCurLevel();
		for (unsigned int lv = 0; lv < tex_->GetLevelNum(); lv++)
		{
			tex_->setLevel(lv);
			vector<TextureBrick *> *bs = tex_->get_bricks();
			for (unsigned int i = 0; i < bs->size(); i++)
			{
				if((*bs)[i]->isLoaded()){
					available_mainmem_buf_size_ += (*bs)[i]->nx() * (*bs)[i]->ny() * (*bs)[i]->nz() * (*bs)[i]->nb(0) / 1.04e6;
					(*bs)[i]->freeBrkData();
				}
			}
			rearrangeLoadedBrkVec();//tex_‚Åload‚µ‚½‚à‚Ì‚¾‚¯‚ðÁ‚·
		}
		tex_->setLevel(cur_lv);
	}


} // namespace FLIVR


