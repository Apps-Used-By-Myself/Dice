#include "DiceZip.h"
#include "filesystem.hpp"
#include <string>
#include <zip.h>
#include <fstream>
namespace fs = std::filesystem;

namespace Zip
{
    // Zip��ȡʧ���쳣
    class ZipExtractionFailedException : std::runtime_error
    {
    public:
        ZipExtractionFailedException(const std::string& what) : std::runtime_error("Failed to extract zip: " + what) {}
    };

    class Unzipper
    {
        // zip����
        zip_error_t error;

        // ������Ϣ/�ļ�����
        char buf[256];

        // ԭzip�ļ��ֽ�
        std::string src;

        // Ŀ���ļ���
        fs::path destFolder;

        // zip�ļ��ṹ
        zip_t* zip = nullptr;

    public:
        Unzipper(const std::string& src, const fs::path& destFolder) : src(src), destFolder(destFolder)
        {
            // ��ʼ��zip_error
            zip_error_init(&error);

            // ����zip_source
            zip_source_t* source = zip_source_buffer_create(this->src.c_str(), this->src.size() * sizeof(char), 1, &error);
            if (error.zip_err != ZIP_ER_OK)
            {
                throw ZipExtractionFailedException(zip_error_strerror(&error));
            }

            // ��zip
            zip = zip_open_from_source(source, ZIP_RDONLY, &error);
            if (error.zip_err != ZIP_ER_OK)
            {
                // �����zipʧ�ܣ�zip_source��Ҫ�ֶ��ͷţ��������libzip�Լ���zip_close/discard��ʱ���ͷ�
                zip_source_free(source);
                throw ZipExtractionFailedException(zip_error_strerror(&error));
            }
        }
        ~Unzipper()
        {
            // ����Ѿ���zip���ͷ��ڴ沢����һ�и���
            if(zip) zip_discard(zip);

            // �ͷ�zip_error
            zip_error_fini(&error);
        }

        void extractAll()
        {
            // ö�������ļ�
            for (int i = 0; i != zip_get_num_entries(zip, 0); i++)
            {
                // ��ʼ���ļ���Ϣ�ṹ��
                zip_stat_t stat;
                zip_stat_init(&stat);

                // ��ȡ��Ϣ
                if (zip_stat_index(zip, i, 0, &stat) == -1)
                {
                    throw ZipExtractionFailedException(zip_strerror(zip));
                }

                // ��Ҫ�ļ����ƺʹ�С
                if(!(stat.valid & ZIP_STAT_NAME))
                {
                    throw ZipExtractionFailedException("Failed to get file name");
                }

                if(!(stat.valid & ZIP_STAT_SIZE))
                {
                    throw ZipExtractionFailedException("Failed to get file size");
                }

                std::string name = stat.name;
                if (name.empty())
                {
                    throw ZipExtractionFailedException("Failed to get file name");
                }

                // ������ļ����򴴽���Ȼ�������¸��ļ�
                if (name[name.length() - 1] == '/')
                {
                    std::error_code ec;
                    fs::create_directories(destFolder / name, ec);
                    continue;
                }

                // ������ļ�
                std::ofstream f(destFolder / name, std::ios::out | std::ios::trunc | std::ios::binary);

                if (!f)
                {
                    throw ZipExtractionFailedException("Failed to open file for writing");
                }

                // ��ѹ���ļ�
                zip_file* file = zip_fopen_index(zip, i, 0);
                if (!file)
                {
                    throw ZipExtractionFailedException(zip_strerror(zip));
                }

                // ��ѹ��д���ļ�
                zip_uint64_t written_size = 0;
                while (stat.size != written_size)
                {
                    zip_int64_t bytes_read = zip_fread(file, buf, sizeof(buf));
                    if (bytes_read == -1)
                    {
                        zip_fclose(file);
                        throw ZipExtractionFailedException("Failed to extract file " + name);
                    }
                    f << std::string(buf, bytes_read);
                    written_size += bytes_read;
                }
                zip_fclose(file);
            }
        }
    };

    void extractZip(const std::string& src, const fs::path& destFolder)
    {
        Unzipper unzip(src, destFolder);
        unzip.extractAll();
    }
}
