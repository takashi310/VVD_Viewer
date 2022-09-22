#define _USE_MATH_DEFINES
#include "swc_reader.h"
#include "../compatibility.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <locale>
#include <algorithm>
#include <map>

vector<string> split(const string &str, const string &delim, const string &abort_delim)
{
	vector<string> res;
	size_t current = 0, found;
	bool aborted = false;
	while ((found = str.find_first_of(delim+abort_delim, current)) != string::npos)
	{
		if (!abort_delim.empty() && abort_delim.find_first_of(str[found]) != string::npos)
		{
			aborted = true;
			break;
		}
		if (found > current) res.push_back(string(str, current, found - current));
		current = found + 1;
	}
	if (!aborted && str.size() > current) res.push_back(string(str, current, str.size() - current));
	return res;
}

void SWCReader::AddSolidSphere(glm::vec3 center, double radius, unsigned int subdiv, float score)
{
	// create 12 vertices of a icosahedron
	float t = (1.0 + sqrt(5.0)) / 2.0;

	vector<glm::vec3> vertices;
	vector<glm::ivec3> triangles;

	if (m_before_sp_subdiv == -1 || subdiv != m_before_sp_subdiv)
	{
		vertices.push_back(glm::vec3(-1,  t,  0));
		vertices.push_back(glm::vec3( 1,  t,  0));
		vertices.push_back(glm::vec3(-1, -t,  0));
		vertices.push_back(glm::vec3( 1, -t,  0));

		vertices.push_back(glm::vec3( 0, -1,  t));
		vertices.push_back(glm::vec3( 0,  1,  t));
		vertices.push_back(glm::vec3( 0, -1, -t));
		vertices.push_back(glm::vec3( 0,  1, -t));

		vertices.push_back(glm::vec3( t,  0, -1));
		vertices.push_back(glm::vec3( t,  0,  1));
		vertices.push_back(glm::vec3(-t,  0, -1));
		vertices.push_back(glm::vec3(-t,  0,  1));

		// 5 faces around point 0
		triangles.push_back(glm::ivec3(0, 11, 5));
		triangles.push_back(glm::ivec3(0, 5, 1));
		triangles.push_back(glm::ivec3(0, 1, 7));
		triangles.push_back(glm::ivec3(0, 7, 10));
		triangles.push_back(glm::ivec3(0, 10, 11));

		// 5 adjacent faces 
		triangles.push_back(glm::ivec3(1, 5, 9));
		triangles.push_back(glm::ivec3(5, 11, 4));
		triangles.push_back(glm::ivec3(11, 10, 2));
		triangles.push_back(glm::ivec3(10, 7, 6));
		triangles.push_back(glm::ivec3(7, 1, 8));

		// 5 faces around point 3
		triangles.push_back(glm::ivec3(3, 9, 4));
		triangles.push_back(glm::ivec3(3, 4, 2));
		triangles.push_back(glm::ivec3(3, 2, 6));
		triangles.push_back(glm::ivec3(3, 6, 8));
		triangles.push_back(glm::ivec3(3, 8, 9));

		// 5 adjacent faces 
		triangles.push_back(glm::ivec3(4, 9, 5));
		triangles.push_back(glm::ivec3(2, 4, 11));
		triangles.push_back(glm::ivec3(6, 2, 10));
		triangles.push_back(glm::ivec3(8, 6, 7));
		triangles.push_back(glm::ivec3(9, 8, 1));

		for (int i = 0; i < vertices.size(); i++)
			vertices[i] = glm::normalize(vertices[i]);

		// refine triangles
		for (int i = 0; i < subdiv; i++)
		{
			vector<glm::ivec3> triangles2;
			for (auto tri : triangles)
			{
				int i0 = tri.x;
				int i1 = tri.y;
				int i2 = tri.z;

				glm::vec3 midpoint;
				int m01, m12, m02;
				vector<glm::vec3>::iterator pos;

				midpoint = (vertices[i0] + vertices[i1]) * 0.5f;
				pos = std::find(vertices.begin(), vertices.end(), midpoint);
				if(pos != vertices.end()) m01 = pos - vertices.begin();
				else { m01 = vertices.size(); midpoint = glm::normalize(midpoint); vertices.push_back(midpoint); }

				midpoint = (vertices[i1] + vertices[i2]) * 0.5f;
				pos = std::find(vertices.begin(), vertices.end(), midpoint);
				if(pos != vertices.end())m12 = pos - vertices.begin();
				else { m12 = vertices.size(); midpoint = glm::normalize(midpoint); vertices.push_back(midpoint); }

				midpoint = (vertices[i0] + vertices[i2]) * 0.5f;
				pos = std::find(vertices.begin(), vertices.end(), midpoint);
				if(pos != vertices.end())m02 = pos - vertices.begin();
				else { m02 = vertices.size(); midpoint = glm::normalize(midpoint); vertices.push_back(midpoint); }

				triangles2.push_back(glm::ivec3(i0,  m01, m02));
				triangles2.push_back(glm::ivec3(i1,  m12, m01));
				triangles2.push_back(glm::ivec3(i2,  m02, m12));
				triangles2.push_back(glm::ivec3(m02, m01, m12));
			}
			triangles = triangles2;
		}

		m_sphere_verts_cache = vertices;
		m_sphere_tris_cache = triangles;
		m_before_sp_subdiv = subdiv;
	}
	else
	{
		vertices = m_sphere_verts_cache;
		triangles = m_sphere_tris_cache;
	}

	for (int i = 0; i < vertices.size(); i++)
		vertices[i] *= radius;

	int v_offset = m_model_verts.size();
	m_model_verts.resize(v_offset + vertices.size()*3);
	m_model_norms.resize(v_offset + vertices.size()*3);
	vector<float>::iterator v_ite = m_model_verts.begin() + v_offset;
	vector<float>::iterator n_ite = m_model_norms.begin() + v_offset;
	for (int i = 0; i < vertices.size(); i++)
	{
		*v_ite++ = vertices[i].x + center.x;
		*v_ite++ = vertices[i].y + center.y;
		*v_ite++ = vertices[i].z + center.z;
		*n_ite++ = vertices[i].x;
		*n_ite++ = vertices[i].y;
		*n_ite++ = vertices[i].z;
	}
    
    if (score >= 0.0f)
    {
        int e_offset = m_model_extra_data.size();
        m_model_extra_data.resize(e_offset + vertices.size());
        vector<float>::iterator e_ite = m_model_extra_data.begin() + e_offset;
        while(e_ite != m_model_extra_data.end())
            *e_ite++ = score;
    }

	int t_offset = m_model_tris.size();
	m_model_tris.resize(t_offset + triangles.size()*3);
	vector<unsigned int>::iterator t_ite = m_model_tris.begin() + t_offset;
	for (int i = 0; i < triangles.size(); i++)
	{
		*t_ite++ = triangles[i][0] + v_offset/3;
		*t_ite++ = triangles[i][1] + v_offset/3;
		*t_ite++ = triangles[i][2] + v_offset/3;
	}

	return;
}

