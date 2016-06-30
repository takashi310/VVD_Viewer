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

#include <FLIVR/ShaderProgram.h>
#include <FLIVR/Texture.h>
#include <FLIVR/TextureRenderer.h>
#include <FLIVR/Utils.h>
#include <algorithm>
#include <inttypes.h>

#include <wx/progdlg.h>

using namespace std;

namespace FLIVR
{
	size_t Texture::mask_undo_num_ = 0;
	Texture::Texture() :
        sort_bricks_(true),
        nx_(0),
		ny_(0),
		nz_(0),
        nc_(0),
        nmask_(-1),
        nlabel_(-1),
		vmin_(0.0),
		vmax_(0.0),
		gmin_(0.0),
		gmax_(0.0),
		use_priority_(false),
		n_p0_(0),
		filetype_(BRICK_FILE_TYPE_NONE),
		brkxml_(false),
		useURL_(false),
		pyramid_lv_num_(0),
		pyramid_cur_lv_(0),
		pyramid_cur_fr_(0),
		pyramid_cur_ch_(0),
		pyramid_copy_lv_(-1),
		b_spcx_(1.0),
		b_spcy_(1.0),
		b_spcz_(1.0),
		s_spcx_(1.0),
		s_spcy_(1.0),
		s_spcz_(1.0),
        mask_undo_pointer_(-1)
	{
		for (size_t i = 0; i < TEXTURE_MAX_COMPONENTS; i++)
		{
			nb_[i] = 0;
			data_[i] = 0;
			ntype_[i] = TYPE_NONE;
		}

		bricks_ = &default_vec_;
	}

	Texture::~Texture()
	{
		if(bricks_){
			for (int i=0; i<(int)(*bricks_).size(); i++)
			{
				if ((*bricks_)[i])
					delete (*bricks_)[i];
			}
			vector<TextureBrick *>().swap(*bricks_);
		}

		clear_undos();

		//release other data
		for (int i=0; i<TEXTURE_MAX_COMPONENTS; i++)
		{
			if (data_[i])
			{
				bool existInPyramid = false;
				for (int j = 0; j < pyramid_.size(); j++)
					if (pyramid_[j].data == data_[i]) existInPyramid = true;

				//delete [] data_[i]->data;
				if (!existInPyramid)
				{
					if (ntype_[i]!=TYPE_MASK) delete [] data_[i]->data;
					nrrdNix(data_[i]);
				}
			}
		}

		//if (ifs_) ifs_.close();

		clearPyramid();
	}

	void Texture::clear_undos()
	{
		//mask data now managed by the undos
		for (size_t i=0; i<mask_undos_.size(); ++i)
		{
			if (mask_undos_[i])
				delete []mask_undos_[i];
			mask_undos_.clear();
			mask_undo_pointer_ = -1;
		}
	}

	int Texture::get_brick_id_point(int ix, int iy, int iz)
	{
		for (unsigned int i = 0; i < (*bricks_).size(); i++)
		{
			int ox = (*bricks_)[i]->ox();
			int oy = (*bricks_)[i]->oy();
			int oz = (*bricks_)[i]->oz();
			int ex = ox + (*bricks_)[i]->nx();
			int ey = oy + (*bricks_)[i]->ny();
			int ez = oz + (*bricks_)[i]->nz();
			
			if (ix >= ox && iy >= oy && iz >= oz &&
				ix < ex && iy < ey && iz < ez)
				return i;
		}

		return -1;
	}

	double Texture::get_brick_original_value(int brick_id, int i, int j, int k, bool normalize)
	{
		if (brick_id < 0 || brick_id >= (*bricks_).size())
			return 0.0;

		TextureBrick *b = (*bricks_)[brick_id];

		Nrrd* data = get_nrrd(0);
		if (!data) return 0.0;
		int bits = data->type;
		int64_t nx = (int64_t)(b->nx());
		int64_t ny = (int64_t)(b->ny());
		int64_t nz = (int64_t)(b->nz());
		int64_t ox = (int64_t)(b->ox());
		int64_t oy = (int64_t)(b->oy());
		int64_t oz = (int64_t)(b->oz());

		if (i<0 || i>=nx || j<0 || j>=ny || k<0 || k>=nz)
		return 0.0;
		uint64_t ii = i, jj = j, kk = k;

		uint64_t index = 0;
		bool tmp_load = false;
		void *d_ptr = NULL;
		if (isBrxml()) 
		{
			index = (nx)*(ny)*(kk) + (nx)*(jj) + (ii);
			tmp_load = !b->isLoaded();
			FileLocInfo *finfo = GetFileName(b->getID());
			d_ptr = b->tex_data_brk(0, finfo);
		}
		else
		{
			index = (nx)*(ny)*(oz+kk) + (nx)*(oy+jj) + (ox+ii);
			d_ptr = data->data;
		}

		if (!d_ptr)
			return 0.0;

		double rval = 0.0;
		if (bits == nrrdTypeUChar)
		{
			unsigned char old_value = ((unsigned char*)(d_ptr))[index];
			rval =  normalize ? double(old_value)/255.0 : double(old_value);
		}
		else if (bits == nrrdTypeUShort)
		{
			unsigned short old_value = ((unsigned short*)(d_ptr))[index];
			rval = normalize ? double(old_value)/65535.0 : double(old_value);
		}
		
		if (tmp_load)
			b->freeBrkData();

		return rval;
	}

