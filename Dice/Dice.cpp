/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2021 w4123���
 * Copyright (C) 2019-2021 String.Empty
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this
 * program. If not, see <http://www.gnu.org/licenses/>.
 */
#define UNICODE
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <mutex>
#include <unordered_map>
#include <exception>
#include <stdexcept>
#include "filesystem.hpp"

#include "APPINFO.h"
#include "DiceFile.hpp"
#include "Jsonio.h"
#include "QQEvent.h"
#include "DDAPI.h"
#include "ManagerSystem.h"
#include "DiceMod.h"
#include "DiceMsgSend.h"
#include "MsgFormat.h"
#include "DiceCloud.h"
#include "CardDeck.h"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "CharacterCard.h"
#include "DiceEvent.h"
#include "DiceSession.h"
#include "DiceGUI.h"
#include "S3PutObject.h"
#include "DiceCensor.h"
#include "EncodingConvert.h"
#include "DiceManager.h"

#ifndef _WIN32
#include <curl/curl.h>
#endif

#pragma warning(disable:4996)
#pragma warning(disable:6031)

using namespace std;

unordered_map<long long, User> UserList{};
ThreadFactory threads;
std::filesystem::path fpFileLoc;
std::unique_ptr<CivetServer> ManagerServer;
IndexHandler h_index;
BasicInfoApiHandler h_basicinfoapi;
CustomMsgApiHandler h_msgapi;
AdminConfigHandler h_config;
MasterHandler h_master;
CustomReplyApiHandler h_customreply;
WebUIPasswordHandler h_webuipassword;
AuthHandler auth_handler;

constexpr auto msgInit{ R"(��ӭʹ��Dice!���������ˣ�
�뷢��.system gui��������ĺ�̨���
����Masterģʽͨ�������󼴿ɳ�Ϊ�ҵ�����~
�ɷ���.help�鿴����
�ο��ĵ��ο�.help����)" };

//��������
void loadData()
{
	ResList logList;
	try
	{
		std::error_code ec;
		std::filesystem::create_directory(DiceDir, ec);	
		loadDir(loadXML<CardTemp>, DiceDir / "CardTemp", mCardTemplet, logList, true);
		fmt->set_help("�ظ��б�", "�ظ��������б�:{list_reply_deck}");
		if (loadDir(loadJMap, DiceDir / "PublicDeck", CardDeck::mExternPublicDeck, logList) < 1)
		{
			loadJMap(fpFileLoc / "PublicDeck.json", CardDeck::mExternPublicDeck);
			loadJMap(fpFileLoc / "ExternDeck.json", CardDeck::mExternPublicDeck);
		}
		map_merge(CardDeck::mPublicDeck, CardDeck::mExternPublicDeck);
		//��ȡ�����ĵ�
		fmt->load(&logList);
		if (int cnt; (cnt = loadJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp)) < 0)
		{
			if (cnt == -1)logList << UTF8toGBK((DiceDir / "conf" / "CustomHelp.json").u8string()) + "����ʧ�ܣ�";
			ifstream ifstreamHelpDoc(fpFileLoc / "HelpDoc.txt");
			if (ifstreamHelpDoc)
			{
				string strName, strMsg;
				while (getline(ifstreamHelpDoc, strName) && getline(ifstreamHelpDoc, strMsg))
				{
					while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
					while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\s"), 2, " ");
					while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
					CustomHelp[strName] = strMsg;
				}
				if (!CustomHelp.empty())
				{
					saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
					logList << "��ʼ���Զ����������" + to_string(CustomHelp.size()) + "��";
				}
			}
			ifstreamHelpDoc.close();
		}
		map_merge(fmt->helpdoc, CustomHelp);
		//��ȡ���дʿ�
		loadDir(load_words, DiceDir / "conf" / "censor", censor, logList, true);
		loadJMap(DiceDir / "conf" / "CustomCensor.json", censor.CustomWords);
		censor.build();
		if (!logList.empty())
		{
			logList << "��չ���ö�ȡ��ϡ�";
			console.log(logList.show(), 1, printSTNow());
		}
	}
	catch (const std::exception& e)
	{
		logList << "��ȡ����ʱ����������󣬳�������޷��������С��볢��������ú����ԡ�" << e.what();
		console.log(logList.show(), 1, printSTNow());
	}
}

