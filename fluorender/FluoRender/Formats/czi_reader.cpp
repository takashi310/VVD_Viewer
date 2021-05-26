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
#include <stdio.h>
#include "../compatibility.h"
#include "czi_reader.h"
#include <libCZI.h>
#include <sstream>
#include <tinyxml2.h>

//#include <fstream>
//#include <bitset>

using namespace libCZI;

CZIReader::CZIReader()
{
    m_time_num = 0;
    m_cur_time = -1;
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
    
    m_datatype = 0;
}

CZIReader::~CZIReader()
{
}

void CZIReader::SetFile(string &file)
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

void CZIReader::SetFile(wstring &file)
{
    m_path_name = file;
    m_id_string = m_path_name;
}

void CZIReader::Preprocess()
{
    FILE* pfile = 0;
    auto stream = libCZI::CreateStreamFromFile(m_path_name.c_str());
    auto cziReader = libCZI::CreateCZIReader();
    cziReader->Open(stream);
    
    auto stats = cziReader->GetStatistics();
    m_slice_min = 0;
    m_slice_num = 1;
    m_chan_min = 0;
    m_chan_num = 1;
    m_time_min = 0;
    m_time_num = 1;
    stats.dimBounds.EnumValidDimensions(
        [this](libCZI::DimensionIndex idx, int start, int size)
        {
            cout << "Index " << libCZI::Utils::DimensionToChar(idx) << " start: " << start << " size: " << size << endl;
            switch(idx)
            {
                case libCZI::DimensionIndex::Z:
                    m_slice_min = start;
                    m_slice_num = size;
                    break;
                case libCZI::DimensionIndex::C:
                    m_chan_min = start;
                    m_chan_num = size;
                    break;
                case libCZI::DimensionIndex::T:
                    m_time_min = start;
                    m_time_num = size;
                    break;
                default:
                    break;
            }
            
            return true;
        });
    
    m_x_min = stats.boundingBox.x;
    m_y_min = stats.boundingBox.y;
    m_x_size = stats.boundingBox.w;
    m_y_size = stats.boundingBox.h;
    
    auto pxtype = libCZI::Utils::TryDeterminePixelTypeForChannel(cziReader.get(), m_chan_min);
    
    switch(pxtype)
    {
        case libCZI::PixelType::Gray8:
            m_datatype = 1;
            m_max_value = 255.0;
            break;
        case libCZI::PixelType::Gray16:
            m_datatype = 2;
            m_max_value = 65535.0;
            break;
        default:
            m_datatype = 0;
            m_error_msg = L"CZI file reader: Unsupported pixel format ";
            m_error_msg += s2ws(libCZI::Utils::PixelTypeToInformalString(pxtype));
            return;
    }
    
    auto meta = cziReader->ReadMetadataSegment();
    auto md = meta->CreateMetaFromMetadataSegment();
    auto docInfo = md->GetDocumentInfo();
    auto mscale = docInfo->GetScalingInfo();
    
    m_xspc = mscale.scaleX != std::numeric_limits<double>::quiet_NaN() ? mscale.scaleX * 1e8 : 1.0;
    m_yspc = mscale.scaleY != std::numeric_limits<double>::quiet_NaN() ? mscale.scaleY * 1e8 : 1.0;
    m_zspc = mscale.scaleZ != std::numeric_limits<double>::quiet_NaN() ? mscale.scaleZ * 1e8 : 1.0;
    if ( mscale.scaleX != std::numeric_limits<double>::quiet_NaN() &&
         mscale.scaleY != std::numeric_limits<double>::quiet_NaN() &&
        (mscale.scaleZ != std::numeric_limits<double>::quiet_NaN() || m_slice_num == 1 ))
        m_valid_spc = true;
    else
        m_valid_spc = false;
    
    //auto meta_xml = md->GetXml();
    
    cout << "Z " << m_slice_min << " " << m_slice_num << " " << stats.boundingBox << " " << libCZI::Utils::PixelTypeToInformalString(pxtype) << endl;
    //cout << meta_xml << endl;
    
    m_cur_time = 0;
    m_data_name = m_path_name.substr(m_path_name.find_last_of(GETSLASH())+1);
    
    SetInfo();
}