	double Texture::get_brick_original_value(int i, int j, int k, bool normalize)
	{
		int bid = get_brick_id_point(i, j, k);
		if (bid < 0 || bid >= get_brick_num())
			return 0.0;

		TextureBrick *b = (*bricks_)[bid];
		int ii = i - b->ox();
		int jj = j - b->oy();
		int kk = k - b->oz();
		
		return get_brick_original_value(bid, ii, jj, kk, normalize);
	}

	vector<TextureBrick*>* Texture::get_sorted_bricks(
		Ray& view, bool is_orthographic)
	{
		if (sort_bricks_)
		{
			for (unsigned int i = 0; i < (*bricks_).size(); i++)
			{
				Point minp((*bricks_)[i]->bbox().min());
				Point maxp((*bricks_)[i]->bbox().max());
				Vector diag((*bricks_)[i]->bbox().diagonal());
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
				(*bricks_)[i]->set_d(d);
			}
			if (TextureRenderer::get_update_order() == 0)
				std::sort((*bricks_).begin(), (*bricks_).end(), TextureBrick::sort_asc);
			else if (TextureRenderer::get_update_order() == 1)
				std::sort((*bricks_).begin(), (*bricks_).end(), TextureBrick::sort_dsc);

			sort_bricks_ = false;
		}
		return bricks_;
	}

	vector<TextureBrick*>* Texture::get_closest_bricks(
		Point& center, int quota, bool skip,
		Ray& view, bool is_orthographic)
	{
		if (sort_bricks_)
		{
			quota_bricks_.clear();
			unsigned int i;
			if (quota >= (int64_t)(*bricks_).size())
				quota = int((*bricks_).size());
			else
			{
				for (i=0; i<(*bricks_).size(); i++)
				{
					Point brick_center = (*bricks_)[i]->bbox().center();
					double d = (brick_center - center).length();
					(*bricks_)[i]->set_d(d);
				}
				std::sort((*bricks_).begin(), (*bricks_).end(), TextureBrick::sort_dsc);
			}

			for (i=0; i<(unsigned int)(*bricks_).size(); i++)
			{
				if (skip)
				{
					if ((*bricks_)[i]->get_priority() == 0)
						quota_bricks_.push_back((*bricks_)[i]);
				}
				else
					quota_bricks_.push_back((*bricks_)[i]);
				if (quota_bricks_.size() == (size_t)quota)
					break;
			}

			for (i = 0; i < quota_bricks_.size(); i++)
			{
				Point minp(quota_bricks_[i]->bbox().min());
				Point maxp(quota_bricks_[i]->bbox().max());
				Vector diag(quota_bricks_[i]->bbox().diagonal());
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
				quota_bricks_[i]->set_d(d);
			}
			if (TextureRenderer::get_update_order() == 0)
				std::sort(quota_bricks_.begin(), quota_bricks_.end(), TextureBrick::sort_asc);
			else if (TextureRenderer::get_update_order() == 1)
				std::sort(quota_bricks_.begin(), quota_bricks_.end(), TextureBrick::sort_dsc);

			sort_bricks_ = false;
		}

		return &quota_bricks_;
	}

	vector<TextureBrick*>* Texture::get_bricks()
	{
		return bricks_;
	}

	vector<TextureBrick*>* Texture::get_quota_bricks()
	{
		return &quota_bricks_;
	}

