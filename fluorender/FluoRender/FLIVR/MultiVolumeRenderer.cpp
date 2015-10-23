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

#include "MultiVolumeRenderer.h"
#include "VolShader.h"
#include "ShaderProgram.h"
#include "../compatibility.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace FLIVR
{
#ifdef _WIN32
#undef min
#undef max
#endif

   double MultiVolumeRenderer::sw_ = 0.0;

   MultiVolumeRenderer::MultiVolumeRenderer()
      : mode_(TextureRenderer::MODE_OVER),
      depth_peel_(0),
      hiqual_(true),
      blend_num_bits_(32),
      blend_slices_(false),
      blend_framebuffer_resize_(false),
      blend_framebuffer_(0),
	  blend_tex_id_(0),
	  label_tex_id_(0),
	  blend_fbo_resize_(false),
	  blend_fbo_(0),
	  blend_tex_(0),
	  blend_id_tex_(0),
	  filter_buffer_resize_(false),
	  filter_buffer_(0),
      filter_tex_id_(0),
      noise_red_(false),
      sfactor_(1.0),
      filter_size_min_(0.0),
      filter_size_max_(0.0),
      filter_size_shp_(0.0),
      imode_(false),
      adaptive_(true),
      irate_(1.0),
      sampling_rate_(1.0),
      num_slices_(0),
      colormap_mode_(0)
   {
   }

   MultiVolumeRenderer::MultiVolumeRenderer(MultiVolumeRenderer& copy)
      : mode_(copy.mode_),
      depth_peel_(copy.depth_peel_),
      hiqual_(copy.hiqual_),
      blend_num_bits_(copy.blend_num_bits_),
      blend_slices_(copy.blend_slices_),
      blend_framebuffer_resize_(false),
      blend_framebuffer_(0),
      blend_tex_id_(0),
	  label_tex_id_(0),
	  blend_fbo_resize_(false),
	  blend_fbo_(0),
	  blend_tex_(0),
	  blend_id_tex_(0),
      filter_buffer_resize_(false),
      filter_buffer_(0),
      filter_tex_id_(0),
      noise_red_(false),
      sfactor_(1.0),
      filter_size_min_(0.0),
      filter_size_max_(0.0),
      filter_size_shp_(0.0),
      imode_(copy.imode_),
      adaptive_(copy.adaptive_),
      irate_(copy.irate_),
      sampling_rate_(copy.sampling_rate_),
      num_slices_(0),
      colormap_mode_(copy.colormap_mode_)
   {
   }

   MultiVolumeRenderer::~MultiVolumeRenderer()
   {
   }

   //mode and sampling rate
   void MultiVolumeRenderer::set_mode(TextureRenderer::RenderMode mode)
   {
      mode_ = mode;
   }

   void MultiVolumeRenderer::set_sampling_rate(double rate)
   {
      sampling_rate_ = rate;
      //irate_ = rate>1.0 ? max(rate / 2.0, 1.0) : rate;
      irate_ = max(rate / 2.0, 0.1);
   }

   void MultiVolumeRenderer::set_interactive_rate(double rate)
   {
      irate_ = rate;
   }

   void MultiVolumeRenderer::set_interactive_mode(bool mode)
   {
      imode_ = mode;
   }

   void MultiVolumeRenderer::set_adaptive(bool b)
   {
      adaptive_ = b;
   }

   int MultiVolumeRenderer::get_slice_num()
   {
      return num_slices_;
   }

   //manages volume renderers for rendering
   void MultiVolumeRenderer::add_vr(VolumeRenderer* vr)
   {
      for (unsigned int i=0; i<vr_list_.size(); i++)
      {
         if (vr_list_[i] == vr)
            return;
      }

      vr_list_.push_back(vr);

      if (vr && vr->tex_)
      {
         bbox_.extend(*(vr->tex_->bbox()));
         res_ = Max(res_, vr->tex_->res());
      }
   }

   void MultiVolumeRenderer::clear_vr()
   {
      vr_list_.clear();
      bbox_.reset();
      res_ = Vector(0.0);
   }

   int MultiVolumeRenderer::get_vr_num()
   {
      return int(vr_list_.size());
   }

   //draw
   void MultiVolumeRenderer::draw(bool draw_wireframe_p,
         bool interactive_mode_p,
         bool orthographic_p,
         double zoom, bool intp)
   {
      draw_volume(interactive_mode_p, orthographic_p, zoom, intp);
      if(draw_wireframe_p)
         draw_wireframe(orthographic_p);
   }
      
#define FLV_COLORTYPE_NUM 4
#define FLV_VRMODE_NUM 2
#define FLV_VR_ALPHA 0
#define FLV_VR_SOLID 1
#define FLV_CTYPE_DEFAULT 0
#define FLV_CTYPE_RAINBOW 1
#define FLV_CTYPE_DEPTH 2
#define FLV_CTYPE_INDEX 3
#define vr_stype(x, y) ((x)+(y)*FLV_COLORTYPE_NUM)

   void MultiVolumeRenderer::draw_volume(bool interactive_mode_p, bool orthographic_p, double zoom, bool intp)
   {
      if (get_vr_num()<=0 || !(vr_list_[0]))
         return;

      set_interactive_mode(adaptive_ && interactive_mode_p);

	  bool updating = (TextureRenderer::get_mem_swap() &&
				  TextureRenderer::get_start_update_loop() &&
				  !TextureRenderer::get_done_update_loop());

	  int mode = (colormap_mode_ != FLV_CTYPE_DEPTH) ? 0 : 3;

	  int i;
	  vector<bool> used_colortype(FLV_COLORTYPE_NUM, false);
	  vector<bool> used_shadertype(FLV_COLORTYPE_NUM*FLV_VRMODE_NUM, false);
	  bool blend_slices = blend_slices_;
	  bool use_id_color = false;
	  
	  double sampling_frq_fac = -1.0;
	  for (i=0; i<(int)vr_list_.size(); i++)
	  {
		  VolumeRenderer* vr = vr_list_[i];
		  if (!vr)
			  continue;
		  if (vr->colormap_mode_ >= 0 && vr->colormap_mode_ < FLV_COLORTYPE_NUM)
		  {
			  int cmode = (colormap_mode_ != FLV_CTYPE_DEPTH) ? vr->colormap_mode_ : FLV_CTYPE_DEPTH;
			  int vmode = vr->solid_ ? FLV_VR_SOLID : FLV_VR_ALPHA;
			  used_colortype[cmode] = true;
			  used_shadertype[vr_stype(cmode, vmode)] = true;
			  if (cmode == FLV_CTYPE_INDEX) use_id_color = true;
		  }
		  else
		  {
			  used_colortype[FLV_CTYPE_DEFAULT] = true;
			  used_shadertype[vr_stype(FLV_CTYPE_DEFAULT, FLV_VR_ALPHA)] = true;
		  }

		  Texture *tex = vr->tex_;
		  if (tex)
		  {
			  Transform *field_trans = tex->transform();
			  Vector spcv[3] = {Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0)};
			  double maxlen = -1;
			  for(int j = 0; j < 3 ; j++)
			  {
				  // index space view direction
				  Vector v;
				  v = field_trans->project(spcv[j]);
				  v.safe_normalize();
				  v = field_trans->project(spcv[j]);

				  double len = Dot(spcv[j], v);;
				  if(len > maxlen) maxlen = len;
			  }
			  if (maxlen > sampling_frq_fac) sampling_frq_fac = maxlen;
		  }
	  }

	  if (use_id_color) blend_slices = true;

	  // Set sampling rate based on interaction.
      double rate = imode_ ? irate_ : sampling_rate_;
	  Vector diag = vr_list_[0]->tex_->bbox()->diagonal();
	  double dt = 0.0025/rate;
	  num_slices_ = (int)(diag.length()/dt);

	  vector<TextureBrick*> bs;
	  unsigned long bs_size = 0; 
	  for (i = 0; i < vr_list_.size(); i++) bs_size += vr_list_[i]->tex_->get_bricks()->size();
	  bs.reserve(bs_size);
	  int remain_brk = 0;
	  int finished_brk = 0;
	  	   
	  int all_timin = INT_MAX, all_timax = -INT_MAX;
	  for (i=0; i<(int)vr_list_.size(); i++)
	  {
		  Texture *tex = vr_list_[i]->tex_;
		  
		  double rate_fac = 1.0;
		  double vr_dt = dt * vr_list_[i]->compute_dt_fac(sampling_frq_fac, &rate_fac);

		  vector<TextureBrick *> *brs = tex->get_bricks();
		  Ray view_ray = vr_list_[i]->compute_view();
		  for (int j = 0; j < brs->size(); j++)
		  {
			  TextureBrick *b = (*brs)[j];

			  b->set_vr(vr_list_[i]);

			  auto vr = vr_list_[i];
			  bool disp = true;
			  if (updating) disp = b->get_disp();
			  
			  if (b->compute_t_index_min_max(view_ray, vr_dt))
			  {
				  b->set_vr(vr_list_[i]);
				  b->set_rate_fac(rate_fac);

				  int tmp = b->timin();
				  all_timin = (all_timin > tmp) ? tmp : all_timin;
				  tmp = b->timax();
				  all_timax = (all_timax < tmp) ? tmp : all_timax;

				  bs.push_back(b);

				  if (!b->drawn(mode) && disp) remain_brk++;
			  }
			  else if (updating && !b->drawn(mode))
			  {
				  b->set_drawn(mode, true);

				  if (disp)
					  TextureRenderer::cur_brick_num_++;
			  }
		  }
	  }
	  bs_size = bs.size();

	  if (remain_brk == 0) return;

      //--------------------------------------------------------------------------
      bool use_shading = vr_list_[0]->shading_;
	  GLboolean use_fog = glIsEnabled(GL_FOG) && colormap_mode_!=FLV_CTYPE_DEPTH;
      GLfloat clear_color[4];
      glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);
      GLint vp[4];
      glGetIntegerv(GL_VIEWPORT, vp);

      // set up blending
 	  glEnablei(GL_BLEND, 0);
	  switch(mode_)
	  {
	  case TextureRenderer::MODE_OVER:
		  glBlendEquationi(0, GL_FUNC_ADD);
		  if (TextureRenderer::update_order_ == 0)
			  glBlendFunci(0, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		  else if (TextureRenderer::update_order_ == 1)
			  glBlendFunci(0, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
		  break;
	  case TextureRenderer::MODE_MIP:
		  glBlendEquationi(0, GL_MAX);
		  glBlendFunci(0, GL_ONE, GL_ONE);
		  break;
	  default:
		  break;
	  }
	  if (used_colortype[FLV_CTYPE_INDEX]) glDisablei(GL_BLEND, 1);

	  // Cache this value to reset, in case another framebuffer is active,
	  // as it is in the case of saving an image from the viewer.
	  GLint cur_framebuffer_id;
	  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur_framebuffer_id);
	  GLint cur_draw_buffer;
	  glGetIntegerv(GL_DRAW_BUFFER, &cur_draw_buffer);
	  GLint cur_read_buffer;
	  glGetIntegerv(GL_READ_BUFFER, &cur_read_buffer);

	  glBindFramebuffer(GL_FRAMEBUFFER, 0);

	  int w = vp[2];
	  int h = vp[3];
	  int w2 = w;
	  int h2 = h;

	  double sf = vr_list_[0]->CalcScaleFactor(w, h, res_.x(), res_.y(), zoom);
	  if (fabs(sf-sfactor_)>0.05)
	  {
		  sfactor_ = sf;
		  blend_framebuffer_resize_ = true;
		  filter_buffer_resize_ = true;
		  blend_fbo_resize_ = true;
	  }
	  else if (sf==1.0 && sfactor_!=1.0)
	  {
		  sfactor_ = sf;
		  blend_framebuffer_resize_ = true;
		  filter_buffer_resize_ = true;
		  blend_fbo_resize_ = true;
	  }

	  w2 = int(w*sfactor_+0.5);
	  h2 = int(h*sfactor_+0.5);


	  static const GLenum draw_buffers[] =
	  {
		  GL_COLOR_ATTACHMENT0,
		  GL_COLOR_ATTACHMENT1
	  };

	  static const GLenum inv_draw_buffers[] =
	  {
		  GL_COLOR_ATTACHMENT1,
		  GL_COLOR_ATTACHMENT0
	  };

	  if(blend_num_bits_ > 8)
	  {
		  if (!glIsFramebuffer(blend_framebuffer_))
		  {
			  glGenFramebuffers(1, &blend_framebuffer_);
			  glGenTextures(1, &blend_tex_id_);
			  glGenTextures(1, &label_tex_id_);

			  glBindFramebuffer(GL_FRAMEBUFFER, blend_framebuffer_);

			  // Initialize texture color renderbuffer
			  glBindTexture(GL_TEXTURE_2D, blend_tex_id_);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT0,
				  GL_TEXTURE_2D, blend_tex_id_, 0);
			  glBindTexture(GL_TEXTURE_2D, 0);

			  glBindTexture(GL_TEXTURE_2D, label_tex_id_);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT1,
				  GL_TEXTURE_2D, label_tex_id_, 0);
			  glBindTexture(GL_TEXTURE_2D, 0);

			  glBindTexture(GL_TEXTURE_2D, 0);
			  glDisable(GL_TEXTURE_2D);
		  }
		  else
		  {
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_framebuffer_);

			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT0,
				  GL_TEXTURE_2D, blend_tex_id_, 0);

			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT1,
				  GL_TEXTURE_2D, label_tex_id_, 0);
		  }

		  if (blend_framebuffer_resize_)
		  {
			  // resize texture color renderbuffer
			  glBindTexture(GL_TEXTURE_2D, blend_tex_id_);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glBindTexture(GL_TEXTURE_2D, 0);

			  glBindTexture(GL_TEXTURE_2D, label_tex_id_);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glBindTexture(GL_TEXTURE_2D, 0);

			  blend_framebuffer_resize_ = false;

			  glBindTexture(GL_TEXTURE_2D, 0);
			  glDisable(GL_TEXTURE_2D);
		  }

		  glDrawBuffers((TextureRenderer::cur_tid_offset_multi_==0 && used_colortype[FLV_CTYPE_INDEX]) ? 2 : 1, draw_buffers);

		  glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
		  glClear(GL_COLOR_BUFFER_BIT);

		  glDrawBuffers(used_colortype[FLV_CTYPE_INDEX] ? 2 : 1, draw_buffers);

		  glViewport(vp[0], vp[1], w2, h2);

		  if (glIsTexture(label_tex_id_) && used_colortype[FLV_CTYPE_INDEX])
		  {
			  glActiveTexture(GL_TEXTURE5);
			  glEnable(GL_TEXTURE_2D);
			  glBindTexture(GL_TEXTURE_2D, label_tex_id_);
			  glActiveTexture(GL_TEXTURE0);
		  }
		  
		  glBindTexture(GL_TEXTURE_2D, 0);
	  }

	  GLint draw_buffer;
	  glGetIntegerv(GL_DRAW_BUFFER, &draw_buffer);
	  GLint read_buffer;
	  glGetIntegerv(GL_READ_BUFFER, &read_buffer);

      //disable depth buffer writing
      glDepthMask(GL_FALSE);

      //--------------------------------------------------------------------------
      // Set up shaders
	  vector<ShaderProgram*> shader(FLV_COLORTYPE_NUM*FLV_VRMODE_NUM, nullptr);
	  bool multi_shader = false;
	  int used_shader_n = 0;
	  int shader_id = 0;
	  for (i = 0; i < FLV_COLORTYPE_NUM; i++)
	  {
		  for (int j = 0; j < FLV_VRMODE_NUM; j++)
		  {
			  if (used_shadertype[vr_stype(i, j)])
			  {
				  bool solid = (j == FLV_VR_SOLID) ? true : false; 
				  shader[vr_stype(i, j)] = VolumeRenderer::vol_shader_factory_.shader(
					  false, vr_list_[0]->tex_->nc(),
					  use_shading, use_fog!=0,
					  depth_peel_, true,
					  hiqual_, 0, i, 0, 0, solid, 1);

				  used_shader_n++;
				  shader_id = vr_stype(i, j);
			  }
		  }
	  }
	  if (used_shader_n > 1) multi_shader = true;

	  if (!multi_shader && !blend_slices)
	  {
		  if (!shader[shader_id]->valid())
			  shader[shader_id]->create();
		  shader[shader_id]->bind();
	  }

	  //takashi_debug