//��ʼ���û�����
void readUserData()
{
	std::error_code ec;
	fs::path dir{ DiceDir / "user" };
	//��ȡ�û���¼
	if (loadBFile(dir / "UserConf.RDconf", UserList) > 0) {
		fs::copy(dir / "UserConf.RDconf", dir / "UserConf.bak", ec);
	}
	else
	{
		map<long long, int> DefaultDice;
		if (loadFile(fpFileLoc / "Default.RDconf", DefaultDice) > 0)
			for (auto p : DefaultDice)
			{
				getUser(p.first).intConf["Ĭ����"] = p.second;
			}
		map<long long, string> DefaultRule;
		if (loadFile(fpFileLoc / "DefaultRule.RDconf", DefaultRule) > 0)
			for (auto p : DefaultRule)
			{
				if (isdigit(static_cast<unsigned char>(p.second[0])))break;
				getUser(p.first).strConf["Ĭ�Ϲ���"] = p.second;
			}
		ifstream ifName(fpFileLoc / "Name.dicedb");
		if (ifName)
		{
			long long GroupID = 0, QQ = 0;
			string name;
			while (ifName >> GroupID >> QQ >> name)
			{
				name = base64_decode(name);
				getUser(QQ).setNick(GroupID, name);
			}
		}
		if (loadFile(DiceDir / "user" / "UserList.txt", UserList) < 1)
		{
			set<long long> WhiteQQ;
			if (loadFile(fpFileLoc / "WhiteQQ.RDconf", WhiteQQ) > 0)
				for (auto qq : WhiteQQ)
				{
					getUser(qq).trust(1);
				}
			//��ȡ����Ա�б�
			set<long long> AdminQQ;
			if (loadFile(fpFileLoc / "AdminQQ.RDconf", AdminQQ) > 0)
				for (auto qq : AdminQQ)
				{
					getUser(qq).trust(4);
				}
			if (console.master())getUser(console.master()).trust(5);
			if (UserList.size()) {
				console.log("��ʼ���û���¼" + to_string(UserList.size()) + "��", 1);
			}
		}
	}
	//��ȡ��ɫ��¼
	if (loadBFile(dir / "PlayerCards.RDconf", PList) > 0)
		fs::copy(dir / "PlayerCards.RDconf", dir / "PlayerCards.bak", ec);
	for (const auto& pl : PList)
	{
		if (!UserList.count(pl.first))getUser(pl.first);
	}
	//��ȡȺ�ļ�¼
	if (loadBFile(DiceDir / "user" / "ChatConf.RDconf", ChatList) > 0) {
		fs::copy(DiceDir / "user" / "ChatConf.RDconf", DiceDir / "user" / "ChatConf.bak", ec);
	}
	else
	{
		set<long long> GroupList;
		map<chatType, int> mDefault;
		if (loadFile(fpFileLoc / "DefaultCOC.MYmap", mDefault) > 0)
			for (const auto& it : mDefault)
			{
				if (it.first.second == msgtype::Private)getUser(it.first.first)
					.setConf("rc����", it.second);
				else chat(it.first.first).setConf("rc����", it.second);
			}
		if (loadFile(fpFileLoc / "DisabledGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("ͣ��ָ��");
			}
		GroupList.clear();
		if (loadFile(fpFileLoc / "DisabledJRRPGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("����jrrp");
			}
		GroupList.clear();
		if (loadFile(fpFileLoc / "DisabledOBGroup.RDconf", GroupList) > 0)
			for (auto p : GroupList)
			{
				chat(p).group().set("����ob");
			}
		GroupList.clear();
		if (loadFile(fpFileLoc / "WhiteGroup.RDconf", GroupList) > 0)
		{
			for (auto g : GroupList)
			{
				chat(g).group().set("���ʹ��").set("����");
			}
		}
		if (loadFile(DiceDir / "user" / "ChatList.txt", ChatList) < 1 && ChatList.size()) {
			console.log("��ʼ��Ⱥ��¼" + to_string(ChatList.size()) + "��", 1);
		}
		saveBFile(DiceDir / "user" / "ChatConf.RDconf", ChatList);
	}
	//��ȡ�����¼
	if (gm->load() < 0)
	{
		multimap<long long, long long> ObserveGroup;
		loadFile(fpFileLoc / "ObserveDiscuss.RDconf", ObserveGroup);
		loadFile(fpFileLoc / "ObserveGroup.RDconf", ObserveGroup);
		for (auto [grp, qq] : ObserveGroup)
		{
			gm->session(grp).sOB.insert(qq);
			gm->session(grp).update();
		}
		if(gm->mSession.size())console.log("��ʼ���Թۼ�¼" + to_string(gm->mSession.size()) + "��", 1);
	}
	//��ȡ��������
	today = make_unique<DiceToday>(dir / "DiceToday.json");
}

//��������
void dataBackUp()
{
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / "conf", ec);
	std::filesystem::create_directory(DiceDir / "user", ec);
	std::filesystem::create_directory(DiceDir / "audit", ec);
	//�����б�
	saveBFile(DiceDir / "user" / "PlayerCards.RDconf", PList);
	saveBFile(DiceDir / "user" / "ChatConf.RDconf", ChatList);
	saveBFile(DiceDir / "user" / "UserConf.RDconf", UserList);
}

bool isIniting{ false };
EVE_Enable(eventEnable)
{
	if (isIniting || Enabled)return;
	isIniting = true;
	llStartTime = time(nullptr);
	#ifndef _WIN32
	CURLcode err;
	err = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (err != CURLE_OK)
	{
		console.log("����: ����libcurlʧ�ܣ�", 1);
	}
#endif
	std::string RootDir = DD::getRootDir();
	if (RootDir.empty()) {	
#ifdef _WIN32
		char path[MAX_PATH];
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		dirExe = std::filesystem::path(path).parent_path();
#endif
	}
	else 
	{
		dirExe = RootDir;
	}
	Dice_Full_Ver_On = Dice_Full_Ver + " on\n" + DD::getDriVer();
	DD::debugLog(Dice_Full_Ver_On);

	mCardTemplet = {
		{
			"COC7", {
				"COC7", SkillNameReplace, BasicCOC7, InfoCOC7, AutoFillCOC7, mVariableCOC7, ExpressionCOC7,
				SkillDefaultVal, {
					{"_default", CardBuild({BuildCOC7},  {"{�������}"}, {})},
					{
						"bg", CardBuild({
											{"�Ա�", "{�Ա�}"}, {"����", "7D6+8"}, {"ְҵ", "{����Աְҵ}"}, {"��������", "{��������}"},
											{"��Ҫ֮��", "{��Ҫ֮��}"}, {"˼������", "{˼������}"}, {"����Ƿ�֮��", "{����Ƿ�֮��}"},
											{"����֮��", "{����֮��}"}, {"����", "{����Ա�ص�}"}
										}, {"{�������}"}, {})
					}
				}
			}
		},
		{"BRP", {
				"BRP", {}, {}, {}, {}, {}, {}, {
					{"__DefaultDice",100}
				}, {
					{"_default", CardBuild({},  {"{�������}"}, {})},
					{
						"bg", CardBuild({
											{"�Ա�", "{�Ա�}"}, {"����", "7D6+8"}, {"ְҵ", "{����Աְҵ}"}, {"��������", "{��������}"},
											{"��Ҫ֮��", "{��Ҫ֮��}"}, {"˼������", "{˼������}"}, {"����Ƿ�֮��", "{����Ƿ�֮��}"},
											{"����֮��", "{����֮��}"}, {"����", "{����Ա�ص�}"}
										}, {"{�������}"}, {})
					}
				}
		}},
		{"DND", {
				"DND", {}, {}, {}, {}, {}, {}, {
					{"__DefaultDice",20}
				}, {
					{"_default", CardBuild({}, {"{�������}"}, {})},
					{
						"bg", CardBuild({
											{"�Ա�", "{�Ա�}"},
										},  {"{�������}"}, {})
					}
				}
		}},
	};
	if ((console.DiceMaid = DD::getLoginQQ()))
	{
		DiceDir = dirExe / ("Dice" + to_string(console.DiceMaid));
		if (!exists(DiceDir)) {
			filesystem::path DiceDirOld(dirExe / "DiceData");
			if (exists(DiceDirOld))rename(DiceDirOld, DiceDir);
			else filesystem::create_directory(DiceDir);
		}
	}
	console.setPath(DiceDir / "conf" / "Console.xml");
	fpFileLoc = DiceDir / "com.w4123.dice";
	{
		std::unique_lock lock(GlobalMsgMutex);
		GlobalMsg["strSelfName"] = DD::getLoginNick();
		if (GlobalMsg["strSelfName"].empty())
		{
			GlobalMsg["strSelfName"] = "����[" + toString(console.DiceMaid % 10000, 4) + "]";
		}
	}
	std::error_code ec;
	std::filesystem::create_directory(DiceDir / "conf", ec);
	std::filesystem::create_directory(DiceDir / "user", ec);
	std::filesystem::create_directory(DiceDir / "audit", ec);
	std::filesystem::create_directory(DiceDir / "mod", ec);
	std::filesystem::create_directory(DiceDir / "plugin", ec);

	ExtensionManagerInstance = std::make_unique<ExtensionManager>();
	if (!console.load())
	{
		DD::sendPrivateMsg(console.DiceMaid, msgInit);
		std::map<string, int> boolConsole;
		loadJMap(fpFileLoc / "boolConsole.json", boolConsole);
		for (auto& [key, val] : boolConsole)
		{
			console.set(key, val);
		}
		console.setClock({ 11, 45 }, "clear");
		console.loadNotice();
		console.save();
	}
	//��ʼ��������
	blacklist = make_unique<DDBlackManager>();
	if (blacklist->loadJson(DiceDir / "conf" / "BlackList.json") < 0)
	{
		blacklist->loadJson(fpFileLoc / "BlackMarks.json");
		int cnt = blacklist->loadHistory(fpFileLoc);
		if (cnt) {
			blacklist->saveJson(DiceDir / "conf" / "BlackList.json");
			console.log("��ʼ��������¼" + to_string(cnt) + "��", 1);
		}
	}
	else {
		blacklist->loadJson(DiceDir / "conf" / "BlackListEx.json", true);
	}
	fmt = make_unique<DiceModManager>();
	{
		std::unique_lock lock(GlobalMsgMutex);
		if (loadJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg) < 0)loadJMap(fpFileLoc / "CustomMsg.json", EditedMsg);
		{	
			//Ԥ�޸ĳ����ظ��ı�
			if (EditedMsg.count("strSelfName"))GlobalMsg["strSelfName"] = EditedMsg["strSelfName"];
			for (auto it : EditedMsg)
			{
				while (it.second.find("��������") != string::npos)it.second.replace(it.second.find("��������"), 8,
																				GlobalMsg["strSelfName"]);
				GlobalMsg[it.first] = it.second;
			}
		}
	}
	loadData();
	//��ȡ�û�����
	gm = make_unique<DiceTableMaster>();
	readUserData();
	for (auto gid : DD::getGroupIDList())
	{
		chat(gid).group().reset("δ��").reset("����");
	}
	// ȷ���߳�ִ�н���
	while (msgSendThreadRunning)this_thread::sleep_for(10ms);
	Aws::InitAPI(options);
	Enabled = true;
	DD::debugLog("Dice.threadInit");
	threads(SendMsg);
	threads(ConsoleTimer);
	threads(warningHandler);
	threads(frqHandler);
	sch.start();

	DD::debugLog("Dice.extensionManagerInit");
	try
	{
		ExtensionManagerInstance->refreshIndex();
		console.log("�ѳɹ�ˢ����������棬" + to_string(ExtensionManagerInstance->getIndexCount()) + "����չ���ã�"
			+ to_string(ExtensionManagerInstance->getUpgradableCount()) + "��������", 0b1);
	}
	catch(const std::exception& e)
	{
		console.log(std::string("ˢ�����������ʧ�ܣ�") + e.what(), 0b1);
	}

	DD::debugLog("Dice.webUIInit");
	WebUIPasswordPath = DiceDir / "conf" / "WebUIPassword.digest";
	if (!std::filesystem::exists(WebUIPasswordPath, ec) || std::filesystem::is_empty(WebUIPasswordPath, ec))
	{		
		setPassword("password");
	}

	// ��ʼ��������
	mg_init_library(0);

	// ��ȡ��������
	bool AllowInternetAccess = console["WebUIAllowInternetAccess"];
	int Port = console["WebUIPort"];

	const char* envAllowInternetAccess = std::getenv("DICE_WEBUI_ALLOW_INTERNET_ACCESS");
	std::string envStrAllowInternetAccess = envAllowInternetAccess ? envAllowInternetAccess : "";

	if (envStrAllowInternetAccess == "1")
	{
		AllowInternetAccess = true;
	}
	else if (envStrAllowInternetAccess == "0")
	{
		AllowInternetAccess = false;
	}

	const char* alternateVariable = std::getenv("DICE_WEBUI_PORT_USE_VARIABLE");
	std::string variable = alternateVariable ? alternateVariable : "DICE_WEBUI_PORT";
	
	try
	{
		const char* envPort = std::getenv(variable.c_str());
		Port = std::stoi(envPort ? envPort : "");
	}
	catch(const std::exception& e)
	{
		;
	}

	if (console["EnableWebUI"])
	{
		try {
			const std::string port_option = std::string(AllowInternetAccess ? "0.0.0.0" : "127.0.0.1") + ":" + std::to_string(Port);

			std::vector<std::string> mg_options = {"listening_ports", port_option.c_str()};

			ManagerServer = std::make_unique<CivetServer>(mg_options);

			ManagerServer->addHandler("/", h_index);
			ManagerServer->addHandler("/api/basicinfo", h_basicinfoapi);
			ManagerServer->addHandler("/api/custommsg", h_msgapi);
			ManagerServer->addHandler("/api/adminconfig", h_config);
			ManagerServer->addHandler("/api/master", h_master);
			ManagerServer->addHandler("/api/customreply", h_customreply);
			ManagerServer->addHandler("/api/webuipassword", h_webuipassword);
			ManagerServer->addAuthHandler("/", auth_handler);
			auto ports = ManagerServer->getListeningPorts();

			if (ports.empty())
			{
				DD::debugLog("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�");
				console.log("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�", 0b1);
			}
			else
			{
				DD::debugLog("Dice! WebUI ���ڶ˿�" + std::to_string(ports[0]) + "����");
				console.log("Dice! WebUI ���ڶ˿�" + std::to_string(ports[0]) 
					+ "���У����ؿ�ͨ�����������localhost:" + std::to_string(ports[0])
					+ "\nĬ���û���Ϊadmin����Ϊpassword����ϸ�̳���鿴 https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming", 0b1);
			}
		} 
		catch(const CivetException& e)
		{
			DD::debugLog("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�");
			console.log("Dice! WebUI ����ʧ�ܣ��˿��ѱ�ʹ�ã�", 0b1);
		}
	}

	console.log(getMsg("strSelfName") + "��ʼ����ɣ���ʱ" + to_string(time(nullptr) - llStartTime) + "��", 0b1,
				printSTNow());
	//��������
	getDiceList();
	getExceptGroup();
	llStartTime = time(nullptr);
	isIniting = false;
}

