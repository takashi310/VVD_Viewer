/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2014 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#include "n5_reader.h"
#include "../compatibility.h"
#include <algorithm>
#include <sstream>

#include "boost/filesystem.hpp"
using namespace boost::filesystem;

using nlohmann::json;

N5Reader::N5Reader()
{
	m_time_num = 0;
	m_chan_num = 0;
	m_slice_num = 0;
	m_x_size = 0;
	m_y_size = 0;

	m_valid_spc = false;
	m_xspc = 0.0;
	m_yspc = 0.0;
	m_zspc = 0.0;

	m_max_value = 0.0;
	m_scalar_scale = 1.0;

	m_batch = false;
	m_cur_batch = -1;
	m_cur_time = -1;

	m_time_id = L"_T";

	m_enable_4d_seq = false;
}

N5Reader::~N5Reader()
{
}

void N5Reader::SetFile(string& file)
{
	if (!file.empty())
	{
		if (!m_path_name.empty())
			m_path_name.clear();
		m_path_name.assign(file.length(), L' ');
		copy(file.begin(), file.end(), m_path_name.begin());
	}
	m_id_string = m_path_name;
}

void N5Reader::SetFile(wstring& file)
{
	m_path_name = file;
	m_id_string = m_path_name;
}

void N5Reader::Preprocess()
{
	m_4d_seq.clear();

	//separate path and name
	int64_t pos = m_path_name.find_last_of(GETSLASH());
	if (pos == -1)
		return;
	wstring path = m_path_name.substr(0, pos + 1);
	wstring name = m_path_name.substr(pos + 1);
	//generate search name for time sequence
	int64_t begin = name.find(m_time_id);
	size_t end = -1;
	size_t id_len = m_time_id.size();
	wstring t_num;
	if (begin != -1)
	{
		size_t j;
		for (j = begin + id_len; j < name.size(); j++)
		{
			wchar_t c = name[j];
			if (iswdigit(c))
				t_num.push_back(c);
			else
				break;
		}
		if (t_num.size() > 0)
			end = j;
		else
			begin = -1;
	}
	//build 4d sequence
	if (begin == -1 || !m_enable_4d_seq)
	{
		TimeDataInfo info;
		info.filenumber = 0;
		info.filename = m_path_name;
		m_4d_seq.push_back(info);
		m_cur_time = 0;
	}
	else
	{
		//search time sequence files
		std::vector<std::wstring> list;
		int64_t begin = m_path_name.find(m_time_id);
		size_t id_len = m_time_id.length();
		FIND_FILES_4D(m_path_name, m_time_id, list, m_cur_time);
		for (size_t i = 0; i < list.size(); i++) {
			TimeDataInfo info;
			std::wstring str = list.at(i);
			std::wstring t_num;
			for (size_t j = begin + id_len; j < str.size(); j++)
			{
				wchar_t c = str[j];
				if (iswdigit(c))
					t_num.push_back(c);
				else break;
			}
			if (t_num.size() > 0)
				info.filenumber = WSTOI(t_num);
			else
				info.filenumber = 0;
			info.filename = str;
			m_4d_seq.push_back(info);
		}
	}
	if (m_4d_seq.size() > 0)
	{
//		std::sort(m_4d_seq.begin(), m_4d_seq.end(), NRRDReader::nrrd_sort);
		for (int t = 0; t < (int)m_4d_seq.size(); t++)
		{
			if (m_4d_seq[t].filename == m_path_name)
			{
				m_cur_time = t;
				break;
			}
		}
	}
	else
		m_cur_time = 0;

	//3D nrrd file
	m_chan_num = 1;
	//get time number
	m_time_num = (int)m_4d_seq.size();
}

void N5Reader::SetSliceSeq(bool ss)
{
	//do nothing
}

bool N5Reader::GetSliceSeq()
{
	return false;
}

void N5Reader::SetTimeSeq(bool ts)
{
	m_enable_4d_seq = ts;
}

bool N5Reader::GetTimeSeq()
{
	return m_enable_4d_seq;
}

void N5Reader::SetTimeId(wstring& id)
{
	m_time_id = id;
}

wstring N5Reader::GetTimeId()
{
	return m_time_id;
}