/*	  ofstream ofs;
	  ofs.open("draw_shader_depth.txt");
	  ofs << shader->getProgram() << endl;
	  ofs.close();
*/

      if (blend_slices && colormap_mode_!=FLV_CTYPE_DEPTH)
      {
		  glEnable(GL_TEXTURE_2D);
		  glBindFramebuffer(GL_FRAMEBUFFER, 0);

         //check blend buffer
		  if (!glIsFramebuffer(blend_fbo_))
		  {
			  glGenFramebuffers(1, &blend_fbo_);
			  if (!glIsTexture(blend_tex_))
				  glGenTextures(1, &blend_tex_);
			  if (!glIsTexture(blend_id_tex_))
				  glGenTextures(1, &blend_id_tex_);
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);
			  // Initialize texture color renderbuffer
			  glBindTexture(GL_TEXTURE_2D, blend_tex_);
			  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT0,
				  GL_TEXTURE_2D, blend_tex_, 0);
			  glBindTexture(GL_TEXTURE_2D, 0);

			  glBindTexture(GL_TEXTURE_2D, blend_id_tex_);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glFramebufferTexture2D(GL_FRAMEBUFFER,
			  GL_COLOR_ATTACHMENT1,
			  GL_TEXTURE_2D, blend_id_tex_, 0);
			  glBindTexture(GL_TEXTURE_2D, 0);

			  glBindFramebuffer(GL_FRAMEBUFFER, 0);
		  }
		  else
		  {
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);

			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT0,
				  GL_TEXTURE_2D, blend_tex_, 0);

			  glFramebufferTexture2D(GL_FRAMEBUFFER,
				  GL_COLOR_ATTACHMENT1,
				  GL_TEXTURE_2D, blend_id_tex_, 0);
			  
			  glBindFramebuffer(GL_FRAMEBUFFER, 0);
		  }

		  if (blend_fbo_resize_)
		  {
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);

			  glBindTexture(GL_TEXTURE_2D, blend_tex_);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glBindTexture(GL_TEXTURE_2D, 0);

			  glBindTexture(GL_TEXTURE_2D, blend_id_tex_);
			  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
				  GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
			  glBindTexture(GL_TEXTURE_2D, 0);

			  blend_fbo_resize_ = false;

			  glBindFramebuffer(GL_FRAMEBUFFER, 0);
		  }

		  if (glIsTexture(blend_id_tex_) && used_colortype[FLV_CTYPE_INDEX])
		  {
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);

			  glActiveTexture(GL_TEXTURE6);
			  glEnable(GL_TEXTURE_2D);
			  glBindTexture(GL_TEXTURE_2D, blend_id_tex_);
			  glActiveTexture(GL_TEXTURE0);

			  glBindFramebuffer(GL_FRAMEBUFFER, 0);
		  }

		  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);

		  glViewport(vp[0], vp[1], w2, h2);
	  }

	  //--------------------------------------------------------------------------
      // enable data texture unit 0
      glActiveTexture(GL_TEXTURE0);

      glEnable(GL_TEXTURE_3D);
	  glEnable(GL_TEXTURE_2D);

	  vector<TextureBrick *> cur_brs;
	  int cur_bid;
	  bool order = TextureRenderer::get_update_order();
	  int start_i = order ? all_timin : all_timax;

	  if (updating)
	  {
		  start_i += TextureRenderer::cur_tid_offset_multi_;
	  }

	  std::sort(bs.begin(), bs.end(), order?TextureBrick::less_timin:TextureBrick::high_timax);
	  cur_bid = 0;
	  for (i = start_i; order?(i <= all_timax):(i >= all_timin); i += order?1:-1)
	  {
		  if (TextureRenderer::get_mem_swap())
		  {
			  uint32_t rn_time = GET_TICK_COUNT();
			  if (rn_time - TextureRenderer::get_st_time() > TextureRenderer::get_up_time())
				  break;
		  }

		  while (cur_bid < bs_size && (order?(bs[cur_bid]->timin() <= i):(bs[cur_bid]->timax() >= i)))
		  {
			  if (updating)
			  {
				  if (bs[cur_bid]->drawn(mode))
				  {
					  cur_bid++;
					  continue;
				  }

				  if (bs[cur_bid]->get_disp())
				  {
					  bs[cur_bid]->compute_polygons2();
					  cur_brs.push_back(bs[cur_bid]);
				  }
				  else
				  {
					  if (!bs[cur_bid]->drawn(mode))
						  bs[cur_bid]->set_drawn(mode, true);
				  }
			  }
			  else
			  {
				  bs[cur_bid]->compute_polygons2();
				  cur_brs.push_back(bs[cur_bid]);
			  }

			  cur_bid++;
		  }

		  if (cur_brs.size() == 0)
			  continue;

		  if (blend_slices && colormap_mode_!=FLV_CTYPE_DEPTH)
		  {
			  glBindFramebuffer(GL_FRAMEBUFFER, 0);
			  
			  //set blend buffer
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);
			  glDrawBuffers(used_colortype[FLV_CTYPE_INDEX] ? 2 : 1, draw_buffers);

			  glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
			  glClear(GL_COLOR_BUFFER_BIT);
			  glEnable(GL_BLEND);
			  glBlendFunc(GL_ONE, GL_ONE);

			  glEnable(GL_TEXTURE_3D);
			  if (used_colortype[FLV_CTYPE_INDEX]) glEnable(GL_TEXTURE_2D);
			  else glDisable(GL_TEXTURE_2D);
		  }

		  for (int j = 0; j < cur_brs.size(); j++)
		  {
			  TextureBrick *b = cur_brs[j];
			  VolumeRenderer *vr = b->get_vr();
			  int vr_cmode = colormap_mode_ != FLV_CTYPE_DEPTH ? vr->colormap_mode_ : FLV_CTYPE_DEPTH;
			  int vr_shader_id = vr_stype(vr_cmode, vr->solid_ ? FLV_VR_SOLID : FLV_VR_ALPHA);

			  double id_mode = 0.0;
			  if (blend_slices && colormap_mode_!=FLV_CTYPE_DEPTH)
			  {
				  if (vr_cmode == FLV_CTYPE_INDEX)
				  {
					  id_mode = 1.0f;
					  glDrawBuffers(2, inv_draw_buffers);
				  }
				  else glDrawBuffers(1, draw_buffers);
			  }

			  int s_v, s_i; 
			  float *vert;
			  uint32_t *indx;
			  b->get_polygon(i, s_v, vert, s_i, indx);
			  if (indx == nullptr || vert == nullptr || s_v == 0 || s_i == 0)
				  continue;

			  if (blend_slices || multi_shader)
			  {
				  if (shader[vr_shader_id])
				  {
					  if (!shader[vr_shader_id]->valid())
						  shader[vr_shader_id]->create();
					  shader[vr_shader_id]->bind();
				  }
			  }

			  if (vr_cmode==1 && vr->colormap_proj_)
			  {
				  float matrix[16];
				  BBox dbox = b->dbox();
				  matrix[0] = float(dbox.max().x()-dbox.min().x());
				  matrix[1] = 0.0f;
				  matrix[2] = 0.0f;
				  matrix[3] = 0.0f;
				  matrix[4] = 0.0f;
				  matrix[5] = float(dbox.max().y()-dbox.min().y());
				  matrix[6] = 0.0f;
				  matrix[7] = 0.0f;
				  matrix[8] = 0.0f;
				  matrix[9] = 0.0f;
				  matrix[10] = float(dbox.max().z()-dbox.min().z());
				  matrix[11] = 0.0f;
				  matrix[12] = float(dbox.min().x());
				  matrix[13] = float(dbox.min().y());
				  matrix[14] = float(dbox.min().z());
				  matrix[15] = 1.0f;
				  shader[vr_shader_id]->setLocalParamMatrix(5, matrix);
			  }

			  Transform *tform = vr->tex_->transform();
			  double mvmat[16];
			  tform->get_trans(mvmat);
			  vr->m_mv_mat2 = glm::mat4(
				  mvmat[0], mvmat[4], mvmat[8], mvmat[12],
				  mvmat[1], mvmat[5], mvmat[9], mvmat[13],
				  mvmat[2], mvmat[6], mvmat[10], mvmat[14],
				  mvmat[3], mvmat[7], mvmat[11], mvmat[15]);
			  vr->m_mv_mat2 = vr->m_mv_mat * vr->m_mv_mat2;
			  shader[vr_shader_id]->setLocalParamMatrix(0, glm::value_ptr(vr->m_proj_mat));
			  shader[vr_shader_id]->setLocalParamMatrix(1, glm::value_ptr(vr->m_mv_mat2));

			  if (depth_peel_ || vr_cmode == FLV_CTYPE_DEPTH)
				  shader[vr_shader_id]->setLocalParam(7, 1.0/double(w2), 1.0/double(h2), 0.0, 0.0);

			  shader[vr_shader_id]->setLocalParam(4, 1.0/b->nx(), 1.0/b->ny(), 1.0/b->nz(), 1.0/rate*b->rate_fac());

			  //for brick transformation
			  float matrix[16];
			  BBox dbox = b->dbox();
			  matrix[0] = float(dbox.max().x()-dbox.min().x());
			  matrix[1] = 0.0f;
			  matrix[2] = 0.0f;
			  matrix[3] = 0.0f;
			  matrix[4] = 0.0f;
			  matrix[5] = float(dbox.max().y()-dbox.min().y());
			  matrix[6] = 0.0f;
			  matrix[7] = 0.0f;
			  matrix[8] = 0.0f;
			  matrix[9] = 0.0f;
			  matrix[10] = float(dbox.max().z()-dbox.min().z());
			  matrix[11] = 0.0f;
			  matrix[12] = float(dbox.min().x());
			  matrix[13] = float(dbox.min().y());
			  matrix[14] = float(dbox.min().z());
			  matrix[15] = 1.0f;
			  shader[vr_shader_id]->setLocalParamMatrix(2, matrix);

			  //fog
			  if (use_fog)
				  shader[vr_shader_id]->setLocalParam(8, vr->m_fog_intensity, vr->m_fog_start, vr->m_fog_end, 0.0);
			  
			  //draw a single slice
			  // set shader parameters
			  light_pos_ = b->vray()->direction();
			  light_pos_.safe_normalize();
			  shader[vr_shader_id]->setLocalParam(0, light_pos_.x(), light_pos_.y(), light_pos_.z(), vr->alpha_);
			  shader[vr_shader_id]->setLocalParam(1, 2.0 - vr->ambient_,
				  vr->shading_?vr->diffuse_:0.0,
				  vr->specular_,
				  vr->shine_);
			  shader[vr_shader_id]->setLocalParam(2, vr->scalar_scale_,
				  vr->gm_scale_,
				  vr->lo_thresh_,
				  vr->hi_thresh_);
			  shader[vr_shader_id]->setLocalParam(3, 1.0/vr->gamma3d_,
				  vr->gm_thresh_,
				  vr->offset_,
				  sw_);
			  double spcx, spcy, spcz;
			  vr->tex_->get_spacings(spcx, spcy, spcz);
			  shader[vr_shader_id]->setLocalParam(5, spcx, spcy, spcz, 1.0);
			  
			  switch (vr_cmode)
			  {
			  case FLV_CTYPE_DEFAULT://normal
				  shader[vr_shader_id]->setLocalParam(6, vr->color_.r(), vr->color_.g(), vr->color_.b(), 0.0);
				  break;
			  case FLV_CTYPE_RAINBOW://colormap
				  shader[vr_shader_id]->setLocalParam(6, vr->colormap_low_value_, vr->colormap_hi_value_,
					  vr->colormap_hi_value_-vr->colormap_low_value_, 0.0);
				  break;
			  case FLV_CTYPE_DEPTH://depth map
				  shader[vr_shader_id]->setLocalParam(6, vr->color_.r(), vr->color_.g(), vr->color_.b(), 0.0);
				  break;
			  case FLV_CTYPE_INDEX://indexed color
				  HSVColor hsv(vr->color_);
				  double luminance = hsv.val();
				  shader[vr_shader_id]->setLocalParam(6, 1.0/double(w2), 1.0/double(h2), luminance, id_mode);
				  break;
			  }

			  double abcd[4];
			  vr->planes_[0]->get(abcd);
			  shader[vr_shader_id]->setLocalParam(10, abcd[0], abcd[1], abcd[2], abcd[3]);
			  vr->planes_[1]->get(abcd);
			  shader[vr_shader_id]->setLocalParam(11, abcd[0], abcd[1], abcd[2], abcd[3]);
			  vr->planes_[2]->get(abcd);
			  shader[vr_shader_id]->setLocalParam(12, abcd[0], abcd[1], abcd[2], abcd[3]);
			  vr->planes_[3]->get(abcd);
			  shader[vr_shader_id]->setLocalParam(13, abcd[0], abcd[1], abcd[2], abcd[3]);
			  vr->planes_[4]->get(abcd);
			  shader[vr_shader_id]->setLocalParam(14, abcd[0], abcd[1], abcd[2], abcd[3]);
			  vr->planes_[5]->get(abcd);
			  shader[vr_shader_id]->setLocalParam(15, abcd[0], abcd[1], abcd[2], abcd[3]);

			  //bind depth texture for rendering shadows
			  if (vr_cmode == FLV_CTYPE_DEPTH)
			  {
				  if (blend_num_bits_ > 8)
					  vr->tex_2d_dmap_ = blend_tex_id_;
				  vr->bind_2d_dmap();
			  }

			  GLint filter;
			  if (intp && vr_cmode != FLV_CTYPE_INDEX)
				  filter = GL_LINEAR;
			  else
				  filter = GL_NEAREST;
			  vr->load_brick(0, 0, &vector<TextureBrick *>(1,b), 0, filter, vr->compression_, 0, false);
			  
			  glBindVertexArray(vr->m_slices_vao);
			  glBindBuffer(GL_ARRAY_BUFFER, vr->m_slices_vbo);
			  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*s_v*6, vert, GL_DYNAMIC_DRAW);
			  glEnableVertexAttribArray(0);
			  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)0);
			  glEnableVertexAttribArray(1);
			  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (const GLvoid*)12);
			  glDrawElements(GL_TRIANGLES, s_i*3, GL_UNSIGNED_INT, indx);
			  glDisableVertexAttribArray(0);
			  glDisableVertexAttribArray(1);
			  glBindBuffer(GL_ARRAY_BUFFER, 0);
			  glBindVertexArray(0);

			  //release depth texture for rendering shadows
			  if (vr_cmode == FLV_CTYPE_DEPTH)
				  vr->release_texture(4, GL_TEXTURE_2D);

		  }//for (int j = 0; j < cur_brs.size(); j++)

		  glFinish();

		  if (blend_slices && colormap_mode_!=FLV_CTYPE_DEPTH)
		  {
			  //set buffer back
			  glBindFramebuffer(GL_FRAMEBUFFER, 0);
			  glBindFramebuffer(GL_FRAMEBUFFER, blend_framebuffer_);
			  glDrawBuffer(draw_buffer);
			  glReadBuffer(read_buffer);

			  glActiveTexture(GL_TEXTURE6);
			  glBindTexture(GL_TEXTURE_2D, blend_id_tex_);
			  glActiveTexture(GL_TEXTURE5);
			  glBindTexture(GL_TEXTURE_2D, label_tex_id_);
			  glActiveTexture(GL_TEXTURE0);
			  glBindTexture(GL_TEXTURE_2D, blend_tex_);

			  glDrawBuffers(used_colortype[FLV_CTYPE_INDEX] ? 2 : 1, draw_buffers);

			  ShaderProgram* img_shader = nullptr;
			  if (used_colortype[FLV_CTYPE_INDEX])
				  img_shader = vr_list_[0]->m_img_shader_factory.shader(IMG_SHDR_BLEND_ID_COLOR_FOR_DEPTH_MODE);
			  else 
				  img_shader = vr_list_[0]->m_img_shader_factory.shader(IMG_SHADER_TEXTURE_LOOKUP);

			  if (img_shader)
			  {
				  if (!img_shader->valid())
				  {
					  img_shader->create();
				  }
				  img_shader->bind();
			  }

			  glActiveTexture(GL_TEXTURE0);
			  glEnable(GL_TEXTURE_2D);
			  glDisable(GL_TEXTURE_3D);

			  //blend
			  glEnablei(GL_BLEND, 0);
			  glBlendEquationi(0, GL_FUNC_ADD);
			  if (TextureRenderer::update_order_ == 0)
				  glBlendFunci(0, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			  else if (TextureRenderer::update_order_ == 1)
				  glBlendFunci(0, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
			  if (used_colortype[FLV_CTYPE_INDEX]) glDisablei(GL_BLEND, 1);
			  //draw
			  vr_list_[0]->draw_view_quad();
			  
			  if (img_shader != nullptr && img_shader->valid())
               img_shader->release();

			  glBindFramebuffer(GL_FRAMEBUFFER, 0);

			  if (use_id_color) glFlush();
		  }//if (blend_slices && colormap_mode_!=FLV_CTYPE_DEPTH)

		  vector<TextureBrick *>::iterator ite = cur_brs.begin();
		  while (ite != cur_brs.end())
		  {
			  if ((order && (*ite)->timax() <= i) || (!order && (*ite)->timin() >= i))
			  {
				  //count up
				  if (updating)
					  (*ite)->set_drawn(mode, true);

				  (*ite)->clear_polygons();
				  TextureRenderer::cur_brick_num_++;
				  finished_brk++;
				  
				  ite = cur_brs.erase(ite);
			  }
			  else ite++;
		  }

	  }//for (i = start_i; order?(i <= all_timax):(i >= all_timin); i += order?1:-1)

	  TextureRenderer::cur_tid_offset_multi_ = i - (order?all_timin:all_timax);
	  
	  if ((order  && TextureRenderer::cur_tid_offset_multi_ > all_timax - all_timin) ||
		  (!order && TextureRenderer::cur_tid_offset_multi_ < all_timin - all_timax) )
	  {
		  TextureRenderer::cur_tid_offset_multi_ = 0;
	  }
	  
	  if (TextureRenderer::get_mem_swap() && remain_brk > 0 && finished_brk == remain_brk)
	  {
		  TextureRenderer::set_clear_chan_buffer(true);
	  }

      if (TextureRenderer::get_mem_swap() &&
            TextureRenderer::get_cur_brick_num() == TextureRenderer::get_total_brick_num())
      {
         TextureRenderer::set_done_update_loop(true);
      }

	  if (used_colortype[FLV_CTYPE_INDEX])
	  {
		  if(glIsTexture(label_tex_id_))
		  {
			  glActiveTexture(GL_TEXTURE5);
			  glBindTexture(GL_TEXTURE_2D, 0);
			  glDisable(GL_TEXTURE_2D);
			  glActiveTexture(GL_TEXTURE0);
		  }

		  if(glIsTexture(blend_id_tex_))
		  {
			  glActiveTexture(GL_TEXTURE6);
			  glBindTexture(GL_TEXTURE_2D, 0);
			  glDisable(GL_TEXTURE_2D);
			  glActiveTexture(GL_TEXTURE0);
		  }
	  }

	  if (!glIsFramebuffer(blend_fbo_))
	  {
		  glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo_);
		  glFramebufferTexture2D(GL_FRAMEBUFFER,
			  GL_COLOR_ATTACHMENT1,
			  GL_TEXTURE_2D, 0, 0);
		  glDrawBuffers(1, draw_buffers);
		  glBindFramebuffer(GL_FRAMEBUFFER, 0);
	  }

	  if (!glIsFramebuffer(blend_framebuffer_))
	  {
		  glBindFramebuffer(GL_FRAMEBUFFER, blend_framebuffer_);
		  glFramebufferTexture2D(GL_FRAMEBUFFER,
			  GL_COLOR_ATTACHMENT1,
			  GL_TEXTURE_2D, 0, 0);
		  glDrawBuffers(1, draw_buffers);
		  glBindFramebuffer(GL_FRAMEBUFFER, 0);
	  }

      //enable depth buffer writing
      glDepthMask(GL_TRUE);

      // Release shader.
	  for (auto &sh : shader)
	  {
		  if (sh != nullptr && sh->valid())
			  sh->release();
	  }

	  glBindFramebuffer(GL_FRAMEBUFFER, 0);

      //release texture
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D, 0);
      glDisable(GL_TEXTURE_3D);

      //reset blending
      glBlendEquation(GL_FUNC_ADD);
      if (TextureRenderer::get_update_order() == 0)
         glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      else if (TextureRenderer::get_update_order() == 1)
         glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
      glDisable(GL_BLEND);

      //output
      if (blend_num_bits_ > 8)
	  {
		  //states
		  GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
		  GLboolean cull_face = glIsEnabled(GL_CULL_FACE);
		  glDisable(GL_DEPTH_TEST);
		  glDisable(GL_CULL_FACE);
		  
		  ShaderProgram* img_shader = 0;

         if (noise_red_ && colormap_mode_!=FLV_CTYPE_DEPTH)
         {
            //FILTERING/////////////////////////////////////////////////////////////////
            if (!glIsTexture(filter_tex_id_))
            {
               glGenTextures(1, &filter_tex_id_);
               glBindTexture(GL_TEXTURE_2D, filter_tex_id_);
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
               glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
                     GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
            }
            if (!glIsFramebuffer(filter_buffer_))
            {
               glGenFramebuffers(1, &filter_buffer_);
               glBindFramebuffer(GL_FRAMEBUFFER, filter_buffer_);
               glFramebufferTexture2D(GL_FRAMEBUFFER,
                     GL_COLOR_ATTACHMENT0,
                     GL_TEXTURE_2D, filter_tex_id_, 0);
            }
            if (filter_buffer_resize_)
            {
               glBindTexture(GL_TEXTURE_2D, filter_tex_id_);
               glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w2, h2, 0,
                     GL_RGBA, GL_FLOAT, NULL);//GL_RGBA16F
               filter_buffer_resize_ = false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, filter_buffer_);

			glBindTexture(GL_TEXTURE_2D, blend_tex_id_);

			img_shader = vr_list_[0]->
				m_img_shader_factory.shader(IMG_SHDR_FILTER_BLUR);
			if (img_shader)
			{
				if (!img_shader->valid())
				{
					img_shader->create();
				}
				img_shader->bind();
			}
			filter_size_min_ = vr_list_[0]->
				CalcFilterSize(4, w, h, res_.x(), res_.y(), zoom, sfactor_);
			img_shader->setLocalParam(0, filter_size_min_/w2, filter_size_min_/h2, 1.0/w2, 1.0/h2);
			vr_list_[0]->draw_view_quad();

			if (img_shader && img_shader->valid())
				img_shader->release();
			///////////////////////////////////////////////////////////////////////////
		 }

		 //go back to normal
		 glBindFramebuffer(GL_FRAMEBUFFER, cur_framebuffer_id);
		 glDrawBuffer(cur_draw_buffer);
		 glReadBuffer(cur_read_buffer);

		 glViewport(vp[0], vp[1], vp[2], vp[3]);

		 if (noise_red_ && colormap_mode_!=2)
			 glBindTexture(GL_TEXTURE_2D, filter_tex_id_);
		 else
			 glBindTexture(GL_TEXTURE_2D, blend_tex_id_);
		 glEnable(GL_BLEND);
		 if (TextureRenderer::get_update_order() == 0)
			 glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		 else if (TextureRenderer::get_update_order() == 1)
			 glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);


		 if (noise_red_ && colormap_mode_!=FLV_CTYPE_DEPTH)
		 {
			 img_shader = vr_list_[0]->
				 m_img_shader_factory.shader(IMG_SHDR_FILTER_SHARPEN);
		 }
		 else
			 img_shader = vr_list_[0]->
			 m_img_shader_factory.shader(IMG_SHADER_TEXTURE_LOOKUP);

		 if (img_shader)
		 {
			 if (!img_shader->valid())
				 img_shader->create();
			 img_shader->bind();
		 }

		 if (noise_red_ && colormap_mode_!=FLV_CTYPE_DEPTH)
		 {
			 filter_size_shp_ = vr_list_[0]->
				 CalcFilterSize(3, w, h, res_.x(), res_.y(), zoom, sfactor_);
			 img_shader->setLocalParam(0, filter_size_shp_/w, filter_size_shp_/h, 0.0, 0.0);
		 }

		 vr_list_[0]->draw_view_quad();

		 if (img_shader && img_shader->valid())
			 img_shader->release();

		 if (depth_test) glEnable(GL_DEPTH_TEST);
		 if (cull_face) glEnable(GL_CULL_FACE);

		 glDisable(GL_BLEND);
	  }

   }

   vector<TextureBrick*> *MultiVolumeRenderer::get_combined_bricks(
         Point& center, Ray& view, bool is_orthographic)
   {
      if (!vr_list_.size())
         return 0;

      if (!vr_list_[0]->tex_->get_sort_bricks())
         return vr_list_[0]->tex_->get_quota_bricks();

      size_t i, j, k;
      vector<TextureBrick*>* bs;
      vector<TextureBrick*>* bs0;
      vector<TextureBrick*>* result;
      Point brick_center;
      double d;

      for (i=0; i<vr_list_.size(); i++)
      {
         //sort each brick list based on distance to center
         bs = vr_list_[i]->tex_->get_bricks();
         for (j=0; j<bs->size(); j++)
         {
            brick_center = (*bs)[j]->bbox().center();
            d = (brick_center - center).length();
            (*bs)[j]->set_d(d);
         }
         std::sort((*bs).begin(), (*bs).end(), TextureBrick::sort_dsc);

         //assign indecis so that bricks can be selected later
         for (j=0; j<bs->size(); j++)
            (*bs)[j]->set_ind(j);
      }

      //generate quota brick list for vr0
      bs0 = vr_list_[0]->tex_->get_bricks();
      result = vr_list_[0]->tex_->get_quota_bricks();
      result->clear();
      int quota = 0;
      int count;
      TextureBrick* pb;
      size_t ind;
      bool found;
      for (i=0; i<vr_list_.size(); i++)
      {
         //insert nonduplicated bricks into result list
         bs = vr_list_[i]->tex_->get_bricks();
         quota = vr_list_[i]->get_quota_bricks_chan();
         //quota = quota/2+1;
         count = 0;
         for (j=0; j<bs->size(); j++)
         {
            pb = (*bs)[j];
            if (pb->get_priority()>0)
               continue;
            ind = pb->get_ind();
            found = false;
            for (k=0; k<result->size(); k++)
            {
               if (ind == (*result)[k]->get_ind())
               {
                  found = true;
                  break;
               }
            }
            if (!found)
            {
               result->push_back((*bs0)[ind]);
               count++;
               if (count == quota)
                  break;
            }
         }
      }
      //reorder result
      for (i = 0; i < result->size(); i++)
      {
         Point minp((*result)[i]->bbox().min());
         Point maxp((*result)[i]->bbox().max());
         Vector diag((*result)[i]->bbox().diagonal());
         minp += diag / 1000.;
         maxp -= diag / 1000.;
         Point corner[8];
         corner[0] = minp;
         corner[1] = Point(minp.x(), minp.y(), maxp.z());
         corner[2] = Point(minp.x(), maxp.y(), minp.z());
         corner[3] = Point(minp.x(), maxp.y(), maxp.z());
         corner[4] = Point(maxp.x(), minp.y(), minp.z());
         corner[5] = Point(maxp.x(), minp.y(), maxp.z());
         corner[6] = Point(maxp.x(), maxp.y(), minp.z());
         corner[7] = maxp;
         double d = 0.0;
         for (unsigned int c = 0; c < 8; c++)
         {
            double dd;
            if (is_orthographic)
            {
               // orthographic: sort bricks based on distance to the view plane
               dd = Dot(corner[c], view.direction());
            }
            else
            {
               // perspective: sort bricks based on distance to the eye point
               dd = (corner[c] - view.origin()).length();
            }
            if (c == 0 || dd < d) d = dd;
         }
         (*result)[i]->set_d(d);
      }
      if (TextureRenderer::get_update_order() == 0)
         std::sort((*result).begin(), (*result).end(), TextureBrick::sort_asc);
      else if (TextureRenderer::get_update_order() == 1)
         std::sort((*result).begin(), (*result).end(), TextureBrick::sort_dsc);
      vr_list_[0]->tex_->reset_sort_bricks();

      //duplicate result into other quota-bricks
      for (i=1; i<vr_list_.size(); i++)
      {
         bs0 = vr_list_[i]->tex_->get_bricks();
         bs = vr_list_[i]->tex_->get_quota_bricks();
         bs->clear();

         for (j=0; j<result->size(); j++)
         {
            ind = (*result)[j]->get_ind();
            bs->push_back((*bs0)[ind]);
         }
         vr_list_[i]->tex_->reset_sort_bricks();
      }

      return result;
   }

   void MultiVolumeRenderer::draw_wireframe(bool orthographic_p)
   {
      if (get_vr_num()<=0)
         return;

      Ray view_ray = vr_list_[0]->compute_view();

      // Set sampling rate based on interaction.
      double rate = imode_ ? irate_ : sampling_rate_;
      Vector diag = bbox_.diagonal();
      Vector cell_diag(diag.x()/res_.x(),
            diag.y()/res_.y(),
            diag.z()/res_.z());
      double dt = cell_diag.length()/
         vr_list_[0]->compute_rate_scale()/rate;
      num_slices_ = (int)(diag.length()/dt);

      vector<double> vertex;
      vector<double> texcoord;
      vector<int> size;
      vertex.reserve(num_slices_*6);
      texcoord.reserve(num_slices_*6);
      size.reserve(num_slices_*6);

      //--------------------------------------------------------------------------
      // render bricks
      // Set up transform
      Transform *tform = vr_list_[0]->tex_->transform();
      double mvmat[16];
      tform->get_trans(mvmat);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixd(mvmat);

      glEnable(GL_DEPTH_TEST);
      GLboolean lighting = glIsEnabled(GL_LIGHTING);
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_3D);
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_FOG);

      vector<TextureBrick*> *bs = vr_list_[0]->tex_->get_sorted_bricks(view_ray, orthographic_p);

      if (bs)
      {
         for (unsigned int i=0; i < bs->size(); i++)
         {
            glColor4d(0.8*(i+1.0)/bs->size(), 0.8*(i+1.0)/bs->size(), 0.8, 1.0);

            TextureBrick* b = (*bs)[i];
            if (!vr_list_[0]->test_against_view(b->bbox())) continue; // Clip against view.

            Point ptmin = b->bbox().min();
            Point ptmax = b->bbox().max();
            Point &pmin(ptmin);
            Point &pmax(ptmax);
            Point corner[8];
            corner[0] = pmin;
            corner[1] = Point(pmin.x(), pmin.y(), pmax.z());
            corner[2] = Point(pmin.x(), pmax.y(), pmin.z());
            corner[3] = Point(pmin.x(), pmax.y(), pmax.z());
            corner[4] = Point(pmax.x(), pmin.y(), pmin.z());
            corner[5] = Point(pmax.x(), pmin.y(), pmax.z());
            corner[6] = Point(pmax.x(), pmax.y(), pmin.z());
            corner[7] = pmax;

            glBegin(GL_LINES);
            {
               for(int i=0; i<4; i++) {
                  glVertex3d(corner[i].x(), corner[i].y(), corner[i].z());
                  glVertex3d(corner[i+4].x(), corner[i+4].y(), corner[i+4].z());
               }
            }
            glEnd();
            glBegin(GL_LINE_LOOP);
            {
               glVertex3d(corner[0].x(), corner[0].y(), corner[0].z());
               glVertex3d(corner[1].x(), corner[1].y(), corner[1].z());
               glVertex3d(corner[3].x(), corner[3].y(), corner[3].z());
               glVertex3d(corner[2].x(), corner[2].y(), corner[2].z());
            }
            glEnd();
            glBegin(GL_LINE_LOOP);
            {
               glVertex3d(corner[4].x(), corner[4].y(), corner[4].z());
               glVertex3d(corner[5].x(), corner[5].y(), corner[5].z());
               glVertex3d(corner[7].x(), corner[7].y(), corner[7].z());
               glVertex3d(corner[6].x(), corner[6].y(), corner[6].z());
            }
            glEnd();

            glColor4d(0.4, 0.4, 0.4, 1.0);

            vertex.clear();
            texcoord.clear();
            size.clear();

            // Scale out dt such that the slices are artificially further apart.
            b->compute_polygons(view_ray, dt * 10, vertex, texcoord, size);
            vr_list_[0]->draw_polygons_wireframe(vertex, texcoord, size, false);
         }
      }

      if(lighting) glEnable(GL_LIGHTING);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
   }

   double MultiVolumeRenderer::num_slices_to_rate(int num_slices)
   {
      if (!bbox_.valid())
         return 1.0;
      Vector diag = bbox_.diagonal();
      Vector cell_diag(diag.x()/*/tex_->nx()*/,
            diag.y()/*/tex_->ny()*/,
            diag.z()/*/tex_->nz()*/);
      double dt = diag.length() / num_slices;
      double rate = cell_diag.length() / dt;

      return rate;
   }

   void MultiVolumeRenderer::resize()
   {
      blend_framebuffer_resize_ = true;
	  blend_fbo_resize_ = true;
   }

} // namespace FLIVR
