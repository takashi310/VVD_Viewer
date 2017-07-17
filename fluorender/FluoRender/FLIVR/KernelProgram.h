#ifndef KernelProgram_h
#define KernelProgram_h

#include <GL/glew.h>
#ifdef _WIN32
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif
#ifdef _DARWIN
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl.h>
#include <OpenCL/cl_gl_ext.h>
#include <OpenGL/CGLCurrent.h>
#endif
#include <string>
#include <vector>
#include <map>

#include "DLLExport.h"

namespace FLIVR
{
	class VolKernel;
	class EXPORT_API KernelProgram
	{
	public:
		KernelProgram(const std::string& source);
		~KernelProgram();

		bool create(std::string &name);
		bool valid(std::string name=std::string());
		void destroy();

		void execute(cl_uint, size_t*, size_t*, std::string name=std::string());

		typedef struct
		{
			cl_uint index;
			size_t size;
			GLuint texture;
			cl_mem buffer;
			void *buf_src;
		} Argument;
		bool matchArg(Argument*, unsigned int&);
		bool delBuf(void*);
		bool delTex(GLuint);
		void setKernelArgConst(int, size_t, void*, std::string name=std::string());
		void setKernelArgBuf(int, cl_mem_flags, size_t, void*, std::string name=std::string());
		void setKernelArgBufWrite(int, cl_mem_flags, size_t, void*, std::string name=std::string());
		void setKernelArgTex2D(int, cl_mem_flags, GLuint, std::string name=std::string());
		void setKernelArgTex3D(int, cl_mem_flags, GLuint, std::string name=std::string());
		void readBuffer(int, void*);
		void readBuffer(void*);
		void writeBuffer(int, void*, size_t, size_t, size_t);
		void writeBuffer(void *buf_ptr, void* pattern, size_t pattern_size, size_t offset, size_t size);

		//initialization
		static void init_kernels_supported();
		static bool init();
		static void clear();
		static void set_device_id(int id);
		static int get_device_id();
		static std::string& get_device_name();

		//info
		std::string &getInfo();

		friend class VolKernel;
#ifdef _DARWIN
        static CGLContextObj gl_context_;
#endif
	protected:
		std::string source_;
		cl_program program_;
		std::map<std::string, cl_kernel> kernel_;
		cl_command_queue queue_;

		std::string info_;

		//memory object to release
		std::vector<Argument> arg_list_;

		static bool init_;
		static cl_device_id device_;
		static cl_context context_;
		static int device_id_;
		static std::string device_name_;
	};
}

#endif