mutex GroupAddMutex;

bool eve_GroupAdd(Chat& grp)
{
	{
		unique_lock<std::mutex> lock_queue(GroupAddMutex);
		if (grp.lastmsg(time(nullptr)).isset("δ��") || grp.isset("����"))grp.reset("δ��").reset("����");
		else return false;
		if (ChatList.size() == 1 && !console.isMasterMode)DD::sendGroupMsg(grp.ID, msgInit);
	}
	long long fromGroup = grp.ID;
	if (grp.Name.empty())
		grp.Name = DD::getGroupName(fromGroup);
	GroupSize_t gsize(DD::getGroupSize(fromGroup));
	if (console["GroupInvalidSize"] > 0 && grp.boolConf.empty() && gsize.currSize > (size_t)console["GroupInvalidSize"]) {
		grp.set("Э����Ч");
	}
	if (!console["ListenGroupAdd"] || grp.isset("����"))return 0;
	string strNow = printSTNow();
	string strMsg(getMsg("strSelfName"));
	try 
	{
		strMsg += "�¼���:" + DD::printGroupInfo(grp.ID);
		if (blacklist->get_group_danger(fromGroup)) 
		{
			grp.leave(blacklist->list_group_warning(fromGroup));
			strMsg += "Ϊ������Ⱥ������Ⱥ";
			console.log(strMsg, 0b10, printSTNow());
			return true;
		}
		if (grp.isset("���ʹ��"))strMsg += "���ѻ�ʹ����ɣ�";
		else if(grp.isset("Э����Ч"))strMsg += "��Ĭ��Э����Ч��";
		if (grp.inviter) {
			strMsg += ",������" + printQQ(chat(fromGroup).inviter);
		}
		console.log(strMsg, 0, strNow);
		int max_trust = 0;
		float ave_trust(0);
		//int max_danger = 0;
		long long ownerQQ = 0;
		ResList blacks;
		std::set<long long> list = DD::getGroupMemberList(fromGroup);
		if (list.empty())
		{
			strMsg += "��ȺԱ����δ���أ�";
		}
		else 
		{
			size_t cntUser{ 0 }, cntMember{ 0 }, cntDiceMaid{ 0 };
			for (auto& each : list) 
			{
				if (each == console.DiceMaid)continue;
				cntMember++;
				if (UserList.count(each)) 
				{
					cntUser++;
					ave_trust += getUser(each).nTrust;
				}
				if (DD::isGroupAdmin(fromGroup, each, false))
				{
					max_trust |= (1 << trustedQQ(each));
					if (blacklist->get_qq_danger(each) > 1)
					{
						strMsg += ",���ֺ���������Ա" + printQQ(each);
						if (grp.isset("���"))
						{
							strMsg += "��Ⱥ��ڣ�";
						}
						else
						{
							DD::sendGroupMsg(fromGroup, blacklist->list_qq_warning(each));
							grp.leave("���ֺ���������Ա" + printQQ(each) + "��Ԥ������Ⱥ");
							strMsg += "������Ⱥ";
							console.log(strMsg, 0b10, strNow);
							return true;
						}
					}
					if (DD::isGroupOwner(fromGroup, each, false))
					{
						ownerQQ = each;
						ave_trust += (gsize.currSize - 1) * trustedQQ(each);
						strMsg += "��Ⱥ��" + printQQ(each) + "��";
					}
					else
					{
						ave_trust += (gsize.currSize - 10) * trustedQQ(each) / 10;
					}
				}
				else if (blacklist->get_qq_danger(each) > 1)
				{
					//max_trust |= 1;
					blacks << printQQ(each);
					if (blacklist->get_qq_danger(each)) 
					{
						AddMsgToQueue(blacklist->list_self_qq_warning(each), fromGroup, msgtype::Group);
					}
				}
				if (DD::isDiceMaid(each)) {
					++cntDiceMaid;
				}
			}
			if (!chat(fromGroup).inviter && list.size() == 2 && ownerQQ)
			{
				chat(fromGroup).inviter = ownerQQ;
				strMsg += "������" + printQQ(ownerQQ);
			}
			if (!cntMember) 
			{
				strMsg += "��ȺԱ����δ���أ�";
			}
			else 
			{
				ave_trust /= cntMember;
				strMsg += "\n�û�Ũ��" + to_string(cntUser * 100 / cntMember) + "% (" + to_string(cntUser) + "/" + to_string(cntMember) + "), ���ζ�" + toString(ave_trust);
			}
			if (cntDiceMaid) 			{
				strMsg += "\n��ʶ��ͬϵDice!" + to_string(cntDiceMaid) + "λ";
			}
		}
		if (!blacks.empty())
		{
			string strNote = "\n���ֺ�����ȺԱ" + blacks.show();
			strMsg += strNote;
		}
		if (console["Private"] && !grp.isset("���ʹ��"))
		{	
			//����СȺ�ƹ�����û���ϰ�����
			if (max_trust > 1 || ave_trust > 0.5)
			{
				grp.set("���ʹ��");
				strMsg += "\n���Զ�׷��ʹ�����";
			}
			else 
			{
				strMsg += "\n�����ʹ�ã�����Ⱥ";
				console.log(strMsg, 1, strNow);
				grp.leave(getMsg("strPreserve"));
				return true;
			}
		}
		if (!blacks.empty())console.log(strMsg, 0b10, strNow);
		else console.log(strMsg, 1, strNow);
	}
	catch (...)
	{
		console.log(strMsg + "\nȺ" + to_string(fromGroup) + "��Ϣ��ȡʧ�ܣ�", 0b1, printSTNow());
		return true;
	}
	if (grp.isset("Э����Ч")) return 0;
	if (!getMsg("strAddGroup").empty())
	{
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strAddGroup"), {fromGroup, msgtype::Group});
	}
	if (console["CheckGroupLicense"] && !grp.isset("���ʹ��"))
	{
		grp.set("δ���");
		this_thread::sleep_for(2s);
		AddMsgToQueue(getMsg("strGroupLicenseDeny"), {fromGroup, msgtype::Group});
	}
	return false;
}

