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
//#include "DataManager.h"
//#include <FLIVR/KernelProgram.h>
//#include <FLIVR/VolKernel.h>
//#include "DLLExport.h"
//
//#ifndef _KERNELEXECUTOR_H_
//#define _KERNELEXECUTOR_H_
//
//class EXPORT_API KernelExecutor
//{
//public:
//	KernelExecutor();
//	~KernelExecutor();
//
//	void SetCode(wxString &code);
//	void LoadCode(wxString &filename);
//	void SetVolume(VolumeData *vd);
//	void SetDuplicate(bool dup);
//	VolumeData* GetVolume();
//	VolumeData* GetResult();
//	void DeleteResult();
//	bool GetMessage(wxString &msg);
//
//	bool Execute();
//
//private:
//	VolumeData *m_vd;
//	VolumeData *m_vd_r;//result
//	bool m_duplicate;//whether duplicate the input volume
//
//	wxString m_code;
//	wxString m_message;
//
//	bool ExecuteKernel(KernelProgram* kernel,
//		GLuint data_id, void* result,
//		size_t brick_x, size_t brick_y,
//		size_t brick_z);
//
//};
//
//#endif//_KERNELEXECUTOR_H_
