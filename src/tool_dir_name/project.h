
#include <iostream>
#include <string>
#include <vector>


#include <iostream>


enum open_mode
{
	READ = 1,	// 目前仅支持读,不支持写和改
	// WRITE = 2,
	APP_WRITE = 3,
	TRUNC_WRITE = 4

};


typedef char mmap_cur;

/// <summary>  
/// 内存文件映射功能.
/// 同一个资源文件，不能同时读和写，否则指向文件的指针会乱掉，导致程序异常奔溃。
/// 参照: https://blog.csdn.net/baidu_38172402/article/details/106673606
/// https://zhuanlan.zhihu.com/p/477641987
/// </summary>

class silly_mmap
{

public:
	silly_mmap() = default;
	silly_mmap(const std::string);
	~silly_mmap();

	bool open_m(const std::string& file, const int mode = open_mode::READ);

	mmap_cur* at(const size_t& offset = 0);

	bool read(mmap_cur* dst, const size_t& size, const size_t& offset = 0);

	bool write(mmap_cur* src, const size_t& size, const size_t& offset = 0);

	void close_m();

	// add
	size_t get_file_size()
	{
		return m_size;
	}


private:
	size_t m_size{ 0 }; 
	std::string m_file;
	mmap_cur* m_mmap{ nullptr };

};