//��������ָ��

EVE_PrivateMsg(eventPrivateMsg)
{
	if (!Enabled)return 0;
	if (fromQQ == console.DiceMaid && !console["ListenSelfEcho"])return 0;
	shared_ptr<FromMsg> Msg(make_shared<FromMsg>(message, fromQQ));
	return Msg->DiceFilter();
}

EVE_GroupMsg(eventGroupMsg)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGroup).group().lastmsg(time(nullptr));
	if (fromQQ == console.DiceMaid && !console["ListenGroupEcho"])return 0;
	if (grp.isset("δ��") || grp.isset("����"))eve_GroupAdd(grp);
	if (!grp.isset("����"))
	{
		shared_ptr<FromMsg> Msg(make_shared<FromMsg>(message, fromGroup, msgtype::Group, fromQQ));
		return Msg->DiceFilter();
	}
	return grp.isset("������Ϣ");
}

EVE_DiscussMsg(eventDiscussMsg)
{
	if (!Enabled)return 0;
	//time_t tNow = time(NULL);
	if (console["LeaveDiscuss"])
	{
		DD::sendDiscussMsg(fromDiscuss, getMsg("strLeaveDiscuss"));
		this_thread::sleep_for(1000ms);
		DD::setDiscussLeave(fromDiscuss);
		return 1;
	}
	Chat& grp = chat(fromDiscuss).discuss().lastmsg(time(nullptr));
	if (blacklist->get_qq_danger(fromQQ) && console["AutoClearBlack"])
	{
		const string strMsg = "���ֺ������û�" + printQQ(fromQQ) + "���Զ�ִ����Ⱥ";
		console.log(printChat({fromDiscuss, msgtype::Discuss}) + strMsg, 0b10, printSTNow());
		grp.leave(strMsg);
		return 1;
	}
	shared_ptr<FromMsg> Msg(make_shared<FromMsg>(message, fromDiscuss, msgtype::Discuss, fromQQ));
	return Msg->DiceFilter() || grp.isset("������Ϣ");
}