void CZIReader::SetSliceSeq(bool ss)
{
    //do nothing
}

bool CZIReader::GetSliceSeq()
{
    return false;
}

void CZIReader::SetTimeSeq(bool ss)
{
    //do nothing
}

bool CZIReader::GetTimeSeq()
{
    return false;
}

void CZIReader::SetTimeId(wstring &id)
{
    //do nothing
}

wstring CZIReader::GetTimeId()
{
    return wstring(L"");
}

void CZIReader::SetBatch(bool batch)
{
    if (batch)
    {
        //read the directory info
        FIND_FILES(m_path_name,L".czi",m_batch_list,m_cur_batch);
        m_batch = true;
    }
    else
        m_batch = false;
}

int CZIReader::LoadBatch(int index)
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

double CZIReader::GetExcitationWavelength(int chan)
{
    for (int i=0; i<(int)m_excitation_wavelength_list.size(); i++)
    {
        if (m_excitation_wavelength_list[i].chan_num == chan)
            return m_excitation_wavelength_list[i].wavelength;
    }
    return 0.0;
}

Nrrd* CZIReader::ConvertNrrd(int t, int c, bool get_max)
{
    Nrrd* output = Convert_ThreadSafe(t, c, get_max);
    size_t i;
    size_t voxelnum = m_x_size * m_y_size * m_slice_num;
    
    if (output && get_max)
    {
        m_max_value = 0.0;
        unsigned short n;
        if (output->type == nrrdTypeUShort) {
            for (i = 0; i < voxelnum; i++) {
                n =  ((unsigned short*)output->data)[i];
                m_max_value = (n > m_max_value)?n:m_max_value;
            }
        }
        if (output->type == nrrdTypeUChar)
        {
            //8 bit
            m_max_value = 255.0;
            m_scalar_scale = 1.0;
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
    }
    
    return output;
}

Nrrd* CZIReader::Convert_ThreadSafe(int t, int c, bool get_max)
{
    Nrrd *data = 0;
    
    if (t>=0 && t<m_time_num &&
        c>=0 && c<m_chan_num &&
        m_slice_num > 0 &&
        m_x_size > 0 &&
        m_y_size > 0)
    {
        //allocate memory for nrrd
        switch (m_datatype)
        {
            case 1://8-bit
            {
                unsigned long long mem_size = (unsigned long long)m_x_size*(unsigned long long)m_y_size*(unsigned long long)m_slice_num;
                unsigned char *val = new (std::nothrow) unsigned char[mem_size];
                
                auto stream = libCZI::CreateStreamFromFile(m_path_name.c_str());
                auto cziReader = libCZI::CreateCZIReader();
                cziReader->Open(stream);
                auto accessor = cziReader->CreateSingleChannelTileAccessor();
                
                auto slice_bd = libCZI::IntRect{m_x_min, m_y_min, m_x_size, m_y_size};
                
                libCZI::CDimCoordinate planeCoord{
                    { libCZI::DimensionIndex::Z, m_slice_min },
                    { libCZI::DimensionIndex::C, c+m_chan_min },
                    { libCZI::DimensionIndex::T, t+m_time_min }
                };
                
                int slice_max = m_slice_min + m_slice_num;
                size_t slice_size = (unsigned long long)m_x_size * (unsigned long long)m_y_size;
                for (size_t i = m_slice_min; i < slice_max; i++)
                {
                    planeCoord.Set(libCZI::DimensionIndex::Z, i);
                    auto multiTileComposit = accessor->Get(libCZI::PixelType::Gray8, slice_bd, &planeCoord, nullptr);
                    auto lockinfo = multiTileComposit->Lock();
                    memcpy(val + i * slice_size, lockinfo.ptrDataRoi, lockinfo.size < slice_size ? lockinfo.size : slice_size);
                }
                
                //create nrrd
                data = nrrdNew();
                nrrdWrap(data, val, nrrdTypeUChar, 3, (size_t)m_x_size, (size_t)m_y_size, (size_t)m_slice_num);
                nrrdAxisInfoSet(data, nrrdAxisInfoSpacing, m_xspc, m_yspc, m_zspc);
                nrrdAxisInfoSet(data, nrrdAxisInfoMax, m_xspc*m_x_size, m_yspc*m_y_size, m_zspc*m_slice_num);
                nrrdAxisInfoSet(data, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
                nrrdAxisInfoSet(data, nrrdAxisInfoSize, (size_t)m_x_size, (size_t)m_y_size, (size_t)m_slice_num);
            }
                break;
            case 2://16-bit
            {
                unsigned long long mem_size = (unsigned long long)m_x_size*(unsigned long long)m_y_size*(unsigned long long)m_slice_num;
                unsigned short *val = new (std::nothrow) unsigned short[mem_size];
                
                auto stream = libCZI::CreateStreamFromFile(m_path_name.c_str());
                auto cziReader = libCZI::CreateCZIReader();
                cziReader->Open(stream);
                auto accessor = cziReader->CreateSingleChannelTileAccessor();
                
                auto slice_bd = libCZI::IntRect{m_x_min, m_y_min, m_x_size, m_y_size};
                
                libCZI::CDimCoordinate planeCoord{
                    { libCZI::DimensionIndex::Z, m_slice_min },
                    { libCZI::DimensionIndex::C, c+m_chan_min },
                    { libCZI::DimensionIndex::T, t+m_time_min }
                };
                
                int slice_max = m_slice_min + m_slice_num;
                size_t slice_size = (unsigned long long)m_x_size * (unsigned long long)m_y_size * 2ULL;
                for (size_t i = m_slice_min; i < slice_max; i++)
                {
                    planeCoord.Set(libCZI::DimensionIndex::Z, i);
                    auto multiTileComposit = accessor->Get(libCZI::PixelType::Gray16, slice_bd, &planeCoord, nullptr);
                    auto lockinfo = multiTileComposit->Lock();
                    memcpy((unsigned char *)val + i * slice_size, lockinfo.ptrDataRoi, lockinfo.size < slice_size ? lockinfo.size : slice_size);
                }
                
                //create nrrd
                data = nrrdNew();
                nrrdWrap(data, val, nrrdTypeUShort, 3, (size_t)m_x_size, (size_t)m_y_size, (size_t)m_slice_num);
                nrrdAxisInfoSet(data, nrrdAxisInfoSpacing, m_xspc, m_yspc, m_zspc);
                nrrdAxisInfoSet(data, nrrdAxisInfoMax, m_xspc*m_x_size, m_yspc*m_y_size, m_zspc*m_slice_num);
                nrrdAxisInfoSet(data, nrrdAxisInfoMin, 0.0, 0.0, 0.0);
                nrrdAxisInfoSet(data, nrrdAxisInfoSize, (size_t)m_x_size, (size_t)m_y_size, (size_t)m_slice_num);
            }
                break;
            default:
                break;
        }
    }
    
    return data;
}

wstring CZIReader::GetCurName(int t, int c)
{
    return wstring(L"");
}

void CZIReader::SetInfo()
{
    wstringstream wss;
    
    wss << L"------------------------\n";
    wss << m_path_name << '\n';
    wss << L"File type: LSM\n";
    wss << L"Width: " << m_x_size << L'\n';
    wss << L"Height: " << m_y_size << L'\n';
    wss << L"Depth: " << m_slice_num << L'\n';
    wss << L"Channels: " << m_chan_num << L'\n';
    wss << L"Frames: " << m_time_num << L'\n';
    
    m_info = wss.str();
}