	//brxmlÇÃÇ∆Ç´ÇÕç≈â∫ëwÇÃspacingÇê›íËÇ∑ÇÈÅiëºÇÃäKëwÇÕé©ìÆìIÇ…åvéZÅj
	void Texture::set_spacings(double x, double y, double z)
	{
		if (!brkxml_)
		{
			spcx_ = x;
			spcy_ = y;
			spcz_ = z;
			Transform tform;
			tform.load_identity();
			Point nmax(nx_*x, ny_*y, nz_*z);
			tform.pre_scale(Vector(nmax));
			set_transform(tform);
		}
		else
		{
			s_spcx_ = x / b_spcx_;
			s_spcy_ = y / b_spcy_;
			s_spcz_ = z / b_spcz_;
			Transform tform;
			tform.load_identity();
			Point nmax(nx_*spcx_*s_spcx_, ny_*spcy_*s_spcy_, nz_*spcz_*s_spcz_);
			tform.pre_scale(Vector(nmax));
			set_transform(tform);
		}
	}

	void Texture::set_base_spacings(double x, double y, double z)
	{
		spcx_ = x;
		spcy_ = y;
		spcz_ = z;
		b_spcx_ = x;
		b_spcy_ = y;
		b_spcz_ = z;
		Transform tform;
		tform.load_identity();
		Point nmax(nx_*x, ny_*y, nz_*z);
		tform.pre_scale(Vector(nmax));
		set_transform(tform);
	}

	void Texture::get_base_spacings(double &x, double &y, double &z)
	{
		x = b_spcx_;
		y = b_spcy_;
		z = b_spcz_;
	}

	void Texture::get_spacings(double &x, double &y, double &z, int lv)
	{
		if (!brkxml_)
		{
			x = spcx_;
			y = spcy_;
			z = spcz_;
		}
		else if (lv < 0 || lv >= pyramid_lv_num_ || pyramid_.empty())
		{
			x = spcx_ * s_spcx_;
			y = spcy_ * s_spcy_;
			z = spcz_ * s_spcz_;
		}
		else if(pyramid_[lv].data)
		{
			int offset = 0;
			if (pyramid_[lv].data->dim > 3) offset = 1; 
			x = pyramid_[lv].data->axis[offset + 0].spacing * s_spcx_;
			y = pyramid_[lv].data->axis[offset + 1].spacing * s_spcy_;
			z = pyramid_[lv].data->axis[offset + 2].spacing * s_spcz_;
		}
	}

	void Texture::set_spacing_scales(double x, double y, double z)
	{
		if (x > 0.0) s_spcx_ = x;
		if (y > 0.0) s_spcy_ = y;
		if (z > 0.0) s_spcz_ = z;
	}
	
	void Texture::get_spacing_scales(double &x, double &y, double &z)
	{
		x = s_spcx_;
		y = s_spcy_;
		z = s_spcz_;
	}

