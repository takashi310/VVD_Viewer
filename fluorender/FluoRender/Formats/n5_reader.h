#ifndef _N5_READER_H_
#define _N5_READER_H_

#include <base_reader.h>
#include <json.hpp>

using namespace std;

#define N5DataTypeChar 0
#define N5DataTypeUChar 1
#define N5DataTypeShort 2
#define N5DataTypeUShort 3
#define N5DataTypeInt 4
#define N5DataTypeUInt 5
#define N5DataTypeFloat 6

#define SerialVersionUID -4521467080388947553L
#define DimensionsKey "dimensions"
#define BlockSizeKey "blockSize"
#define DataTypeKey "dataType"
#define CompressionKey "compression"
#define CompressionTypeKey "compressionType"

class DataBlock
{
public:

	int m_type;
	void* m_data;

	DataBlock() {
		m_data = NULL; m_type = 0;
	}
	DataBlock(void* data, int type) {
		m_data = data; m_type = type;
	}
	~DataBlock() {
		if (m_data)
		{
			switch (m_type)
			{
			case N5DataTypeChar:
			case N5DataTypeUChar:
				delete[] static_cast<unsigned char*>(m_data);
				break;
			case N5DataTypeShort:
			case N5DataTypeUShort:
				delete[] static_cast<unsigned short*>(m_data);
				break;
			case N5DataTypeInt:
			case N5DataTypeUInt:
				delete[] static_cast<unsigned int*>(m_data);
				break;
			case N5DataTypeFloat:
				delete[] static_cast<float*>(m_data);
				break;
			default:
				delete[] m_data;
			}
			m_data = nullptr;
		}
	}
};

class DatasetAttributes
{
public:

	vector<long> m_dimensions;
	vector<int> m_blockSize;
	int m_dataType;
	int m_compression;

	DatasetAttributes(vector<long> dimensions, vector<int> blockSize, int dataType, int compression)
	{
		m_dimensions = dimensions;
		m_blockSize = blockSize;
		m_dataType = dataType;
		m_compression = compression;
	}

	vector<long> getDimensions() {

		return m_dimensions;
	}

	int getNumDimensions() {

		return m_dimensions.size();
	}

	vector<int> getBlockSize() {

		return m_blockSize;
	}

	int getCompression() {

		return m_compression;
	}

	int getDataType() {

		return m_dataType;
	}
	/*
	HashMap<String, Object> asMap() {

		final HashMap<String, Object> map = new HashMap<>();
		map.put(dimensionsKey, dimensions);
		map.put(blockSizeKey, blockSize);
		map.put(dataTypeKey, dataType.toString());
		map.put(compressionKey, compression);
		return map;
	}
	*/
};

class EXPORT_API N5Reader : public BaseReader
{
public:
	N5Reader();
	~N5Reader();

	void SetFile(string& file);
	void SetFile(wstring& file);
	void SetSliceSeq(bool ss);
	bool GetSliceSeq();
	void SetTimeSeq(bool ts);
	bool GetTimeSeq();
	void SetTimeId(wstring& id);
	wstring GetTimeId();
	void Preprocess();
	void SetBatch(bool batch);
	int LoadBatch(int index);
	Nrrd* Convert_ThreadSafe(int t, int c, bool get_max);
	wstring GetCurName(int t, int c);

	wstring GetPathName() { return m_path_name; }
	wstring GetDataName() { return L""; }
	int GetTimeNum() { return 0; }
	int GetCurTime() { return 0; }
	int GetChanNum() { return 0; }
	double GetExcitationWavelength(int chan) { return 0.0; }
	int GetSliceNum() { return 0; }
	int GetXSize() { return 0; }
	int GetYSize() { return 0; }
	bool IsSpcInfoValid() { return false; }
	double GetXSpc() { return 0.0; }
	double GetYSpc() { return 0.0; }
	double GetZSpc() { return 0.0; }
	double GetMaxValue() { return 0.0; }
	double GetScalarScale() { return 0.0; }
	bool GetBatch() { return false; }
	int GetBatchNum() { return 0; }
	int GetCurBatch() { return 0; }
	void SetInfo();

	void getJson();
	map<wstring, wstring> getAttributes(wstring pathName);
	DataBlock readBlock(wstring pathName, const DatasetAttributes& datasetAttributes, const vector<long> gridPosition);
	vector<wstring> list(wstring pathName);
	wstring getDataBlockPath(wstring datasetPathName, const vector<long>& gridPosition);
	wstring getAttributesPath(wstring pathName);
	wstring removeLeadingSlash(const wstring pathName);

protected:
	Nrrd* ConvertNrrd(int t, int c, bool get_max);

private:
	wstring m_data_name;

	//4d sequence
	struct TimeDataInfo
	{
		int filenumber;
		wstring filename;
	};
	vector<TimeDataInfo> m_4d_seq;
	int m_cur_time;

	int m_time_num;
	int m_chan_num;
	int m_slice_num;
	int m_x_size;
	int m_y_size;
	bool m_valid_spc;
	double m_xspc;
	double m_yspc;
	double m_zspc;
	double m_max_value;
	double m_scalar_scale;

	//time sequence id
	wstring m_time_id;

	bool m_enable_4d_seq;

	nlohmann::json jf;

private:
	static bool nrrd_sort(const TimeDataInfo& info1, const TimeDataInfo& info2);
};

#endif//_LBL_READER_H_