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
#ifndef _V3DPBD_READER_H_
#define _V3DPBD_READER_H_

#include <base_reader.h>
#include <stdio.h>
//#include <windows.h>
#include <vector>

using namespace std;

class EXPORT_API V3DPBDReader : public BaseReader
{
public:
	V3DPBDReader();
	~V3DPBDReader();

	void SetFile(string &file);
	void SetFile(wstring &file);
	void SetSliceSeq(bool ss);
	bool GetSliceSeq();
	void SetTimeSeq(bool ts);
	bool GetTimeSeq();
	void SetTimeId(wstring &id);
	wstring GetTimeId();
	void Preprocess();
	void SetBatch(bool batch);
	int LoadBatch(int index);
	Nrrd* Convert_ThreadSafe(int t, int c, bool get_max) { return NULL; }
	wstring GetCurName(int t, int c);

	wstring GetPathName() {return m_path_name;}
	wstring GetDataName() {return m_data_name;}
	int GetTimeNum() {return m_time_num;}
	int GetCurTime() {return m_cur_time;}
	int GetChanNum() {return m_chan_num;}
	double GetExcitationWavelength(int chan) {return 0.0;}
	int GetSliceNum() {return m_slice_num;}
	int GetXSize() {return m_x_size;}
	int GetYSize() {return m_y_size;}
	bool IsSpcInfoValid() {return m_valid_spc;}
	double GetXSpc() {return m_xspc;}
	double GetYSpc() {return m_yspc;}
	double GetZSpc() {return m_zspc;}
	double GetMaxValue() {return m_max_value;}
	double GetScalarScale() {return m_scalar_scale;}
	bool GetBatch() {return m_batch;}
	int GetBatchNum() {return (int)m_batch_list.size();}
	int GetCurBatch() {return m_cur_batch;}
	void SetInfo();
    
    int loadRaw2StackPBD(const char* filename);
    size_t decompressPBD8(unsigned char* sourceData, unsigned char* targetData, size_t sourceLength);
    size_t decompressPBD16(unsigned char* sourceData, unsigned char* targetData, size_t sourceLength);
    void updateCompressionBuffer8(unsigned char* updatedCompressionBuffer);
    void updateCompressionBuffer16(unsigned char* updatedCompressionBuffer);
    void updateCompressionBuffer3(unsigned char * updatedCompressionBuffer);
    int pbd3GetRepeatCountFromBytes(unsigned char keyByte, unsigned char valueByte, unsigned char* repeatValue);
    size_t decompressPBD3(unsigned char * sourceData, unsigned char * targetData, size_t sourceLength);
    
    static char checkMachineEndian();
    void swap2bytes(void *targetp);
    void swap4bytes(void *targetp);

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
    int m_bd;

	//time sequence id
	wstring m_time_id;

	bool m_enable_4d_seq;
    
    size_t totalReadBytes;
    size_t maxDecompressionSize;
    size_t channelLength;
    std::vector<unsigned char> compressionBuffer;
    unsigned char * decompressionBuffer;
    unsigned char * compressionPosition;
    unsigned char * decompressionPosition;
    int decompressionPrior;
    std::vector<char> keyread;
    int loadDatatype;
    
    unsigned char* pbd3_source_min;
    unsigned char* pbd3_source_max;
    int pbd3_current_channel;
    unsigned char pbd3_current_min;
    unsigned char pbd3_current_max;
    
    size_t pbd_sz[4];

	string m_cur_file;

private:
	static bool nrrd_sort(const TimeDataInfo& info1, const TimeDataInfo& info2);
};

#endif//_V3DPBD_READER_H_
