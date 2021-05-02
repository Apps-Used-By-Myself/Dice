#pragma once

#include <exception>
#include <stdexcept>
#include <map>
#include <memory>
#include <string>
#include <shared_mutex>
#include <sstream>
#include <fstream>
#include "json.hpp"
#include "filesystem.hpp"
#include "DiceNetwork.h"
#include "DiceZip.h"
#include "DiceExtensionManager.h"
#include "EncodingConvert.h"

extern std::filesystem::path DiceDir;
void loadData();

// Currently support three different types of extensions
enum class ExtensionType
{
    Deck,
    Lua,
    CardTemplate
};

constexpr const char* ExtensionTypeToString(const ExtensionType& e)
{
    switch (e)
    {
    case ExtensionType::Deck:
        return "Deck";
    case ExtensionType::Lua:
        return "Lua";
    case ExtensionType::CardTemplate:
        return "CardTemplate";
    }
    return "";
}

NLOHMANN_JSON_SERIALIZE_ENUM( ExtensionType, {
    { ExtensionType::Deck, ExtensionTypeToString(ExtensionType::Deck) },
    { ExtensionType::Lua, ExtensionTypeToString(ExtensionType::Lua) },
    { ExtensionType::CardTemplate, ExtensionTypeToString(ExtensionType::CardTemplate) }
})

// �����Ϣ��
// Json serializable
struct ExtensionInfo
{
    // �������
    std::string name;
    // ����汾���ַ���
    // �磺1.2.3
    std::string version;
    // ����汾�ţ�����
    // ��Ϊ��������
    int version_code;
    // ����
    std::string desc;
    // ��������
    std::string download_link;
    // ����
    std::string author;
    // ����
    ExtensionType type;

    // ����GB18030����
    operator std::string() const
    {
        std::stringstream ss;
        ss << "���ƣ�" << UTF8toGBK(name) << std::endl;
        ss << "�汾��" << UTF8toGBK(version) << std::endl;
        ss << "������" << UTF8toGBK(desc) << std::endl;
        ss << "���ߣ�" << UTF8toGBK(author) << std::endl;
        ss << "���ͣ�" << UTF8toGBK(ExtensionTypeToString(type)) << std::endl;
        return ss.str();
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExtensionInfo, name, version, version_code, desc, download_link, author, type);

// ��������࣬�ṩ��װж�صȹ��ܣ���ע����Ҫʹ��UTF-8����
class ExtensionManager 
{
    // ����������б���
    std::map<std::string, ExtensionInfo> _index;
    std::shared_mutex _indexMutex;

    // �Ѱ�װ����б�
    std::map<std::string, ExtensionInfo> _installedIndex;
    std::shared_mutex _installedIndexMutex;

    // ��װĿ¼
    std::filesystem::path deckPath = DiceDir / "PublicDeck";
    std::filesystem::path luaPath = DiceDir / "plugin";
    std::filesystem::path cardTemplatePath = DiceDir / "CardTemp";

public:
    // �ӷ�������ȡ���²���б�
    // @throws ConcurrentIndexRefreshException, IndexRefreshFailedException
    void refreshIndex()
    {
        std::unique_lock lock(_indexMutex, std::try_to_lock);
        if (!lock.owns_lock())
        {
            throw ConcurrentIndexRefreshException();
        }

        // get&parse
        std::string des;
        if (!Network::GET("raw.githubusercontent.com", "/Dice-Developer-Team/DiceExtensions/main/index.json", 443, des, true))
        {
            throw IndexRefreshFailedException("Network: " + des);
        }

        try
        {
            nlohmann::json j = nlohmann::json::parse(des);
            _index.clear();
            for (const auto& item : j)
            {
                _index[item["name"].get<std::string>()] = item.get<ExtensionInfo>();
            }
        }
        catch(const std::exception& e)
        {
            throw IndexRefreshFailedException(std::string("Json Parse: ") + e.what());
        }
    }

    // ��ȡĳ�����Ͳ��Ӧ�ð�װ��λ��
    // @param e ��Ӧ����Ĳ����Ϣ
    std::filesystem::path getInstallPath(const ExtensionInfo& e)
    {
        string name = e.name;
#ifdef _WIN32
        name = UTF8toGBK(name);
#endif
        if (e.type == ExtensionType::CardTemplate)
        {
            return cardTemplatePath / name;
        }
        if (e.type == ExtensionType::Deck)
        {
            return deckPath / name;
        }
        if (e.type == ExtensionType::Lua)
        {
            return luaPath / name;
        }
        return "";
    }

    // ��ȡIndex�еİ�����
    size_t getIndexCount()
    {
        std::shared_lock lock(_indexMutex);
        return _index.size();
    }

    std::map<std::string, ExtensionInfo> getIndex()
    {
        std::shared_lock lock(_indexMutex);
        return _index;
    }



    // д�������Ϣ
    // @param e ��Ӧ����Ĳ����Ϣ
    // @param p д��·��
    void writeExtensionInfo(const ExtensionInfo& e, const std::filesystem::path& p)
    {
        std::ofstream f(p);
        f << nlohmann::json(e).dump();
    }


    // ��ѯ�����Ϣ��nameΪUTF-8����
    // @throws ExtensionNotFoundException
    ExtensionInfo queryPackage(const std::string& name)
    {
        std::shared_lock lock(_indexMutex);
        if (_index.count(name))
        {
            return _index.at(name);
        }
        throw ExtensionNotFoundException(name);
    }

    // ��ѯ�Ѱ�װ��Ϣ��nameΪUTF-8����
    // @throws ExtensionNotFoundException
    ExtensionInfo queryInstalledPackage(const std::string& name)
    {
        std::shared_lock lock(_installedIndexMutex);
        if (_installedIndex.count(name))
        {
            return _installedIndex.at(name);
        }
        throw ExtensionNotFoundException(name);
    }

    // ��װ��չ��nameΪUTF-8����
    // @throws ExtensionNotFoundException, PackageInstallFailedException, ZipExtractionFailedException and maybe other exceptions
    void installPackage(const std::string& name)
    {   
        ExtensionInfo ext = queryPackage(name);
        std::string des;

        if (!Network::GET("raw.githubusercontent.com", ("/Dice-Developer-Team/DiceExtensions/main/" + UrlEncode(ext.name) + ".zip").c_str(), 443, des, true))
        {
            throw PackageInstallFailedException("Network: " + des);
        }

        std::filesystem::path installPath = getInstallPath(ext);
        std::error_code ec1;
        std::filesystem::create_directories(installPath, ec1);
        Zip::extractZip(des, installPath);
        writeExtensionInfo(ext, installPath / ".info.json");
        loadData();
    }

    // ж����չ��nameΪUTF-8����
    void removePackage(const std::string& name)
    {

    }

    class ExtensionNotFoundException : public std::runtime_error
    {
    public:
        ExtensionNotFoundException(const std::string& what) : std::runtime_error("Unable to find package with name " + what) {}
    };

    class ConcurrentIndexRefreshException : public std::runtime_error
    {
    public:
        ConcurrentIndexRefreshException() : std::runtime_error("Failed to acquire lock: Another thread is refreshing the index") {}
    };

    class IndexRefreshFailedException : public std::runtime_error
    {
    public:
        IndexRefreshFailedException(const std::string& what) : std::runtime_error("Failed to refresh index: " + what) {}
    };

    class PackageInstallFailedException : public std::runtime_error
    {
    public:
        PackageInstallFailedException(const std::string& what) : std::runtime_error("Failed to install package: " + what) {}
    };
};