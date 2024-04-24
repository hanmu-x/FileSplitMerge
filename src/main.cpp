
//#include "tool_dir_name/project.h"
//#include "tool_dir_name/config.hpp"

#include <boost/timer.hpp> // 计时函数

#include <filesystem>
#include <fstream>
#include <vector>
#include <regex>
#include <set>
#include <iostream>

// 分隔后文件夹的格式, 原文件名_chunk
#define TZX_FILE_SUFFIX "_chunk"
#define TZX_CHUNK_NAME "%s.%d"
#define TZX_MERGE_NAME  "_merge"

#define SPLIT_JSON "split_info.json"
#define SRC_FILE_NAME "source_file_name"
#define SRC_FILE_SIZE "source_file_size"
#define SPLIT_FILES "split_files"
#define SPLIT_FILE_NUM "split_file_number"
#define SPLIT_FILE "file"

    // 定义正则表达式
std::regex pattern("([a-zA-Z]+)\\.([0-9]+)");

////////////////////////////////////////////////////////////////////////////////////////////

struct FileHeader
{
    unsigned long long totalFileSize{ 0 };      //原始文件的总大小
    unsigned long long currentFileSize{ 0 };    //当前区块文件的大小
    unsigned int currentFileIndex{ 0 };         //当前区块文件的索引
    unsigned int totalFiles{ 0 };               //区块文件总数 
    char extension[20];                         //文件扩展名
};
static unsigned int size_head = sizeof(FileHeader) * 8;

 // 测试完成
/// <summary>
/// 分隔文件函数,将一个文件按照规定的大小分割为多个小块
/// </summary>
/// <param name="inputFile">被分割的文件</param>
/// <param name="chunkSize">分割后每块文件的大小(单位字节): 如果要分割为500mb每块,该数值为 500 * 1024 * 1024 </param>
/// <param name="inputFiles">切分后后的所有文件名</param>
/// <returns></returns>
bool splitFile(const std::string& inputFile, const size_t chunkSize)
{
    std::string inputFileName = std::filesystem::path(inputFile).filename().stem().string();    // 文件名
    std::string fileExtension = std::filesystem::path(inputFile).extension().string();        // 获取文件扩展名
    std::string dirname = inputFileName + TZX_FILE_SUFFIX;
    std::filesystem::path chunkDir = std::filesystem::path(inputFile).parent_path();
    chunkDir.append(dirname);
    if (!std::filesystem::exists(chunkDir))
    {
        std::filesystem::create_directories(chunkDir);
    }

    std::ifstream inputFileStream(inputFile, std::ios::binary | std::ios::ate);
    if (!inputFileStream.is_open())
    {
        std::cout << "Error opening file: " << inputFile << std::endl;
        return false;
    }

    size_t fileSize = static_cast<size_t>(inputFileStream.tellg());
    inputFileStream.seekg(0, std::ios::beg);
    //如果文件大小小于
    size_t numberOfChunks = (fileSize + chunkSize - 1) / chunkSize;
    for (size_t i = 0; i < numberOfChunks; ++i)
    {
        size_t chunkOffset = i * chunkSize;
        size_t chunkSizeActual = std::min(chunkSize, fileSize - chunkOffset);
        FileHeader header;
        header.totalFileSize = fileSize;
        header.currentFileSize = chunkSizeActual;
        header.currentFileIndex = static_cast<unsigned int>(i + 1);
        header.totalFiles = static_cast<unsigned int>(numberOfChunks);
        strcpy(header.extension, fileExtension.c_str());

        char buff[256];
        sprintf(buff, TZX_CHUNK_NAME, inputFileName.c_str(), static_cast<int>(i+1));
        std::filesystem::path tempChunkDir = chunkDir;
        std::string chunkFileName = tempChunkDir.append(buff).string();

        char* buffer = new char[chunkSizeActual + size_head];
        // 移动文件读取指针到当前块的起始位置
        inputFileStream.seekg(chunkOffset, std::ios::beg);
        inputFileStream.read(buffer, chunkSizeActual);
        std::ofstream chunkFile(chunkFileName, std::ios::binary);
        if (!chunkFile.is_open())
        {
            std::cout << "Error creating chunk file: " << chunkFileName << std::endl;
            delete[] buffer;
            return false;
        }
        chunkFile.write(reinterpret_cast<const char*>(&header), size_head);
        chunkFile.write(buffer, chunkSizeActual);
        chunkFile.close();
        delete[] buffer;
        std::cout << "save split: " << chunkFileName << std::endl;
    }

    inputFileStream.close();
    return true;
}

