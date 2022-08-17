#pragma once
/**
 * ��lua���õ���
 * 2022/8/17 ��չSelfData��ȫ�ַ�ΧӦ��
 */
#include <mutex>
#include <filesystem>
#include "DiceAttrVar.h"
class SelfData : public std::enable_shared_from_this<SelfData> {
	enum FileType { Bin, Json };
	FileType type{ Bin };
	std::filesystem::path pathFile;
public:
	std::mutex exWrite;
	SelfData(const std::filesystem::path& p);
	AttrObject data;
	void save();
};
//filename stem by native charset
extern dict<ptr<SelfData>> selfdata_byFile;
//filename stem by gbk charset
extern dict<ptr<SelfData>> selfdata_byStem;