	bool Texture::build(Nrrd* nv_nrrd, Nrrd* gm_nrrd,
		double vmn, double vmx,
		double gmn, double gmx,
		vector<FLIVR::TextureBrick*>* brks)
	{

		/*
		size_t axis_size[4];
		nrrdAxisInfoGet_nva(nv_nrrd, nrrdAxisInfoSize, axis_size);
		double axis_min[4];
		nrrdAxisInfoGet_nva(nv_nrrd, nrrdAxisInfoMin, axis_min);
		double axis_max[4];
		nrrdAxisInfoGet_nva(nv_nrrd, nrrdAxisInfoMax, axis_max);
		*/
		int numc = gm_nrrd ? 2 : 1;
		int numb[2];
		if (nv_nrrd)
		{
			if (nv_nrrd->type == nrrdTypeChar ||
				nv_nrrd->type == nrrdTypeUChar)
				numb[0] = 1;
			else
				numb[0] = 2;
		}
		else
			numb[0] = 0;
		numb[1] = gm_nrrd ? 1 : 0;

		BBox bb(Point(0,0,0), Point(1,1,1)); 
		
		size_t dim = nv_nrrd->dim;
		std::vector<int> size(dim);

		int offset = 0;
		if (dim > 3) offset = 1; 

		Transform tform;
		tform.load_identity();

		for (size_t p = 0; p < dim; p++) 
			size[p] = (int)nv_nrrd->axis[p + offset].size;

		if(!brks)
		{
			//when all of them equal to old values, the result should be the same as an old one.  
			if (size[0] != nx() || size[1] != ny() || size[2] != nz() ||
				numc != nc() || numb[0] != nb(0) ||
				bb.min() != bbox()->min() ||
				bb.max() != bbox()->max() ||
				vmn != vmin() ||
				vmx != vmax() ||
				gmn != gmin() ||
				gmx != gmax() )
			{
				clearPyramid();
				bricks_ = &default_vec_;
				build_bricks((*bricks_), size[0], size[1], size[2], numc, numb);
				set_size(size[0], size[1], size[2], numc, numb);
			}
			brkxml_ = false;
		}
		else
		{
			bricks_ = brks;
			set_size(size[0], size[1], size[2], numc, numb);
		}

		set_bbox(bb);
		set_minmax(vmn, vmx, gmn, gmx);
		set_transform(tform);

		n_p0_ = 0;
		if(!brks)
		{
			for (unsigned int i = 0; i < (*bricks_).size(); i++)
			{
				TextureBrick *tb = (*bricks_)[i];
				tb->set_nrrd(nv_nrrd, 0);
				tb->set_nrrd(gm_nrrd, 1);
				//			if (use_priority_)
				//			{
				//				if (!brkxml_) tb->set_priority();
				//				else tb->set_priority_brk(&ifs_, filetype_);
				//				n_p0_ += tb->get_priority()==0?1:0;
				//			}
			}
		}

		BBox tempb;
		tempb.extend(transform_.project(bbox_.min()));
		tempb.extend(transform_.project(bbox_.max()));
		spcx_ = (tempb.max().x() - tempb.min().x()) / double(nx_);
		spcy_ = (tempb.max().y() - tempb.min().y()) / double(ny_);
		spcz_ = (tempb.max().z() - tempb.min().z()) / double(nz_);

		set_nrrd(nv_nrrd, 0);
		set_nrrd(gm_nrrd, 1);

		return true;
	}

	void Texture::build_bricks(vector<TextureBrick*> &bricks, 
		int sz_x, int sz_y, int sz_z,
		int numc, int* numb)
	{
		bool force_pow2 = false;
		if (ShaderProgram::init())
			force_pow2 = !ShaderProgram::texture_non_power_of_two();

		int max_texture_size = 2048;
		if (ShaderProgram::init())
			max_texture_size = ShaderProgram::max_texture_size();

		//further determine the max texture size
		if (TextureRenderer::get_mem_swap())
		{
			double data_size = double(sz_x)*double(sz_y)*double(sz_z)*double(numb[0])/1.04e6;
			if (data_size > TextureRenderer::get_mem_limit() ||
				data_size > TextureRenderer::get_large_data_size())
				max_texture_size = TextureRenderer::get_force_brick_size();
		}

		// Initial brick size
		int bsize[3];
/*
		if (force_pow2)
		{
			if (Pow2(sz_x) > (unsigned)sz_x) 
				bsize[0] = Min(int(Pow2(sz_x))/2, max_texture_size);
			if (Pow2(sz_y) > (unsigned)sz_y) 
				bsize[1] = Min(int(Pow2(sz_x))/2, max_texture_size);
			if (Pow2(sz_z) > (unsigned)sz_z) 
				bsize[2] = Min(int(Pow2(sz_x))/2, max_texture_size);
		}
		else
		{
*/			bsize[0] = Min(int(Pow2(sz_x)), max_texture_size);
			bsize[1] = Min(int(Pow2(sz_y)), max_texture_size);
			bsize[2] = Min(int(Pow2(sz_z)), max_texture_size);
//		}

		//old code:  memory leak?
		//bricks.clear();

		//new code
		for(int i = 0; i < bricks.size(); i++)
		{
			bricks[i]->freeBrkData(); 
			delete bricks[i];
		}
		vector<TextureBrick *>().swap(bricks);
		//new code

		int i, j, k;
		int mx, my, mz, mx2, my2, mz2, ox, oy, oz;
		double tx0, ty0, tz0, tx1, ty1, tz1;
		double bx1, by1, bz1;
		double dx0, dy0, dz0, dx1, dy1, dz1;
		for (k = 0; k < sz_z; k += bsize[2])
		{
			if (k) k--;
			for (j = 0; j < sz_y; j += bsize[1])
			{
				if (j) j--;
				for (i = 0; i < sz_x; i += bsize[0])
				{
					if (i) i--;
					mx = Min(bsize[0], sz_x - i);
					my = Min(bsize[1], sz_y - j);
					mz = Min(bsize[2], sz_z - k);

					mx2 = mx;
					my2 = my;
					mz2 = mz;
					if (force_pow2)
					{
						mx2 = Pow2(mx);
						my2 = Pow2(my);
						mz2 = Pow2(mz);
					}

					// Compute Texture Box.
					tx0 = i?((mx2 - mx + 0.5) / mx2): 0.0;
					ty0 = j?((my2 - my + 0.5) / my2): 0.0;
					tz0 = k?((mz2 - mz + 0.5) / mz2): 0.0;

					tx1 = 1.0 - 0.5 / mx2;
					if (mx < bsize[0]) tx1 = 1.0;
					if (sz_x - i == bsize[0]) tx1 = 1.0;

					ty1 = 1.0 - 0.5 / my2;
					if (my < bsize[1]) ty1 = 1.0;
					if (sz_y - j == bsize[1]) ty1 = 1.0;

					tz1 = 1.0 - 0.5 / mz2;
					if (mz < bsize[2]) tz1 = 1.0;
					if (sz_z - k == bsize[2]) tz1 = 1.0;

					BBox tbox(Point(tx0, ty0, tz0), Point(tx1, ty1, tz1));

					// Compute BBox.
					bx1 = Min((i + bsize[0] - 0.5) / (double)sz_x, 1.0);
					if (sz_x - i == bsize[0]) bx1 = 1.0;

					by1 = Min((j + bsize[1] - 0.5) / (double)sz_y, 1.0);
					if (sz_y - j == bsize[1]) by1 = 1.0;

					bz1 = Min((k + bsize[2] - 0.5) / (double)sz_z, 1.0);
					if (sz_z - k == bsize[2]) bz1 = 1.0;

					BBox bbox(Point(i==0?0:(i+0.5) / (double)sz_x,
						j==0?0:(j+0.5) / (double)sz_y,
						k==0?0:(k+0.5) / (double)sz_z),
						Point(bx1, by1, bz1));

					ox = i-(mx2-mx);
					oy = j-(my2-my);
					oz = k-(mz2-mz);

					dx0 = (double)ox / sz_x;
					dy0 = (double)oy / sz_y;
					dz0 = (double)oz / sz_z;
					dx1 = (double)(ox + mx2) / sz_x;
					dy1 = (double)(oy + my2) / sz_y;
					dz1 = (double)(oz + mz2) / sz_z;

					BBox dbox(Point(dx0, dy0, dz0), Point(dx1, dy1, dz1));

					TextureBrick *b = new TextureBrick(0, 0, mx2, my2, mz2, numc, numb, 
						ox, oy, oz, mx2, my2, mz2, bbox, tbox, dbox);
					bricks.push_back(b);
				}
			}
		}
	}