void SWCReader::AddSolidCylinder(glm::vec3 p1, glm::vec3 p2, double radius1, double radius2, unsigned int subdiv)
{
	int sectors = 2*(subdiv+1)+2;

	double half_length = glm::distance(p1, p2) / 2.0;
	glm::vec3 center = glm::vec3((p1.x+p2.x)/2.0, (p1.y+p2.y)/2.0, (p1.z+p2.z)/2.0);

	float const S = 1./(float)sectors;

	vector<float> vertices;
	vector<float> normals;
	vector<unsigned int> triangles;

	if (m_before_cy_subdiv == -1 || subdiv != m_before_cy_subdiv)
	{
		vertices.resize(2*sectors*3);
		normals.resize(2*sectors*3);
		triangles.resize(2*sectors*3);

		vector<float>::iterator v = vertices.begin();
		vector<float>::iterator n = normals.begin();
		for(int s = 0; s < sectors; s++) {
			float const x = cos(2*M_PI * s * S);
			float const y = sin(2*M_PI * s * S);

			*v++ = x;
			*v++ = y;
			*v++ = -1.0;

			*n++ = x;
			*n++ = y;
			*n++ = 0.0;
		}
		for(int s = 0; s < sectors; s++) {
			float const x = cos(2*M_PI * s * S);
			float const y = sin(2*M_PI * s * S);

			*v++ = x;
			*v++ = y;
			*v++ = 1.0;

			*n++ = x;
			*n++ = y;
			*n++ = 0.0;
		}

		std::vector<unsigned int>::iterator i = triangles.begin();
		int s;
		for(s = 0; s < sectors-1; s++) {
			*i++ = s;
			*i++ = s + 1;
			*i++ = sectors + s + 1;

			*i++ = sectors + s + 1;
			*i++ = sectors + s;
			*i++ = s;
		}

		*i++ = s;
		*i++ = 0;
		*i++ = sectors;

		*i++ = sectors;
		*i++ = sectors + s;
		*i++ = s;

		m_cylinder_verts_cache = vertices;
		m_cylinder_norms_cache = normals;
		m_cylinder_tris_cache = triangles;
		m_before_cy_subdiv = subdiv;
	}
	else
	{
		vertices = m_cylinder_verts_cache;
		normals = m_cylinder_norms_cache;
		triangles = m_cylinder_tris_cache;
	}

	vector<float>::iterator v = vertices.begin();
	vector<float>::iterator n = normals.begin();
	double nz = (radius1 - radius2) / (half_length*2.0);
	double nd = sqrt(1.0 + nz*nz);
	nz /= nd;
	for(int s = 0; s < sectors; s++) {
		*v++ *= radius1;
		*v++ *= radius1;
		*v++ *= half_length;

		*n++ /= nd;
		*n++ /= nd;
		*n++ = nz;
	}
	for(int s = 0; s < sectors; s++) {
		*v++ *= radius2;
		*v++ *= radius2;
		*v++ *= half_length;

		*n++ /= nd;
		*n++ /= nd;
		*n++ = nz;
	}

	glm::dvec3 dd = glm::normalize(glm::dvec3(p2-p1));
	glm::dvec3 ez = glm::dvec3(0.0, 0.0, 1.0);
	if (dd != ez)
	{
		double angle = acos(glm::dot(ez, dd));
		glm::dvec3 n = glm::cross(ez, dd);
		RotateVertices(vertices, glm::vec3(0.0), angle, glm::vec3(n));
		RotateVertices(normals, glm::vec3(0.0), angle, glm::vec3(n));
	}

	int v_offset = m_model_verts.size();
	m_model_verts.resize(v_offset + vertices.size());
	m_model_norms.resize(v_offset + vertices.size());
	vector<float>::iterator mv_ite = m_model_verts.begin() + v_offset;
	vector<float>::iterator mn_ite = m_model_norms.begin() + v_offset;
	vector<float>::iterator v_ite = vertices.begin();
	vector<float>::iterator n_ite = normals.begin();
	for (int i = 0; i < vertices.size()/3; i++)
	{
		*mv_ite++ = *v_ite++ + center.x;
		*mn_ite++ = *n_ite++;
		*mv_ite++ = *v_ite++ + center.y;
		*mn_ite++ = *n_ite++;
		*mv_ite++ = *v_ite++ + center.z;
		*mn_ite++ = *n_ite++;
	}

	int t_offset = m_model_tris.size();
	m_model_tris.resize(t_offset + triangles.size());
	vector<unsigned int>::iterator mt_ite = m_model_tris.begin() + t_offset;
	vector<unsigned int>::iterator t_ite = triangles.begin();
	for (int i = 0; i < triangles.size()/3; i++)
	{
		*mt_ite++ = *t_ite++ + v_offset/3;
		*mt_ite++ = *t_ite++ + v_offset/3;
		*mt_ite++ = *t_ite++ + v_offset/3;
	}

	return;
}