EVE_GroupMemberIncrease(eventGroupMemberAdd)
{
	if (!Enabled)return 0;
	Chat& grp = chat(fromGroup);
	if (grp.isset("����"))return 0;
	if (fromQQ != console.DiceMaid)
	{
		if (chat(fromGroup).strConf.count("��Ⱥ��ӭ"))
		{
			string strReply = chat(fromGroup).strConf["��Ⱥ��ӭ"];
			while (strReply.find("{at}") != string::npos)
			{
				strReply.replace(strReply.find("{at}"), 4, "[CQ:at,qq=" + to_string(fromQQ) + "]");
			}
			while (strReply.find("{@}") != string::npos)
			{
				strReply.replace(strReply.find("{@}"), 3, "[CQ:at,qq=" + to_string(fromQQ) + "]");
			}
			while (strReply.find("{nick}") != string::npos)
			{
				strReply.replace(strReply.find("{nick}"), 6, DD::getQQNick(fromQQ));
			}
			while (strReply.find("{qq}") != string::npos)
			{
				strReply.replace(strReply.find("{qq}"), 4, to_string(fromQQ));
			}
			grp.update(time(nullptr));
			AddMsgToQueue(strip(strReply), fromGroup, msgtype::Group);
		}
		if (blacklist->get_qq_danger(fromQQ))
		{
			const string strNow = printSTNow();
			string strNote = printGroup(fromGroup) + "����" + getMsg("strSelfName") + "�ĺ������û�" + printQQ(
				fromQQ) + "��Ⱥ";
			AddMsgToQueue(blacklist->list_self_qq_warning(fromQQ), fromGroup, msgtype::Group);
			if (grp.isset("����"))strNote += "��Ⱥ���壩";
			else if (grp.isset("���"))strNote += "��Ⱥ��ڣ�";
			else if (grp.isset("Э����Ч"))strNote += "��ȺЭ����Ч��";
			else if (DD::isGroupAdmin(fromGroup, console.DiceMaid, false))strNote += "��Ⱥ����Ȩ�ޣ�";
			else if (console["LeaveBlackQQ"])
			{
				strNote += "������Ⱥ��";
				grp.leave("���ֺ������û�" + printQQ(fromQQ) + "��Ⱥ,��Ԥ������Ⱥ");
			}
			console.log(strNote, 0b10, strNow);
		}
	}
	else{
		if (!grp.inviter)grp.inviter = operatorQQ;
		{
			unique_lock<std::mutex> lock_queue(GroupAddMutex);
			if (!grp.tLastMsg)grp.set("δ��");
		}
		return eve_GroupAdd(grp);
	}
	return 0;
}

