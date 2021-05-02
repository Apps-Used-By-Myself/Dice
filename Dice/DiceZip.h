#pragma once
#include <string>
#include <stdexcept>
#include "filesystem.hpp"

namespace Zip
{
    // Zip��ȡʧ���쳣
    class ZipExtractionFailedException : public std::runtime_error
    {
    public:
        ZipExtractionFailedException(const std::string& what) : std::runtime_error("Failed to extract zip: " + what) {}
    };

    // ��ȡһ��Zip�ļ�
    // @param src �ڴ��е�zip�ļ��ֽ���
    // @param destFolder Ŀ���ļ���, ���ȱ�֤���ļ��д���
    // @throws ZipExtractionFailedException
    void extractZip(const std::string& src, const std::filesystem::path& destFolder);
}