	//add one more texture component as the volume mask
	bool Texture::add_empty_mask()
	{
		if (nc_>0 && nc_<=2 && nmask_==-1)
		{
			//fix to texture2
			nmask_ = 2;
			nb_[nmask_] = 1;
			ntype_[nmask_] = TYPE_MASK;

			int i;
			for (i=0; i<(int)(*bricks_).size(); i++)
			{
				(*bricks_)[i]->nmask(nmask_);
				(*bricks_)[i]->nb(1, nmask_);
				(*bricks_)[i]->ntype(TextureBrick::TYPE_MASK, nmask_);
			}
			return true;
		}
		else
			return false;
	}

	//add one more texture component as the labeling volume
	bool Texture::add_empty_label()
	{
		if (nc_>0 && nc_<=2 && nlabel_==-1)
		{
			if (nmask_==-1)	//no mask
				nlabel_ = 3;
			else			//label is after mask
				nlabel_ = nmask_+1;
			nb_[nlabel_] = 4;

			int i;
			for (i=0; i<(int)(*bricks_).size(); i++)
			{
				(*bricks_)[i]->nlabel(nlabel_);
				(*bricks_)[i]->nb(4, nlabel_);
				(*bricks_)[i]->ntype(TextureBrick::TYPE_LABEL, nlabel_);
			}
			return true;
		}
		else
			return false;
	}

	void Texture::delete_mask()
	{
		if (nmask_ > 0)
		{
			nb_[nmask_] = 0;

			int i;
			for (i=0; i<(int)(*bricks_).size(); i++)
			{
				(*bricks_)[i]->nmask(-1);
				(*bricks_)[i]->nb(0, nmask_);
				(*bricks_)[i]->ntype(TextureBrick::TYPE_NONE, nmask_);
			}

			if (data_[nmask_])
			{
				delete [] data_[nmask_]->data;
				data_[nmask_] = NULL;
			}

			nmask_ = -1; 
		}
	}