bool verifyFileHeaders(const std::vector<FileHeader>& fileHeaders)
{
    if (fileHeaders.empty())
    {
        std::cout << "fileHeaders is empty " << std::endl;
        return false;
    }
    // 使用集合来跟踪已经遇到的currentFileIndex
    std::set<unsigned int> seenIndices;
    for (const auto& header : fileHeaders)
    {
        // 检查currentFileIndex是否在期望范围内
        if (header.currentFileIndex < 1 || header.currentFileIndex > header.totalFiles)
        {
            std::cout << "Invalid currentFileIndex: " << header.currentFileIndex << std::endl;
            return false;
        }
        // 检查currentFileIndex是否重复
        if (!seenIndices.insert(header.currentFileIndex).second)
        {
            std::cout << "Duplicate currentFileIndex: " << header.currentFileIndex << std::endl;
            return false;
        }
    }
    // 检查是否每个currentFileIndex都出现过
    if (seenIndices.size() != fileHeaders.back().totalFiles)
    {
        std::cout << "Not all currentFileIndex values are present." << std::endl;
        return false;
    }
    return true;
}

bool mergeFiles(const std::string& inputDirectory, std::string outputFileName = "")
{
    bool status = false;
    std::vector<std::string> inputFiles;
    std::string chunkDirName;
    std::vector<FileHeader> FileHeaders;
    if (!std::filesystem::exists(inputDirectory) || !std::filesystem::is_directory(inputDirectory))   //目录不存在或者不是目录
    {
        std::cout << "Directory does not exist or is not a directory: " << inputDirectory << std::endl;
        return status;
    }
    // 遍历输入目录中的文件
    for (const auto& entry : std::filesystem::directory_iterator(inputDirectory))
    {
        if (entry.is_regular_file())  // 是文件而不是目录 
        {
            std::string oneName = entry.path().filename().string();  // 文件名
            std::smatch matches;
            // 匹配:
            if (std::regex_match(oneName, matches, pattern))
            {
                //// 如果匹配成功，输出字符和数字
                inputFiles.push_back(entry.path().string());
            }
        }
    }
    if (inputFiles.empty())  // 匹配文件为空
    {
        std::cout << "Directory matching file is empty: " << inputDirectory << std::endl;
        return status;
    }
    for (const auto& inputFile : inputFiles)  // 读取所有文件的头部信息,检验文件是否全
    {
        std::ifstream inputFileStream(inputFile, std::ios::binary);
        if (!inputFileStream.is_open())
        {
            std::cout << "Error opening input file: " << inputFile << std::endl;
            return status;
        }
        FileHeader header;
        inputFileStream.read(reinterpret_cast<char*>(&header), size_head);
        FileHeaders.push_back(header);
        inputFileStream.close();
    }
    if (!verifyFileHeaders(FileHeaders))    // 检测所有文件的头部信息看文件是否有缺失
    {
        std::cout << "Do something else if the verification fails " << std::endl;
        return status;
    }
    if (outputFileName.empty())     // 合并文件为空
    {
        std::filesystem::path chunkDir = std::filesystem::path(inputDirectory);
        chunkDirName = std::filesystem::path(inputDirectory).filename().string();    // 文件名
        chunkDirName = chunkDirName + TZX_MERGE_NAME + FileHeaders.begin()->extension;  // 扩展名
        outputFileName = chunkDir.append(chunkDirName).string();
    }
    std::ofstream outputFile(outputFileName, std::ios::binary);
    if (!outputFile.is_open())
    {
        std::cout << "Error creating output file: " << outputFileName << std::endl;
        return status;
    }
    // 输入文件
    for (const auto& inputFile : inputFiles)
    {
        std::ifstream inputFileStream(inputFile, std::ios::binary);
        if (!inputFileStream.is_open())
        {
            std::cout << "Error opening input file: " << inputFile << std::endl;
            return status;
        }
        inputFileStream.seekg(size_head, std::ios::beg);
        outputFile << inputFileStream.rdbuf();
        inputFileStream.close();
        std::cout << "merge: " << inputFile << std::endl;
    }
    outputFile.close();
    std::cout << "Successed merge: " << outputFileName << std::endl;
    status = true;
    return status;
}


