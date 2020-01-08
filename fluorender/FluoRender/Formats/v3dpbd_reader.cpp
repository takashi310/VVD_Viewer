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
#include "v3dpbd_reader.h"
#include "../compatibility.h"
#include <algorithm>
#include <sstream>

static const unsigned char oooooool = 1;
static const unsigned char oooooolo = 2;
static const unsigned char ooooooll = 3;
static const unsigned char oooooloo = 4;
static const unsigned char ooooolol = 5;
static const unsigned char ooooollo = 6;
static const unsigned char ooooolll = 7;

static const unsigned char ooolllll = 63;
static const unsigned char olllllll = 127;
static const unsigned char looooooo = 128;
static const unsigned char lloooooo = 192;
static const unsigned char lllooooo = 224;

static const short int PBD_3_BIT_DTYPE = 33;

static const size_t   PBD_3_REPEAT_MAX = 4096;
static const double   PBD_3_LITERAL_MAXEFF = 2.66;
static const double   PBD_3_DIFF_MAXEFF = 5.2;
static const size_t   PBD_3_DIFF_REPEAT_THRESHOLD = 14;
static const size_t   PBD_3_MIN_DIFF_LENGTH = 6;
static const size_t   PBD_3_MAX_DIFF_LENGTH = 210;
static const int      PBD_3_REPEAT_STARTING_POSITION = 0;
static const int      PBD_3_REPEAT_COUNT = 1;
static const size_t   PBD_3_MAX_LITERAL = 24;

V3DPBDReader::V3DPBDReader()
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
    
    decompressionBuffer = NULL;
}

V3DPBDReader::~V3DPBDReader()
{
}

void V3DPBDReader::SetFile(string &file)
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

void V3DPBDReader::SetFile(wstring &file)
{
	m_path_name = file;
	m_id_string = m_path_name;
}