	void Texture::delete_label()
	{
		if (nlabel_ > 0)
		{
			nb_[nlabel_] = 0;

			int i;
			for (i=0; i<(int)(*bricks_).size(); i++)
			{
				(*bricks_)[i]->nlabel(-1);
				(*bricks_)[i]->nb(0, nlabel_);
				(*bricks_)[i]->ntype(TextureBrick::TYPE_NONE, nlabel_);
			}

			if (data_[nlabel_])
			{
				delete [] data_[nlabel_]->data;
				data_[nlabel_] = NULL;
			}

			nlabel_ = -1; 
		}
	}

	//set nrrd
	void Texture::set_nrrd(Nrrd* data, int index)
	{
		if (index>=0&&index<TEXTURE_MAX_COMPONENTS)
		{
			bool existInPyramid = false;
			for (int i = 0; i < pyramid_.size(); i++)
				if (pyramid_[i].data == data) existInPyramid = true;
			
			if (data_[index] && data && !existInPyramid)
			{
				if (index != nmask_)
					delete [] data_[index]->data;
				nrrdNix(data_[index]);
			}

			data_[index] = data;
			if (!existInPyramid)
			{
				for (int i=0; i<(int)(*bricks_).size(); i++)
				{
					(*bricks_)[i]->set_nrrd(data, index);
				}
				//add to undo list
				if (index==nmask_)
					set_mask(data->data);
			}
		}
	}

	void Texture::set_data_file(vector<FileLocInfo *> *fname, int type)
	{
		filename_ = fname;
		filetype_ = type; 
	}

	FileLocInfo *Texture::GetFileName(int id)
	{
		if (id < 0 || id >= filename_->size()) return NULL;
		return (*filename_)[id];
	}

	void Texture::set_FrameAndChannel(int fr, int ch)
	{
		if (!brkxml_) return;

		pyramid_cur_fr_ = fr;
		pyramid_cur_ch_ = ch;

		int lv = pyramid_cur_lv_;

		for (int i = 0; i < pyramid_.size(); i++)
		{
			if (fr < 0 || fr >= filenames_[i].size()) continue;
			if (ch < 0 || ch >= filenames_[i][fr].size()) continue;
			pyramid_[i].filenames = &filenames_[i][fr][ch];
		}
		if (lv == pyramid_cur_lv_) set_data_file(pyramid_[lv].filenames, pyramid_[lv].filetype);

	}

	void Texture::setLevel(int lv)
	{
		if (lv < 0 || lv >= pyramid_lv_num_ || !brkxml_ || pyramid_cur_lv_ == lv) return;
		pyramid_cur_lv_ = lv;
		build(pyramid_[pyramid_cur_lv_].data, 0, 0, 256, 0, 0, &pyramid_[pyramid_cur_lv_].bricks);
		set_data_file(pyramid_[pyramid_cur_lv_].filenames, pyramid_[pyramid_cur_lv_].filetype);
		
		int offset = 0;
		if (pyramid_[lv].data->dim > 3) offset = 1; 
		spcx_ = pyramid_[lv].data->axis[offset + 0].spacing;
		spcy_ = pyramid_[lv].data->axis[offset + 1].spacing;
		spcz_ = pyramid_[lv].data->axis[offset + 2].spacing;
		Transform tform;
		tform.load_identity();
		Point nmax(nx_*spcx_*s_spcx_, ny_*spcy_*s_spcy_, nz_*spcz_*s_spcz_);
		tform.pre_scale(Vector(nmax));
		set_transform(tform);
	}

	bool Texture::buildPyramid(vector<Pyramid_Level> &pyramid, vector<vector<vector<vector<FileLocInfo *>>>> &filenames, bool useURL)
	{
		if (pyramid.empty()) return false;
		if (pyramid.size() != filenames.size()) return false;

		brkxml_ = true;
		useURL_ = useURL;

		clearPyramid();

		pyramid_lv_num_ = pyramid.size();
		pyramid_ = pyramid;
		filenames_ = filenames;
		for (int i = 0; i < pyramid_.size(); i++)
		{
			if(!pyramid_[i].data || pyramid_[i].bricks.empty())
			{
				clearPyramid();
				return false;
			}
			
			for (int j = 0; j < pyramid_[i].bricks.size(); j++)
			{
				pyramid_[i].bricks[j]->set_nrrd(pyramid_[i].data, 0);
				pyramid_[i].bricks[j]->set_nrrd(0, 1);
			}
		}
		setLevel(pyramid_lv_num_ - 1);
		pyramid_cur_lv_ = -1;

		return true;
	}