int main() 
{
    size_t chunkSize = 200 * 1024 * 1024;  // 100 MB

    std::string mergedFileName = "E:/11_ReadFile/123.tif";

    boost::timer tm1; // 定义后计时开始
    tm1.restart();  // 从新从这里开始计时

    std::string inputFile = "D:/1_wangyingjie/readfile/split/ETOPO.tif";
    std::string outputDir = "D:/1_wangyingjie/readfile/split/ETOPO_chunk";

    std::vector<std::string> inputFiles;
    bool sp = splitFile(inputFile, chunkSize);  // 拆分
    if (sp)
    {
        std::cout << "splitFile successed: " << inputFile << std::endl;
    }
    else
    {
        std::cout << "splitFile failed: " << inputFile << std::endl;
    }



    tm1.restart();  // 从新从这里开始计时
    //bool me = mergeFiles(outputDir, mergedFileName);
    bool me = mergeFiles(outputDir);  // 合并
    if (me)
    {
        std::cout << "mergeFiles successed" << std::endl;
    }
    else
    {
        std::cout << "mergeFiles failed" << std::endl;
    }
    std::cout << tm1.elapsed() << std::endl;  // 单位是秒
    return 0;
}






/// 生产json文件测试 ##################################################
//struct SplitInfo
//{
//    std::string sourceFileName;
//    unsigned long long sourceFileSize;
//    std::vector<std::string> splitFiles;
//    int splitFileNumber;
//};
//bool readSplitInfoFromJSON(const std::string jsonfile, SplitInfo& splitInfo)
//{
//    //std::string jsonFilePath = directory + "/split_info.json"; // JSON 文件路径
//    //std::filesystem::path jsonFilePath(directory);
//    //jsonFilePath.append(SPLIT_JSON);
//
//    // 尝试打开 JSON 文件
//    std::ifstream jsonFile;
//    jsonFile.open(jsonfile, std::ios::binary | std::ios::in);
//    if (!jsonFile.is_open())
//    {
//        std::cout << "Error opening JSON file: " << jsonfile << std::endl;
//        return false;
//    }
//
//    Json::Reader reader;
//    Json::Value root;
//    if (reader.parse(jsonFile, root))
//    {
//        splitInfo.sourceFileName = root[SRC_FILE_NAME].asString();
//        splitInfo.sourceFileSize = root[SRC_FILE_SIZE].asUInt64();
//        int i = 0;
//        Json::Value splitFileNameJson = root[SPLIT_FILES];
//        for (Json::ValueIterator it = splitFileNameJson.begin(); it != splitFileNameJson.end(); ++it)
//        {
//            splitInfo.splitFiles.push_back((*it)[SPLIT_FILE].asString());
//        }
//
//        splitInfo.splitFileNumber = root[SPLIT_FILE_NUM].asInt();
//    }
//
//    return true;
//}
//
///// <summary>
///// 分隔文件函数,将一个文件按照规定的大小分割为多个小块
///// </summary>
///// <param name="inputFile">被分割的文件</param>
///// <param name="chunkSize">分割后每块文件的大小(单位字节): 如果要分割为500mb每块,该数值为 500 * 1024 * 1024 </param>
///// <param name="inputFiles">切分后后的所有文件名</param>
///// <returns></returns>
//bool splitFile(const std::string& inputFile, const size_t chunkSize)
//{
//    std::vector<std::string> inputFiles;
//    std::string inputFileName = std::filesystem::path(inputFile).filename().stem().string();
//    std::string inputFileNamefull = std::filesystem::path(inputFile).filename().string();
//    std::string dirname = inputFileName + TZX_FILE_SUFFIX;
//    std::filesystem::path chunkDir = std::filesystem::path(inputFile).parent_path();
//    chunkDir.append(dirname);
//    if (!std::filesystem::exists(chunkDir))
//    {
//        std::filesystem::create_directories(chunkDir);
//    }
//
//    std::ifstream inputFileStream(inputFile, std::ios::binary | std::ios::ate);
//    if (!inputFileStream.is_open())
//    {
//        std::cout << "Error opening file: " << inputFile << std::endl;
//        return false;
//    }
//
//    size_t fileSize = static_cast<size_t>(inputFileStream.tellg());
//    inputFileStream.seekg(0, std::ios::beg);
//    //如果文件大小小于
//    size_t numberOfChunks = (fileSize + chunkSize - 1) / chunkSize;
//
//    Json::Value jsonRoot;
//    jsonRoot[SRC_FILE_NAME] = inputFileNamefull;
//    jsonRoot[SRC_FILE_SIZE] = fileSize;
//    jsonRoot[SPLIT_FILE_NUM] = numberOfChunks;
//    Json::Value splitFileNames;
//
//    for (size_t i = 0; i < numberOfChunks; ++i)
//    {
//        size_t chunkOffset = i * chunkSize;
//        size_t chunkSizeActual = std::min(chunkSize, fileSize - chunkOffset);
//
//        //std::vector<char> buffer(chunkSizeActual);
//        char buff[256];
//        sprintf(buff, TZX_CHUNK_NAME, inputFileName.c_str(), static_cast<int>(i + 1));
//        std::filesystem::path tempChunkDir = chunkDir;
//        std::string chunkFileName = tempChunkDir.append(buff).string();
//        inputFiles.push_back(chunkFileName);
//        Json::Value FileName;
//        FileName[SPLIT_FILE] = chunkFileName;
//
//        splitFileNames.append(FileName);
//
//        char* buffer = new char[chunkSizeActual];
//        // 移动文件读取指针到当前块的起始位置
//        inputFileStream.seekg(chunkOffset, std::ios::beg);
//        inputFileStream.read(buffer, chunkSizeActual);
//        std::ofstream chunkFile(chunkFileName, std::ios::binary);
//        if (!chunkFile.is_open())
//        {
//            std::cout << "Error creating chunk file: " << chunkFileName << std::endl;
//            delete[] buffer;
//            return false;
//        }
//        chunkFile.write(buffer, chunkSizeActual);
//        chunkFile.close();
//        delete[] buffer;
//        std::cout << "save split: " << chunkFileName << std::endl;
//    }
//    inputFileStream.close();
//
//    jsonRoot[SPLIT_FILES] = splitFileNames;
//    std::filesystem::path jsonFilePath = chunkDir;
//    jsonFilePath.append(SPLIT_JSON);
//    std::ofstream jsonFile(jsonFilePath, std::ios::out);
//    if (!jsonFile.is_open())
//    {
//        std::cout << "Error creating JSON file: " << jsonFilePath << std::endl;
//        return false;
//    }
//
//    jsonFile << jsonRoot;
//
//    jsonFile.close();
//
//
//    return true;
//}
//
///// <summary>
///// 将多个文件合并为一个文件
///// </summary>
///// <param name="outputFileName">合并后的文件</param>
///// <param name="inputFiles">需要合并的小文件</param>
///// <returns></returns>
//bool mergeFiles(const std::string jsonfile, std::string outputFileName = "")
//{
//    SplitInfo split_info;
//    if (!readSplitInfoFromJSON(jsonfile, split_info))
//    {
//        std::cout << "Error read json: " << jsonfile << std::endl;
//        return false;
//    }
//    if (split_info.splitFileNumber != split_info.splitFiles.size())
//    {
//        std::cout << "Error The number of files is incorrect " << std::endl;
//        return false;
//    }
//    // 判断文件是否都存在
//    for (const auto& file : split_info.splitFiles)
//    {
//        if (!std::filesystem::exists(file))
//        {
//            std::cout << "Error missing file: " << file << std::endl;
//            return false;
//        }
//    }
//    if (outputFileName.empty())     // 合并文件为空
//    {
//        std::filesystem::path chunkDir = std::filesystem::path(jsonfile).parent_path();
//        chunkDir.append(split_info.sourceFileName);
//        outputFileName = chunkDir.string();
//    }
//    // 打开合并文件,
//    std::ofstream outputFile(outputFileName, std::ios::binary);
//    if (!outputFile.is_open())
//    {
//        std::cout << "Error creating output file: " << outputFileName << std::endl;
//        return false;
//    }
//    for (const auto& inputFile : split_info.splitFiles)
//    {
//        std::ifstream inputFileStream(inputFile, std::ios::binary);
//
//        if (!inputFileStream.is_open()) {
//            std::cout << "Error opening input file: " << inputFile << std::endl;
//            return false;
//        }
//        outputFile << inputFileStream.rdbuf();
//        inputFileStream.close();
//
//        std::cout << "merge: " << inputFile << std::endl;
//    }
//    outputFile.close();
//    return true;
//}
//int main()
//{
//    std::string  directory = "D:/1_wangyingjie/readfile/split/ETOPO_chunk";
//    boost::timer tm1; // 定义后计时开始
//    tm1.restart();  // 从新从这里开始计时
//
//
//    size_t chunkSize = 100 * 1024 * 1024;  // 100 MB
//
//    std::string inputFile = "E:/11_ReadFile/ETOPO.tif";
//    std::string mergedFileName = "D:/1_wangyingjie/readfile/split/merged_ETOPO.tif";
//    std::vector<std::string> inputFiles;
//    bool sp = splitFile(inputFile, chunkSize);
//    if (sp)
//    {
//        std::cout << "splitFile successed" << std::endl;
//    }
//    else
//    {
//        std::cout << "splitFile failed" << std::endl;
//    }
//    std::cout << tm1.elapsed() << std::endl;  // 单位是秒
//
//
//    std::filesystem::path jsonFilePath(directory);
//    jsonFilePath.append(SPLIT_JSON);
//    SplitInfo splitInfo;
//    readSplitInfoFromJSON(jsonFilePath.string(), splitInfo);
//
//    tm1.restart();  // 从新从这里开始计时
//    bool me = mergeFiles(jsonFilePath.string());
//    if (me)
//    {
//        std::cout << "mergeFiles successed" << std::endl;
//    }
//    else
//    {
//        std::cout << "mergeFiles failed" << std::endl;
//    }
//    std::cout << tm1.elapsed() << std::endl;  // 单位是秒
//    return 0;
//
//
//
//
//
//    ////std::string inputFile_1 = "//192.168.0.179/Locl2/HN1.rar";
//    ////std::string inputFile_2 = "//192.168.0.179/Locl2/HN2.rar";
//    ////std::string inputFile_3 = "//192.168.0.179/Locl2/HN3.rar";
//    //std::string inputFile_4 = "//192.168.0.179/Locl2/HN4.rar";
//    //std::vector<std::string> vecinputFiles;
//    ////vecinputFiles.push_back(inputFile_1);
//    ////vecinputFiles.push_back(inputFile_2);
//    ////vecinputFiles.push_back(inputFile_3);
//    //vecinputFiles.push_back(inputFile_4);
//
//    ////std::string mergedFileName = "D:/1_wangyingjie/readfile/split/dly_merged.rar";
//    ////boost::timer tm1; // 定义后计时开始
//    ////tm1.restart();  // 从新从这里开始计时
//    ////std::string inputFile = "D:/1_wangyingjie/readfile/split/dly.rar";
//    //for (int i = 0; i < vecinputFiles.size(); i++)
//    //{
//    //    std::vector<std::string> inputFiles;
//    //    bool sp = splitFile(vecinputFiles[i], chunkSize, inputFiles);
//    //    if (sp)
//    //    {
//    //        std::cout << "splitFile successed: " << vecinputFiles[i] << std::endl;
//    //    }
//    //    else
//    //    {
//    //        std::cout << "splitFile failed: " << vecinputFiles[i] << std::endl;
//    //    }
//    //}
//
//    //tm1.restart();  // 从新从这里开始计时
//    //bool me = mergeFiles(mergedFileName, inputFiles);
//    //if (me)
//    //{
//    //    std::cout << "mergeFiles successed" << std::endl;
//    //}
//    //else
//    //{
//    //    std::cout << "mergeFiles failed" << std::endl;
//    //}
//    //std::cout << tm1.elapsed() << std::endl;  // 单位是秒
//    return 0;
//}