void SWCReader::RotateVertices(vector<float> &vertices, glm::vec3 center, float angle, glm::vec3 axis)
{
	double c = cos(angle);
	double s = sin(angle);
	double d = sqrt(axis.x*axis.x + axis.y*axis.y + axis.z*axis.z);
	double dx = axis.x / d;
	double dy = axis.y / d;
	double dz = axis.z / d;
	double tx, ty, tz;

	unsigned int i;
	int numvertices = vertices.size()/3;
	double mat[9] =
	{dx*dx*(1-c)+c,    dx*dy*(1-c)-dz*s, dx*dz*(1-c)+dy*s,
	dy*dx*(1-c)+dz*s, dy*dy*(1-c)+c,    dy*dz*(1-c)-dx*s,
	dx*dz*(1-c)-dy*s, dy*dz*(1-c)+dx*s, dz*dz*(1-c)+c};
	for (i = 0 ; i < numvertices ; i++)
	{
		tx = vertices[3 * i + 0] - center.x;
		ty = vertices[3 * i + 1] - center.y;
		tz = vertices[3 * i + 2] - center.z;
		vertices[3 * i + 0] = float(
			mat[0]*tx +
			mat[1]*ty +
			mat[2]*tz);
		vertices[3 * i + 1] = float(
			mat[3]*tx +
			mat[4]*ty +
			mat[5]*tz);
		vertices[3 * i + 2] = float(
			mat[6]*tx +
			mat[7]*ty +
			mat[8]*tz);
		vertices[3 * i + 0] += center.x;
		vertices[3 * i + 1] += center.y;
		vertices[3 * i + 2] += center.z;
	}
}

