#ifndef _SWC_READER_H_
#define _SWC_READER_H_

#include <vector>
#include <base_reader.h>
#include <glm.h>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

class SWCReader
{
public:
	SWCReader();
	~SWCReader();

	void SetFile(string &file);
	void SetFile(wstring &file);
	void Preprocess();
	GLMmodel* Convert();
	
	wstring GetPathName() {return m_path_name;}
	wstring GetDataName() {return m_data_name;}
	void SetInfo();

	GLMmodel *GenerateSolidModel(double def_r, double r_scale, unsigned int subdiv);
	GLMmodel *GenerateWireModel();

	static bool DeepCopy(SWCReader *in, SWCReader *out);

private:
	wstring m_data_name;
	wstring m_path_name;
	vector<glm::vec4> m_vertices; //4th element: radius
	vector<glm::ivec2> m_edges;
	vector<GLfloat> m_model_verts;
	vector<GLfloat> m_model_norms;
	vector<GLuint> m_model_tris;

	vector<glm::vec3> m_sphere_verts_cache;
	vector<glm::ivec3> m_sphere_tris_cache;
	int m_before_sp_subdiv;

	vector<GLfloat> m_cylinder_verts_cache;
	vector<GLfloat> m_cylinder_norms_cache;
	vector<GLuint> m_cylinder_tris_cache;
	int m_before_cy_subdiv;
	double m_before_cy_r1;
	double m_before_cy_r2;

private:
	static void RotateVertices(vector<GLfloat> &vertices, glm::vec3 center, GLfloat angle, glm::vec3 axis);
	void AddSolidSphere(glm::vec3 center, double radius, unsigned int subdiv);
	void AddSolidCylinder(glm::vec3 p1, glm::vec3 p2, double radius1, double radius2, unsigned int subdiv);
};

#endif//_SWC_READER_H_