// // 测试完成
///// <summary>
///// 分隔文件函数,将一个文件按照规定的大小分割为多个小块
///// </summary>
///// <param name="inputFile">被分割的文件</param>
///// <param name="chunkSize">分割后每块文件的大小(单位字节): 如果要分割为500mb每块,该数值为 500 * 1024 * 1024 </param>
///// <param name="inputFiles">切分后后的所有文件名</param>
///// <returns></returns>
//bool splitFile(const std::string& inputFile, const size_t chunkSize, std::vector<std::string>& inputFiles)
//{
//    std::string inputFileName = std::filesystem::path(inputFile).filename().stem().string();
//    std::string dirname = inputFileName + TZX_FILE_SUFFIX;
//    std::filesystem::path chunkDir = std::filesystem::path(inputFile).parent_path();
//    chunkDir.append(dirname);
//    if (!std::filesystem::exists(chunkDir))
//    {
//        std::filesystem::create_directories(chunkDir);
//    }
//
//    std::ifstream inputFileStream(inputFile, std::ios::binary | std::ios::ate);
//    if (!inputFileStream.is_open())
//    {
//        std::cout << "Error opening file: " << inputFile << std::endl;
//        return false;
//    }
//
//    size_t fileSize = static_cast<size_t>(inputFileStream.tellg());
//    inputFileStream.seekg(0, std::ios::beg);
//    //如果文件大小小于
//    size_t numberOfChunks = (fileSize + chunkSize - 1) / chunkSize;
//    for (size_t i = 0; i < numberOfChunks; ++i)
//    {
//        size_t chunkOffset = i * chunkSize;
//        size_t chunkSizeActual = std::min(chunkSize, fileSize - chunkOffset);
//
//        //std::vector<char> buffer(chunkSizeActual);
//        char buff[256];
//        sprintf(buff, TZX_CHUNK_NAME, inputFileName.c_str(), static_cast<int>(i+1));
//        std::filesystem::path tempChunkDir = chunkDir;
//        std::string chunkFileName = tempChunkDir.append(buff).string();
//        inputFiles.push_back(chunkFileName);
//
//        char* buffer = new char[chunkSizeActual];
//        // 移动文件读取指针到当前块的起始位置
//        inputFileStream.seekg(chunkOffset, std::ios::beg);
//        inputFileStream.read(buffer, chunkSizeActual);
//        std::ofstream chunkFile(chunkFileName, std::ios::binary);
//        if (!chunkFile.is_open())
//        {
//            std::cout << "Error creating chunk file: " << chunkFileName << std::endl;
//            delete[] buffer;
//            return false;
//        }
//        chunkFile.write(buffer, chunkSizeActual);
//        chunkFile.close();
//        delete[] buffer;
//        std::cout << "save split: " << chunkFileName << std::endl;
//    }
//
//    inputFileStream.close();
//    return true;
//}
//
///// <summary>
///// 将多个文件合并为一个文件
///// </summary>
///// <param name="outputFileName">合并后的文件</param>
///// <param name="inputFiles">需要合并的小文件</param>
///// <returns></returns>
//bool mergeFiles(const std::string& outputFileName, const std::vector<std::string>& inputFiles)
//{
//    std::ofstream outputFile(outputFileName, std::ios::binary);
//    if (!outputFile.is_open())
//    {
//        std::cout << "Error creating output file: " << outputFileName << std::endl;
//        return false;
//    }
//    for (const auto& inputFile : inputFiles)
//    {
//        std::ifstream inputFileStream(inputFile, std::ios::binary);
//
//        if (!inputFileStream.is_open()) {
//            std::cout << "Error opening input file: " << inputFile << std::endl;
//            return false;
//        }
//        outputFile << inputFileStream.rdbuf();
//        inputFileStream.close();
//
//        std::cout << "merge: " << inputFile << std::endl;
//    }
//
//    outputFile.close();
//    return true;
//}
//
//
//int main() 
//{
//    size_t chunkSize = 500 * 1024 * 1024;  // 500 MB
//
//    //std::string inputFile_1 = "//192.168.0.179/Locl2/HN1.rar";
//    //std::string inputFile_2 = "//192.168.0.179/Locl2/HN2.rar";
//    //std::string inputFile_3 = "//192.168.0.179/Locl2/HN3.rar";
//    std::string inputFile_4 = "//192.168.0.179/Locl2/HN4.rar";
//    std::vector<std::string> vecinputFiles;
//    //vecinputFiles.push_back(inputFile_1);
//    //vecinputFiles.push_back(inputFile_2);
//    //vecinputFiles.push_back(inputFile_3);
//    vecinputFiles.push_back(inputFile_4);
//
//    //std::string mergedFileName = "D:/1_wangyingjie/readfile/split/dly_merged.rar";
//    //boost::timer tm1; // 定义后计时开始
//    //tm1.restart();  // 从新从这里开始计时
//    //std::string inputFile = "D:/1_wangyingjie/readfile/split/dly.rar";
//    for (int i = 0; i < vecinputFiles.size(); i++)
//    {
//        std::vector<std::string> inputFiles;
//        bool sp = splitFile(vecinputFiles[i], chunkSize, inputFiles);
//        if (sp)
//        {
//            std::cout << "splitFile successed: " << vecinputFiles[i] << std::endl;
//        }
//        else
//        {
//            std::cout << "splitFile failed: " << vecinputFiles[i] << std::endl;
//        }
//    }
//
//
//
//    //tm1.restart();  // 从新从这里开始计时
//    //bool me = mergeFiles(mergedFileName, inputFiles);
//    //if (me)
//    //{
//    //    std::cout << "mergeFiles successed" << std::endl;
//    //}
//    //else
//    //{
//    //    std::cout << "mergeFiles failed" << std::endl;
//    //}
//    //std::cout << tm1.elapsed() << std::endl;  // 单位是秒
//    return 0;
//}












