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
#ifndef _H5J_READER_H_
#define _H5J_READER_H_

#include <base_reader.h>
#include <cstdio>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <stdint.h>
#include <string>
#include <sstream>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/pixfmt.h>
	#include <libavutil/opt.h>
	#include <libavutil/imgutils.h>
}

using namespace std;

class EXPORT_API H5JReader : public BaseReader
{
public:
	H5JReader();
	~H5JReader();

	//int GetType() { return READER_H5J_TYPE; }

	void SetFile(string &file);
	void SetFile(wstring &file);
	void SetSliceSeq(bool ss);
	bool GetSliceSeq();
	void SetTimeId(wstring &id);
	wstring GetTimeId();
	void Preprocess();
	
	/**
	 * This method swaps the byte order of a short.
	 * @param num The short to swap byte order.
	 * @return The short with bytes swapped.
	 */
	uint16_t SwapShort(uint16_t num);
	/**
	 * This method swaps the byte order of a word.
	 * @param num The word to swap byte order.
	 * @return The word with bytes swapped.
	 */
	uint32_t SwapWord(uint32_t num);
	/**
	 * This method swaps the byte order of a 8byte number.
	 * @param num The 8byte to swap byte order.
	 * @return The 8byte with bytes swapped.
	 */
	uint64_t SwapLong(uint64_t num);
	/**
	 * Determines the number of pages in a tiff.
	 * @throws An exception if a tiff is not open.
	 * @return The number of pages in the file.
	 */
	void SetBatch(bool batch);
	int LoadBatch(int index);
	Nrrd* Convert(int t, int c, bool get_max);
	wstring GetCurDataName(int t, int c);
	wstring GetCurMaskName(int t, int c);
	wstring GetCurLabelName(int t, int c);

	wstring GetPathName() {return m_path_name;}
	wstring GetDataName() {return m_data_name;}
	int GetCurTime() {return m_cur_time;}
	int GetTimeNum() {return m_time_num;}
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

	wstring GetCurName(int t, int c);
	void SetTimeSeq(bool ts);
	bool GetTimeSeq();

private:
	wstring m_data_name;
	wstring m_time_id;
	bool m_enable_4d_seq;

	struct TimeDataInfo
	{
		int type;	//0-single file;1-sequence
		int filenumber;	//filenumber for sorting
		wstring stack; //stack file name
	};
	vector<TimeDataInfo> m_4d_seq;

	bool m_slice_seq;
	int m_time_num;
	int m_cur_time;
	int m_chan_num;
	int m_slice_num;
	int m_x_size;
	int m_y_size;
	bool m_valid_spc;
	double m_xspc;
	double m_yspc;
	double m_zspc;
	wstring m_unit;
	double m_max_value;
	double m_scalar_scale;

	int m_paddingR;
	int m_paddingB;

private:
	static bool h5j_sort(const TimeDataInfo& info1, const TimeDataInfo& info2);
	
};

class H5JMemStream
{
	unsigned char *m_data;
	size_t m_datasize;
	size_t m_pos;

public:
	H5JMemStream(unsigned char *data, size_t datasize);
	void seek(size_t pos);
	size_t read(unsigned char *buf, size_t len);
};

class FFMpegMemDecoder {
public:
	FFMpegMemDecoder(void *ibytes, size_t isize, size_t paddingR = 0, size_t paddingB = 0, int frame_num = 0);
	~FFMpegMemDecoder();
	size_t getDecodedDataSize() { return _outbuf_size; }
	void close();
	int getImageWidth() { return _width; }
	int getImageHeight() { return _height; }
	int getImagePaddedWidth() { return _width - _paddingR; }
	int getImagePaddedHeight() { return _height - _paddingB; }
	void setImagePaddingR(int paddingR) { _paddingR = paddingR; }
	void setImagePaddingB(int paddingB) { _paddingB = paddingB; }
	int getFrameNum() { return _frame_num; }
	AVPixelFormat getPixelFormat();
	int getBytesPerPixel() { return _bytesperpixel; }
	void start();
	void extractBytes(unsigned char *input, int frameid, unsigned char *output);
	void processImage(int frameid, unsigned char *output);
	void *grab(void* out = NULL);
	//int hw_device_setup_for_decode(AVCodec *decoder);


private:
	wstring _filename;
	AVFormatContext *_format_context;
	AVStream *_video_stream;
	AVCodecContext *_video_codec;
	AVFrame *picture, *picture_rgb;
	AVPacket pkt;
	SwsContext* img_convert_ctx;
	int _bytesperpixel;
	bool _frame_grabbed;
	long _time_stamp;
	bool deinterlace;
	int _components_per_frame;

	long _frame_count;
	long _frame_num;
	bool _flush;

	H5JMemStream *_memstream;
	unsigned char *_buffer;
	size_t _buf_size;

	int _width;
	int _height;
	int _paddingR;
	int _paddingB;

	size_t _outbuf_size;

};

#endif//_TIF_READER_H_