GLMmodel *SWCReader::GenerateSolidModel(double def_r, double r_scale, unsigned int subdiv)
{
	if (m_vertices.empty()) return NULL;

	if (!m_model_verts.empty()) m_model_verts.clear();
	if (!m_model_norms.empty()) m_model_norms.clear();
    if (!m_model_extra_data.empty()) m_model_extra_data.clear();
	if (!m_model_tris.empty()) m_model_tris.clear();

	m_model_verts.push_back(0.0f);
	m_model_verts.push_back(0.0f);
	m_model_verts.push_back(0.0f);
	m_model_norms.push_back(0.0f);
	m_model_norms.push_back(0.0f);
	m_model_norms.push_back(0.0f);
    if (m_extra_data.size() > 0)
        m_model_extra_data.push_back(0.0f);

	//generate solid spheres
    for (int i = 0; i < m_vertices.size(); i++)
	{
		glm::vec3 v = glm::vec3(m_vertices[i]);
		double r = m_vertices[i].w;

		if (r <= 0.0) r = def_r;
        if (m_extra_data.size() > i)
            AddSolidSphere(v, r*r_scale, subdiv, m_extra_data[i]);
        else
            AddSolidSphere(v, r*r_scale, subdiv);
	}

	//generate solid cylinders
	for (auto e : m_edges)
	{
		glm::vec3 p1 = glm::vec3(m_vertices[e[0]]);
		double r1 = m_vertices[e[0]].w;
		glm::vec3 p2 = glm::vec3(m_vertices[e[1]]);
		double r2 = m_vertices[e[1]].w;

		if (r1 <= 0.0) r1 = def_r;
		if (r2 <= 0.0) r2 = def_r;
		int cy_subdiv = (int)pow(2.0, subdiv+2);
		AddSolidCylinder(p1, p2, r1*r_scale, r2*r_scale, cy_subdiv);
	}

	GLMmodel *model = (GLMmodel*)malloc(sizeof(GLMmodel));
	float *verts = (float*)malloc(sizeof(float)*m_model_verts.size());
	float *norms = (float*)malloc(sizeof(float)*m_model_norms.size());
	GLMtriangle *tris = (GLMtriangle*)malloc(sizeof(GLMtriangle)*m_model_tris.size()/3);
	unsigned int *gtris = (unsigned int*)malloc(sizeof(unsigned int)*m_model_tris.size()/3);

	memcpy(verts, m_model_verts.data(), sizeof(float)*m_model_verts.size());
	memcpy(norms, m_model_norms.data(), sizeof(float)*m_model_norms.size());
	
	int numtris = m_model_tris.size()/3;
	vector<unsigned int>::iterator t_ite = m_model_tris.begin();
	for (int i = 0; i < numtris; i++)
	{
		tris[i].vindices[0] = *t_ite;
		tris[i].nindices[0] = *t_ite++;
		tris[i].vindices[1] = *t_ite;
		tris[i].nindices[1] = *t_ite++;
		tris[i].vindices[2] = *t_ite;
		tris[i].nindices[2] = *t_ite++;
		gtris[i] = i;
	}

	model->pathname    = STRDUP(ws2s(m_path_name).c_str());
	model->mtllibname    = NULL;
	model->numvertices   = m_model_verts.size()/3-1;
	model->vertices    = verts;
	model->numnormals    = m_model_norms.size()/3-1;
	model->normals     = norms;
	model->numtexcoords  = 0;
	model->texcoords       = NULL;
	model->numfacetnorms = 0;
	model->facetnorms    = NULL;
	model->numtriangles  = numtris;
	model->triangles       = tris;
	model->numlines = 0;
	model->lines = NULL;
	model->nummaterials  = 0;
	model->materials       = NULL;
	model->numgroups       = 0;
	model->groups      = NULL;
	model->position[0]   = 0.0;
	model->position[1]   = 0.0;
	model->position[2]   = 0.0;
	model->hastexture = false;
	
	GLMgroup* group;
	group = (GLMgroup*)malloc(sizeof(GLMgroup));
	group->name = STRDUP("default");
	group->material = 0;
	group->numtriangles = numtris;
	group->triangles = gtris;
	group->next = model->groups;
	model->groups = group;
	model->numgroups++;

	return model;
}