//int main()
//{
//	// Example usage
//	std::string inputFilePath = "D:/1_wangyingjie/readfile/split/HN2.rar";
//	std::string outputDirectory = "D:/1_wangyingjie/readfile/split/output_2";
//	std::string mergedFileName = "D:/1_wangyingjie/readfile/split/merged_file.rar";
//	size_t chunkSizeMB = 500 * 1024 * 1024;
//
//	boost::timer tm1; // 定义后计时开始
//	tm1.restart();  // 从新从这里开始计时
//	std::vector<std::string> inputFiles;
//	bool sp = splitFile(inputFilePath, outputDirectory, chunkSizeMB, inputFiles);
//	if (sp)
//	{
//		std::cout << "splitFile successed" << std::endl;
//	}
//	else
//	{
//		std::cout << "splitFile failed" << std::endl;
//	}
//	std::cout << tm1.elapsed() << std::endl;  // 单位是秒
//	tm1.restart();  // 从新从这里开始计时
//	bool me = mergeFiles(mergedFileName, inputFiles);
//	if (me)
//	{
//		std::cout << "mergeFiles successed" << std::endl;
//	}
//	else
//	{
//		std::cout << "mergeFiles failed" << std::endl;
//	}
//	std::cout << tm1.elapsed() << std::endl;  // 单位是秒
//	return 0;
//
//}

// 
//

















































//int main()
//{
//
//	Config config;
//#ifndef NDEBUG
//	std::string configPath = "../../../../Config/my_config.json";
//#else
//	std::string configPath = "./my_config.json";
//#endif
//    if (config.read_config(configPath))
//    {
//        std::cout << "Read config file succession " << std::endl;
//    }
//    else
//    {
//        std::cout << "ERROR : Failed to read config file " << std::endl;
//        return 1;
//    }
//
//    std::filesystem::path data_1(DEFAULT_DATA_DIR);
//    data_1 += "/geo_db/example6.db";
//
//	tool_class tc;
//
//
//	return 0;
//}