	void Texture::clearPyramid()
	{
		if (!brkxml_) return;

		if (pyramid_.empty()) return;

		for (int i=0; i<(int)pyramid_.size(); i++)
		{
			for (int j=0; j<(int)pyramid_[i].bricks.size(); j++)
			{
				if (pyramid_[i].bricks[j]) delete pyramid_[i].bricks[j];
			}
			vector<TextureBrick *>().swap(pyramid_[i].bricks);
			nrrdNix(pyramid_[i].data);
		}
		vector<Pyramid_Level>().swap(pyramid_);

		for (int i=0; i<(int)filenames_.size(); i++)
			for (int j=0; j<(int)filenames_[i].size(); j++)
				for (int k=0; k<(int)filenames_[i][j].size(); k++)
					for (int n=0; n<(int)filenames_[i][j][k].size(); n++)
						if (filenames_[i][j][k][n]) delete filenames_[i][j][k][n];
		
		vector<vector<vector<vector<FileLocInfo *>>>>().swap(filenames_);

		pyramid_lv_num_ = 0;
		pyramid_cur_lv_ = -1;
	}

	Nrrd *Texture::loadData(int &lv)
	{
		if (!brkxml_) return NULL;

		int level = lv;
		if (level < 0 || level > pyramid_.size()) level = pyramid_copy_lv_;
		if (level < 0 || level > pyramid_.size()) level = pyramid_cur_lv_;
		if (pyramid_cur_fr_ < 0 || pyramid_cur_fr_ >= filenames_[level].size()) return NULL;
		if (pyramid_cur_ch_ < 0 || pyramid_cur_ch_ >= filenames_[level][pyramid_cur_fr_].size()) return NULL;
		
		vector<TextureBrick*> *bricks = &pyramid_[level].bricks;
		
		int bnum = bricks->size();
		if (bnum <= 0) return NULL;

		Nrrd *data;
		data = nrrdNew();
		nrrdCopy(data, pyramid_[level].data);
		
		int nbyte = 0;
		if (data->type == nrrdTypeChar || data->type == nrrdTypeUChar) nbyte = 1;
		else nbyte = 2;

		size_t dim = data->dim;
		std::vector<int> size(dim);

		int offset = 0;
		if (dim > 3) offset = 1; 

		for (size_t p = 0; p < dim; p++) 
			size[p] = (int)data->axis[p + offset].size;

		int width  = size[0];
		int height = size[1];
		int depth  = size[2];

		data->data = malloc(width * height * depth * nbyte);
		memset(data->data, 0, width * height * depth * nbyte);

		wxProgressDialog *prog_diag = new wxProgressDialog(
			"FluoRender: Load Volume Data...",
			"Loading... Please wait.",
			100, 0,
			wxPD_SMOOTH|wxPD_ELAPSED_TIME|wxPD_AUTO_HIDE);
		int progress = 0;

		for(int i = 0; i < bnum; i++)
		{
			bool tmp = false;
			if(!(*bricks)[i]->isLoaded())
			{
				FileLocInfo *finfo = filenames_[level][pyramid_cur_fr_][pyramid_cur_ch_][(*bricks)[i]->getID()];
				(*bricks)[i]->tex_data_brk(0, finfo);
				tmp = true;
			}

			//naive
			int ox = (*bricks)[i]->ox();
			int oy = (*bricks)[i]->oy();
			int oz = (*bricks)[i]->oz();
			int nx = (*bricks)[i]->nx();
			int ny = (*bricks)[i]->ny();
			int nz = (*bricks)[i]->nz();

			unsigned char *ptr_dst = (unsigned char *)(data->data);
			long long offset = (long long)(oz) * (long long)(width) * (long long)(height) +
							   (long long)(oy) * (long long)(width) +
							   (long long)(ox);

			ptr_dst += offset * (long long)nbyte;
			
			const unsigned char *ptr_src = (const unsigned char *)(*bricks)[i]->getBrickData();
			
			for (int z = 0; z < nz; z++)
			{
				for (int y = 0; y < ny; y++)
				{
					memcpy(ptr_dst, ptr_src, nx*nbyte);
					ptr_dst += (long long)(width) * (long long)nbyte;
					ptr_src += (long long)(nx) * (long long)nbyte;
				}
				ptr_dst += (long long)(width) * (long long)(height-ny) * (long long)nbyte;
			}

			if (tmp) (*bricks)[i]->freeBrkData();

			if (prog_diag)
			{
				progress++;
				prog_diag->Update(100*progress/bnum);
			}
		}

		delete prog_diag;

		lv = level;

		return data;
	}