SWCReader::SWCReader()
{
	m_before_sp_subdiv = -1;
	m_before_cy_subdiv = -1;
	m_before_cy_r1 = -1.0;
	m_before_cy_r2 = -1.0;
}

SWCReader::~SWCReader()
{
}

void SWCReader::SetFile(string &file)
{
	if (!file.empty())
	{
		if (!m_path_name.empty())
			m_path_name.clear();
		m_path_name.assign(file.length(), L' ');
		copy(file.begin(), file.end(), m_path_name.begin());
	}
}

void SWCReader::SetFile(wstring &file)
{
	m_path_name = file;
}

void SWCReader::Preprocess()
{
	if (!m_vertices.empty()) m_vertices.clear();
	if (!m_edges.empty()) m_edges.clear();
    if (!m_extra_data.empty()) m_extra_data.clear();

	//separate path and name
	int64_t pos = m_path_name.find_last_of(GETSLASH());
	if (pos == -1)
		return;
	wstring path = m_path_name.substr(0, pos+1);
	wstring name = m_path_name.substr(pos+1);

	ifstream ifs(ws2s(m_path_name));
	if (ifs.fail())
	{
		cerr << "file open error: " << ws2s(m_path_name) << endl;
		return;
	}

	string line, token;
	vector<vector<string>> str_data;
	map<int, int> id_corresp;
	int maxid = 0;
	while (getline(ifs, line))
	{
		if (line.empty() || line[0] == '#')
			continue;
		stringstream ls(line);
		vector<string> tokens = split(line, " ", "#");
		if (tokens.size() >= 7)
		{
			glm::vec4 v4;
			float fval;
			int ival;
			int id = STOI(tokens[0].c_str());
			fval = STOD(tokens[2].c_str());
			v4.x = fval;
			fval = STOD(tokens[3].c_str());
			v4.y = fval;
			fval = STOD(tokens[4].c_str());
			v4.z = -fval;
			if (tokens[5] != "NA")
			{
				fval = STOD(tokens[5].c_str());
				v4.w = fval;
			}
			else
				v4.w = 0.0;
            
            if (tokens.size() >= 8)
            {
                fval = STOD(tokens[7].c_str());
                m_extra_data.push_back(fval);
            }

			int newid = m_vertices.size();
			id_corresp[id] = newid;
			m_vertices.push_back(v4);
			
			ival = STOI(tokens[6].c_str());
			if (ival != -1)
				m_edges.push_back(glm::ivec2(id, ival));
            
		}
	}

	for (auto &e : m_edges)
	{
		e[0] = id_corresp[e[0]];
		e[1] = id_corresp[e[1]];
	}

	return;
}

bool SWCReader::DeepCopy(SWCReader *in, SWCReader *out)
{
	if (in == nullptr || out == nullptr)
		return false;

	out->m_data_name = in->m_data_name;
	out->m_path_name = in->m_path_name;
	out->m_vertices = in->m_vertices; //4th element: radius
    out->m_extra_data = in->m_extra_data;
	out->m_edges = in->m_edges;
	out->m_model_verts = in->m_model_verts;
	out->m_model_norms = in->m_model_norms;
	out->m_model_tris = in->m_model_tris;
    out->m_model_extra_data = in->m_model_extra_data;

	out->m_sphere_verts_cache = in->m_sphere_verts_cache;
	out->m_sphere_tris_cache = in->m_sphere_tris_cache;
	out->m_before_sp_subdiv = in->m_before_sp_subdiv;

	out->m_cylinder_verts_cache = in->m_cylinder_verts_cache;
	out->m_cylinder_norms_cache = in->m_cylinder_norms_cache;
	out->m_cylinder_tris_cache = in->m_cylinder_tris_cache;
	out->m_before_cy_subdiv = in->m_before_cy_subdiv;
	out->m_before_cy_r1 = in->m_before_cy_r1;
	out->m_before_cy_r2 = in->m_before_cy_r2;

	return true;
}
