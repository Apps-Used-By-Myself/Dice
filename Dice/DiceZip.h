#pragma once
#include <string>
#include "filesystem.hpp"

namespace Zip
{
    // ��ȡһ��Zip�ļ�
    // @param src �ڴ��е�zip�ļ��ֽ���
    // @param destFolder Ŀ���ļ���, ���ȱ�֤���ļ��д���
    // @throws ZipExtractionFailedException
    void extractZip(const std::string& src, const std::filesystem::path& destFolder);
}