void N5Reader::SetBatch(bool batch)
{
	if (batch)
	{
		//read the directory info
		wstring search_path = m_path_name.substr(0, m_path_name.find_last_of(GETSLASH())) + GETSLASH();
		wstring search_str = search_path + L".nrrd";
		FIND_FILES(search_path, L".nrrd", m_batch_list, m_cur_batch, L"");
		m_batch = true;
	}
	else
		m_batch = false;
}

int N5Reader::LoadBatch(int index)
{
	int result = -1;
	if (index >= 0 && index < (int)m_batch_list.size())
	{
		m_path_name = m_batch_list[index];
		Preprocess();
		result = index;
		m_cur_batch = result;
	}
	else
		result = -1;

	return result;
}

Nrrd* N5Reader::ConvertNrrd(int t, int c, bool get_max)
{
	if (t < 0 || t >= m_time_num)
		return 0;

	size_t i;

	wstring str_name = m_4d_seq[t].filename;
	m_data_name = str_name.substr(str_name.find_last_of(GETSLASH()) + 1);
	FILE* nrrd_file = 0;
	if (!WFOPEN(&nrrd_file, str_name.c_str(), L"rb"))
		return 0;

	Nrrd* output = nrrdNew();
	NrrdIoState* nio = nrrdIoStateNew();
	nrrdIoStateSet(nio, nrrdIoStateSkipData, AIR_TRUE);
	if (nrrdRead(output, nrrd_file, nio))
	{
		fclose(nrrd_file);
		return 0;
	}
	nio = nrrdIoStateNix(nio);
	rewind(nrrd_file);
	if (!(output->dim == 3 || output->dim == 2))
	{
		delete[]output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	if (output->dim == 2)
	{
		output->axis[2].size = 1;
		output->axis[2].spacing = 1.0;
		output->axis[0].spaceDirection[2] = 0.0;
		output->axis[1].spaceDirection[2] = 0.0;
		output->axis[2].spaceDirection[0] = 0.0;
		output->axis[2].spaceDirection[1] = 0.0;
		output->axis[2].spaceDirection[2] = 1.0;
	}

	m_slice_num = int(output->axis[2].size);
	m_x_size = int(output->axis[0].size);
	m_y_size = int(output->axis[1].size);
	m_xspc = output->axis[0].spacing;
	m_yspc = output->axis[1].spacing;
	m_zspc = output->axis[2].spacing;

	if (!(m_xspc > 0.0 && m_xspc < 100.0))
	{
		double n = output->axis[0].spaceDirection[0] * output->axis[0].spaceDirection[0] +
			output->axis[0].spaceDirection[1] * output->axis[0].spaceDirection[1] +
			output->axis[0].spaceDirection[2] * output->axis[0].spaceDirection[2];
		m_xspc = sqrt(n);
	}
	if (!(m_yspc > 0.0 && m_yspc < 100.0))
	{
		double n = output->axis[1].spaceDirection[0] * output->axis[1].spaceDirection[0] +
			output->axis[1].spaceDirection[1] * output->axis[1].spaceDirection[1] +
			output->axis[1].spaceDirection[2] * output->axis[1].spaceDirection[2];
		m_yspc = sqrt(n);
	}
	if (!(m_zspc > 0.0 && m_zspc < 100.0))
	{
		double n = output->axis[2].spaceDirection[0] * output->axis[2].spaceDirection[0] +
			output->axis[2].spaceDirection[1] * output->axis[2].spaceDirection[1] +
			output->axis[2].spaceDirection[2] * output->axis[2].spaceDirection[2];
		m_zspc = sqrt(n);
	}
	nrrdAxisInfoSet(output, nrrdAxisInfoSpacing, m_xspc, m_yspc, m_zspc);
	nrrdAxisInfoSet(output, nrrdAxisInfoMax, m_xspc * output->axis[0].size, m_yspc * output->axis[1].size, m_zspc * output->axis[2].size);

	if (m_xspc > 0.0 && m_xspc < 100.0 &&
		m_yspc>0.0 && m_yspc < 100.0 &&
		m_zspc>0.0 && m_zspc < 100.0)
		m_valid_spc = true;
	else
	{
		m_valid_spc = false;
		m_xspc = 1.0;
		m_yspc = 1.0;
		m_zspc = 1.0;
	}
	size_t voxelnum = (size_t)m_slice_num * (size_t)m_x_size * (size_t)m_y_size;
	size_t data_size = voxelnum;
	if (output->type == nrrdTypeUShort || output->type == nrrdTypeShort)
		data_size *= 2;
	output->data = new unsigned char[data_size];

	//if (data_size >= 1073741824UL)
	//	get_max = false;

	if (nrrdRead(output, nrrd_file, NULL))
	{
		delete[] output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	if (output->dim == 2)
	{
		output->dim = 3;
		output->axis[2].size = 1;
		output->axis[2].spacing = 1.0;
		output->axis[0].spaceDirection[2] = 0.0;
		output->axis[1].spaceDirection[2] = 0.0;
		output->axis[2].spaceDirection[0] = 0.0;
		output->axis[2].spaceDirection[1] = 0.0;
		output->axis[2].spaceDirection[2] = 1.0;
	}

	// turn signed into unsigned
	if (output->type == nrrdTypeChar) {
		for (i = 0; i < voxelnum; i++) {
			char val = ((char*)output->data)[i];
			unsigned char n = val + 128;
			((unsigned char*)output->data)[i] = n;
		}
		output->type = nrrdTypeUChar;
	}
	m_max_value = 0.0;
	// turn signed into unsigned
	unsigned short min_value = 32768, n;
	if (output->type == nrrdTypeShort || (output->type == nrrdTypeUShort && get_max)) {
		for (i = 0; i < voxelnum; i++) {
			if (output->type == nrrdTypeShort) {
				short val = ((short*)output->data)[i];
				n = val + 32768;
				((unsigned short*)output->data)[i] = n;
				min_value = (n < min_value) ? n : min_value;
			}
			else {
				n = ((unsigned short*)output->data)[i];
			}
			if (get_max)
				m_max_value = (n > m_max_value) ? n : m_max_value;
		}
	}
	//find max value
	if (output->type == nrrdTypeUChar)
	{
		//8 bit
		m_max_value = 255.0;
		m_scalar_scale = 1.0;
	}
	else if (output->type == nrrdTypeShort)
	{
		m_max_value -= min_value;
		//16 bit
		for (i = 0; i < voxelnum; i++) {
			((unsigned short*)output->data)[i] =
				((unsigned short*)output->data)[i] - min_value;
		}
		if (m_max_value > 0.0)
			m_scalar_scale = 65535.0 / m_max_value;
		else
			m_scalar_scale = 1.0;
		output->type = nrrdTypeUShort;
	}
	else if (output->type == nrrdTypeUShort)
	{
		if (m_max_value > 0.0)
			m_scalar_scale = 65535.0 / m_max_value;
		else
		{
			m_max_value = 65535.0;
			m_scalar_scale = 1.0;
		}
	}
	else
	{
		delete[]output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	m_cur_time = t;
	fclose(nrrd_file);

	SetInfo();

	return output;
}

Nrrd* N5Reader::Convert_ThreadSafe(int t, int c, bool get_max)
{
	if (t < 0 || t >= m_time_num)
		return 0;

	size_t i;

	wstring str_name = m_4d_seq[t].filename;
	wstring data_name = str_name.substr(str_name.find_last_of(GETSLASH()) + 1);
	FILE* nrrd_file = 0;
	if (!WFOPEN(&nrrd_file, str_name.c_str(), L"rb"))
		return 0;

	Nrrd* output = nrrdNew();
	NrrdIoState* nio = nrrdIoStateNew();
	nrrdIoStateSet(nio, nrrdIoStateSkipData, AIR_TRUE);
	if (nrrdRead(output, nrrd_file, nio))
	{
		fclose(nrrd_file);
		return 0;
	}
	nio = nrrdIoStateNix(nio);
	rewind(nrrd_file);
	if (!(output->dim == 3 || output->dim == 2))
	{
		delete[]output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	if (output->dim == 2)
	{
		output->axis[2].size = 1;
		output->axis[2].spacing = 1.0;
		output->axis[0].spaceDirection[2] = 0.0;
		output->axis[1].spaceDirection[2] = 0.0;
		output->axis[2].spaceDirection[0] = 0.0;
		output->axis[2].spaceDirection[1] = 0.0;
		output->axis[2].spaceDirection[2] = 1.0;
	}

	int slice_num = int(output->axis[2].size);
	int x_size = int(output->axis[0].size);
	int y_size = int(output->axis[1].size);
	double xspc = output->axis[0].spacing;
	double yspc = output->axis[1].spacing;
	double zspc = output->axis[2].spacing;

	if (!(xspc > 0.0 && xspc < 100.0))
	{
		double n = output->axis[0].spaceDirection[0] * output->axis[0].spaceDirection[0] +
			output->axis[0].spaceDirection[1] * output->axis[0].spaceDirection[1] +
			output->axis[0].spaceDirection[2] * output->axis[0].spaceDirection[2];
		xspc = sqrt(n);
	}
	if (!(yspc > 0.0 && yspc < 100.0))
	{
		double n = output->axis[1].spaceDirection[0] * output->axis[1].spaceDirection[0] +
			output->axis[1].spaceDirection[1] * output->axis[1].spaceDirection[1] +
			output->axis[1].spaceDirection[2] * output->axis[1].spaceDirection[2];
		yspc = sqrt(n);
	}
	if (!(zspc > 0.0 && zspc < 100.0))
	{
		double n = output->axis[2].spaceDirection[0] * output->axis[2].spaceDirection[0] +
			output->axis[2].spaceDirection[1] * output->axis[2].spaceDirection[1] +
			output->axis[2].spaceDirection[2] * output->axis[2].spaceDirection[2];
		zspc = sqrt(n);
	}
	nrrdAxisInfoSet(output, nrrdAxisInfoSpacing, xspc, yspc, zspc);
	nrrdAxisInfoSet(output, nrrdAxisInfoMax, xspc * output->axis[0].size, yspc * output->axis[1].size, zspc * output->axis[2].size);

	bool valid_spc = false;
	if (xspc > 0.0 && xspc < 100.0 &&
		yspc>0.0 && yspc < 100.0 &&
		zspc>0.0 && zspc < 100.0)
		valid_spc = true;
	else
	{
		valid_spc = false;
		xspc = 1.0;
		yspc = 1.0;
		zspc = 1.0;
	}
	size_t voxelnum = (size_t)m_slice_num * (size_t)m_x_size * (size_t)m_y_size;
	size_t data_size = voxelnum;
	if (output->type == nrrdTypeUShort || output->type == nrrdTypeShort)
		data_size *= 2;
	output->data = new unsigned char[data_size];

	//if (data_size >= 1073741824UL)
	//	get_max = false;

	if (nrrdRead(output, nrrd_file, NULL))
	{
		delete[] output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	if (output->dim == 2)
	{
		output->dim = 3;
		output->axis[2].size = 1;
		output->axis[2].spacing = 1.0;
		output->axis[0].spaceDirection[2] = 0.0;
		output->axis[1].spaceDirection[2] = 0.0;
		output->axis[2].spaceDirection[0] = 0.0;
		output->axis[2].spaceDirection[1] = 0.0;
		output->axis[2].spaceDirection[2] = 1.0;
	}

	// turn signed into unsigned
	if (output->type == nrrdTypeChar) {
		for (i = 0; i < voxelnum; i++) {
			char val = ((char*)output->data)[i];
			unsigned char n = val + 128;
			((unsigned char*)output->data)[i] = n;
		}
		output->type = nrrdTypeUChar;
	}

	double max_value = 0.0;
	double scalar_scale = 1.0;
	// turn signed into unsigned
	unsigned short min_value = 32768, n;
	if (output->type == nrrdTypeShort || (output->type == nrrdTypeUShort && get_max)) {
		for (i = 0; i < voxelnum; i++) {
			if (output->type == nrrdTypeShort) {
				short val = ((short*)output->data)[i];
				n = val + 32768;
				((unsigned short*)output->data)[i] = n;
				min_value = (n < min_value) ? n : min_value;
			}
			else {
				n = ((unsigned short*)output->data)[i];
			}
			if (get_max)
				max_value = (n > max_value) ? n : max_value;
		}
	}
	//find max value
	if (output->type == nrrdTypeUChar)
	{
		//8 bit
		max_value = 255.0;
		scalar_scale = 1.0;
	}
	else if (output->type == nrrdTypeShort)
	{
		max_value -= min_value;
		//16 bit
		for (i = 0; i < voxelnum; i++) {
			((unsigned short*)output->data)[i] =
				((unsigned short*)output->data)[i] - min_value;
		}
		if (max_value > 0.0)
			scalar_scale = 65535.0 / max_value;
		else
			scalar_scale = 1.0;
		output->type = nrrdTypeUShort;
	}
	else if (output->type == nrrdTypeUShort)
	{
		if (max_value > 0.0)
			scalar_scale = 65535.0 / max_value;
		else
		{
			max_value = 65535.0;
			scalar_scale = 1.0;
		}
	}
	else
	{
		delete[]output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	//m_cur_time = t;
	fclose(nrrd_file);

	//SetInfo();

	return output;
}

bool N5Reader::nrrd_sort(const TimeDataInfo& info1, const TimeDataInfo& info2)
{
	return info1.filenumber < info2.filenumber;
}

wstring N5Reader::GetCurName(int t, int c)
{
	return m_4d_seq[t].filename;
}

void N5Reader::SetInfo()
{
	wstringstream wss;

	wss << L"------------------------\n";
	wss << m_path_name << '\n';
	wss << L"File type: N5\n";
	wss << L"Width: " << m_x_size << L'\n';
	wss << L"Height: " << m_y_size << L'\n';
	wss << L"Depth: " << m_slice_num << L'\n';
	wss << L"Channels: " << m_chan_num << L'\n';
	wss << L"Frames: " << m_time_num << L'\n';

	m_info = wss.str();
}


void N5Reader::getJson()
{
    boost::filesystem::path::imbue(std::locale( std::locale(), new std::codecvt_utf8_utf16<wchar_t>()));
	boost::filesystem::path path(m_path_name);
    std::ifstream ifs(path.string());
    
	jf = json::parse(ifs);

	vector<long> dim_str = jf[DimensionsKey].get<vector<long>>();
	
	int dataType = jf[DataTypeKey].get<int>();

	vector<int> blockSize = jf[BlockSizeKey].get<vector<int>>();

	int compression = jf[CompressionKey].get<int>();

	/* version 0 */
	if (jf.find(CompressionKey) != jf.end() && jf.find(CompressionTypeKey) != jf.end()) {
		auto cptype = jf[CompressionTypeKey].get<string>();
		if (cptype == "raw")
			compression = 0;
		else if (cptype == "gzip")
			compression = 1;
		else if (cptype == "bzip2")
			compression = 2;
		else if (cptype == "lz4")
			compression = 3;
		else if (cptype == "xz")
			compression = 4;
	}
}

/*
template <class T> T N5Reader::parseAttribute(string key, T clazz, const json j)
{
	if (attribute != null && j.contains(attribute))
		return j.at(attribute).get<T>();
	else
		return null;
}

HashMap<String, JsonElement> readAttributes(final Reader reader, final Gson gson) throws IOException {

	final Type mapType = new TypeToken<HashMap<String, JsonElement>>() {}.getType();
	final HashMap<String, JsonElement> map = gson.fromJson(reader, mapType);
	return map == null ? new HashMap<>() : map;
}
*/
/**
 * Return a reasonable class for a {@link JsonPrimitive}.  Possible return
 * types are
 * <ul>
 * <li>boolean</li>
 * <li>double</li>
 * <li>String</li>
 * <li>Object</li>
 * </ul>
 *
 * @param jsonPrimitive
 * @return
 */
/*
public static Class< ? > classForJsonPrimitive(final JsonPrimitive jsonPrimitive) {

	if (jsonPrimitive.isBoolean())
		return boolean.class;
	else if (jsonPrimitive.isNumber())
		return double.class;
	else if (jsonPrimitive.isString())
		return String.class;
	else return Object.class;
}
*/

/**
 * Best effort implementation of {@link N5Reader#listAttributes(String)}
 * with limited type resolution.  Possible return types are
 * <ul>
 * <li>null</li>
 * <li>boolean</li>
 * <li>double</li>
 * <li>String</li>
 * <li>Object</li>
 * <li>boolean[]</li>
 * <li>double[]</li>
 * <li>String[]</li>
 * <li>Object[]</li>
 * </ul>
 */
/*
@Override
public default Map<String, Class< ? >> listAttributes(final String pathName) throws IOException {

	final HashMap<String, JsonElement> jsonElementMap = getAttributes(pathName);
	final HashMap<String, Class< ? >> attributes = new HashMap<>();
	jsonElementMap.forEach(
		(key, jsonElement) -> {
		final Class< ? > clazz;
		if (jsonElement.isJsonNull())
			clazz = null;
		else if (jsonElement.isJsonPrimitive())
			clazz = classForJsonPrimitive((JsonPrimitive)jsonElement);
		else if (jsonElement.isJsonArray()) {
			final JsonArray jsonArray = (JsonArray)jsonElement;
			if (jsonArray.size() > 0) {
				final JsonElement firstElement = jsonArray.get(0);
				if (firstElement.isJsonPrimitive())
					clazz = Array.newInstance(classForJsonPrimitive((JsonPrimitive)firstElement), 0).getClass();
				else
					clazz = Object[].class;
			}
			else
				clazz = Object[].class;
		}
		else
			clazz = Object.class;
		attributes.put(key, clazz);
	});
	return attributes;
}
*/

map<wstring, wstring> N5Reader::getAttributes(wstring pathName)
{
	map<wstring, wstring> mmap;
	path ppath(m_path_name + GETSLASH() + getAttributesPath(pathName));
	if (exists(pathName) && !exists(ppath))
		return mmap;

	//mmap = jf.fromJson(reader, mapType);
	return mmap;
}


DataBlock N5Reader::readBlock(wstring pathName, const DatasetAttributes &datasetAttributes, const vector<long> gridPosition)
{
	DataBlock ret;

	path ppath(m_path_name + GETSLASH() + getDataBlockPath(pathName, gridPosition));
	if (!exists(ppath))
		return ret;
	
	return readBlock(ppath.wstring(), datasetAttributes, gridPosition);
}

vector<wstring> N5Reader::list(wstring pathName)
{
	vector<wstring> ret;
	path parent_path(m_path_name + GETSLASH() + pathName);

	if (!exists(parent_path))
		return ret;

	directory_iterator end_itr; // default construction yields past-the-end

	for (directory_iterator itr(parent_path); itr != end_itr; ++itr)
	{
		if (is_directory(itr->status()))
		{
			//auto p = relative(itr->path(), parent_path);
			//ret.push_back(p.wstring());
		}
	}

	return ret;
}

/**
 * Constructs the path for a data block in a dataset at a given grid position.
 *
 * The returned path is
 * <pre>
 * $datasetPathName/$gridPosition[0]/$gridPosition[1]/.../$gridPosition[n]
 * </pre>
 *
 * This is the file into which the data block will be stored.
 *
 * @param datasetPathName
 * @param gridPosition
 * @return
 */
wstring N5Reader::getDataBlockPath(wstring datasetPathName, const vector<long> &gridPosition) {

	wstringstream wss;
	wss << removeLeadingSlash(datasetPathName) << GETSLASH();
	for (int i = 0; i < gridPosition.size(); ++i)
		wss << gridPosition[i];

	return wss.str();
}

/**
 * Constructs the path for the attributes file of a group or dataset.
 *
 * @param pathName
 * @return
 */
wstring N5Reader::getAttributesPath(wstring pathName) {

	return removeLeadingSlash(pathName) + L"attributes.json";
}

/**
 * Removes the leading slash from a given path and returns the corrected path.
 * It ensures correctness on both Unix and Windows, otherwise {@code pathName} is treated
 * as UNC path on Windows, and {@code Paths.get(pathName, ...)} fails with {@code InvalidPathException}.
 */
wstring N5Reader::removeLeadingSlash(const wstring pathName)
{
	return (pathName.rfind(L"/", 0) == 0) || (pathName.rfind(L"\\", 0) == 0) ? pathName.substr(1) : pathName;
}
