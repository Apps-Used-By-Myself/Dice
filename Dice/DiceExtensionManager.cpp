#include <exception>
#include <stdexcept>
#include <map>
#include <memory>
#include <string>
#include <shared_mutex>
#include <fstream>
#include "json.hpp"
#include "filesystem.hpp"
#include "DiceNetwork.h"
#include "DiceZip.h"


namespace fs = std::filesystem;

extern std::filesystem::path DiceDir;

// Currently support three different types of extensions
enum class ExtensionType
{
    Deck,
    Lua,
    CardTemplate
};

NLOHMANN_JSON_SERIALIZE_ENUM( ExtensionType, {
    { ExtensionType::Deck, "Deck"},
    { ExtensionType::Lua, "Lua"},
    { ExtensionType::CardTemplate, "CardTemplate"},
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
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExtensionInfo, name, version, version_code, desc, download_link, author, type);

// ��������࣬�ṩ��װж�صȹ���
class ExtensionManager 
{
    // ����������б���
    std::map<std::string, ExtensionInfo> _index;
    std::shared_mutex _indexMutex;

    // �Ѱ�װ����б�
    std::map<std::string, ExtensionInfo> _installedIndex;
    std::shared_mutex _installedIndexMutex;

    // ��װĿ¼
    fs::path deckPath = DiceDir / "PublicDeck";
    fs::path luaPath = DiceDir / "lua";
    fs::path cardTemplatePath = DiceDir / "idk";

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
    fs::path getInstallPath(const ExtensionInfo& e)
    {
        if (e.type == ExtensionType::CardTemplate)
        {
            return cardTemplatePath / e.name;
        }
        if (e.type == ExtensionType::Deck)
        {
            return deckPath / e.name;
        }
        if (e.type == ExtensionType::Lua)
        {
            return luaPath / e.name;
        }
    }

    // д�������Ϣ
    // @param e ��Ӧ����Ĳ����Ϣ
    // @param p д��·��
    void writeExtensionInfo(const ExtensionInfo& e, const fs::path& p)
    {
        std::ofstream f(p);
        f << nlohmann::json(e).dump();
    }


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

    ExtensionInfo queryInstalledPackage(const std::string& name)
    {
        std::shared_lock lock(_installedIndexMutex);
        if (_installedIndex.count(name))
        {
            return _installedIndex.at(name);
        }
        throw ExtensionNotFoundException(name);
    }

    // @throws ExtensionNotFoundException, PackageInstallFailedException, ZipExtractionFailedException and maybe other exceptions
    void installPackage(const std::string& name)
    {   
        ExtensionInfo ext = queryPackage(name);
        std::string des;

        if (!Network::GET("raw.githubusercontent.com", ("/Dice-Developer-Team/DiceExtensions/main/" + ext.name + ".zip").c_str(), 443, des, true))
        {
            throw PackageInstallFailedException("Network: " + des);
        }

        fs::path installPath = getInstallPath(ext);
        std::error_code ec1;
        fs::create_directories(installPath, ec1);
        Zip::extractZip(des, installPath);
        writeExtensionInfo(ext, installPath / ".info.json");

    }

    void removePackage(const std::string& name)
    {

    }

    class ExtensionNotFoundException : std::runtime_error
    {
    public:
        ExtensionNotFoundException(const std::string& what) : std::runtime_error(("Unable to find package with name " + what).c_str()) {}
    };

    class ConcurrentIndexRefreshException : std::runtime_error
    {
    public:
        ConcurrentIndexRefreshException() : std::runtime_error("Failed to acquire lock: Another thread is refreshing the index") {}
    };

    class IndexRefreshFailedException : std::runtime_error
    {
    public:
        IndexRefreshFailedException(const std::string& what) : std::runtime_error("Failed to refresh index: " + what) {}
    };

    class PackageInstallFailedException : std::runtime_error
    {
    public:
        PackageInstallFailedException(const std::string& what) : std::runtime_error("Failed to install package: " + what) {}
    };
};