EVE_GroupMemberKicked(eventGroupMemberKicked){
	if (!Enabled) return 0;
	if (fromQQ == 0)return 0; // ����Mirai�ڻ�����������ȺʱҲ�����һ���������
	Chat& grp = chat(fromGroup);
	if (beingOperateQQ == console.DiceMaid)
	{
		grp.set("����");
		if (!console || grp.isset("����"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printQQ(fromQQ) + "��" + printQQ(beingOperateQQ) + "�Ƴ���" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (!console["ListenGroupKick"] || trustedQQ(fromQQ) > 1 || grp.isset("���") || grp.isset("Э����Ч") || ExceptGroups.count(fromGroup)) return 0;
		DDBlackMarkFactory mark{fromQQ, fromGroup};
		mark.sign().type("kick").time(strNow).note(strNow + " " + strNote).comment(getMsg("strSelfCall") + "ԭ����¼");
		if (grp.inviter && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";��Ⱥ�����ߣ�" + printQQ(grp.inviter);
			if (console["KickedBanInviter"])mark.inviterQQ(grp.inviter).note(strNow + " " + strNote);
		}
		grp.reset("���ʹ��").reset("����");
		blacklist->create(mark.product());
	}
	else if (mDiceList.count(beingOperateQQ) && console["ListenGroupKick"])
	{
		if (!console || grp.isset("����"))return 0;
		string strNow = printSTime(stNow);
		string strNote = printQQ(fromQQ) + "��" + printQQ(beingOperateQQ) + "�Ƴ���" + printChat(grp);
		console.log(strNote, 0b1000, strNow);
		if (trustedQQ(fromQQ) > 1 || grp.isset("���") || grp.isset("Э����Ч") || ExceptGroups.count(fromGroup)) return 0;
		DDBlackMarkFactory mark{fromQQ, fromGroup};
		mark.type("kick").time(strNow).note(strNow + " " + strNote).DiceMaid(beingOperateQQ).masterQQ(mDiceList[beingOperateQQ]).comment(strNow + " " + printQQ(console.DiceMaid) + "Ŀ��");
		grp.reset("���ʹ��").reset("����");
		blacklist->create(mark.product());
	}
	return 0;
}

EVE_GroupBan(eventGroupBan)
{
	if (!Enabled) return 0;
	Chat& grp = chat(fromGroup);
	if (grp.isset("����") || (beingOperateQQ != console.DiceMaid && !mDiceList.count(beingOperateQQ)) || !console[
		"ListenGroupBan"])return 0;
	if (!duration || !duration[0])
	{
		if (beingOperateQQ == console.DiceMaid)
		{
			console.log(getMsg("strSelfName") + "��" + printGroup(fromGroup) + "�б��������", 0b10, printSTNow());
			return 1;
		}
	}
	else
	{
		string strNow = printSTNow();
		long long llOwner = 0;
		string strNote = "��" + printGroup(fromGroup) + "��," + printQQ(beingOperateQQ) + "��" + printQQ(operatorQQ) + "����" + duration;
		if (!console["ListenGroupBan"] || trustedQQ(operatorQQ) > 1 || grp.isset("���") || grp.isset("Э����Ч") || ExceptGroups.count(fromGroup)) 
		{
			console.log(strNote, 0b10, strNow);
			return 1;
		}
		DDBlackMarkFactory mark{operatorQQ, fromGroup};
		mark.type("ban").time(strNow).note(strNow + " " + strNote);
		if (mDiceList.count(operatorQQ))mark.fromQQ(0);
		if (beingOperateQQ == console.DiceMaid)
		{
			if (!console)return 0;
			mark.sign().comment(getMsg("strSelfCall") + "ԭ����¼");
		}
		else
		{
			mark.DiceMaid(beingOperateQQ).masterQQ(mDiceList[beingOperateQQ]).comment(strNow + " " + printQQ(console.DiceMaid) + "Ŀ��");
		}
		//ͳ��Ⱥ�ڹ���
		int intAuthCnt = 0;
		string strAuthList;
		for (auto admin : DD::getGroupAdminList(fromGroup))
		{
			if (DD::isGroupOwner(fromGroup, admin, false)) {
				llOwner = admin;
			}
			else 
			{
				strAuthList += '\n' + printQQ(admin);
				intAuthCnt++;
			}
		}
		strAuthList = "��Ⱥ��" + printQQ(llOwner) + ",���й���Ա" + to_string(intAuthCnt) + "��" + strAuthList;
		mark.note(strNow + " " + strNote);
		if (grp.inviter && beingOperateQQ == console.DiceMaid && trustedQQ(grp.inviter) < 2)
		{
			strNote += ";��Ⱥ�����ߣ�" + printQQ(grp.inviter);
			if (console["BannedBanInviter"])mark.inviterQQ(grp.inviter);
			mark.note(strNow + " " + strNote);
		}
		grp.reset("����").reset("���ʹ��").leave();
		console.log(strNote + strAuthList, 0b1000, strNow);
		blacklist->create(mark.product());
		return 1;
	}
	return 0;
}

EVE_GroupInvited(eventGroupInvited)
{
	if (!Enabled) return 0;
	if (!console["ListenGroupRequest"])return 0;
	if (groupset(fromGroup, "����") < 1)
	{
		this_thread::sleep_for(3s);
		const string strNow = printSTNow();
		string strMsg = "Ⱥ����������ԣ�" + printQQ(fromQQ) + ",Ⱥ:" +
			DD::printGroupInfo(fromGroup);
		if (ExceptGroups.count(fromGroup)) {
			strMsg += "\n�Ѻ��ԣ�Ĭ��Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGroup, 3);
		}
		else if (blacklist->get_group_danger(fromGroup))
		{
			strMsg += "\n�Ѿܾ���Ⱥ�ں������У�";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGroup, 2);
		}
		else if (blacklist->get_qq_danger(fromQQ))
		{
			strMsg += "\n�Ѿܾ����û��ں������У�";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGroup, 2);
		}
		else if (Chat& grp = chat(fromGroup).group(); grp.isset("���ʹ��")) {
			grp.set("δ��");
			grp.inviter = fromQQ;
			strMsg += "\n��ͬ�⣨Ⱥ�����ʹ�ã�";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGroup, 1);
		}
		else if (trustedQQ(fromQQ))
		{
			grp.set("���ʹ��").set("δ��").reset("δ���").reset("Э����Ч");
			grp.inviter = fromQQ;
			strMsg += "\n��ͬ�⣨�������û���";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGroup, 1);
		}
		else if (grp.isset("Э����Ч")) {
			grp.set("δ��");
			strMsg += "\n�Ѻ��ԣ�Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGroup, 3);
		}
		else if (console["GroupInvalidSize"] > 0 && DD::getGroupSize(grp.ID).currSize > (size_t)console["GroupInvalidSize"]) {
			grp.set("Э����Ч");
			strMsg += "\n�Ѻ��ԣ���ȺĬ��Э����Ч��";
			console.log(strMsg, 0b10, strNow);
			DD::answerGroupInvited(fromGroup, 3);
		}
		else if (console && console["Private"])
		{
			DD::sendPrivateMsg(fromQQ, getMsg("strPreserve"));
			strMsg += "\n�Ѿܾ�����ǰ��˽��ģʽ��";
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGroup, 2);
		}
		else
		{
			grp.set("δ��");
			grp.inviter = fromQQ;
			strMsg += "��ͬ��";
			this_thread::sleep_for(2s);
			console.log(strMsg, 1, strNow);
			DD::answerGroupInvited(fromGroup, true);
		}
		return 1;
	}
	return 0;
}
EVE_FriendRequest(eventFriendRequest) {
	if (!Enabled) return 0;
	if (!console["ListenFriendRequest"])return 0;
	string strMsg = "��������������� " + printQQ(fromQQ) + ":" + message;
	this_thread::sleep_for(3s);
	if (blacklist->get_qq_danger(fromQQ)) {
		strMsg += "\n�Ѿܾ����û��ں������У�";
		DD::answerFriendRequest(fromQQ, 2, "");
		console.log(strMsg, 0b10, printSTNow());
	}
	else if (trustedQQ(fromQQ)) {
		strMsg += "\n��ͬ�⣨�������û���";
		DD::answerFriendRequest(fromQQ, 1, getMsg("strAddFriendWhiteQQ"));
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 2 && !UserList.count(fromQQ)) {
		strMsg += "\n�Ѿܾ������û���¼��";
		DD::answerFriendRequest(fromQQ, 2, getMsg("strFriendDenyNotUser"));
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["AllowStranger"] < 1) {
		strMsg += "\n�Ѿܾ����������û���";
		DD::answerFriendRequest(fromQQ, 2, getMsg("strFriendDenyNoTrust"));
		console.log(strMsg, 1, printSTNow());
	}
	else {
		strMsg += "\n��ͬ��";
		DD::answerFriendRequest(fromQQ, 1, getMsg("strAddFriend"));
		console.log(strMsg, 1, printSTNow());
	}
	return 1;
}
EVE_FriendAdded(eventFriendAdd) {
	if (!Enabled) return 0;
	if (!console["ListenFriendAdd"])return 0;
	this_thread::sleep_for(3s);
	getMsg("strAddFriendWhiteQQ").empty()
		? AddMsgToQueue(getMsg("strAddFriend"), fromQQ)
		: AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromQQ);
	return 0;
}