void V3DPBDReader::Preprocess()
{
	m_4d_seq.clear();

	//separate path and name
	int64_t pos = m_path_name.find_last_of(GETSLASH());
	if (pos == -1)
		return;
	wstring path = m_path_name.substr(0, pos+1);
	wstring name = m_path_name.substr(pos+1);
	//generate search name for time sequence
	int64_t begin = name.find(m_time_id);
	size_t end = -1;
	size_t id_len = m_time_id.size();
	wstring t_num;
	if (begin != -1)
	{
		size_t j;
		for (j=begin+id_len; j<name.size(); j++)
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
		std::sort(m_4d_seq.begin(), m_4d_seq.end(), V3DPBDReader::nrrd_sort);
		for (int t=0; t<(int)m_4d_seq.size(); t++)
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

void V3DPBDReader::SetSliceSeq(bool ss)
{
	//do nothing
}

bool V3DPBDReader::GetSliceSeq()
{
	return false;
}

void V3DPBDReader::SetTimeSeq(bool ts)
{
	m_enable_4d_seq = ts;
}

bool V3DPBDReader::GetTimeSeq()
{
	return m_enable_4d_seq;
}

void V3DPBDReader::SetTimeId(wstring &id)
{
	m_time_id = id;
}

wstring V3DPBDReader::GetTimeId()
{
	return m_time_id;
}

void V3DPBDReader::SetBatch(bool batch)
{
	if (batch)
	{
		//read the directory info
		wstring search_path = m_path_name.substr(0, m_path_name.find_last_of(GETSLASH())) + GETSLASH();
		wstring search_str = search_path + L".nrrd";
		FIND_FILES(search_path,L".nrrd",m_batch_list,m_cur_batch,L"");
		m_batch = true;
	}
	else
		m_batch = false;
}

int V3DPBDReader::LoadBatch(int index)
{
	int result = -1;
	if (index>=0 && index<(int)m_batch_list.size())
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

Nrrd* V3DPBDReader::Convert(int t, int c, bool get_max)
{
	if (t<0 || t>=m_time_num)
		return 0;

	size_t i;

	wstring str_name = m_4d_seq[t].filename;
	m_data_name = str_name.substr(str_name.find_last_of(GETSLASH())+1);
	FILE* nrrd_file = 0;
	if (!WFOPEN(&nrrd_file, str_name.c_str(), L"rb"))
		return 0;

	Nrrd *output = nrrdNew();
	NrrdIoState *nio = nrrdIoStateNew();
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
		delete []output->data;
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
	
	if (!(m_xspc>0.0 && m_xspc<100.0))
	{
		double n = output->axis[0].spaceDirection[0]*output->axis[0].spaceDirection[0] +
				   output->axis[0].spaceDirection[1]*output->axis[0].spaceDirection[1] + 
				   output->axis[0].spaceDirection[2]*output->axis[0].spaceDirection[2];
		m_xspc = sqrt(n);
	}
	if (!(m_yspc>0.0 && m_yspc<100.0))
	{
		double n = output->axis[1].spaceDirection[0]*output->axis[1].spaceDirection[0] +
				   output->axis[1].spaceDirection[1]*output->axis[1].spaceDirection[1] + 
				   output->axis[1].spaceDirection[2]*output->axis[1].spaceDirection[2];
		m_yspc = sqrt(n);
	}
	if (!(m_zspc>0.0 && m_zspc<100.0))
	{
		double n = output->axis[2].spaceDirection[0]*output->axis[2].spaceDirection[0] +
				   output->axis[2].spaceDirection[1]*output->axis[2].spaceDirection[1] + 
				   output->axis[2].spaceDirection[2]*output->axis[2].spaceDirection[2];
		m_zspc = sqrt(n);
	}
	nrrdAxisInfoSet(output, nrrdAxisInfoSpacing, m_xspc, m_yspc, m_zspc);
	nrrdAxisInfoSet(output, nrrdAxisInfoMax, m_xspc*output->axis[0].size, m_yspc*output->axis[1].size, m_zspc*output->axis[2].size);

	if (m_xspc>0.0 && m_xspc<100.0 &&
		m_yspc>0.0 && m_yspc<100.0 &&
		m_zspc>0.0 && m_zspc<100.0)
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
		delete [] output->data;
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
		for (i=0; i<voxelnum; i++) {
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
		for (i=0; i<voxelnum; i++) {
			if (output->type == nrrdTypeShort) {
				short val = ((short*)output->data)[i];
				n = val + 32768;
				((unsigned short*)output->data)[i] = n;
				min_value = (n < min_value)?n:min_value;
			} else {
				n =  ((unsigned short*)output->data)[i];
			}
			if (get_max)
				m_max_value = (n > m_max_value)?n:m_max_value;
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
		for (i=0; i<voxelnum; i++) {
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
		delete []output->data;
		nrrdNix(output);
		fclose(nrrd_file);
		return 0;
	}

	m_cur_time = t;
	fclose(nrrd_file);

	SetInfo();

	return output;
}

bool V3DPBDReader::nrrd_sort(const TimeDataInfo& info1, const TimeDataInfo& info2)
{
	return info1.filenumber < info2.filenumber;
}

wstring V3DPBDReader::GetCurName(int t, int c)
{
	return m_4d_seq[t].filename;
}

void V3DPBDReader::SetInfo()
{
	wstringstream wss;

	wss << L"------------------------\n";
	wss << m_path_name << '\n';
	wss << L"File type: NRRD\n";
	wss << L"Width: " << m_x_size << L'\n';
	wss << L"Height: " << m_y_size << L'\n';
	wss << L"Depth: " << m_slice_num << L'\n';
	wss << L"Channels: " << m_chan_num << L'\n';
	wss << L"Frames: " << m_time_num << L'\n';

	m_info = wss.str();
}

size_t V3DPBDReader::decompressPBD8(unsigned char * sourceData, unsigned char * targetData, size_t sourceLength) {
    
    // Decompress data
    size_t cp=0;
    size_t dp=0;
    const unsigned char mask=0x0003;
    unsigned char p0,p1,p2,p3;
    unsigned char value=0;
    unsigned char pva=0;
    unsigned char pvb=0;
    int leftToFill=0;
    int fillNumber=0;
    unsigned char * toFill=0;
    unsigned char sourceChar=0;
    while(cp<sourceLength) {
        
        value=sourceData[cp];
        
        if (value<33) {
            // Literal 0-32
            unsigned char count=value+1;
            for (size_t j=cp+1;j<cp+1+count;j++) {
                targetData[dp++]=sourceData[j];
            }
            cp+=(count+1);
            decompressionPrior=targetData[dp-1];
        } else if (value<128) {
            // Difference 33-127
            leftToFill=value-32;
            while(leftToFill>0) {
                fillNumber=(leftToFill<4 ? leftToFill : 4);
                sourceChar=sourceData[++cp];
                toFill = targetData+dp;
                p0=sourceChar & mask;
                sourceChar >>= 2;
                p1=sourceChar & mask;
                sourceChar >>= 2;
                p2=sourceChar & mask;
                sourceChar >>= 2;
                p3=sourceChar & mask;
                pva=(p0==3?-1:p0)+decompressionPrior;
                
                *toFill=pva;
                if (fillNumber>1) {
                    toFill++;
                    pvb=pva+(p1==3?-1:p1);
                    *toFill=pvb;
                    if (fillNumber>2) {
                        toFill++;
                        pva=(p2==3?-1:p2)+pvb;
                        *toFill=pva;
                        if (fillNumber>3) {
                            toFill++;
                            *toFill=(p3==3?-1:p3)+pva;
                        }
                    }
                }
                
                decompressionPrior = *toFill;
                dp+=fillNumber;
                leftToFill-=fillNumber;
            }
            cp++;
        } else {
            // Repeat 128-255
            unsigned char repeatCount=value-127;
            unsigned char repeatValue=sourceData[++cp];
            
            for (int j=0;j<repeatCount;j++) {
                targetData[dp++]=repeatValue;
            }
            decompressionPrior=repeatValue;
            cp++;
        }
        
    }
    return dp;
}

size_t V3DPBDReader::decompressPBD16(unsigned char * sourceData, unsigned char * targetData, size_t sourceLength)
{
    // bool debug=false;
    
    // Decompress data
    size_t cp=0;
    size_t dp=0;
    unsigned char code=0;
    int leftToFill=0;
    unsigned char sourceChar=0;
    unsigned char carryOver=0;
    uint16_t* target16Data=(uint16_t*)targetData;
    
    unsigned char d0,d1,d2,d3,d4;
    
    while(cp<sourceLength) {
        
        code=sourceData[cp];
        
        //if (debug) qDebug() << "decompressPBD16  dPos=" << decompPos << " dBuf=" << decompBuf << " decompressionPrior=" << decompressionPrior << " debugThreshold=" << debugThreshold << " cp=" << cp << " code=" << code;
        
        // Literal 0-31
        if (code<32) {
            unsigned char count=code+1;
            //if (debug) qDebug() << "decompressPBD16  literal count=" << count;
            uint16_t * initialOffset = (uint16_t*)(sourceData + cp + 1);
            for (int j=0;j<count;j++) {
                target16Data[dp++]=initialOffset[j];
                //if (debug) qDebug() << "decompressPBD16 added literal value=" << target16Data[dp-1] << " at position=" << ((decompressionPosition-decompressionBuffer) + 2*(dp-1));
            }
            cp+=(count*2+1);
            decompressionPrior=target16Data[dp-1];
            //if (debug) qDebug() << "debug: literal set decompressionPrior=" << decompressionPrior;
        }
        
        // NOTE: For the difference sections, we will unroll conditional
        // logic and explicitly go through the full cycle byte-boundary cycle.
        
        // Difference 3-bit 32-79
        else if (code<80) {
            leftToFill=code-31;
            //if (debug) qDebug() << "decompressPBD16 leftToFill start=" << leftToFill << " decompressionPrior=" << decompressionPrior;
            while(leftToFill>0) {
                
                // 332
                d0=d1=d2=d3=d4=0;
                sourceChar=sourceData[++cp];
                d0=sourceChar;
                d0 >>= 5;
                target16Data[dp++]=decompressionPrior+(d0<5?d0:4-d0);
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1] << " d0=" << d0;
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d1=sourceChar;
                d1 >>= 2;
                d1 &= ooooolll;
                target16Data[dp]=target16Data[dp-1]+(d1<5?d1:4-d1);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d2=sourceChar;
                d2 &= ooooooll;
                carryOver=d2;
                
                // 1331
                d0=d1=d2=d3=d4=0;
                sourceChar=sourceData[++cp];
                d0=sourceChar;
                carryOver <<= 1;
                d0 >>= 7;
                d0 |= carryOver;
                target16Data[dp]=target16Data[dp-1]+(d0<5?d0:4-d0);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d1=sourceChar;
                d1 >>= 4;
                d1 &= ooooolll;
                target16Data[dp]=target16Data[dp-1]+(d1<5?d1:4-d1);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d2=sourceChar;
                d2 >>= 1;
                d2 &= ooooolll;
                target16Data[dp]=target16Data[dp-1]+(d2<5?d2:4-d2);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d3=sourceChar;
                d3 &= oooooool;
                carryOver=d3;
                
                // 233
                d0=d1=d2=d3=d4=0;
                sourceChar=sourceData[++cp];
                d0=sourceChar;
                d0 >>= 6;
                carryOver <<= 2;
                d0 |= carryOver;
                target16Data[dp]=target16Data[dp-1]+(d0<5?d0:4-d0);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d1=sourceChar;
                d1 >>= 3;
                d1 &= ooooolll;
                target16Data[dp]=target16Data[dp-1]+(d1<5?d1:4-d1);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                d2=sourceChar;
                d2 &= ooooolll;
                target16Data[dp]=target16Data[dp-1]+(d2<5?d2:4-d2);
                dp++;
                //if (debug) qDebug() << "debug: position " << (dp-1) << " diff value=" << target16Data[dp-1];
                leftToFill--;
                if (leftToFill==0) {
                    break;
                }
                decompressionPrior=target16Data[dp-1];
            }
            decompressionPrior=target16Data[dp-1];
            //if (debug) qDebug() << "debug: diff set decompressionPrior=" << decompressionPrior;
            cp++;
        } else if (code<223) {
            cerr << "DEBUG: Mistakenly received unimplemented code of " << code << " at dp=" << dp << " cp=" << cp << " decompressionPrior=" << decompressionPrior << endl;
        }
        // Repeat 223-255
        else {
            unsigned char repeatCount=code-222;
            cp++;
            uint16_t repeatValue=*((uint16_t*)(sourceData + cp));
            //if (debug) qDebug() << "decompressPBD16  repeatCount=" << repeatCount << " repeatValue=" << repeatValue;
            for (int j=0;j<repeatCount;j++) {
                target16Data[dp++]=repeatValue;
            }
            decompressionPrior=repeatValue;
            //if (debug) qDebug() << "debug: repeat set decompressionPrior=" << decompressionPrior;
            cp+=2;
            //if (debug) qDebug() << "decompressPBD16  finished adding repeats at dp=" << dp << " cp=" << cp;
        }
        
    }
    return dp*2;
}


// This is the main decompression function, which is basically a wrapper for the decompression function
// of ImageLoaderBasic, except it determines the boundary of acceptable processing given the contents of the
// most recently read block. This function assumes it is called sequentially (i.e., not in parallel)
// and so doesn't have to check that the prior processing step is finished. updatedCompressionBuffer
// points to the first invalid data position, i.e., all previous positions starting with compressionBuffer
// are valid.
void V3DPBDReader::updateCompressionBuffer8(unsigned char * updatedCompressionBuffer) {
    //printf("d1\n");
    if (compressionPosition==0) {
        // Just starting
        compressionPosition=&compressionBuffer[0];
    }
    unsigned char * lookAhead=compressionPosition;
    while(lookAhead<updatedCompressionBuffer) {
        unsigned char lav=*lookAhead;
        // We will keep going until we find nonsense or reach the end of the block
        if (lav<33) {
            // Literal values - the actual number of following literal values
            // is equal to the lav+1, so that if lav==32, there are 33 following
            // literal values.
            if ( lookAhead+lav+1 < updatedCompressionBuffer ) {
                // Then we can process the whole literal section - we can move to
                // the next position
                lookAhead += (lav+2);
            } else {
                break; // leave lookAhead in current maximum position
            }
        } else if (lav<128) {
            // Difference section. The number of difference entries is equal to lav-32, so that
            // if lav==33, the minimum, there will be 1 difference entry. For a given number of
            // difference entries, there are a factor of 4 fewer compressed entries. With the
            // equation below, lav-33 will be 4 when lav==37, which is 5 entries, requiring 2 bytes, etc.
            unsigned char compressedDiffEntries=(lav-33)/4 + 1;
            if ( lookAhead+compressedDiffEntries < updatedCompressionBuffer ) {
                // We can process this section, so advance to next position to evaluate
                lookAhead += (compressedDiffEntries+1);
            } else {
                break; // leave in current max position
            }
        } else {
            // Repeat section. Number of repeats is equal to lav-127, but the very first
            // value is the value to be repeated. The total number of compressed positions
            // is always == 2
            if ( lookAhead+1 < updatedCompressionBuffer ) {
                lookAhead += 2;
            } else {
                break; // leave in current max position
            }
        }
    }
    // At this point, lookAhead is in an invalid position, which if equal to updatedCompressionBuffer
    // means the entire compressed update can be processed.
    size_t compressionLength=lookAhead-compressionPosition;
    if (decompressionPosition==0) {
        // Needs to be initialized
        decompressionPosition=decompressionBuffer;
    }
    //qDebug() << "updateCompressionBuffer calling decompressPBD compressionPosition=" << compressionPosition << " decompressionPosition=" << decompressionPosition
    //        << " size=" << compressionLength << " previousTotalDecompSize=" << getDecompressionSize() << " maxDecompSize=" << maxDecompressionSize;
    size_t dlength=decompressPBD8(compressionPosition, decompressionPosition, compressionLength);
    compressionPosition=lookAhead;
    decompressionPosition+=dlength;
    //printf("d2\n");
}


// This is the main decompression function, which is basically a wrapper for the decompression function
// of ImageLoaderBasic, except it determines the boundary of acceptable processing given the contents of the
// most recently read block. This function assumes it is called sequentially (i.e., not in parallel)
// and so doesn't have to check that the prior processing step is finished. updatedCompressionBuffer
// points to the first invalid data position, i.e., all previous positions starting with compressionBuffer
// are valid.
void V3DPBDReader::updateCompressionBuffer16(unsigned char * updatedCompressionBuffer) {
    //printf("d1\n");
    if (compressionPosition==0) {
        // Just starting
        compressionPosition=&compressionBuffer[0];
    }
    unsigned char * lookAhead=compressionPosition;
    while(lookAhead<updatedCompressionBuffer) {
        // We assume at this point that lookAhead is at a code position
        unsigned char lav=*lookAhead;
        // We will keep going until we find nonsense or reach the end of the block
        if (lav<32) {
            // Literal values - the actual number of following literal values
            // is equal to the lav+1, so that if lav==31, there are 32 following
            // literal values.
            if ( lookAhead+((lav+1)*2) < updatedCompressionBuffer ) {
                // Then we can process the whole literal section - we can move to
                // the next position
                lookAhead += ( (lav+1)*2 + 1); // +1 is for code
            } else {
                break; // leave lookAhead in current maximum position
            }
        } else if (lav<80) {
            // Difference section, 3-bit encoding.
            // The number of difference entries is equal to lav-31, so that
            // if lav==32, the minimum, there will be 1 difference entry.
            unsigned char compressedDiffBytes=int(((((lav-31)*3)*1.0)/8.0)-0.0001) + 1;
            //            int a=(lav-31)*3;
            //            double b=a*1.0;
            //            double c=b/8.0;
            //            double d=c-0.0001;
            //            int e=int(d);
            //            int f=e+1;
            //            qDebug() << "lav=" << lav << " lav-31=" << (lav-31) << " cdb=" << compressedDiffBytes;
            //            qDebug() << "a=" << a << " b=" << b << " c=" << c << " d=" << d << " e=" << e << " f=" << f;
            if ( lookAhead+compressedDiffBytes < updatedCompressionBuffer ) {
                // We can process this section, so advance to next position to evaluate
                lookAhead += (compressedDiffBytes+1); // +1 is for code
            } else {
                break; // leave in current max position
            }
        } else if (lav<183) {
            // Difference section, 4-bit encoding.
            // The number of difference entries is equal to lav-79, so that
            // if lav==80, the minimum, there will be 1 difference entry.
            unsigned char compressedDiffBytes=int(((((lav-79)*4)*1.0)/8.0)-0.0001) + 1;
            if ( lookAhead+compressedDiffBytes < updatedCompressionBuffer ) {
                // We can process this section, so advance to next position to evaluate
                lookAhead += (compressedDiffBytes+1); // +1 is for code
            } else {
                break; // leave in current max position
            }
        } else if (lav<223) {
            // Difference section, 5-bit encoding.
            // The number of difference entries is equal to lav-182, so that
            // if lav==183, the minimum, there will be 1 difference entry.
            unsigned char compressedDiffBytes=int(((((lav-182)*5)*1.0)/8.0)-0.0001) + 1;
            if ( lookAhead+compressedDiffBytes < updatedCompressionBuffer ) {
                // We can process this section, so advance to next position to evaluate
                lookAhead += (compressedDiffBytes+1); // +1 is for code
            } else {
                break; // leave in current max position
            }
        } else {
            // Repeat section. Number of repeats is equal to lav-222, but the very first
            // value is the value to be repeated. The total number of compressed positions
            // is always == 3, one for the code and 2 for the 16-bit value
            if ( lookAhead+2 < updatedCompressionBuffer ) {
                lookAhead += 3;
            } else {
                break; // leave in current max position
            }
        }
    }
    // At this point, lookAhead is in an invalid position, which if equal to updatedCompressionBuffer
    // means the entire compressed update can be processed.
    size_t compressionLength=lookAhead-compressionPosition;
    if (decompressionPosition==0) {
        // Needs to be initialized
        decompressionPosition=decompressionBuffer;
    }
    //qDebug() << "updateCompressionBuffer calling decompressPBD compressionPosition=" << compressionPosition << " decompressionPosition=" << decompressionPosition
    //        << " size=" << compressionLength << " previousTotalDecompSize=" << getDecompressionSize() << " maxDecompSize=" << maxDecompressionSize;
    size_t dlength=decompressPBD16(compressionPosition, decompressionPosition, compressionLength);
    compressionPosition=lookAhead;
    decompressionPosition+=dlength;
    //printf("d2\n");
}

void V3DPBDReader::updateCompressionBuffer3(unsigned char * updatedCompressionBuffer)
{
    bool DEBUG_SUMMARY = false;
    size_t uP=(size_t)updatedCompressionBuffer;
    //  cerr << "updateCompressionBuffer3 - updatedCompressionBuffer=" << uP << "\n";
    
    if (compressionPosition==0) {
        compressionPosition=&compressionBuffer[0];
        int channelCount=pbd_sz[3];
        cerr << "channelCount=" << channelCount << "\n";
        pbd3_source_min = new unsigned char[channelCount];
        pbd3_source_max = new unsigned char[channelCount];
        for (int i=0;i<channelCount;i++) {
            pbd3_source_min[i]=compressionBuffer[i*2];
            pbd3_source_max[i]=compressionBuffer[i*2+1];
            int iMin=pbd3_source_min[i];
            int iMax=pbd3_source_max[i];
            cerr << "Using channel " << i << " pbd3 min=" << iMin << " max=" << iMax << "\n";
        }
        compressionPosition+=(channelCount*2);
    }
    pbd3_current_min=pbd3_source_min[pbd3_current_channel];
    pbd3_current_max=pbd3_source_max[pbd3_current_channel];
    unsigned char * lookAhead=compressionPosition;
    while(lookAhead<updatedCompressionBuffer) {
        unsigned char lav=*lookAhead;
        // We will keep going until we find nonsense or reach the end of the block
        if (lav<24) {
            // Literal values - the actual number of following literal values
            // is equal to the lav+1, so that if lav==23, there are 24 following
            // literal values. For pbd3, the number of bytes encoded will be:
            int debugLiteralCount=(lav+1);
            if (DEBUG_SUMMARY) cerr << "pre: literal count=" << debugLiteralCount << "\n";
            int impliedBits=(lav+1)*3;
            unsigned char * trialPosition = 0;
            if (impliedBits % 8 == 0) {
                trialPosition = impliedBits/8 + lookAhead;
            } else {
                trialPosition = (impliedBits/8 + 1) + lookAhead;
            }
            if ( trialPosition < updatedCompressionBuffer ) {
                // Then we can process the whole literal section - we can move to next position
                lookAhead = trialPosition + 1;
            } else {
                break; // leave lookAhead in current maximum position
            }
        } else if (lav<128) {
            // Difference section. The number of difference doublets is equal to lav-22, so that
            // if lav==24, the minimum, there will be 2 difference doublets.
            int doublets=lav-22;
            if (DEBUG_SUMMARY) cerr << "pre: doublet count=" << doublets << "\n";
            int impliedBits = doublets*3;
            unsigned char * trialPosition=0;
            int dBytes=0;
            if (impliedBits % 8 == 0) {
                trialPosition = impliedBits/8 + lookAhead;
                dBytes = impliedBits/8 + 1;
            } else {
                trialPosition = (impliedBits/8 + 1) + lookAhead;
                dBytes = impliedBits/8 + 2;
            }
            //    if (DEBUG_SUMMARY) {
            //      if (doublets==31) {
            //        unsigned char * dd = lookAhead;
            //        int testDi1=*(dd+1);
            //        int testDi2=*(dd+2);
            //        int testDi3=*(dd+3);
            //        if (testDi1==32 && testDi2==38 && testDi3==16) {
            //          cerr << "=========== Found doublet 31 with first bytes 32, 38, 16\n";
            //          for (; dd < (lookAhead + dBytes); dd++) {
            //        int di=*dd;
            //        cerr << "byte = " << di << "\n";
            //          }
            //          exit(1);
            //        }
            //      }
            //    }
            if ( trialPosition < updatedCompressionBuffer ) {
                // We can process this section, so advance to next position to evaluate
                lookAhead = trialPosition+1;
            } else {
                break; // leave in current max position
            }
        } else {
            // Repeat section. Takes two bytes.
            unsigned char* trialPosition = lookAhead + 1;
            if (DEBUG_SUMMARY) {
                unsigned char key=*lookAhead;
                unsigned char value=*(lookAhead+1);
                unsigned char repeatValue=0;
                int repeatCount=pbd3GetRepeatCountFromBytes(key, value, &repeatValue);
                int rv=repeatValue;
                cerr << "pre: repeatCount=" << repeatCount << " value=" << rv << "\n";
            }
            if (trialPosition < updatedCompressionBuffer) {
                lookAhead = trialPosition + 1;
            } else {
                break; // leave in current max position
            }
        }
    }
    // At this point, lookAhead is in an invalid position, which if equal to updatedCompressionBuffer
    // means the entire compressed update can be processed.
    size_t compressionLength=lookAhead-compressionPosition;
    if (decompressionPosition==0) {
        // Needs to be initialized
        decompressionPosition=decompressionBuffer;
    }
    //qDebug() << "updateCompressionBuffer calling decompressPBD compressionPosition=" << compressionPosition << " decompressionPosition=" << decompressionPosition
    //        << " size=" << compressionLength << " previousTotalDecompSize=" << getDecompressionSize() << " maxDecompSize=" << maxDecompressionSize;
    
    if (DEBUG_SUMMARY) {
        size_t cStart=(size_t)&compressionBuffer[0];
        size_t cCurrent=(size_t)compressionPosition;
        size_t cOffset=cCurrent-cStart;
        cerr << "Compression Buffer Offset=" << cOffset << " " << std::hex << cOffset << std::dec << "\n";
    }
    
    size_t dlength=decompressPBD3(compressionPosition, decompressionPosition, compressionLength);
    compressionPosition=lookAhead;
    decompressionPosition+=dlength;
    //    if (DEBUG_SUMMARY) exit(1);
    //printf("d2\n");
}

size_t V3DPBDReader::decompressPBD3(unsigned char * sourceData, unsigned char * targetData, size_t sourceLength)
{
    bool DEBUG_SUMMARY = false;
    bool DEBUG = false;
    
    size_t cp = 0;
    size_t dp = 0;
    unsigned char value = 0;
    int sMax = pbd3_current_max;
    int sMin = pbd3_current_min;
    int sourceRange = sMax - sMin;
    unsigned char bitOffset = 0;
    unsigned char currentByte=sourceData[cp];
    
    size_t dStart = (size_t)decompressionBuffer;
    size_t tStart = (size_t)targetData;
    
    size_t i = 0L;
    
    size_t sdStart = (size_t)sourceData;
    size_t sdEnd = sdStart + sourceLength;
    
    unsigned char valueByLevel[8];
    
    for (int ii=0;ii<8;ii++) {
        valueByLevel[ii]=(unsigned char)(sMin + (sourceRange*ii)/7);
        int pv=valueByLevel[ii];
    }
    
    int iv=0;
    
    if (DEBUG_SUMMARY) cerr << "==== decompressPBD3 ========================================\n";
    
    while(cp<sourceLength) {
        
        i = dp+(tStart-dStart);
        
        //    if (i>388514000) {
        //      DEBUG=true;
        //      DEBUG_SUMMARY=true;
        //    }
        
        if (DEBUG) {
            cerr << "i=" << i << "\n";
        }
        
        if (DEBUG) {
            iv=sourceData[cp];
            cerr << "Value Pre=" << iv << "\n";
        }
        
        if (bitOffset>0) {
            cp++;
            bitOffset=0;
            if (cp>=sourceLength) {
                break;
            }
        }
        
        value=sourceData[cp++];
        
        if (DEBUG) {
            iv = value;
            cerr << "Value Post=" << iv << "\n";
        }
        
        // Debug
        int ic=0;
        
        if (value<24) {
            // Literal 0-23
            bitOffset=0; // clear
            currentByte=sourceData[cp];
            
            if (DEBUG) {
                ic=currentByte;
                cerr << " " << ic;
            }
            
            unsigned char count=value+1;
            int iCount=count;
            
            if (DEBUG_SUMMARY) {
                cerr << "decompress literal, count=" << iCount << " i=" << i << "\n";
            }
            
            int l=0;
            while (l<count) {
                if (bitOffset>7) {
                    cp++;
                    currentByte=sourceData[cp];
                    
                    if (DEBUG) {
                        ic=currentByte;
                        cerr << " " << ic;
                    }
                    
                    bitOffset-=8;
                }
                if (bitOffset<6) {
                    unsigned char mask=ooooolll;
                    mask <<= bitOffset;
                    unsigned char v=currentByte & mask;
                    v >>= bitOffset;
                    targetData[dp++]=valueByLevel[v];
                    bitOffset+=3;
                    l++;
                } else if (bitOffset==6) {
                    unsigned char nextByte=sourceData[cp+1];
                    unsigned char mask=lloooooo;
                    unsigned char v=currentByte & mask;
                    v >>= 6;
                    mask=oooooool;
                    unsigned char v2 = nextByte & mask;
                    v2 <<= 2;
                    v |= v2;
                    targetData[dp++]=valueByLevel[v];
                    bitOffset+=3;
                    l++;
                } else if (bitOffset==7) {
                    unsigned char nextByte=sourceData[cp+1];
                    unsigned char mask=looooooo;
                    unsigned char v=currentByte & mask;
                    v >>= 7;
                    mask=ooooooll;
                    unsigned char v2 = nextByte & mask;
                    v2 <<= 1;
                    v |= v2;
                    targetData[dp++]=valueByLevel[v];
                    bitOffset+=3;
                    l++;
                }
            }
            
            if (DEBUG) {
                cerr << "\n";
            }
            
            if (bitOffset>8) {
                cp++;
            }
            
        } else if (value<128) {
            // Difference 24-128
            bitOffset=0; // clear
            unsigned char previousValue=0;
            if (!(dp==0 && (tStart==dStart))) {
                for (int fl=0;fl<8;fl++) {
                    if (targetData[dp-1]==valueByLevel[fl]) {
                        previousValue=fl;
                    }
                }
            }
            //      int pvi=previousValue;
            //      cerr << "Starting pv=" << pvi << "\n";
            
            int doubletCount = value-22;
            
            if (DEBUG_SUMMARY) {
                cerr << "decompress diff, doubletCount=" << doubletCount << " i=" << i << "\n";
                int totalBytes=0;
                if ( (doubletCount*3)/8 % 8 == 0) {
                    totalBytes=(doubletCount*3)/8;
                } else {
                    totalBytes=(doubletCount*3)/8+1;
                }
                for (int j=0;j<totalBytes;j++) {
                    int jv=sourceData[cp+j];
                    cerr << "byte " << j << " = " << jv << " " << std::hex << jv << std::dec << "\n";
                }
            }
            
            unsigned char currentByte=sourceData[cp];
            
            int v2c=0;
            int vC=0;
            unsigned char v=0;
            
            for (int d=0;d<doubletCount;d++) {
                int bo=bitOffset;
                if (bitOffset>7) {
                    cp++;
                    bitOffset-=8;
                    currentByte=sourceData[cp];
                }
                int cB=currentByte;
                int nB=sourceData[cp+1];
                int bO=bitOffset;
                if (DEBUG_SUMMARY) cerr << "   currentByte=" << cB << "  nextByte=" << nB << " bitOffset=" << bO << "\n";
                if (bitOffset<6) {
                    unsigned char mask=ooooolll;
                    mask <<= bitOffset;
                    v=currentByte & mask;
                    v >>= bitOffset;
                } else if (bitOffset==6) {
                    unsigned char nextByte=sourceData[cp+1];
                    unsigned char mask=lloooooo;
                    v=currentByte & mask;
                    v >>= 6;
                    mask=oooooool;
                    unsigned char v2 = nextByte & mask;
                    v2 <<= 2;
                    v |= v2;
                } else if (bitOffset==7) {
                    unsigned char nextByte=sourceData[cp+1];
                    unsigned char mask=looooooo;
                    v=currentByte & mask;
                    v >>= 7;
                    mask=ooooooll;
                    unsigned char v2 = nextByte & mask;
                    vC=v;
                    v2 <<= 1;
                    v2c=v2;
                    unsigned char vt=v;
                    if (DEBUG_SUMMARY) cerr << " bitOffset 7 check: v2=" << v2c << " v=" << vC << "\n";
                    v = vt | v2;
                    vC=v;
                    if (DEBUG_SUMMARY) cerr << " bitOffset 7 check: v=" << vC << "\n";
                }
                
                //        targetData[dp++]=valueByLevel[0];
                //        targetData[dp++]=valueByLevel[0];
                
                int pvC=previousValue;
                if (DEBUG_SUMMARY) cerr << " previousValue= " << pvC << "\n";
                
                if (v==0) {        // 0, 0
                    if (DEBUG_SUMMARY) {
                        cerr << "0, 0\n";
                    }
                    targetData[dp++]=valueByLevel[previousValue];
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==1) { // 0, -1
                    if (DEBUG_SUMMARY) {
                        cerr << "0, -1\n";
                    }
                    targetData[dp++]=valueByLevel[previousValue];
                    previousValue--;
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==2) { // -1, 0
                    if (DEBUG_SUMMARY) {
                        cerr << "-1, 0\n";
                    }
                    previousValue--;
                    targetData[dp++]=valueByLevel[previousValue];
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==3) { // -1, 1
                    if (DEBUG_SUMMARY) {
                        cerr << "-1, 1\n";
                    }
                    previousValue--;
                    targetData[dp++]=valueByLevel[previousValue];
                    previousValue++;
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==4) { // 0, 1
                    if (DEBUG_SUMMARY) {
                        cerr << "0, 1\n";
                    }
                    targetData[dp++]=valueByLevel[previousValue];
                    previousValue++;
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==5) { // 1, 0
                    if (DEBUG_SUMMARY) {
                        cerr << "1, 0\n";
                    }
                    previousValue++;
                    targetData[dp++]=valueByLevel[previousValue];
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==6) { // 1, -1
                    if (DEBUG_SUMMARY) {
                        cerr << "1, -1\n";
                    }
                    previousValue++;
                    targetData[dp++]=valueByLevel[previousValue];
                    previousValue--;
                    targetData[dp++]=valueByLevel[previousValue];
                } else if (v==7) { // 1, 1
                    if (DEBUG_SUMMARY) {
                        cerr << "1, 1\n";
                    }
                    previousValue++;
                    targetData[dp++]=valueByLevel[previousValue];
                    previousValue++;
                    targetData[dp++]=valueByLevel[previousValue];
                } else {
                    cerr << "v should never be out of the range of 0-7\n";
                    exit(1);
                }
                
                if (previousValue>7) {
                    
                    i = dp+(tStart-dStart);
                    
                    int pv=previousValue;
                    cerr << "Previous value went out of range, =" << pv << " i=" << i << "\n";
                    exit(1);
                }
                
                bitOffset+=3;
            }
            
            if (bitOffset>8) {
                cp++;
            }
            
        } else {
            // Repeat 128-255
            unsigned char repeatValue=0;
            int repeatCount = pbd3GetRepeatCountFromBytes(value, sourceData[cp++], &repeatValue);
            size_t iCheck=dp+(tStart-dStart);
            
            if (DEBUG_SUMMARY) {
                int ik=value;
                int iv=sourceData[cp];
                cerr << "decompress repeat, count=" << repeatCount << " keyByte=" << ik << " valueByte=" << iv << " i=" << iCheck << "\n";
            }
            
            for (int ri=0;ri<repeatCount;ri++) {
                targetData[dp++]=valueByLevel[repeatValue];
            }
        }
    }
    
    i=dp+(tStart-dStart);
    
    //    if (DEBUG_SUMMARY) cerr << "Decompression size=" << i << "\n";
    
    return dp;
}

int V3DPBDReader::pbd3GetRepeatCountFromBytes(unsigned char keyByte, unsigned char valueByte, unsigned char* repeatValue) {
    // Repeat 128-255
    int ik=keyByte;
    int iv=valueByte;
    unsigned char b0=keyByte-128;
    unsigned char mask=oooooool;
    unsigned char b1=valueByte & mask;
    b1 <<= 7;
    b0 |= b1;
    mask = 30; // ooollllo
    b1 = valueByte & mask;
    b1 >>= 1;
    valueByte >>= 5;
    *repeatValue=valueByte;
    int repeatCount=0;
    unsigned char* rp = (unsigned char*)(&repeatCount);
    rp[0]=b0;
    rp[1]=b1;
    if (repeatCount<0 || repeatCount > 4095) {
        cerr << "Repeat count should not be " << repeatCount << "\n";
        return -1;
    }
    repeatCount++;
    return repeatCount;
}

char V3DPBDReader::checkMachineEndian()
{
    char e='N'; //for unknown endianness
    
    long int a=0x44332211;
    unsigned char * p = (unsigned char *)&a;
    if ((*p==0x11) && (*(p+1)==0x22) && (*(p+2)==0x33) && (*(p+3)==0x44))
        e = 'L';
    else if ((*p==0x44) && (*(p+1)==0x33) && (*(p+2)==0x22) && (*(p+3)==0x11))
        e = 'B';
    else if ((*p==0x22) && (*(p+1)==0x11) && (*(p+2)==0x44) && (*(p+3)==0x33))
        e = 'M';
    else
        e = 'N';
    
    //printf("[%c] \n", e);
    return e;
}

void V3DPBDReader::swap2bytes(void *targetp)
{
    unsigned char * tp = (unsigned char *)targetp;
    unsigned char a = *tp;
    *tp = *(tp+1);
    *(tp+1) = a;
}

void V3DPBDReader::swap4bytes(void *targetp)
{
    unsigned char * tp = (unsigned char *)targetp;
    unsigned char a = *tp;
    *tp = *(tp+3);
    *(tp+3) = *tp;
    a = *(tp+1);
    *(tp+1) = *(tp+2);
    *(tp+2) = a;
}

int V3DPBDReader::loadRaw2StackPBD(const char* filename)
{
    decompressionPrior = 0;
    int berror = 0;
    
    /* Read header */
    char formatkey[] = "v3d_volume_pkbitdf_encod";
    size_t lenkey = strlen(formatkey);
    
    FILE* fid = fopen(filename, "rb");
    if (!fid)
        return -1;
    fseek(fid, 0, SEEK_END);
    size_t fileSize = ftell(fid);
    rewind(fid);
    
#ifndef _MSC_VER //added by PHC, 2010-05-21
    if (fileSize<lenkey+2+4*4+1) // datatype has 2 bytes, and sz has 4*4 bytes and endian flag has 1 byte.
    {
        stringstream msg;
        msg << "The size of your input file is too small and is not correct, -- it is too small to contain the legal header.\n";
        msg << "The fseek-ftell produces a file size = " << fileSize;
        return -1;
    }
#endif
    
    keyread.resize(lenkey+1);
    size_t nread = fread(&keyread[0], 1, lenkey, fid);
    if (nread!=lenkey)
    {
        return -1;
        //return exitWithError(std::string("File unrecognized or corrupted file."));
    }
    keyread[lenkey] = '\0';
    
    size_t i;
    if (strcmp(formatkey, &keyread[0])) /* is non-zero then the two strings are different */
    {
        return -1;
        //return exitWithError("Unrecognized file format.");
    }
    
    char endianCodeData;
    fread(&endianCodeData, 1, 1, fid);
    if (endianCodeData!='B' && endianCodeData!='L')
    {
        return -1;
        //return exitWithError("This program only supports big- or little- endian but not other format. Check your data endian.");
    }
    
    char endianCodeMachine;
    endianCodeMachine = checkMachineEndian();
    if (endianCodeMachine!='B' && endianCodeMachine!='L')
    {
        return -1;
        //return exitWithError("This program only supports big- or little- endian but not other format. Check your data endian.");
    }
    
    int b_swap = (endianCodeMachine==endianCodeData)?0:1;
    
    short int dcode = 0;
    fread(&dcode, 2, 1, fid); /* because I have already checked the file size to be bigger than the header, no need to check the number of actual bytes read. */
    if (b_swap)
        swap2bytes((void *)&dcode);
    
    int datatype;
    
    switch (dcode)
    {
        case PBD_3_BIT_DTYPE:
            datatype = PBD_3_BIT_DTYPE; // used for pbd3
            break;
            
        case 1:
            datatype = 1; /* temporarily I use the same number, which indicates the number of bytes for each data point (pixel). This can be extended in the future. */
            break;
            
        case 2:
            datatype = 2;
            break;
            
        case 4:
            datatype = 4;
            break;
            
        default:
            stringstream msg;
            msg << "Unrecognized data type code [";
            msg << dcode;
            msg << "The file type is incorrect or this code is not supported in this version.";
            return -1;
    }
    
    // qDebug() << "Setting datatype=" << datatype;
    
    if (datatype==1 || datatype==PBD_3_BIT_DTYPE) {
        m_bd = 8;
    } else if (datatype==2) {
        m_bd = 16;
    } else {
        return -1;
        //return exitWithError("ImageLoader::loadRaw2StackPBD : only datatype=1 or datatype=2 supported");
    }
    loadDatatype = datatype; // used for threaded loading
    
    // qDebug() << "Finished setting datatype=" << image->getDatatype();
    
    size_t unitSize = datatype; // temporarily I use the same number, which indicates the number of bytes for each data point (pixel). This can be extended in the future.
    
    int32_t mysz[4];
    mysz[0]=mysz[1]=mysz[2]=mysz[3]=0;
    int tmpn=fread(mysz, 4, 4, fid); // because I have already checked the file size to be bigger than the header, no need to check the number of actual bytes read.
    //int tmpn=fileStream.read((char*)mysz, 16); // because I have already checked the file size to be bigger than the header, no need to check the number of actual bytes read.
    if (tmpn!=16)
    {
        stringstream msg;
        msg << "This program only reads [" << tmpn << "] units.";
        //return exitWithError(msg.str());
        return -1;
    }
    
    if (b_swap && (unitSize==2 || unitSize==4)) {
        stringstream msg;
        msg << "b_swap true and unitSize > 1 - this is not implemented in current code";
        //return exitWithError(msg.str());
        return -1;
    }
    
    if (b_swap)
    {
        for (i=0;i<4;i++)
        {
            //swap2bytes((void *)(mysz+i));
            printf("mysz raw read unit[%ld]: [%d] ", i, mysz[i]);
            swap4bytes((void *)(mysz+i));
            printf("swap unit: [%d][%0x] \n", mysz[i], mysz[i]);
        }
    }
    
    std::vector<size_t> sz(4, 0); // avoid memory leak
    // V3DLONG * sz = new V3DLONG [4]; // reallocate the memory if the input parameter is non-null. Note that this requests the input is also an NULL point, the same to img.
    
    size_t totalUnit = 1;
    for (i=0;i<4;i++)
    {
        sz[i] = (size_t)mysz[i];
        pbd_sz[i]=sz[i];
        cerr << "Set pbd_sz " << i << " to " << pbd_sz[i] << "\n";
        totalUnit *= sz[i];
    }
    
    //mexPrintf("The input file has a size [%ld bytes], different from what specified in the header [%ld bytes]. Exit.\n", fileSize, totalUnit*unitSize+4*4+2+1+lenkey);
    //mexPrintf("The read sizes are: %ld %ld %ld %ld\n", sz[0], sz[1], sz[2], sz[3]);
    
    size_t headerSize=4*4+2+1+lenkey;
    size_t compressedBytes=fileSize-headerSize;
    maxDecompressionSize=totalUnit*unitSize;
    channelLength=sz[0]*sz[1]*sz[2];
    compressionBuffer.resize(compressedBytes);
    pbd3_current_channel=0;
    
    size_t remainingBytes = compressedBytes;
    //V3DLONG nBytes2G = V3DLONG(1024)*V3DLONG(1024)*V3DLONG(1024)*V3DLONG(2);
    size_t readStepSizeBytes = size_t(1024)*20000; // PREVIOUS
    totalReadBytes = 0;
    
    // done reading header
    // Transfer data to My4DImage
    
    m_x_size = sz[0];
    m_y_size = sz[1];
    m_slice_num = sz[2];
    m_chan_num = sz[3];
    unsigned long long mem_size = (unsigned long long)m_x_size*(unsigned long long)m_y_size*(unsigned long long)m_slice_num*(unsigned long long)m_chan_num*(unsigned long long)(m_bd/8);
    unsigned char *val = new (std::nothrow) unsigned char[mem_size];
    
    // Allocating memory can take seconds.  So send a message
    short int blankImageDataType=datatype;
    if (datatype==PBD_3_BIT_DTYPE) {
        blankImageDataType=1;
    }
    if (decompressionBuffer)
        delete [] decompressionBuffer;
    decompressionBuffer = val;
    
    while (remainingBytes>0)
    {
        size_t curReadBytes = (remainingBytes<readStepSizeBytes) ? remainingBytes : readStepSizeBytes;
        pbd3_current_channel=totalReadBytes/channelLength;
        cerr << "pbd3_current_channel " << pbd3_current_channel << "\n";
        size_t bytesToChannelBoundary=(pbd3_current_channel+1)*channelLength - totalReadBytes;
        curReadBytes = (curReadBytes>bytesToChannelBoundary) ? bytesToChannelBoundary : curReadBytes;
        nread = fread(&compressionBuffer[0]+totalReadBytes, 1, curReadBytes, fid);
        totalReadBytes+=nread;
        
        cerr << "nread=" << nread << " curReadBytes=" << curReadBytes << " bytesToChannelBoundary=" << bytesToChannelBoundary << " totalReadBytes=" << totalReadBytes << "\n";
        
        if (nread!=curReadBytes)
        {
            stringstream msg;
            msg << "Something wrong in file reading. The program reads [";
            msg << nread << " data points] but the file says there should be [";
            msg << curReadBytes << " data points].";
            return -1;
        }
        remainingBytes -= nread;
        
        if (datatype==1) {
            updateCompressionBuffer8(&compressionBuffer[0]+totalReadBytes);
        } else if (datatype==PBD_3_BIT_DTYPE) {
            updateCompressionBuffer3(&compressionBuffer[0]+totalReadBytes);
        } else {
            // assume datatype==2
            updateCompressionBuffer16(&compressionBuffer[0]+totalReadBytes);
        }
    }
    // qDebug() << "ImageLoader::loadRaw2StackPBD" << filename << stopwatch.elapsed() << __FILE__ << __LINE__;
    // qDebug() << "Total time elapsed after all reads is " << stopwatch.elapsed() / 1000.0 << " seconds";
    
    // clean and return
    // fclose(fid); //bug fix on 060412
    
    return berror;
}