	//mask undo management
	bool Texture::trim_mask_undos_head()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return true;
		if (mask_undos_.size() <= mask_undo_num_+1)
			return true;
		if (mask_undo_pointer_ == 0)
			return false;
		while (mask_undos_.size()>mask_undo_num_+1 &&
			mask_undo_pointer_>0 &&
			mask_undo_pointer_<mask_undos_.size())
		{
			delete []mask_undos_.front();
			mask_undos_.erase(mask_undos_.begin());
			mask_undo_pointer_--;
		}
		return true;
	}

	bool Texture::trim_mask_undos_tail()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return true;
		if (mask_undos_.size() <= mask_undo_num_+1)
			return true;
		if (mask_undo_pointer_ == mask_undos_.size()-1)
			return false;
		while (mask_undos_.size()>mask_undo_num_+1 &&
			mask_undo_pointer_>=0 &&
			mask_undo_pointer_<mask_undos_.size()-1)
		{
			delete []mask_undos_.back();
			mask_undos_.pop_back();
		}
		return true;
	}

	bool Texture::get_undo()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return false;
		if (mask_undo_pointer_ <= 0)
			return false;
		return true;
	}

	bool Texture::get_redo()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return false;
		if (mask_undo_pointer_ >= mask_undos_.size()-1)
			return false;
		return true;
	}

	void Texture::set_mask(void* mask_data)
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return;

		if (mask_undo_pointer_>-1 &&
			mask_undo_pointer_<mask_undos_.size()-1)
		{
			mask_undos_.insert(
				mask_undos_.begin()+mask_undo_pointer_+1,
				mask_data);
			mask_undo_pointer_++;
			if (!trim_mask_undos_head())
				trim_mask_undos_tail();
		}
		else
		{
			mask_undos_.push_back(mask_data);
			mask_undo_pointer_ = mask_undos_.size()-1;
			trim_mask_undos_head();
		}
	}

	void Texture::push_mask()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return;
		if (mask_undo_pointer_<0 ||
			mask_undo_pointer_>mask_undos_.size()-1)
			return;

		//duplicate at pointer position
		unsigned long long mem_size = (unsigned long long)nx_*
			(unsigned long long)ny_*(unsigned long long)nz_;
		void* new_data = (void*)new (std::nothrow) unsigned char[mem_size];
		memcpy(new_data, mask_undos_[mask_undo_pointer_], size_t(mem_size));
		if (mask_undo_pointer_<mask_undos_.size()-1)
		{
			mask_undos_.insert(
				mask_undos_.begin()+mask_undo_pointer_+1,
				new_data);
			mask_undo_pointer_++;
			if (!trim_mask_undos_head())
				trim_mask_undos_tail();
		}
		else
		{
			mask_undos_.push_back(new_data);
			mask_undo_pointer_++;
			trim_mask_undos_head();
		}

		//update mask data
		nrrdWrap_va(data_[nmask_],
			mask_undos_[mask_undo_pointer_],
			nrrdTypeUChar, 3, (size_t)nx_,
			(size_t)ny_, (size_t)nz_);
	}

	void Texture:: mask_undos_backward()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return;
		if (mask_undo_pointer_<=0 ||
			mask_undo_pointer_>mask_undos_.size()-1)
			return;

		//move pointer
		mask_undo_pointer_--;

		//update mask data
		nrrdWrap_va(data_[nmask_],
			mask_undos_[mask_undo_pointer_],
			nrrdTypeUChar, 3, (size_t)nx_,
			(size_t)ny_, (size_t)nz_);
	}

	void Texture::mask_undos_forward()
	{
		if (nmask_<=-1 || mask_undo_num_==0)
			return;
		if (mask_undo_pointer_<0 ||
			mask_undo_pointer_>mask_undos_.size()-2)
			return;

		//move pointer
		mask_undo_pointer_++;

		//update mask data
		nrrdWrap_va(data_[nmask_],
			mask_undos_[mask_undo_pointer_],
			nrrdTypeUChar, 3, (size_t)nx_,
			(size_t)ny_, (size_t)nz_);
	}

} // namespace FLIVR