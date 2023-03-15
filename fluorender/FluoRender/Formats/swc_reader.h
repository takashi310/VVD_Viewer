#ifndef _SWC_READER_H_
#define _SWC_READER_H_

#include <vector>
#include <base_reader.h>
#include <glm.h>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

class EXPORT_API SWCReader
{
public:
	SWCReader();
	~SWCReader();

	void SetFile(string &file);
	void SetFile(wstring &file);
	void Preprocess();
	//GLMmodel* Convert();
	
	wstring GetPathName() {return m_path_name;}
	wstring GetDataName() {return m_data_name;}
	//void SetInfo();

	GLMmodel *GenerateSolidModel(double def_r, double r_scale, unsigned int subdiv);
	//GLMmodel *GenerateWireModel();
    
    vector<float>* GetModelExtraData()
    {
        return m_model_extra_data.size() > 0 ? &m_model_extra_data : NULL;
    }
    
    wstring GetGroupName(size_t id)
    {
        if (id < 0 || id >= m_group_names.size())
            return L"";
        return m_group_names[id];
    }
    size_t GetGroupNum() { return m_group_names.size(); }

	static bool DeepCopy(SWCReader *in, SWCReader *out);

private:
	wstring m_data_name;
	wstring m_path_name;
	vector<glm::vec4> m_vertices; //4th element: radius
    vector<float> m_extra_data;
	vector<glm::ivec2> m_edges;
	vector<float> m_model_verts;
	vector<float> m_model_norms;
    vector<float> m_model_uvs;
    vector<float> m_model_extra_data;
	vector<unsigned int> m_model_tris;
    
    vector<vector<unsigned int>> m_model_group_tris;
    vector<wstring> m_group_names;
    vector<size_t> m_v_group_id;

	vector<glm::vec3> m_sphere_verts_cache;
	vector<glm::ivec3> m_sphere_tris_cache;
	int m_before_sp_subdiv;

	vector<float> m_cylinder_verts_cache;
	vector<float> m_cylinder_norms_cache;
	vector<unsigned int> m_cylinder_tris_cache;
	int m_before_cy_subdiv;
	double m_before_cy_r1;
	double m_before_cy_r2;

private:
	static void RotateVertices(vector<float> &vertices, glm::vec3 center, float angle, glm::vec3 axis);
	void AddSolidSphere(glm::vec3 center, double radius, unsigned int subdiv, float score = -1.0f, int groupid = -1);
	void AddSolidCylinder(glm::vec3 p1, glm::vec3 p2, double radius1, double radius2, unsigned int subdiv, int groupid = -1);
};

#endif//_SWC_READER_H_