EVE_Menu(eventMasterMode)
{
	if (!Enabled) return 0;
	if (console)
	{
		console.isMasterMode = false;
		console.killMaster();
#ifdef _WIN32
		MessageBoxA(nullptr, "Masterģʽ�ѹرա�\nmaster�����", "Masterģʽ�л�", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else
	{
		console.isMasterMode = true;
		console.save();
#ifdef _WIN32
		MessageBoxA(nullptr, "Masterģʽ�ѿ�����\n����������﷢��.master public/private", "Masterģʽ�л�", MB_OK | MB_ICONINFORMATION);
#endif
	}
	return 0;
}

#ifdef _WIN32
#include <shellapi.h>
EVE_Menu(eventGUI)
{
	if (!Enabled) return 0;
	auto port{ ManagerServer->getListeningPorts()[0] };
	ShellExecute(nullptr, TEXT("open"), (TEXT("http://127.0.0.1:") + std::to_wstring(port)).c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}
#endif

void global_exit() {
	Enabled = false;
	mg_exit_library();
	ManagerServer = nullptr;
	ExtensionManagerInstance = nullptr;
	dataBackUp();
	sch.end();
	censor = {};
	fmt.reset();
	gm.reset();
	PList.clear();
	ChatList.clear();
	UserList.clear();
	console.reset();
	EditedMsg.clear();
	blacklist.reset();
	Aws::ShutdownAPI(options);
#ifndef _WIN32
	curl_global_cleanup();
#endif
	threads.exit();
}

EVE_Disable(eventDisable)
{
	global_exit();
}

EVE_Exit(eventExit)
{
	if (Enabled)global_exit();
}

EVE_Menu(eventGlobalSwitch) {
	if (!Enabled) return 0;
	if (console["DisabledGlobal"]) {
		console.set("DisabledGlobal", 0);
#ifdef _WIN32
		MessageBoxA(nullptr, "�����ѽ�����Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
#endif
	}
	else {
		console.set("DisabledGlobal", 1);
#ifdef _WIN32
		MessageBoxA(nullptr, "������ȫ�־�Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
#endif
	}

	return 0;
}