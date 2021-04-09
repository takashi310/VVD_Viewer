#ifndef _PLY_READER_H_
#define _PLY_READER_H_

#include <vector>
#include <base_reader.h>
#include <glm.h>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

class EXPORT_API PLYReader
{
public:
	PLYReader();
	~PLYReader();

	void SetFile(string &file);
	void SetFile(wstring &file);
	//GLMmodel* Convert();
	
	wstring GetPathName() {return m_path_name;}
	wstring GetDataName() {return m_data_name;}
	//void SetInfo();

    GLMmodel *GetSolidModel();

	static bool DeepCopy(PLYReader *in, PLYReader *out);

private:
	wstring m_data_name;
	wstring m_path_name;
};

#endif//_PLY_READER_H_
