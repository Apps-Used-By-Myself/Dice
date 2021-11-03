#include "DDAPI.h"
#include "DiceEvent.h"
#include "Jsonio.h"
#include "MsgFormat.h"
#include "DiceCensor.h"
#include "DiceMod.h"
#include "ManagerSystem.h"
#include "BlackListManager.h"
#include "CharacterCard.h"
#include "DiceSession.h"
#include "GetRule.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"
#include "DiceGUI.h"
#include "DiceStatic.hpp"
#include <memory>
#include <ctime>
using namespace std;

FromMsg& FromMsg::initVar(const std::initializer_list<const std::string>& replace_str) {
	int index = 0;
	for (const auto& s : replace_str) {
		strVar[to_string(index++)] = s;
	}
	return *this;
}
void FromMsg::formatReply() {
	if (!strVar.count("nick") || strVar["nick"].empty())strVar["nick"] = getName(fromQQ, fromGroup);
	if (!strVar.count("pc") || strVar["pc"].empty())getPCName(*this);
	if (!strVar.count("at") || strVar["at"].empty())strVar["at"] = fromChat.second != msgtype::Private ? "[CQ:at,qq=" + to_string(fromQQ) + "]" : strVar["nick"];
	if (msgMatch.ready())strReply = convert_realw2a(msgMatch.format(convert_a2realw(strReply.c_str())).c_str());
	strReply = format(strReply, GlobalMsg, strVar);
}

void FromMsg::reply(const std::string& msgReply, bool isFormat) {
	strReply = msgReply;
	reply(isFormat);
}

void FromMsg::reply(const char* msgReply, bool isFormat) {
	strReply = msgReply;
	reply(isFormat);
}

void FromMsg::reply(const std::string& msgReply, const std::initializer_list<const std::string>& replace_str) {
	initVar(replace_str);
	strReply = msgReply;
	reply();
}

void FromMsg::reply(bool isFormat) {
	if (isVirtual && fromQQ == console.DiceMaid && fromChat.second == msgtype::Private)return;
	isAns = true;
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	if (isFormat)
		formatReply();
	AddMsgToQueue(strReply, fromChat);
	if (LogList.count(fromSession) && gm->session(fromSession).is_logging()) {
		filter_CQcode(strReply, fromGroup);
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		logout << GBKtoUTF8(getMsg("strSelfName")) + "(" + to_string(console.DiceMaid) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(strReply) << endl << endl;
	}
}

void FromMsg::replyHidden(const std::string& msgReply) {
	strReply = msgReply;
	replyHidden();
}
void FromMsg::replyHidden() {
	isAns = true;
	while (isspace(static_cast<unsigned char>(strReply[0])))
		strReply.erase(strReply.begin());
	formatReply();
	if (LogList.count(fromSession) && gm->session(fromSession).is_logging()
		&& (fromChat.second == msgtype::Private || !console["ListenGroupEcho"])) {
		filter_CQcode(strReply, fromGroup);
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		logout << GBKtoUTF8(getMsg("strSelfName")) + "(" + to_string(console.DiceMaid) + ") " + printTTime(fromTime) << endl
			<< '*' << GBKtoUTF8(strReply) << endl << endl;
	}
	strReply = "��" + printChat(fromChat) + "�� " + strReply;
	AddMsgToQueue(strReply, fromQQ);
	if (gm->has_session(fromSession)) {
		for (auto qq : gm->session(fromSession).get_ob()) {
			if (qq != fromQQ) {
				AddMsgToQueue(strReply, qq);
			}
		}
	}
}

void FromMsg::fwdMsg()
{
	if (LinkList.count(fromSession) && LinkList[fromSession].second && fromQQ != console.DiceMaid && strLowerMessage.find(".link") != 0)
	{
		string strFwd;
		if (trusted < 5)strFwd += printFrom();
		strFwd += strMsg;
		if (long long aim = LinkList[fromSession].first;aim < 0) {
			AddMsgToQueue(strFwd, ~aim);
		}
		else if (ChatList.count(aim)) {
			AddMsgToQueue(strFwd, aim, chat(aim).isGroup ? msgtype::Group : msgtype::Discuss);
		}
	}
	if (LogList.count(fromSession) && strLowerMessage.find(".log") != 0) {
		string msg = strMsg;
		filter_CQcode(msg, fromGroup);
		ofstream logout(gm->session(fromSession).log_path(), ios::out | ios::app);
		if (!strVar.count("nick") || strVar["nick"].empty())strVar["nick"] = getName(fromQQ, fromGroup);
		if (!strVar.count("pc") || strVar["pc"].empty())getPCName(*this);
		logout << GBKtoUTF8(strVar["pc"]) + "(" + to_string(fromQQ) + ") " + printTTime(fromTime) << endl
			<< GBKtoUTF8(msg) << endl << endl;
	}
}

int FromMsg::AdminEvent(const string& strOption)
{
	if (strOption == "isban")
	{
		strVar["target"] = readDigit();
		blacklist->isban(this);
		return 1;
	}
	if (strOption == "state")
	{
		ResList res;
		res << "Servant:" + printQQ(console.DiceMaid)
			<< "Master:" + printQQ(console.master())
			<< console.listClock().dot("\t").show()
			<< (console["Private"] ? "˽��ģʽ" : "����ģʽ");
		if (console["LeaveDiscuss"])res << "����������";
		if (console["DisabledGlobal"])res << "ȫ�־�Ĭ��";
		if (console["DisabledMe"])res << "ȫ�ֽ���.me";
		if (console["DisabledJrrp"])res << "ȫ�ֽ���.jrrp";
		if (console["DisabledDraw"])res << "ȫ�ֽ���.draw";
		if (console["DisabledSend"])res << "ȫ�ֽ���.send";
		if (trusted > 3)
			res << "����Ⱥ������" + to_string(DD::getGroupIDList().size())
			<< "Ⱥ��¼����" + to_string(ChatList.size())
			<< "��������" + to_string(DD::getFriendQQList().size())
			<< "�û���¼����" + to_string(UserList.size())
			<< "�����û�����" + to_string(today->cnt())
			<< (!PList.empty() ? "��ɫ����¼����" + to_string(PList.size()) : "")
			<< "�������û�����" + to_string(blacklist->mQQDanger.size())
			<< "������Ⱥ����" + to_string(blacklist->mGroupDanger.size())
			<< (censor.size() ? "���дʿ��ģ��" + to_string(censor.size()) : "");
		reply(getMsg("strSelfName") + "�ĵ�ǰ���" + res.show());
		return 1;
	}
	if (trusted < 4)
	{
		reply(getMsg("strNotAdmin"));
		return -1;
	}
	if (auto it = Console::intDefault.find(strOption);it != Console::intDefault.end())
	{
		int intSet = 0;
		switch (readNum(intSet))
		{
		case 0:
			console.set(it->first, intSet);
			note("�ѽ�" + getMsg("strSelfName") + "��" + it->first + "����Ϊ" + to_string(intSet), 0b10);
			break;
		case -1:
			reply(getMsg("strSelfName") + "����Ϊ" + to_string(console[strOption.c_str()]));
			break;
		case -2:
			reply("{nick}���ò���������Χ��");
			break;
		}
		return 1;
	}
	if (strOption == "delete")
	{
		note("�Ѿ���������ԱȨ�ޡ�", 0b100);
		getUser(fromQQ).trust(3);
		console.NoticeList.erase({fromQQ, msgtype::Private});
		return 1;
	}
	if (strOption == "on")
	{
		if (console["DisabledGlobal"])
		{
			console.set("DisabledGlobal", 0);
			note("��ȫ�ֿ���" + getMsg("strSelfName"), 3);
		}
		else
		{
			reply(getMsg("strSelfName") + "���ھ�Ĭ�У�");
		}
		return 1;
	}
	if (strOption == "off")
	{
		if (console["DisabledGlobal"])
		{
			reply(getMsg("strSelfName") + "�Ѿ���Ĭ��");
		}
		else
		{
			console.set("DisabledGlobal", 1);
			note("��ȫ�ֹر�" + getMsg("strSelfName"), 0b10);
		}
		return 1;
	}
	if (strOption == "dicelist")
	{
		getDiceList();
		strReply = "��ǰ�����б�";
		for (auto& [diceQQ, masterQQ] : mDiceList)
		{
			strReply += "\n" + printQQ(diceQQ);
		}
		reply();
		return 1;
	}
	if (strOption == "censor") {
		readSkipSpace();
		if (strMsg[intMsgCnt] == '+') {
			intMsgCnt++;
			strVar["danger_level"] = readToColon();
			Censor::Level danger_level = censor.get_level(strVar["danger_level"]);
			readSkipColon();
			ResList res;
			while (intMsgCnt != strMsg.length()) {
				string item = readItem();
				if (!item.empty()) {
					censor.add_word(item, danger_level);
					res << item;
				}
			}
			if (res.empty()) {
				reply("{nick}δ�����������дʣ�");
			}
			else {
				note("{nick}�����{danger_level}�����д�" + to_string(res.size()) + "��:" + res.show(), 1);
			}
		}
		else if (strMsg[intMsgCnt] == '-') {
			intMsgCnt++;
			ResList res,resErr;
			while (intMsgCnt != strMsg.length()) {
				string item = readItem();
				if (!item.empty()) {
					if (censor.rm_word(item))
						res << item;
					else
						resErr << item;
				}
			}
			if (res.empty()) {
				reply("{nick}δ������Ƴ����дʣ�");
			}
			else {
				note("{nick}���Ƴ����д�" + to_string(res.size()) + "��:" + res.show(), 1);
			}
			if (!resErr.empty())
				reply("{nick}�Ƴ����������д�" + to_string(resErr.size()) + "��:" + resErr.show());
		}
		else
			reply(fmt->get_help("censor"));
		return 1;
	}
	if (strOption == "only")
	{
		if (console["Private"])
		{
			reply(getMsg("strSelfName") + "�ѳ�Ϊ˽�����");
		}
		else
		{
			console.set("Private", 1);
			note("�ѽ�" + getMsg("strSelfName") + "��Ϊ˽�á�", 0b10);
		}
		return 1;
	}
	if (strOption == "public")
	{
		if (console["Private"])
		{
			console.set("Private", 0);
			note("�ѽ�" + getMsg("strSelfName") + "��Ϊ���á�", 0b10);
		}
		else
		{
			reply(getMsg("strSelfName") + "�ѳ�Ϊ�������");
		}
		return 1;
	}
	if (strOption == "clock")
	{
		bool isErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			isErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+')
		{
			intMsgCnt++;
		}
		string strType = readPara();
		if (strType.empty())
		{
			reply(getMsg("strSelfName") + "�Ķ�ʱ�б�" + console.listClock().show());
			return 1;
		}
		Console::Clock cc{0, 0};
		switch (readClock(cc))
		{
		case 0:
			if (isErase)
			{
				if (console.rmClock(cc, strType))reply(
					getMsg("strSelfName") + "�޴˶�ʱ��Ŀ");
				else note("���Ƴ�" + getMsg("strSelfName") + "��" + printClock(cc) + "�Ķ�ʱ" + strType, 0b10);
			}
			else
			{
				console.setClock(cc, strType);
				note("������" + getMsg("strSelfName") + "��" + printClock(cc) + "�Ķ�ʱ" + strType, 0b10);
			}
			break;
		case -1:
			reply(getMsg("strParaEmpty"));
			break;
		case -2:
			reply(getMsg("strParaIllegal"));
			break;
		default: break;
		}
		return 1;
	}
	if (strOption == "notice")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		else if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		if (chatType cTarget; readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("��ǰ֪ͨ����" + to_string(list.size()) + "����" + list.show());
			return 1;
		}
		else
		{
			if (boolErase)
			{
				console.rmNotice(cTarget);
				note("�ѽ�" + getMsg("strSelfName") + "��֪ͨ����" + printChat(cTarget) + "�Ƴ�", 0b1);
				return 1;
			}
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-')
			{
				int intAdd = 0;
				int intReduce = 0;
				while (intMsgCnt < strMsg.length())
				{
					bool isReduce = strMsg[intMsgCnt] == '-';
					string strNum = readDigit();
					if (strNum.empty() || strNum.length() > 1)break;
					if (int intNum = stoi(strNum); intNum > 9)continue;
					else
					{
						if (isReduce)intReduce |= (1 << intNum);
						else intAdd |= (1 << intNum);
					}
					readSkipSpace();
				}
				if (intAdd)console.addNotice(cTarget, intAdd);
				if (intReduce)console.redNotice(cTarget, intReduce);
				if (intAdd | intReduce)note(
					"�ѽ�" + getMsg("strSelfName") + "�Դ���" + printChat(cTarget) + "֪ͨ�������Ϊ" + to_binary(
						console.showNotice(cTarget)), 0b1);
				else reply(getMsg("strParaIllegal"));
				return 1;
			}
			int intLV;
			switch (readNum(intLV))
			{
			case 0:
				if (intLV < 0 || intLV > 63)
				{
					reply(getMsg("strParaIllegal"));
					return 1;
				}
				console.setNotice(cTarget, intLV);
				note("�ѽ�" + getMsg("strSelfName") + "�Դ���" + printChat(cTarget) + "֪ͨ�������Ϊ" + to_string(intLV), 0b1);
				break;
			case -1:
				reply("����" + printChat(cTarget) + "��" + getMsg("strSelfName") + "����֪ͨ����Ϊ��" + to_binary(
					console.showNotice(cTarget)));
				break;
			case -2:
				reply(getMsg("strParaIllegal"));
				break;
			}
		}
		return 1;
	}
	if (strOption == "ext")
	{
		try
		{
			string action = readPara();	
			if (action == "install")
			{
				string package = readRest();
				ExtensionManagerInstance->installPackage(GBKtoUTF8(package));
				reply("�ѳɹ���װ" + package);
			}
			else if (action == "query")
			{
				string package = readRest();
				reply(ExtensionManagerInstance->queryPackage(GBKtoUTF8(package)));
			}
			else if (action == "update")
			{
				ExtensionManagerInstance->refreshIndex();
				reply("�ѳɹ�ˢ����������棬" + to_string(ExtensionManagerInstance->getIndexCount()) + "����չ���ã�"
					+ to_string(ExtensionManagerInstance->getUpgradableCount()) + "��������");
			}
			else if (action == "list")
			{
				string re = "������չ:\n";
				auto index = ExtensionManagerInstance->getIndex();
				for (const auto& i : index)
				{
					re += UTF8toGBK(i.second.name) + " ";
				}
				reply(re);
			}
			else if (action == "search")
			{
				string package = readRest();
				string re = "�������:\n";
				auto index = ExtensionManagerInstance->getIndex();
				for (const auto& i : index)
				{
					string GBKname = UTF8toGBK(i.second.name);
					if(GBKname.find(package) != string::npos)
					{
						re += GBKname + " ";
					}		
				}
				reply(re);
			}
			else if (action == "listinstalled")
			{
				string re = "�Ѱ�װ��չ:\n";
				auto index = ExtensionManagerInstance->getInstalledIndex();
				for (const auto& i : index)
				{
					re += UTF8toGBK(i.second.first.name) + " ";
				}
				reply(re);
			}
			else if (action == "remove")
			{
				string package = readRest();
				ExtensionManagerInstance->removePackage(GBKtoUTF8(package));
				reply("�ѳɹ�ж��" + package);
			}
			else if (action == "queryinstalled")
			{
				string package = readRest();
				reply(ExtensionManagerInstance->queryInstalledPackage(GBKtoUTF8(package)).first);
			}
			else if (action == "searchinstalled")
			{
				string package = readRest();
				string re = "�������:\n";
				auto index = ExtensionManagerInstance->getInstalledIndex();
				for (const auto& i : index)
				{
					string GBKname = UTF8toGBK(i.second.first.name);
					if (GBKname.find(package) != string::npos)
					{
						re += GBKname + " ";
					}		
				}
				reply(re);
			}
			else if (action == "upgrade")
			{
				string package = readRest();
				if (package.empty())
				{
					int cnt = ExtensionManagerInstance->upgradeAllPackages();
					reply("�ɹ�����" + std::to_string(cnt) + "����չ");
				}
				else
				{
					if (ExtensionManagerInstance->upgradePackage(GBKtoUTF8(package)))
					{
						reply(package + "�ѳɹ�������");
					}
					else
					{
						reply(package + "��������");
					}
				}
			}
			else 
			{
				reply("Unknown command");
			}
		}
		catch (const std::exception& e)
		{
			reply(UTF8toGBK(e.what(), true));
		}
		return 1;
	}
	if (strOption == "recorder")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		chatType cTarget;
		if (readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("��ǰ֪ͨ����" + to_string(list.size()) + "����" + list.show());
			return 1;
		}
		if (boolErase)
		{
			if (console.showNotice(cTarget) & 0b1)
			{
				note("��ֹͣ����" + printChat(cTarget) + "��", 0b1);
				console.redNotice(cTarget, 0b1);
			}
			else
			{
				reply("�ô��ڲ�������־֪ͨ��");
			}
		}
		else
		{
			if (console.showNotice(cTarget) & 0b1)
			{
				reply("�ô����ѽ�����־֪ͨ��");
			}
			else
			{
				console.addNotice(cTarget, 0b11011);
				reply("�������־����" + printChat(cTarget) + "��");
			}
		}
		return 1;
	}
	if (strOption == "monitor")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		chatType cTarget;
		if (readChat(cTarget))
		{
			ResList list = console.listNotice();
			reply("��ǰ֪ͨ����" + to_string(list.size()) + "����" + list.show());
			return 1;
		}
		if (boolErase)
		{
			if (console.showNotice(cTarget) & 0b100000)
			{
				console.redNotice(cTarget, 0b100000);
				note("���Ƴ����Ӵ���" + printChat(cTarget) + "��", 0b1);
			}
			else
			{
				reply("�ô��ڲ������ڼ����б�");
			}
		}
		else
		{
			if (console.showNotice(cTarget) & 0b100000)
			{
				reply("�ô����Ѵ����ڼ����б�");
			}
			else
			{
				console.addNotice(cTarget, 0b100000);
				note("����Ӽ��Ӵ���" + printChat(cTarget) + "��", 0b1);
			}
		}
		return 1;
	}
	if (strOption == "blackfriend")
	{
		ResList res;
		for(long long qq: DD::getFriendQQList()){
			if (blacklist->get_qq_danger(qq))
				res << printQQ(qq);
		}
		if (res.empty())
		{
			reply("�����б����޺������û���", false);
		}
		else
		{
			reply("�����б��ں������û���" + res.show(), false);
		}
		return 1;
	}
	if (strOption == "whitegroup")
	{
		readSkipSpace();
		bool isErase = false;
		if (strMsg[intMsgCnt] == '-')
		{
			intMsgCnt++;
			isErase = true;
		}
		if (string strGroup; !(strGroup = readDigit()).empty())
		{
			long long llGroup = stoll(strGroup);
			if (isErase)
			{
				if (groupset(llGroup, "���ʹ��") > 0 || groupset(llGroup, "����") > 0)
				{
					chat(llGroup).reset("���ʹ��").reset("����");
					note("���Ƴ�" + printGroup(llGroup) + "��" + getMsg("strSelfName") + "��ʹ�����");
				}
				else
				{
					reply("��Ⱥδӵ��" + getMsg("strSelfName") + "��ʹ����ɣ�");
				}
			}
			else
			{
				if (groupset(llGroup, "���ʹ��") > 0)
				{
					reply("��Ⱥ��ӵ��" + getMsg("strSelfName") + "��ʹ����ɣ�");
				}
				else
				{
					chat(llGroup).set("���ʹ��").reset("δ���");
					if (!chat(llGroup).isset("����") && !chat(llGroup).isset("δ��"))AddMsgToQueue(
						getMsg("strAuthorized"), llGroup, msgtype::Group);
					note("�����" + printGroup(llGroup) + "��" + getMsg("strSelfName") + "��ʹ�����");
				}
			}
			return 1;
		}
		ResList res;
		for (auto& [id, grp] : ChatList)
		{
			string strGroup;
			if (grp.isset("���ʹ��") || grp.isset("����") || grp.isset("���"))
			{
				strGroup = printChat(grp);
				if (grp.isset("���ʹ��"))strGroup += "-���ʹ��";
				if (grp.isset("����"))strGroup += "-����";
				if (grp.isset("���"))strGroup += "-���";
				res << strGroup;
			}
		}
		reply("��ǰ������Ⱥ" + to_string(res.size()) + "����" + res.show());
		return 1;
	}
	if (strOption == "frq")
	{
		reply("��ǰ��ָ��Ƶ��" + to_string(FrqMonitor::getFrqTotal()));
		return 1;
	}
	else 
	{
		bool boolErase = false;
		strVar["note"] = readPara();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		long long llTargetID = readID();
		if (strOption == "dismiss")
		{
			if (ChatList.count(llTargetID))
			{
				note("����" + getMsg("strSelfName") + "�˳�" + printChat(chat(llTargetID)), 0b10);
				chat(llTargetID).reset("����").leave();
			}
			else
			{
				reply(getMsg("strGroupGetErr"));
			}
			return 1;
		}
		else if (strOption == "boton")
		{
			if (ChatList.count(llTargetID))
			{
				if (groupset(llTargetID, "ͣ��ָ��") > 0)
				{
					chat(llTargetID).reset("ͣ��ָ��");
					note("����" + getMsg("strSelfName") + "��" + printGroup(llTargetID) + "����ָ���");
				}
				else reply(getMsg("strSelfName") + "���ڸ�Ⱥ����ָ��!");
			}
			else
			{
				reply(getMsg("strGroupGetErr"));
			}
		}
		else if (strOption == "botoff")
		{
			if (groupset(llTargetID, "ͣ��ָ��") < 1)
			{
				chat(llTargetID).set("ͣ��ָ��");
				note("����" + getMsg("strSelfName") + "��" + printGroup(llTargetID) + "ͣ��ָ���", 0b1);
			}
			else reply(getMsg("strSelfName") + "���ڸ�Ⱥͣ��ָ��!");
			return 1;
		}
		else if (strOption == "blackgroup")
		{
			if (llTargetID == 0)
			{
				ResList res;
				for (auto [each, danger] : blacklist->mGroupDanger) {
					res << printGroup(each) + ":" + to_string(danger);
				}
				reply(res.show(), false);
				return 1;
			}
			strVar["time"] = printSTNow();
			do
			{
				if (boolErase)
				{
					blacklist->rm_black_group(llTargetID, this);
				}
				else
				{
					blacklist->add_black_group(llTargetID, this);
				}
			} 
			while ((llTargetID = readID()));
			return 1;
		}
		else if (strOption == "whiteqq")
		{
			if (llTargetID == 0)
			{
				strReply = "��ǰ�������û��б�";
				for (auto& [qq, user] : UserList)
				{
					if (user.nTrust)strReply += "\n" + printQQ(qq) + ":" + to_string(user.nTrust);
				}
				reply();
				return 1;
			}
			do
			{
				if (boolErase)
				{
					if (trustedQQ(llTargetID))
					{
						if (trusted <= trustedQQ(llTargetID))
						{
							reply(getMsg("strUserTrustDenied"));
						}
						else 
						{
							getUser(llTargetID).trust(0);
							note("���ջ�" + getMsg("strSelfName") + "��" + printQQ(llTargetID) + "�����Ρ�", 0b1);
						}
					}
					else
					{
						reply(printQQ(llTargetID) + "������" + getMsg("strSelfName") + "�İ�������");
					}
				}
				else 
				{
					if (trustedQQ(llTargetID))
					{
						reply(printQQ(llTargetID) + "�Ѽ���" + getMsg("strSelfName") + "�İ�����!");
					}
					else
					{
						getUser(llTargetID).trust(1);
						note("�����" + getMsg("strSelfName") + "��" + printQQ(llTargetID) + "�����Ρ�", 0b1);
						strVar["user_nick"] = getName(llTargetID);
						AddMsgToQueue(format(getMsg("strWhiteQQAddNotice"), GlobalMsg, strVar), llTargetID);
					}
				}
			}
			while ((llTargetID = readID()));
			return 1;
		}
		else if (strOption == "blackqq")
		{
			if (llTargetID == 0) 
			{
				ResList res;
				for (auto [each, danger] : blacklist->mQQDanger) 
				{
					res << printQQ(each) + ":" + to_string(danger);
				}
				reply(res.show(), false);
				return 1;
			}
			strVar["time"] = printSTNow();
			do 
			{
				if (boolErase)
				{
					blacklist->rm_black_qq(llTargetID, this);
				}
				else
				{
					blacklist->add_black_qq(llTargetID, this);
				}
			}
			while ((llTargetID = readID()));
			return 1;
		}
		else reply(getMsg("strAdminOptionEmpty"));
		return 0;
	}
}

int FromMsg::MasterSet() 
{
	const std::string strOption = readPara();
	if (strOption.empty())
	{
		reply(getMsg("strAdminOptionEmpty"));
		return -1;
	}
	if (strOption == "groupclr")
	{
		strVar["clear_mode"] = readRest();
		cmd_key = "clrgroup";
		sch.push_job(*this);
		return 1;
	}
	if (strOption == "delete")
	{
		if (console.master() != fromQQ)
		{
			reply(getMsg("strNotMaster"));
			return 1;
		}
		reply("�㲻����" + getMsg("strSelfName") + "��Master��");
		console.killMaster();
		return 1;
	}
	else if (strOption == "reset")
	{
		if (console.master() != fromQQ)
		{
			reply(getMsg("strNotMaster"));
			return 1;
		}
		const string strMaster = readDigit();
		if (strMaster.empty() || stoll(strMaster) == console.master())
		{
			reply("Master��Ҫ��ǲ{strSelfCall}!");
		}
		else
		{
			console.newMaster(stoll(strMaster));
			note("�ѽ�Masterת�ø�" + printQQ(console.master()));
		}
		return 1;
	}
	if (strOption == "admin")
	{
		bool boolErase = false;
		readSkipSpace();
		if (strMsg[intMsgCnt] == '-')
		{
			boolErase = true;
			intMsgCnt++;
		}
		if (strMsg[intMsgCnt] == '+') { intMsgCnt++; }
		long long llAdmin = readID();
		if (llAdmin)
		{
			if (boolErase)
			{
				if (trustedQQ(llAdmin) > 3)
				{
					note("���ջ�" + printQQ(llAdmin) + "��" + getMsg("strSelfName") + "�Ĺ���Ȩ�ޡ�", 0b100);
					console.rmNotice({llAdmin, msgtype::Private});
					getUser(llAdmin).trust(0);
				}
				else
				{
					reply("���û��޹���Ȩ�ޣ�");
				}
			}
			else
			{
				if (trustedQQ(llAdmin) > 3)
				{
					reply("���û����й���Ȩ�ޣ�");
				}
				else
				{
					getUser(llAdmin).trust(4);
					console.addNotice({llAdmin, msgtype::Private}, 0b1110);
					note("�����" + printQQ(llAdmin) + "��" + getMsg("strSelfName") + "�Ĺ���Ȩ�ޡ�", 0b100);
				}
			}
			return 1;
		}
		ResList list;
		for (const auto& [qq, user] : UserList)
		{
			if (user.nTrust > 3)list << printQQ(qq);
		}
		reply(getMsg("strSelfName") + "�Ĺ���Ȩ��ӵ���߹�" + to_string(list.size()) + "λ��" + list.show());
		return 1;
	}
	return AdminEvent(strOption);
}

int FromMsg::BasicOrder()
{
	if (strMsg[0] != '.')return 0;
	intMsgCnt++;
	int intT = static_cast<int>(fromChat.second);
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
		intMsgCnt++;
	//strVar["nick"] = getName(fromQQ, fromGroup);
	//getPCName(*this);
	//strVar["at"] = intT ? "[CQ:at,qq=" + to_string(fromQQ) + "]" : strVar["nick"];
	isAuth = trusted > 3 || intT != GroupT || DD::isGroupAdmin(fromGroup, fromQQ, true) || pGrp->inviter == fromQQ;
	//ָ��ƥ��
	if(console["DebugMode"])console.log("listen:" + strMsg, 0, printSTNow());
	if (strLowerMessage.substr(intMsgCnt, 9) == "authorize")
	{
		intMsgCnt += 9;
		readSkipSpace();
		if (intT != GroupT || strMsg[intMsgCnt] == '+')
		{
			long long llTarget = readID();
			if (llTarget)
			{
				pGrp = &chat(llTarget);
			}
			else
			{
				reply(getMsg("strGroupIDEmpty"));
				return 1;
			}
		}
		if (pGrp->isset("���ʹ��") && !pGrp->isset("δ���") && !pGrp->isset("Э����Ч"))return 0;
		if (trusted > 0)
		{
			pGrp->set("���ʹ��").reset("δ���").reset("Э����Ч");
			note("����Ȩ" + printGroup(pGrp->ID) + "���ʹ��", 1);
			AddMsgToQueue(getMsg("strGroupAuthorized", strVar), pGrp->ID, msgtype::Group);
		}
		else
		{
			if (!console["CheckGroupLicense"] && !console["Private"] && !isCalled)return 0;
			string strInfo = readRest();
			if (strInfo.empty())console.log(printQQ(fromQQ) + "����" + printGroup(pGrp->ID) + "���ʹ��", 0b10, printSTNow());
			else console.log(printQQ(fromQQ) + "����" + printGroup(pGrp->ID) + "���ʹ�ã����ԣ�" + strInfo, 0b100, printSTNow());
			reply(getMsg("strGroupLicenseApply"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
	{
		intMsgCnt += 7;
		if (!intT)
		{
			string QQNum = readDigit();
			if (QQNum.empty())
			{
				reply(getMsg("strDismissPrivate"));
				return -1;
			}
			long long llGroup = stoll(QQNum);
			if (!ChatList.count(llGroup))
			{
				reply(getMsg("strGroupNotFound"));
				return 1;
			}
			Chat& grp = chat(llGroup);
			if (grp.isset("����") || grp.isset("δ��"))
			{
				reply(getMsg("strGroupAway"));
			}
			if (trustedQQ(fromQQ) > 2) {
				grp.leave(getMsg("strAdminDismiss", strVar));
				reply(getMsg("strGroupExit"));
			}
			else if(DD::isGroupAdmin(llGroup, fromQQ, true) || (grp.inviter == fromQQ))
			{
				reply(getMsg("strDismiss"));
			}
			else
			{
				reply(getMsg("strPermissionDeniedErr"));
			}
			return 1;
		}
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(console.DiceMaid) || (QQNum.length() == 4 && stoll(QQNum) == DD::getLoginQQ() % 10000)){
			if (trusted > 2) 
			{
				pGrp->leave(getMsg("strAdminDismiss", strVar));
				return 1;
			}
			if (pGrp->isset("Э����Ч"))return 0;
			if (isAuth)
			{
				pGrp->leave(getMsg("strDismiss"));
			}
			else
			{
				if (!isCalled && (pGrp->isset("ͣ��ָ��") || DD::getGroupSize(fromGroup).currSize > 200))AddMsgToQueue(getMsg("strPermissionDeniedErr", strVar), fromQQ);
				else reply(getMsg("strPermissionDeniedErr"));
			}
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "warning")
	{
		intMsgCnt += 7;
		string strWarning = readRest();
		AddWarning(strWarning, fromQQ, fromGroup);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "master" && console.isMasterMode)
	{
		intMsgCnt += 6;
		if (!console.master())
		{
			string strOption = readRest();
			if (strOption == "public"){
				console.set("BelieveDiceList", 1);
				console.set("LeaveBlackQQ", 1);
				console.set("BannedBanInviter", 1);
				console.set("KickedBanInviter", 1);
			}
			else{
				console.set("Private", 1);
			}
			console.newMaster(fromQQ);
		}
		else if (trusted > 4 || console.master() == fromQQ)
		{
			return MasterSet();
		}
		else
		{
			if (isCalled)reply(getMsg("strNotMaster"));
			return 1;
		}
		return 1;
	}
	else if (intT != PrivateT && pGrp->isset("Э����Ч")){
		return 1;
	}
	if (blacklist->get_qq_danger(fromQQ) || (intT != PrivateT && blacklist->get_group_danger(fromGroup)))
	{
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
	{
		intMsgCnt += 3;
		string Command = readPara();
		string QQNum = readDigit();
		if (QQNum.empty() || QQNum == to_string(DD::getLoginQQ()) || (QQNum.length() == 4 && stoll(QQNum) == DD::getLoginQQ() %
			10000))
		{
			if (Command == "on")
			{
				if (console["DisabledGlobal"])reply(getMsg("strGlobalOff"));
				else if (intT == GroupT && ((console["CheckGroupLicense"] && pGrp->isset("δ���")) || (console["CheckGroupLicense"] == 2 && !pGrp->isset("���ʹ��"))))reply(getMsg("strGroupLicenseDeny"));
				else if (intT)
				{
					if (isAuth || trusted >2)
					{
						if (groupset(fromGroup, "ͣ��ָ��") > 0)
						{
							chat(fromGroup).reset("ͣ��ָ��");
							reply(getMsg("strBotOn"));
						}
						else
						{
							reply(getMsg("strBotOnAlready"));
						}
					}
					else
					{
						if (groupset(fromGroup, "ͣ��ָ��") > 0 && DD::getGroupSize(fromGroup).currSize > 200)AddMsgToQueue(
							getMsg("strPermissionDeniedErr", strVar), fromQQ);
						else reply(getMsg("strPermissionDeniedErr"));
					}
				}
			}
			else if (Command == "off")
			{
				if (isAuth || trusted > 2)
				{
					if (groupset(fromGroup, "ͣ��ָ��"))
					{
						if (!isCalled && QQNum.empty() && pGrp->isGroup && DD::getGroupSize(fromGroup).currSize > 200)AddMsgToQueue(getMsg("strBotOffAlready", strVar), fromQQ);
						else reply(getMsg("strBotOffAlready"));
					}
					else 
					{
						chat(fromGroup).set("ͣ��ָ��");
						reply(getMsg("strBotOff"));
					}
				}
				else
				{
					if (groupset(fromGroup, "ͣ��ָ��"))AddMsgToQueue(getMsg("strPermissionDeniedErr", strVar), fromQQ);
					else reply(getMsg("strPermissionDeniedErr"));
				}
			}
			else if (!Command.empty() && !isCalled && pGrp->isset("ͣ��ָ��"))
			{
				return 0;
			}
			else if (intT == GroupT && pGrp->isset("ͣ��ָ��") && DD::getGroupSize(fromGroup).currSize > 500 && !isCalled)
			{
				AddMsgToQueue(Dice_Full_Ver_On + getMsg("strBotMsg"), fromQQ);
			}
			else
			{
				this_thread::sleep_for(1s);
				reply(Dice_Full_Ver_On + getMsg("strBotMsg"));
			}
		}
		return 1;
	}
	if (isDisabled || (!isCalled || !console["DisabledListenAt"]) && (groupset(fromGroup, "ͣ��ָ��") > 0))
	{
		if (intT == PrivateT)
		{
			reply(getMsg("strGlobalOff"));
			return 1;
		}
		return 0;
	}
	if (strLowerMessage.substr(intMsgCnt, 7) == "helpdoc" && trusted > 3)
	{
		intMsgCnt += 7;
		while (strMsg[intMsgCnt] == ' ')
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			reply(getMsg("strHlpNameEmpty"));
			return true;
		}
		strVar["key"] = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strMsg.length())
		{
			CustomHelp.erase(strVar["key"]);
			if (auto it = HelpDoc.find(strVar["key"]); it != HelpDoc.end())
				fmt->set_help(it->first, it->second);
			else
				fmt->rm_help(strVar["key"]);
			reply(format(getMsg("strHlpReset"), {strVar["key"]}));
		}
		else
		{
			string strHelpdoc = strMsg.substr(intMsgCnt);
			CustomHelp[strVar["key"]] = strHelpdoc;
			fmt->set_help(strVar["key"], strHelpdoc);
			reply(format(getMsg("strHlpSet"), {strVar["key"]}));
		}
		saveJMap(DiceDir / "conf" / "CustomHelp.json", CustomHelp);
		return true;
	}
	if (strLowerMessage.substr(intMsgCnt, 4) == "help")
	{
		intMsgCnt += 4;
		while (strLowerMessage[intMsgCnt] == ' ')
			intMsgCnt++;
		strVar["help_word"] = readRest();
		if (intT)
		{
			if (!isAuth && (strVar["help_word"] == "on" || strVar["help_word"] == "off"))
			{
				reply(getMsg("strPermissionDeniedErr"));
				return 1;
			}
			strVar["option"] = "����help";
			if (strVar["help_word"] == "off")
			{
				if (groupset(fromGroup, strVar["option"]) < 1)
				{
					chat(fromGroup).set(strVar["option"]);
					reply(getMsg("strGroupSetOn"));
				}
				else
				{
					reply(getMsg("strGroupSetOnAlready"));
				}
				return 1;
			}
			if (strVar["help_word"] == "on")
			{
				if (groupset(fromGroup, strVar["option"]) > 0)
				{
					chat(fromGroup).reset(strVar["option"]);
					reply(getMsg("strGroupSetOff"));
				}
				else
				{
					reply(getMsg("strGroupSetOffAlready"));
				}
				return 1;
			}
			if (groupset(fromGroup, strVar["option"]) > 0)
			{
				reply(getMsg("strGroupSetOnAlready"));
				return 1;
			}
		}
		std::thread th(&DiceModManager::_help, fmt.get(), shared_from_this());
		th.detach();
		return true;
	}
	return 0;
}

int FromMsg::InnerOrder() {
	if (strMsg[0] != '.')return 0;
	if (WordCensor()) {
		return 1;
	}
	if (strLowerMessage.substr(intMsgCnt, 8) == "setreply") {
		return 0;
	}
	else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome") {
		intMsgCnt += 7;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("welcome"));
			return 1;
		}
		if (fromChat.second != msgtype::Group) {
			reply(getMsg("strWelcomePrivate"));
			return 1;
		}
		if (isAuth) {
			string strWelcomeMsg = strMsg.substr(intMsgCnt);
			if (strWelcomeMsg == "clr") {
				if (chat(fromGroup).strConf.count("��Ⱥ��ӭ")) {
					chat(fromGroup).rmText("��Ⱥ��ӭ");
					reply(getMsg("strWelcomeMsgClearNotice"));
				}
				else {
					reply(getMsg("strWelcomeMsgClearErr"));
				}
			}
			else if (strWelcomeMsg == "show") {
				string strWelcome{ chat(fromGroup).strConf["��Ⱥ��ӭ"] };
				if (strWelcome.empty())reply(getMsg("strWelcomeMsgEmpty"));
				else reply(strWelcome, false);	//ת����ע�����
			}
			else if (readPara() == "set") {
				chat(fromGroup).setText("��Ⱥ��ӭ", strip(readRest()));
				reply(getMsg("strWelcomeMsgUpdateNotice"));
			}
			else {
				chat(fromGroup).setText("��Ⱥ��ӭ", strWelcomeMsg);
				reply(getMsg("strWelcomeMsgUpdateNotice"));
			}
		}
		else {
			reply(getMsg("strPermissionDeniedErr"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "groups") {
		if (trusted < 4) {
			reply(getMsg("strNotAdmin"));
			return 1;
		}
		intMsgCnt += 6;
		string strOption = readPara();
		if (strOption == "list") {
			strVar["list_mode"] = readPara();
			cmd_key = "lsgroup";
			sch.push_job(*this);
		}
		else if (strOption == "clr") {
			if (trusted < 5) {
				reply(getMsg("strNotMaster"));
				return 1;
			}
			int cnt = clearGroup();
			note("���������Ⱥ��¼" + to_string(cnt) + "��", 0b10);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "setcoc") {
		if (!isAuth) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		string strRule = readDigit();
		if (strRule.empty()) {
			if (fromChat.second != msgtype::Private)chat(fromGroup).rmConf("rc����");
			else getUser(fromQQ).rmIntConf("rc����");
			reply(getMsg("strDefaultCOCClr"));
			return 1;
		}
		if (strRule.length() > 1) {
			reply(getMsg("strDefaultCOCNotFound"));
			return 1;
		}
		int intRule = stoi(strRule);
		switch (intRule) {
		case 0:
			reply(getMsg("strDefaultCOCSet") + "0 ������\n��1��ɹ�\n����50��96-100��ʧ�ܣ���50��100��ʧ��");
			break;
		case 1:
			reply(getMsg("strDefaultCOCSet") + "1\n����50��1��ɹ�����50��1-5��ɹ�\n����50��96-100��ʧ�ܣ���50��100��ʧ��");
			break;
		case 2:
			reply(getMsg("strDefaultCOCSet") + "2\n��1-5��<=�ɹ��ʴ�ɹ�\n��100���96-99��>�ɹ��ʴ�ʧ��");
			break;
		case 3:
			reply(getMsg("strDefaultCOCSet") + "3\n��1-5��ɹ�\n��96-100��ʧ��");
			break;
		case 4:
			reply(getMsg("strDefaultCOCSet") + "4\n��1-5��<=ʮ��֮һ��ɹ�\n����50��>=96+ʮ��֮һ��ʧ�ܣ���50��100��ʧ��");
			break;
		case 5:
			reply(getMsg("strDefaultCOCSet") + "5\n��1-2��<���֮һ��ɹ�\n����50��96-100��ʧ�ܣ���50��99-100��ʧ��");
			break;
		case 6:
			reply(getMsg("strDefaultCOCSet") + "6\n��ɫ������\n��1��������ͬ<=�ɹ��ʴ�ɹ�\n��100��������ͬ>�ɹ��ʴ�ʧ��");
			break;
		default:
			reply(getMsg("strDefaultCOCNotFound"));
			return 1;
		}
		if (fromChat.second != msgtype::Private)chat(fromGroup).setConf("rc����", intRule);
		else getUser(fromQQ).setConf("rc����", intRule);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 6) == "system") {
		intMsgCnt += 6;
		if (console && trusted < 4) {
			reply(getMsg("strNotAdmin"));
			return -1;
		}
		string strOption = readPara();
#ifdef _WIN32
		if (strOption == "gui") {
			reply("Dice! GUI�ѱ����ã��뿼��ʹ��Dice! WebUI https://forum.kokona.tech/d/721-dice-webui-shi-yong-shuo-ming");
			thread th(GUIMain);
			th.detach();
			return 1;
		}
#endif
		if (strOption == "save") {
			dataBackUp();
			note("���ֶ�����{self}�����ݡ�", 0b1);
			return 1;
		}
		if (strOption == "load") {
			loadData();
			note("���ֶ�����{self}�����á�", 0b1);
			return 1;
		}
		if (strOption == "state")
		{
			time_t tt = time(nullptr);
#ifdef _MSC_VER
			localtime_s(&stNow, &tt);
#else
			localtime_r(&tt, &stNow);
#endif
#ifdef _WIN32
			double mbFreeBytes = 0, mbTotalBytes = 0;
			long long milDisk(getDiskUsage(mbFreeBytes, mbTotalBytes));
#endif
			ResList res;
			res << "����ʱ��:" + printSTime(stNow)
#ifdef _WIN32
				<< "�ڴ�ռ��:" + to_string(getRamPort()) + "%"
				<< "CPUռ��:" + toString(getWinCpuUsage() / 10.0) + "%"
				<< "Ӳ��ռ��:" + toString(milDisk / 10.0) + "%(����:" + toString(mbFreeBytes) + "GB/ " + toString(mbTotalBytes) + "GB)"
#endif
				<< "����ʱ��:" + printDuringTime(time(nullptr) - llStartTime)
				<< "����ָ����:" + to_string(today->get("frq"))
				<< "������ָ����:" + to_string(FrqMonitor::sumFrqTotal);
			reply(res.show());
			return 1;
		}
		if (strOption == "clrimg") {
			reply("�ǿ�Q��ܲ���Ҫ�˹���");
			return -1;
		}
		else if (strOption == "reload") {
			if (trusted < 5 && fromQQ != console.master()) {
				reply(getMsg("strNotMaster"));
				return -1;
			}
			cmd_key = "reload";
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "remake") {
			
			if (trusted < 5 && fromQQ != console.master()) {
				reply(getMsg("strNotMaster"));
				return -1;
			}
			cmd_key = "remake";
			sch.push_job(*this);
			return 1;
		}
		else if (strOption == "die") {
			if (trusted < 5 && fromQQ != console.master()) {
				reply(getMsg("strNotMaster"));
				return -1;
			}
			cmd_key = "die";
			sch.push_job(*this);
			return 1;
		}
		if (strOption == "rexplorer")
		{
#ifdef _WIN32
			if (trusted < 5 && fromQQ != console.master())
			{
				reply(getMsg("strNotMaster"));
				return -1;
			}
			system(R"(taskkill /f /fi "username eq %username%" /im explorer.exe)");
			system(R"(start %SystemRoot%\explorer.exe)");
			this_thread::sleep_for(3s);
			note("��������Դ��������\n��ǰ�ڴ�ռ�ã�" + to_string(getRamPort()) + "%");
#endif
		}
		else if (strOption == "cmd")
		{
#ifdef _WIN32
			if (fromQQ != console.master())
			{
				reply(getMsg("strNotMaster"));
				return -1;
			}
			string strCMD = readRest() + "\ntimeout /t 10";
			system(strCMD.c_str());
			reply("�����������С�");
			return 1;
#endif
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "admin") {
		intMsgCnt += 5;
		return AdminEvent(readPara());
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "cloud") {
		intMsgCnt += 5;
		string strOpt = readPara();
		if (trusted < 4 && fromQQ != console.master()) {
			reply(getMsg("strNotAdmin"));
			return 1;
		}
		if (strOpt == "update") {
			strVar["ver"] = readPara();
			if (strVar["ver"].empty()) {
				Cloud::checkUpdate(this);
			}
			else if (strVar["ver"] == "dev" || strVar["ver"] == "release") {
				cmd_key = "update";
				sch.push_job(*this);
			}
			return 1;
		}
		else if (strOpt == "black") {
			cmd_key = "cloudblack";
			sch.push_job(*this);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc7d" || strLowerMessage.substr(intMsgCnt, 4) == "cocd") {
		COC7D(strVar["res"]);
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "coc6d") {
		COC6D(strVar["res"]);
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "group") {
		intMsgCnt += 5;
		long long llGroup(fromGroup);
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("group"));
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "all") {
			if (trusted < 5) {
				reply(getMsg("strNotMaster"));
				return 1;
			}
			intMsgCnt += 3;
			readSkipSpace();
			if (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-') {
				bool isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				string strOption = strVar["option"] = readRest();
				if (!mChatConf.count(strVar["option"])) {
					reply(getMsg("strGroupSetNotExist"));
					return 1;
				}
				int Cnt = 0;
				if (isSet) {
					for (auto& [id, grp] : ChatList) {
						if (grp.isset(strOption))continue;
						grp.set(strOption);
						Cnt++;
					}
					strVar["cnt"] = to_string(Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
				else {
					for (auto& [id, grp] : ChatList) {
						if (!grp.isset(strOption))continue;
						grp.reset(strOption);
						Cnt++;
					}
					strVar["cnt"] = to_string(Cnt);
					note(getMsg("strGroupSetAll"), 0b100);
				}
			}
			return 1;
		}
		if (string& strGroup = strVar["group_id"] = readDigit(false); !strGroup.empty()) {
			llGroup = stoll(strGroup);
			if (!ChatList.count(llGroup)) {
				reply(getMsg("strGroupNotFound"));
				return 1;
			}
			if (getGroupAuth(llGroup) < 0) {
				reply(getMsg("strGroupDenied"));
				return 1;
			}
		}
		else if (fromChat.second != msgtype::Group)return 0;
		else strVar["group_id"] = to_string(fromGroup);
		Chat& grp = chat(llGroup);
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if(strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'){
			int cntSet{ 0 };
			bool isSet{ strMsg[intMsgCnt] == '+' };
			do{
				isSet = strMsg[intMsgCnt] == '+';
				intMsgCnt++;
				strVar["option"] = readPara();
				readSkipSpace();
				if (!mChatConf.count(strVar["option"])) {
					reply(getMsg("strGroupSetInvalid"));
					continue;
				}
				if (getGroupAuth(llGroup) >= get<string, short>(mChatConf, strVar["option"], 0)) {
					if (isSet) {
						if (groupset(llGroup, strVar["option"]) < 1) {
							chat(llGroup).set(strVar["option"]);
							++cntSet;
							if (strVar["Option"] == "���ʹ��") {
								AddMsgToQueue(getMsg("strGroupAuthorized", strVar), fromQQ, msgtype::Group);
							}
						}
						else {
							reply(getMsg("strGroupSetOnAlready"));
						}
					}
					else if (grp.isset(strVar["option"])) {
						++cntSet;
						chat(llGroup).reset(strVar["option"]);
					}
					else {
						reply(getMsg("strGroupSetOffAlready"));
					}
				}
				else {
					reply(getMsg("strGroupSetDenied"));
				}
			} while (strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-');
			if (cntSet == 1) {
				isSet ? reply(getMsg("strGroupSetOn")) : reply(getMsg("strGroupSetOff"));
			}
			else if(cntSet > 1) {
				strVar["opt_list"] = grp.listBoolConf();
				reply(getMsg("strGroupMultiSet"));
			}
			return 1;
		}
		bool isInGroup{ fromGroup == llGroup || DD::isGroupMember(llGroup,console.DiceMaid,true) };
		string Command = readPara();
		strVar["group"] = DD::printGroupInfo(llGroup);
		if (Command.empty()) {
			reply(fmt->get_help("group"));
			return 1;
		}
		else if (Command == "state") {
			ResList res;
			res << "��{group}��";
			res << grp.listBoolConf();
			for (const auto& it : grp.intConf) {
				res << it.first + "��" + to_string(it.second);
			}
			res << "��¼������" + printDate(grp.tCreated);
			res << "����¼��" + printDate(grp.tUpdated);
			if (grp.inviter)res << "�����ߣ�" + printQQ(grp.inviter);
			res << string("��Ⱥ��ӭ��") + (grp.isset("��Ⱥ��ӭ") ? "������" : "��");
			reply(getMsg("strSelfName") + res.show());
			return 1;
		}
		if (!grp.isGroup || (fromGroup == llGroup && fromChat.second != msgtype::Group)) {
			reply(getMsg("strGroupNot"));
			return 1;
		}
		else if (Command == "info") {
			reply(DD::printGroupInfo(llGroup), false);
			return 1;
		}
		else if (!isInGroup) {
			reply(getMsg("strGroupNotIn"));
			return 1;
		}
		else if (Command == "survey") {
			int cntDiver(0);
			long long dayMax(0);
			set<string> sBlackQQ;
			int cntUser(0);
			size_t cntDice(0);
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int cntSize(0);
			set<long long> list{ DD::getGroupMemberList(llGroup) };
			if (list.empty()) {
				reply("{self}���س�Ա�б�ʧ�ܣ�");
				return 1;
			}
			for (auto each : list) {
				if (each == console.DiceMaid)continue;
				if (long long lst{ DD::getGroupLastMsg(llGroup,each) }; lst > 0) {
					long long dayDive((tNow - lst) / intTDay);
					if (dayDive > dayMax)dayMax = dayDive;
					if (dayDive > 30)++cntDiver;
				}
				if (blacklist->get_qq_danger(each) > 1) {
					sBlackQQ.insert(printQQ(each));
				}
				if (UserList.count(each))++cntUser;
				if (DD::isDiceMaid(each))++cntDice;
				++cntSize;
			}
			ResList res;
			res << "��{group}��"
				<< "{self}�û�ռ��: " + to_string(cntUser * 100 / (cntSize)) + "%"
				<< (cntDice ? "ͬϵ����: " + to_string(cntDice) : "")
				<< (cntDiver ? "30��ǱˮȺԱ: " + to_string(cntDiver) : "");
			if (!sBlackQQ.empty()) {
				if (sBlackQQ.size() > 8)
					res << getMsg("strSelfName") + "�ĺ�������Ա" + to_string(sBlackQQ.size()) + "��";
				else {
					res << getMsg("strSelfName") + "�ĺ�������Ա:{blackqq}";
					ResList blacks;
					for (const auto& each : sBlackQQ) {
						blacks << each;
					}
					strVar["blackqq"] = blacks.show();
				}
			}
			reply(res.show());
			return 1;
		}
		else if (Command == "diver") {
			bool bForKick = false;
			if (strLowerMessage.substr(intMsgCnt, 5) == "4kick") {
				bForKick = true;
				intMsgCnt += 5;
			}
			std::priority_queue<std::pair<time_t, string>> qDiver;
			time_t tNow = time(nullptr);
			const int intTDay = 24 * 60 * 60;
			int cntSize(0);
			for (auto each : DD::getGroupMemberList(llGroup)) {
				long long lst{ DD::getGroupLastMsg(llGroup,each) };
				time_t intLastMsg = (tNow - lst) / intTDay;
				if (lst > 0 || intLastMsg > 30) {
					qDiver.emplace(intLastMsg, (bForKick ? to_string(each)
												: printQQ(each)));
				}
				++cntSize;
			}
			if (!cntSize) {
				reply("{self}���س�Ա�б�ʧ�ܣ�");
				return 1;
			}
			else if (qDiver.empty()) {
				reply("{self}δ����ǱˮȺ��Ա��");
				return 1;
			}
			int intCnt(0);
			ResList res;
			while (!qDiver.empty()) {
				res << (bForKick ? qDiver.top().second
						: (qDiver.top().second + to_string(qDiver.top().first) + "��"));
				if (++intCnt > 15 && intCnt > cntSize / 80)break;
				qDiver.pop();
			}
			bForKick ? reply("(.group " + to_string(llGroup) + " kick " + res.show(1))
				: reply("Ǳˮ��Ա�б�:" + res.show(1));
			return 1;
		}
		if (bool isAdmin = DD::isGroupAdmin(llGroup, fromQQ, true); Command == "pause") {
			if (!isAdmin && trusted < 4) {
				reply(getMsg("strPermissionDeniedErr"));
				return 1;
			}
			int secDuring(-1);
			string strDuring{ readDigit() };
			if (!strDuring.empty())secDuring = stoi(strDuring);
			DD::setGroupWholeBan(llGroup, secDuring);
			reply(getMsg("strGroupWholeBan"));
			return 1;
		}
		else if (Command == "restart") {
			if (!isAdmin && trusted < 4) {
				reply(getMsg("strPermissionDeniedErr"));
				return 1;
			}
			DD::setGroupWholeBan(llGroup, 0);
			reply(getMsg("strGroupWholeUnban"));
			return 1;
		}
		else if (Command == "card") {
			if (long long llqq = readID()) {
				if (trusted < 4 && !isAdmin && llqq != fromQQ) {
					reply(getMsg("strPermissionDeniedErr"));
					return 1;
				}
				if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
					reply(getMsg("strSelfPermissionErr"));
					return 1;
				}
				while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
					intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
				strVar["card"] = readRest();
				strVar["target"] = getName(llqq, llGroup);
				DD::setGroupCard(llGroup, llqq, strVar["card"]);
				reply(getMsg("strGroupCardSet"));
			}
			else {
				reply(getMsg("strQQIDEmpty"));
			}
			return 1;
		}
		else if ((!isAdmin && (!DD::isGroupOwner(llGroup, console.DiceMaid,true) || trusted < 5))) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		else if (Command == "ban") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
				reply(getMsg("strSelfPermissionErr"));
				return 1;
			}
			string& QQNum = strVar["ban_qq"] = readDigit();
			if (QQNum.empty()) {
				reply(getMsg("strQQIDEmpty"));
				return -1;
			}
			long long llMemberQQ = stoll(QQNum);
			strVar["member"] = getName(llMemberQQ, llGroup);
			string strMainDice = readDice();
			if (strMainDice.empty()) {
				reply(getMsg("strValueErr"));
				return -1;
			}
			const int intDefaultDice = get(getUser(fromQQ).intConf, string("Ĭ����"), 100);
			RD rdMainDice(strMainDice, intDefaultDice);
			rdMainDice.Roll();
			int intDuration{ rdMainDice.intTotal };
			strVar["res"] = rdMainDice.FormShortString();
			DD::setGroupBan(llGroup, llMemberQQ, intDuration * 60);
			if (intDuration <= 0)
				reply(getMsg("strGroupUnban"));
			else reply(getMsg("strGroupBan"));
		}
		else if (Command == "kick") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			if (!DD::isGroupAdmin(llGroup, console.DiceMaid, true)) {
				reply(getMsg("strSelfPermissionErr"));
				return 1;
			}
			long long llMemberQQ = readID();
			if (!llMemberQQ) {
				reply(getMsg("strQQIDEmpty"));
				return -1;
			}
			ResList resKicked, resDenied, resNotFound;
			do {
				if (int auth{ DD::getGroupAuth(llGroup, llMemberQQ,0) }) {
					if (auth > 1) {
						resDenied << printQQ(llMemberQQ);
						continue;
					}
					DD::setGroupKick(llGroup, llMemberQQ);
					resKicked << printQQ(llMemberQQ);
				}
				else resNotFound << printQQ(llMemberQQ);
			} while ((llMemberQQ = readID()));
			strReply = getMsg("strSelfName");
			if (!resKicked.empty())strReply += "���Ƴ�ȺԱ��" + resKicked.show() + "\n";
			if (!resDenied.empty())strReply += "�Ƴ�ʧ�ܣ�" + resDenied.show() + "\n";
			if (!resNotFound.empty())strReply += "�Ҳ�������" + resNotFound.show();
			reply();
			return 1;
		}
		else if (Command == "title") {
			if (!DD::isGroupOwner(llGroup, console.DiceMaid,true)) {
				reply(getMsg("strSelfPermissionErr"));
				return 1;
			}
			if (long long llqq = readID()) {
				while (!isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) && intMsgCnt != strMsg.length())
					intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))intMsgCnt++;
				strVar["title"] = readRest();
				DD::setGroupTitle(llGroup, llqq, strVar["title"]);
				strVar["target"] = getName(llqq, llGroup);
				reply(getMsg("strGroupTitleSet"));
			}
			else {
				reply(getMsg("strQQIDEmpty"));
			}
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "reply") {
		intMsgCnt += 5;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("reply"));
			return 1;
		}
		unsigned int intMsgTmpCnt{ intMsgCnt };
		string action{ readPara() };
		if (action == "on" && fromGroup) {
			string& option{ strVar["option"] = "���ûظ�" };
			if (!chat(fromGroup).isset(option)) {
				reply(getMsg("strGroupSetOffAlready"));
			}
			else if (trusted > 0 || isAuth) {
				chat(fromGroup).reset(option);
				reply(getMsg("strReplyOn"));
			}
			else {
				reply(getMsg("strWhiteQQDenied"));
			}
			return 1;
		}
		else if (action == "off" && fromGroup) {
			string& option{ strVar["option"] = "���ûظ�" };
			if (chat(fromGroup).isset(option)) {
				reply(getMsg("strGroupSetOnAlready"));
			}
			else if (trusted > 0 || isAuth) {
				chat(fromGroup).set(option);
				reply(getMsg("strReplyOff"));
			}
			else {
				reply(getMsg("strWhiteQQDenied"));
			}
			return 1;
		}
		else if (action == "show"){
			if (trusted < 2) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			strVar["key"] = readRest();
			fmt->show_reply(shared_from_this());
			return 1;
		}
		else if (action == "set") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			DiceMsgReply trigger;
			string& key{ strVar["key"] };
			string attr{ readToColon() };
			while (!attr.empty()) {
				if (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':'))intMsgCnt++;
				if (attr == "Type") {	//Type=Order|Reply
					string type{ readUntilTab() };
					if(DiceMsgReply::sType.count(type))trigger.type = (DiceMsgReply::Type)DiceMsgReply::sType[type];
				}
				else if (DiceMsgReply::sMode.count(attr)) {	//Mode=Key
					trigger.mode = (DiceMsgReply::Mode)DiceMsgReply::sMode[attr];
					if (trigger.mode == DiceMsgReply::Mode::Regex) {
						try
						{
							std::wregex re(convert_a2realw(strVar["key"].c_str()), std::regex::ECMAScript);
						}
						catch (const std::regex_error& e)
						{
							strVar["err"] = e.what();
							reply(getMsg("strRegexInvalid"));
							return -1;
						}
					}
					key = readUntilTab();
				}
				else if (DiceMsgReply::sEcho.count(attr)) {	//Echo=Reply
					trigger.echo = (DiceMsgReply::Echo)DiceMsgReply::sEcho[attr];
					if (trigger.echo == DiceMsgReply::Echo::Deck) {
						while (intMsgCnt != strMsg.length()) {
							string item = readItem();
							if (!item.empty())trigger.deck.push_back(item);
						}
					}
					else trigger.text = readRest();
					break;
				}
				attr = readToColon();
			}
			if (key.empty()) {
				reply(getMsg("strReplyKeyEmpty"));
			}
			else {
				fmt->set_reply(key, trigger);
				reply(getMsg("strReplySet"));
			}
			return 1;
		}
		else if (action == "list") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			strVar["res"] = fmt->list_reply();
			reply(getMsg("strReplyList"));
			return 1;
		}
		else if (action == "del") {
			if (trusted < 4) {
				reply(getMsg("strNotAdmin"));
				return -1;
			}
			string& key{ strVar["key"] = readRest() };
			if (fmt->del_reply(key)) {
				reply(getMsg("strReplyDel"));
			}
			else {
				reply(getMsg("strReplyKeyNotFound"));
			}
			return 1;
		}
		intMsgCnt = intMsgTmpCnt;
		DiceMsgReply rep;
		if (strLowerMessage.substr(intMsgCnt, 2) == "re") {
			intMsgCnt += 2;
			rep.mode = DiceMsgReply::Mode::Regex;
		}
		if (trusted < 4) {
			reply(getMsg("strNotAdmin"));
			return -1;
		}
		string& key{ strVar["key"] = readUntilSpace() };
		if (key.empty()) {
			reply(fmt->get_help("reply"));
			return -1;
		}
		
		if(rep.mode == DiceMsgReply::Mode::Regex)
		{
			try
			{
				std::wregex re(convert_a2realw(key.c_str()), std::regex::ECMAScript);
			}
			catch (const std::regex_error& e)
			{
				strVar["err"] = e.what();
				reply(getMsg("strRegexInvalid"));
				return -1;
			}
		}
		readItems(rep.deck);
		if (rep.deck.empty()) {
			fmt->del_reply(key);
			reply(getMsg("strReplyDel"), { strVar["key"] });
		}
		else {
			fmt->set_reply(key, rep);
			reply(getMsg("strReplySet"), { strVar["key"] });
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 5) == "rules") {
		intMsgCnt += 5;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("rules"));
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == ':')
				intMsgCnt++;
			string strDefaultRule = strMsg.substr(intMsgCnt);
			if (strDefaultRule.empty()) {
				getUser(fromQQ).rmStrConf("Ĭ�Ϲ���");
				reply(getMsg("strRuleReset"));
			}
			else {
				for (auto& n : strDefaultRule)
					n = toupper(static_cast<unsigned char>(n));
				getUser(fromQQ).setConf("Ĭ�Ϲ���", strDefaultRule);
				reply(getMsg("strRuleSet"));
			}
		}
		else {
			string strSearch = strMsg.substr(intMsgCnt);
			for (auto& n : strSearch)
				n = toupper(static_cast<unsigned char>(n));
			string strReturn;
			if (getUser(fromQQ).strConf.count("Ĭ�Ϲ���") && strSearch.find(':') == string::npos &&
				GetRule::get(getUser(fromQQ).strConf["Ĭ�Ϲ���"], strSearch, strReturn)) {
				reply(strReturn);
			}
			else if (GetRule::analyze(strSearch, strReturn)) {
				reply(strReturn);
			}
			else {
				reply(getMsg("strRuleErr") + strReturn);
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "coc6") {
		intMsgCnt += 4;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 2) {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum > 10) {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		if (intNum == 0) {
			reply(getMsg("strCharacterCannotBeZero"));
			return 1;
		}
		COC6(strVar["res"], intNum);
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "deck") {
		if (trusted < 4 && console["DisabledDeck"]) {
			reply(getMsg("strDisabledDeckGlobal"));
			return 1;
		}
		intMsgCnt += 4;
		string strRoom = readDigit(false);
		long long llRoom = strRoom.empty() ? fromSession : stoll(strRoom);
		if (llRoom == 0)llRoom = fromSession;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("deck"));
			return 1;
		}
		string strPara = readPara();
		if (strPara == "show") {
			if (gm->has_session(llRoom))
				gm->session(llRoom).deck_show(this);
			else reply(getMsg("strDeckListEmpty"));
		}
		else if ((!isAuth || llRoom != fromSession) && !trusted) {
			reply(getMsg("strWhiteQQDenied"));
		}
		else if (strPara == "set") {
			gm->session(llRoom).deck_set(this);
		}
		else if (strPara == "reset") {
			gm->session(llRoom).deck_reset(this);
		}
		else if (strPara == "del") {
			gm->session(llRoom).deck_del(this);
		}
		else if (strPara == "clr") {
			gm->session(llRoom).deck_clr(this);
		}
		else if (strPara == "new") {
			gm->session(llRoom).deck_new(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "draw") {
		if (trusted < 4 && console["DisabledDraw"]) {
			reply(getMsg("strDisabledDrawGlobal"));
			return 1;
		}
		strVar["option"] = "����draw";
		if (fromChat.second != msgtype::Private && groupset(fromGroup, strVar["option"]) > 0) {
			reply(getMsg("strGroupSetOnAlready"));
			return 1;
		}
		intMsgCnt += 4;
		bool isPrivate(false);
		if (strMsg[intMsgCnt] == 'h' && isspace(static_cast<unsigned char>(strMsg[intMsgCnt + 1]))) {
			strVar["hidden"];
			isPrivate = true;
			++intMsgCnt;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		vector<string> ProDeck;
		vector<string>* TempDeck = nullptr;
		string& key{ strVar["deck_name"] = readAttrName() };
		while (!strVar["deck_name"].empty() && strVar["deck_name"][0] == '_') {
			isPrivate = true;
			strVar["hidden"];
			strVar["deck_name"].erase(strVar["deck_name"].begin());
		}
		if (strVar["deck_name"].empty()) {
			reply(fmt->get_help("draw"));
			return 1;
		}
		else {
			if (gm->has_session(fromSession) && gm->session(fromSession).has_deck(key)) {
				gm->session(fromSession)._draw(this);
				return 1;
			}
			else if (CardDeck::findDeck(strVar["deck_name"]) == 0) {
				strReply = getMsg("strDeckNotFound");
				reply(strReply);
				return 1;
			}
			ProDeck = CardDeck::mPublicDeck[strVar["deck_name"]];
			TempDeck = &ProDeck;
		}
		int intCardNum = 1;
		switch (readNum(intCardNum)) {
		case 0:
			if (intCardNum == 0) {
				reply(getMsg("strNumCannotBeZero"));
				return 1;
			}
			break;
		case -1: break;
		case -2:
			reply(getMsg("strParaIllegal"));
			console.log("����:" + printQQ(fromQQ) + "��" + getMsg("strSelfName") + "ʹ���˷Ƿ�ָ�����\n" + strMsg, 1,
						printSTNow());
			return 1;
		}
		ResList Res;
		while (intCardNum--) {
			Res << CardDeck::drawCard(*TempDeck);
			if (TempDeck->empty())break;
		}
		strVar["res"] = Res.dot("|").show();
		strVar["cnt"] = to_string(Res.size());
		strVar["nick"] = getName(fromQQ, fromGroup);
	    getPCName(*this);
		initVar({ strVar["pc"], strVar["res"] });
		if (isPrivate) {
			reply(getMsg("strDrawHidden"));
			replyHidden(getMsg("strDrawCard"));
		}
		else
			reply(getMsg("strDrawCard"));
		if (intCardNum > 0) {
			reply(getMsg("strDeckEmpty"));
			return 1;
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "init") {
		intMsgCnt += 4;
		strVar["table_name"] = "�ȹ�";
		string strCmd = readPara();
		if (strCmd.empty()|| fromChat.second == msgtype::Private) {
			reply(fmt->get_help("init"));
		}
		else if (!gm->has_session(fromSession) || !gm->session(fromSession).table_count("�ȹ�")) {
			reply(getMsg("strGMTableNotExist"));
		}
		else if (strCmd == "show" || strCmd == "list") {
			strVar["res"] = gm->session(fromSession).table_prior_show("�ȹ�");
			reply(getMsg("strGMTableShow"));
		}
		else if (strCmd == "del") {
			strVar["table_item"] = readRest();
			if (strVar["table_item"].empty())
				reply(getMsg("strGMTableItemEmpty"));
			else if (gm->session(fromSession).table_del("�ȹ�", strVar["table_item"]))
				reply(getMsg("strGMTableItemDel"));
			else
				reply(getMsg("strGMTableItemNotFound"));
		}
		else if (strCmd == "clr") {
			gm->session(fromSession).table_clr("�ȹ�");
			reply(getMsg("strGMTableClr"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp") {
		if (console["DisabledJrrp"]) {
			reply("&strDisabledJrrpGlobal");
			return 1;
		}
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string Command = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
		if (fromChat.second == msgtype::Group) {
			if (Command == "on") {
				if (isAuth) {
					if (groupset(fromGroup, "����jrrp") > 0) {
						chat(fromGroup).reset("����jrrp");
						reply("�ɹ��ڱ�Ⱥ������JRRP!");
					}
					else {
						reply("�ڱ�Ⱥ��JRRPû�б�����!");
					}
				}
				else {
					reply(getMsg("strPermissionDeniedErr"));
				}
				return 1;
			}
			if (Command == "off") {
				if (isAuth) {
					if (groupset(fromGroup, "����jrrp") < 1) {
						chat(fromGroup).set("����jrrp");
						reply("�ɹ��ڱ�Ⱥ�н���JRRP!");
					}
					else {
						reply("�ڱ�Ⱥ��JRRPû�б�����!");
					}
				}
				else {
					reply(getMsg("strPermissionDeniedErr"));
				}
				return 1;
			}
			if (groupset(fromGroup, "����jrrp") > 0) {
				reply("�ڱ�Ⱥ��JRRP�����ѱ�����");
				return 1;
			}
		}
		else if (fromChat.second != msgtype::Discuss) {
			if (Command == "on") {
				if (groupset(fromGroup, "����jrrp") > 0) {
					chat(fromGroup).reset("����jrrp");
					reply("�ɹ��ڴ˶�������������JRRP!");
				}
				else {
					reply("�ڴ˶���������JRRPû�б�����!");
				}
				return 1;
			}
			if (Command == "off") {
				if (groupset(fromGroup, "����jrrp") < 1) {
					chat(fromGroup).set("����jrrp");
					reply("�ɹ��ڴ˶��������н���JRRP!");
				}
				else {
					reply("�ڴ˶���������JRRPû�б�����!");
				}
				return 1;
			}
			if (groupset(fromGroup, "����jrrp") > 0) {
				reply("�ڴ˶���������JRRP�ѱ�����!");
				return 1;
			}
		}
		strVar["nick"] = getName(fromQQ, fromGroup);
		strVar["res"] = to_string(today->getJrrp(fromQQ));
		reply(getMsg("strJrrp"), { strVar["nick"], strVar["res"] });
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "link") {
		intMsgCnt += 4;
		if (trusted < 3) {
			reply(getMsg("strNotAdmin"));
			return true;
		}
		strVar["option"] = readPara();
		if (strVar["option"] == "close") {
			gm->session(fromSession).link_close(this);
		}
		else if (strVar["option"] == "start") {
			gm->session(fromSession).link_start(this);
		}
		else if (strVar["option"] == "with" || strVar["option"] == "from" || strVar["option"] == "to") {
			gm->session(fromSession).link_new(this);
		}
		else {
			reply(fmt->get_help("link"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "name") {
		intMsgCnt += 4;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;

		string type = readPara();
		string strNum = readDigit();
		if (strNum.length() > 1 && strNum != "10") {
			reply(getMsg("strNameNumTooBig"));
			return 1;
		}
		int intNum = strNum.empty() ? 1 : stoi(strNum);
		if (intNum == 0) {
			reply(getMsg("strNameNumCannotBeZero"));
			return 1;
		}
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("�������_" + type)) ? "�������_" + type : "�������";
		vector<string> TempDeck(CardDeck::mPublicDeck[strDeckName]);
		ResList Res;
		while (intNum--) {
			Res << CardDeck::drawCard(TempDeck, true);
		}
		strVar["res"] = Res.dot("��").show();
		reply(getMsg("strNameGenerator"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "send") {
		intMsgCnt += 4;
		readSkipSpace();
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("send"));
			return 1;
		}
		//�ȿ���Master��������ָ��Ŀ�귢��
		if (trusted > 2) {
			chatType ct;
			if (!readChat(ct, true)) {
				readSkipColon();
				string strFwd(readRest());
				if (strFwd.empty()) {
					reply(getMsg("strSendMsgEmpty"));
				}
				else {
					AddMsgToQueue(strFwd, ct);
					reply(getMsg("strSendMsg"));
				}
				return 1;
			}
			else if (strLowerMessage.substr(intMsgCnt, 6) == "notice" && trusted > 3) {
				intMsgCnt += 6;
				int intLv = 0;
				string strNum{ readDigit(false) };
				while (!strNum.empty()){
					if (strNum.length() > 1)break;
					if (int intNum = stoi(strNum); intNum > 9)continue;
					else 					{
						intLv |= (1 << intNum);
					}
					if (strLowerMessage[intMsgCnt] == '+')++intMsgCnt;
					strNum = readDigit(false);
				}
				string strNotice(readRest());
				if (intLv && !strNotice.empty()){
					console.log(strNotice, intLv);
					reply(getMsg("strSendMsg"));
				}
				else reply(getMsg("strParaEmpty"));
				return 1;
			}
			readSkipColon();
		}
		else if (!console) {
			reply(getMsg("strSendMsgInvalid"));
			return 1;
		}
		else if (console["DisabledSend"] && trusted < 3) {
			reply(getMsg("strDisabledSendGlobal"));
			return 1;
		}
		string strInfo = readRest();
		if (strInfo.empty()) {
			reply(getMsg("strSendMsgEmpty"));
			return 1;
		}
		string strFwd = ((trusted > 4) ? "| " : ("| " + printFrom())) + strInfo;
		console.log(strFwd, 0b100, printSTNow());
		reply(getMsg("strSendMasterMsg"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 4) == "user") {
		intMsgCnt += 4;
		string strOption = readPara();
		if (strOption.empty())return 0;
		if (strOption == "state") {
			User& user = getUser(fromQQ);
			strVar["user"] = printQQ(fromQQ);
			ResList rep;
			rep << "���μ���" + to_string(trusted)
				<< "��{nick}�ĵ�һӡ���Լ����" + printDate(user.tCreated)
				<< (!(user.strNick.empty()) ? "����¼{nick}��" + to_string(user.strNick.size()) + "���ƺ�" : "û�м�¼{nick}�ĳƺ�")
				<< ((PList.count(fromQQ)) ? "������{nick}��" + to_string(PList[fromQQ].size()) + "�Ž�ɫ��" : "�޽�ɫ����¼")
				<< user.show();
			reply("{user}" + rep.show());
			return 1;
		}
		if (strOption == "trust") {
			if (trusted < 4 && fromQQ != console.master()) {
				reply(getMsg("strNotAdmin"));
				return 1;
			}
			string strTarget = readDigit();
			if (strTarget.empty()) {
				reply(getMsg("strQQIDEmpty"));
				return 1;
			}
			long long llTarget = stoll(strTarget);
			if (trustedQQ(llTarget) >= trusted && !console.is_self(fromQQ) && fromQQ != llTarget) {
				reply(getMsg("strUserTrustDenied"));
				return 1;
			}
			strVar["user"] = printQQ(llTarget);
			strVar["trust"] = readDigit();
			if (strVar["trust"].empty()) {
				if (!UserList.count(llTarget)) {
					reply(getMsg("strUserNotFound"));
					return 1;
				}
				strVar["trust"] = to_string(trustedQQ(llTarget));
				reply(getMsg("strUserTrustShow"));
				return 1;
			}
			User& user = getUser(llTarget);
			if (short intTrust = stoi(strVar["trust"]); intTrust < 0 || intTrust > 255 || (intTrust >= trusted && fromQQ
																						   != console.master())) {
				reply(getMsg("strUserTrustIllegal"));
				return 1;
			}
			else {
				user.trust(intTrust);
			}
			reply(getMsg("strUserTrusted"));
			return 1;
		}
		if (strOption == "diss") {
			if (trusted < 4 && fromQQ != console.master()) {
				reply(getMsg("strNotAdmin"));
				return 1;
			}
			strVar["note"] = readPara();
			long long llTargetID(readID());
			if (!llTargetID) {
				reply(getMsg("strQQIDEmpty"));
			}
			else if (trustedQQ(llTargetID) >= trusted) {
				reply(getMsg("strUserTrustDenied"));
			}
			else {
				blacklist->add_black_qq(llTargetID, this);
				UserList.erase(llTargetID);
				PList.erase(llTargetID);
			}
			return 1;
		}
		if (strOption == "kill") {
			if (trusted < 4 && fromQQ != console.master()) {
				reply(getMsg("strNotAdmin"));
				return 1;
			}
			long long llTarget = readID();
			if (trustedQQ(llTarget) >= trusted && fromQQ != console.master()) {
				reply(getMsg("strUserTrustDenied"));
				return 1;
			}
			strVar["user"] = printQQ(llTarget);
			if (!llTarget || !UserList.count(llTarget)) {
				reply(getMsg("strUserNotFound"));
				return 1;
			}
			UserList.erase(llTarget);
			reply("��Ĩ��{user}���û���¼");
			return 1;
		}
		if (strOption == "clr") {
			if (trusted < 5) {
				reply(getMsg("strNotMaster"));
				return 1;
			}
			int cnt = clearUser();
			note("��������Ч������û���¼" + to_string(cnt) + "��", 0b10);
			return 1;
		}
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "coc") {
		intMsgCnt += 3;
		if (strLowerMessage[intMsgCnt] == '7')
			intMsgCnt++;
		if (strLowerMessage[intMsgCnt] == 's')
			intMsgCnt++;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 1 && strNum != "10") {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0) {
			reply(getMsg("strCharacterCannotBeZero"));
			return 1;
		}
		COC7(strVar["res"], intNum);
		reply(getMsg("strCOCBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		string strNum;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strNum += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		if (strNum.length() > 1 && strNum != "10") {
			reply(getMsg("strCharacterTooBig"));
			return 1;
		}
		const int intNum = stoi(strNum.empty() ? "1" : strNum);
		if (intNum == 0) {
			reply(getMsg("strCharacterCannotBeZero"));
			return 1;
		}
		DND(strVar["res"], intNum);
		reply(getMsg("strDNDBuild"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "log") {
		intMsgCnt += 3;
		string strPara = readPara();
		if (strPara.empty()) {
			reply(fmt->get_help("log"));
		}
		else if (DiceSession& game = gm->session(fromSession); strPara == "new") {
			game.log_new(this);
		}
		else if (strPara == "on") {
			game.log_on(this);
		}
		else if (strPara == "off") {
			game.log_off(this);
		}
		else if (strPara == "end") {
			game.log_end(this);
		}
		else {
			reply(fmt->get_help("log"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "nnn") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string type = readPara();
		string strDeckName = (!type.empty() && CardDeck::mPublicDeck.count("�������_" + type)) ? "�������_" + type : "�������";
		strVar["nick"] = getName(fromQQ, fromGroup);
		strVar["new_nick"] = strip(CardDeck::drawCard(CardDeck::mPublicDeck[strDeckName], true));
		getUser(fromQQ).setNick(fromGroup, strVar["new_nick"]);
		const string strReply = format(getMsg("strNameSet"), { strVar["nick"], strVar["new_nick"] });
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "set") {
		intMsgCnt += 3;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		strVar["default"] = readDigit();
		while (strVar["default"][0] == '0')
			strVar["default"].erase(strVar["default"].begin());
		if (strVar["default"].empty())
			strVar["default"] = "100";
		for (auto charNumElement : strVar["default"])
			if (!isdigit(static_cast<unsigned char>(charNumElement))) {
				reply(getMsg("strSetInvalid"));
				return 1;
			}
		if (strVar["default"].length() > 4) {
			reply(getMsg("strSetTooBig"));
			return 1;
		}
		strVar["nick"] = getName(fromQQ, fromGroup);
	    getPCName(*this);
		const int intDefaultDice = stoi(strVar["default"]);
		if (PList.count(fromQQ)) {
			PList[fromQQ][fromGroup]["__DefaultDice"] = intDefaultDice;
			reply("�ѽ�" + strVar["pc"] + "��Ĭ�������͸���ΪD" + strVar["default"]);
			return 1;
		}
		if (intDefaultDice == 100)
			getUser(fromQQ).rmIntConf("Ĭ����");
		else
			getUser(fromQQ).setConf("Ĭ����", intDefaultDice);
		
		reply("�ѽ�" + strVar["nick"] + "��Ĭ�������͸���ΪD" + strVar["default"]);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 3) == "str" && trusted > 3) {
		string strName;
		while (!isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && intMsgCnt != strLowerMessage.length()
			   ) {
			strName += strMsg[intMsgCnt];
			intMsgCnt++;
		}
		while (strMsg[intMsgCnt] == ' ')intMsgCnt++;
		if (intMsgCnt == strMsg.length() || strMsg.substr(intMsgCnt) == "show") {
			std::shared_lock lock(GlobalMsgMutex);
			const auto it = GlobalMsg.find(strName);
			if (it != GlobalMsg.end())AddMsgToQueue(it->second, fromChat);
			return 1;
		}
		string strMessage = strMsg.substr(intMsgCnt);
		if (strMessage == "reset") {
			{
				std::unique_lock lock(GlobalMsgMutex);
				EditedMsg.erase(strName);
				GlobalMsg[strName] = "";
			}
			note("�����" + strName + "���Զ��壬�����´�������ָ�Ĭ�����á�", 0b1);
		}
		else {
			{
				std::unique_lock lock(GlobalMsgMutex);
				if (strMessage == "NULL")strMessage = "";
				EditedMsg[strName] = strMessage;
				GlobalMsg[strName] = strMessage;
			}
			note("���Զ���" + strName + "���ı�", 0b1);
		}
		saveJMap(DiceDir / "conf" / "CustomMsg.json", EditedMsg);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "en") {
	intMsgCnt += 2;
	while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
		intMsgCnt++;
	if (strMsg.length() == intMsgCnt) {
		reply(fmt->get_help("en"));
		return 1;
	}
	string& strAttr = strVar["attr"] = readAttrName();
	string strCurrentValue{ readDigit(false) };
	CharaCard* pc{ PList.count(fromQQ) ? &getPlayer(fromQQ)[fromGroup] : nullptr };
	int intVal{ 0 };
		//��ȡ����ԭֵ
		if (strCurrentValue.empty()) {
			if (pc && !strAttr.empty() && (pc->stored(strAttr))) {
				intVal = getPlayer(fromQQ)[fromGroup][strAttr].to_int();
			}
			else {
				reply(getMsg("strEnValEmpty"));
				return 1;
			}
		}
		else {
			if (strCurrentValue.length() > 3) {
				reply(getMsg("strEnValInvalid"));
				return 1;
			}
			intVal = stoi(strCurrentValue);
		}
		readSkipSpace();
		//�ɱ�ɳ�ֵ���ʽ
		string strEnChange;
		string strEnFail;
		string strEnSuc = "1D10";
		//�ԼӼ�������ͷȷ���뼼��ֵ������
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strEnChange = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);
			//û��'/'ʱĬ�ϳɹ��仯ֵ
			if (strEnChange.find('/') != std::string::npos) {
				strEnFail = strEnChange.substr(0, strEnChange.find('/'));
				strEnSuc = strEnChange.substr(strEnChange.find('/') + 1);
			}
			else strEnSuc = strEnChange;
		}
		if (strAttr.empty())strAttr = getMsg("strEnDefaultName");
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		//�ɳ��춨����������ͳ�ƣ�������춨ͳ��
		if (pc)pc->cntRollStat(intTmpRollRes, 100);
		strVar["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intVal) + " ";
		if (intTmpRollRes <= intVal && intTmpRollRes <= 95) {
			if (strEnFail.empty()) {
				strVar["res"] += getMsg("strFailure");
				reply(getMsg("strEnRollNotChange"));
				return 1;
			}
			strVar["res"] += getMsg("strFailure");
			RD rdEnFail(strEnFail);
			if (rdEnFail.Roll()) {
				reply(getMsg("strValueErr"));
				return 1;
			}
			intVal = intVal + rdEnFail.intTotal;
			strVar["change"] = rdEnFail.FormCompleteString();
			strVar["final"] = to_string(intVal);
			reply(getMsg("strEnRollFailure"));
		}
		else {
			strVar["res"] += getMsg("strSuccess");
			RD rdEnSuc(strEnSuc);
			if (rdEnSuc.Roll()) {
				reply(getMsg("strValueErr"));
				return 1;
			}
			intVal = intVal + rdEnSuc.intTotal;
			strVar["change"] = rdEnSuc.FormCompleteString();
			strVar["final"] = to_string(intVal);
			reply(getMsg("strEnRollSuccess"));
		}
		if (pc)pc->set(strAttr, intVal);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "li") {
		string strAns = "{pc}�ķ����-�ܽ�֢״:\n";
		LongInsane(strAns);
		reply(strAns);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "me") {
		if (trusted < 4 && console["DisabledMe"]) {
			reply(getMsg("strDisabledMeGlobal"));
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (fromChat.second == msgtype::Private) {
			string strGroupID = readDigit();
			if (strGroupID.empty()) {
				reply(getMsg("strGroupIDEmpty"));
				return 1;
			}
			const long long llGroupID = stoll(strGroupID);
			if (groupset(llGroupID, "ͣ��ָ��") && trusted < 4) {
				reply(getMsg("strDisabledErr"));
				return 1;
			}
			if (groupset(llGroupID, "����me") && trusted < 5) {
				reply(getMsg("strMEDisabledErr"));
				return 1;
			}
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			string strAction = strip(readRest());
			if (strAction.empty()) {
				reply(getMsg("strActionEmpty"));
				return 1;
			}
			string strReply = getName(fromQQ, llGroupID) + strAction;
			DD::sendGroupMsg(llGroupID, strReply);
			reply(getMsg("strSendSuccess"));
			return 1;
		}
		string strAction = strLowerMessage.substr(intMsgCnt);
		if (!isAuth && (strAction == "on" || strAction == "off")) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		if (strAction == "off") {
			if (groupset(fromGroup, "����me") < 1) {
				chat(fromGroup).set("����me");
				reply(getMsg("strMeOff"));
			}
			else {
				reply(getMsg("strMeOffAlready"));
			}
			return 1;
		}
		if (strAction == "on") {
			if (groupset(fromGroup, "����me") > 0) {
				chat(fromGroup).reset("����me");
				reply(getMsg("strMeOn"));
			}
			else {
				reply(getMsg("strMeOnAlready"));
			}
			return 1;
		}
		if (groupset(fromGroup, "����me")) {
			reply(getMsg("strMEDisabledErr"));
			return 1;
		}
		strAction = strip(readRest());
		if (strAction.empty()) {
			reply(getMsg("strActionEmpty"));
			return 1;
		}
		strVar["nick"] = getName(fromQQ, fromGroup);
	    getPCName(*this);
		trusted > 4 ? reply(strAction) : reply(strVar["pc"] + strAction);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "nn") {
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		strVar["nick"] = getName(fromQQ, fromGroup);
		strVar["new_nick"] = strip(strMsg.substr(intMsgCnt));
		filter_CQcode(strVar["new_nick"]);
		if (strVar["new_nick"].length() > 50) {
			reply(getMsg("strNameTooLongErr"));
			return 1;
		}
		if (!strVar["new_nick"].empty()) {
			getUser(fromQQ).setNick(fromGroup, strVar["new_nick"]);
			reply(getMsg("strNameSet"), { strVar["nick"], strVar["new_nick"] });
		}
		else {
			if (getUser(fromQQ).rmNick(fromGroup)) {
				reply(getMsg("strNameClr"), { strVar["nick"] });
			}
			else {
				reply(getMsg("strNameDelEmpty"));
			}
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ob") {
		if (fromChat.second == msgtype::Private) {
			reply(fmt->get_help("ob"));
			return 1;
		}
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		const string strOption = strLowerMessage.substr(intMsgCnt, strMsg.find(' ', intMsgCnt) - intMsgCnt);

		if (!isAuth && (strOption == "on" || strOption == "off")) {
			reply(getMsg("strPermissionDeniedErr"));
			return 1;
		}
		strVar["option"] = "����ob";
		if (strOption == "off") {
			if (groupset(fromGroup, strVar["option"]) < 1) {
				chat(fromGroup).set(strVar["option"]);
				gm->session(fromSession).clear_ob();
				reply(getMsg("strObOff"));
			}
			else {
				reply(getMsg("strObOffAlready"));
			}
			return 1;
		}
		if (strOption == "on") {
			if (groupset(fromGroup, strVar["option"]) > 0) {
				chat(fromGroup).reset(strVar["option"]);
				reply(getMsg("strObOn"));
			}
			else {
				reply(getMsg("strObOnAlready"));
			}
			return 1;
		}
		if (groupset(fromGroup, strVar["option"]) > 0) {
			reply(getMsg("strObOffAlready"));
			return 1;
		}
		if (strOption == "list") {
			gm->session(fromSession).ob_list(this);
		}
		else if (strOption == "clr") {
			if (isAuth) {
				gm->session(fromSession).ob_clr(this);
			}
			else {
				reply(getMsg("strPermissionDeniedErr"));
			}
		}
		else if (strOption == "exit") {
			gm->session(fromSession).ob_exit(this);
		}
		else {
			gm->session(fromSession).ob_enter(this);
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "pc") {
		intMsgCnt += 2;
		string strOption = readPara();
		if (strOption.empty()) {
			reply(fmt->get_help("pc"));
			return 1;
		}
		Player& pl = getPlayer(fromQQ);
		if (strOption == "tag") {
			strVar["char"] = readRest();
			switch (pl.changeCard(strVar["char"], fromGroup)) {
			case 1:
				reply(getMsg("strPcCardReset"));
				break;
			case 0:
				reply(getMsg("strPcCardSet"));
				break;
			case -5:
				reply(getMsg("strPcNameNotExist"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "show") {
			string strName = readRest();
			CharaCard& pc{ pl.getCard(strName, fromGroup) };
			strVar["char"] = pc.getName();
			strVar["type"] = pc.Attr["__Type"].to_str();
			strVar["show"] = pc.show(true);
			reply(getMsg("strPcCardShow"));
			return 1;
		}
		if (strOption == "new") {
			strVar["char"] = strip(readRest());
			filter_CQcode(strVar["char"]);
			switch (pl.newCard(strVar["char"], fromGroup)) {
			case 0:
				strVar["type"] = pl[fromGroup].Attr["__Type"].to_str();
				strVar["show"] = pl[fromGroup].show(true);
				if (strVar["show"].empty())reply(getMsg("strPcNewEmptyCard"));
				else reply(getMsg("strPcNewCardShow"));
				break;
			case -1:
				reply(getMsg("strPcCardFull"));
				break;
			case -4:
				reply(getMsg("strPcNameExist"));
				break;
			case -6:
				reply(getMsg("strPcNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "build") {
			strVar["char"] = strip(readRest());
			filter_CQcode(strVar["char"]);
			switch (pl.buildCard(strVar["char"], false, fromGroup)) {
			case 0:
				strVar["show"] = pl[strVar["char"]].show(true);
				reply(getMsg("strPcCardBuild"));
				break;
			case -1:
				reply(getMsg("strPcCardFull"));
				break;
			case -2:
				reply(getMsg("strPcTempInvalid"));
				break;
			case -6:
				reply(getMsg("strPCNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "list") {
			strVar["show"] = pl.listCard();
			reply(getMsg("strPcCardList"));
			return 1;
		}
		if (strOption == "nn") {
			strVar["new_name"] = strip(readRest());
			filter_CQcode(strVar["new_name"]);
			strVar["old_name"] = pl[fromGroup].getName();
			switch (pl.renameCard(strVar["old_name"], strVar["new_name"])) {
			case 0:
				reply(getMsg("strPcCardRename"));
				break;
			case -3:
				reply(getMsg("strPCNameEmpty"));
				break;
			case -4:
				reply(getMsg("strPCNameExist"));
				break;
			case -6:
				reply(getMsg("strPCNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "del") {
			strVar["char"] = strip(readRest());
			switch (pl.removeCard(strVar["char"])) {
			case 0:
				reply(getMsg("strPcCardDel"));
				break;
			case -5:
				reply(getMsg("strPcNameNotExist"));
				break;
			case -7:
				reply(getMsg("strPcInitDelErr"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "redo") {
			strVar["char"] = strip(readRest());
			pl.buildCard(strVar["char"], true, fromGroup);
			strVar["show"] = pl[strVar["char"]].show(true);
			reply(getMsg("strPcCardRedo"));
			return 1;
		}
		if (strOption == "grp") {
			strVar["show"] = pl.listMap();
			reply(getMsg("strPcGroupList"));
			return 1;
		}
		if (strOption == "cpy") {
			string strName = strip(readRest());
			filter_CQcode(strName);
			strVar["char1"] = strName.substr(0, strName.find('='));
			strVar["char2"] = (strVar["char1"].length() + 1 < strName.length())
				? strip(strName.substr(strVar["char1"].length() + 1))
				: pl[fromGroup].getName();
			switch (pl.copyCard(strVar["char1"], strVar["char2"], fromGroup)) {
			case 0:
				reply(getMsg("strPcCardCpy"));
				break;
			case -1:
				reply(getMsg("strPcCardFull"));
				break;
			case -3:
				reply(getMsg("strPcNameEmpty"));
				break;
			case -6:
				reply(getMsg("strPcNameInvalid"));
				break;
			default:
				reply(getMsg("strUnknownErr"));
				break;
			}
			return 1;
		}
		if (strOption == "stat") {
			CharaCard& pc{ pl[fromGroup] };
			bool isEmpty{ true };
			ResList res;
			int intFace{ pc.count("__DefaultDice")
				? pc.call(string("__DefaultDice"))
				: get(getUser(fromQQ).intConf, string("Ĭ����"), 100) };
			string strFace{ to_string(intFace) };
			string keyStatCnt{ "__StatD" + strFace + "Cnt" };	//��������
			if (intFace <= 100 && pc.count(keyStatCnt)) {
				int cntRoll{ pc[keyStatCnt].to_int() };	
				if (cntRoll > 0) {
					isEmpty = false;
					res << "D" + strFace + "ͳ�ƴ���: " + to_string(cntRoll);
					int sumRes{ pc["__StatD" + strFace + "Sum"].to_int() };		//������
					int sumResSqr{ pc["__StatD" + strFace + "SqrSum"].to_int() };	//����ƽ����
					DiceEst stat{ intFace,cntRoll,sumRes,sumResSqr };
					if (stat.estMean > 0)
						res << "��ֵ: " + toString(stat.estMean, 2, true) + " [" + toString(stat.expMean) + "]";
					if (stat.pNormDist) {
						if (stat.pNormDist < 0.5)res << "��ֵ����" + toString(100 - stat.pNormDist * 100, 2) + "%���û�";
						else res << "��ֵ����" + toString(stat.pNormDist * 100, 2) + "%���û�";
					}
					if (stat.estStd > 0) {
						res << "��׼��: " + toString(stat.estStd, 2) + " [" + toString(stat.expStd) + "]";
					}
					/*if (stat.pZtest > 0) {
						res << "Z���顰ƫ�ġ�ˮƽ: " + toString(stat.pZtest * 100, 2) + "%";
					}*/
				}
			}
			string keyRcCnt{ "__StatRcCnt" };	//rc/sc�춨����
			if (pc.count(keyRcCnt)) {
				int cntRc{ pc["__StatRcCnt"].to_int() };
				if (cntRc > 0) {
					isEmpty = false;
					res << "�춨ͳ�ƴ���: " + to_string(cntRc);
					int sumRcSuc{ pc["__StatRcSumSuc"].to_int() };//ʵ�ʳɹ���
					int sumRcRate{ pc["__StatRcSumRate"].to_int() };//�ܳɹ���
					res << "�춨[����]�ɹ���: " + toString((double)sumRcSuc / cntRc * 100) + "%" + "(" + to_string(sumRcSuc) + ") [" + toString((double)sumRcRate / cntRc) + "%]";
					if (pc.count("__StatRcCnt5") || pc.count("__StatRcCnt96"))
						res << "5- | 96+ ������: " + toString((double)pc["__StatRcCnt5"].to_int() / cntRc * 100) + "%" + "(" + pc["__StatRcCnt5"].to_str() + ") | " + toString((double)pc["__StatRcCnt96"].to_int() / cntRc * 100) + "%" + "(" + pc["__StatRcCnt96"].to_str() + ")";
					if(pc.count("__StatRcCnt1")|| pc.count("__StatRcCnt100"))
						res << "1 | 100 ���ִ���: " + to_string(pc["__StatRcCnt1"].to_int()) + " | " + to_string(pc["__StatRcCnt100"].to_int());
				}
			}
			if (isEmpty) {
				reply(getMsg("strPcStatEmpty"));
			}
			else {
				strVar["stat"] = res.show();
				reply(getMsg("strPcStatShow"));
			}
			return 1;
		}
		if (strOption == "clr") {
			PList.erase(fromQQ);
			reply(getMsg("strPcClr"));
			return 1;
		}
		if (strOption == "type") {
			strVar["new_type"] = strip(readRest());
			if (strVar["new_type"].empty()) {
				strVar["attr"] = "ģ����";
				strVar["val"] = pl[fromGroup].Attr["__Type"].to_str();
				reply(getMsg("strProp"));
			}
			else {
				pl[fromGroup].setType(strVar["new_type"]);
				reply(getMsg("strSetPropSuccess"));
			}
			return 1;
		}
		if (strOption == "temp") {
			CardTemp& temp{ *pl[fromGroup].pTemplet};
			reply(temp.show());
			return 1;
		}
		reply(fmt->get_help("pc"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ra" || strLowerMessage.substr(intMsgCnt, 2) == "rc") {
		intMsgCnt += 2;
		if (strMsg.length() == intMsgCnt) {
			reply(fmt->get_help("rc"));
			return 1;
		}
		int intRule = fromChat.second != msgtype::Private
			? get(chat(fromGroup).intConf, string("rc����"), 0)
			: get(getUser(fromQQ).intConf, string("rc����"), 0);
		int intTurnCnt = 1;
		bool isHidden(false);
		if (strMsg[intMsgCnt] == 'h' && isspace(static_cast<unsigned char>(strMsg[intMsgCnt + 1]))) {
			isHidden = true;
			++intMsgCnt;
		}
		else if (readSkipSpace(); strMsg[intMsgCnt] == '_') {
			isHidden = true;
			++intMsgCnt;
		}
		readSkipSpace();
		if (strMsg.find('#') != string::npos) {
			string strTurnCnt = strMsg.substr(intMsgCnt, strMsg.find('#') - intMsgCnt);
			//#�ܷ�ʶ����Ч
			if (strTurnCnt.empty())intMsgCnt++;
			else if ((strTurnCnt.length() == 1 && isdigit(static_cast<unsigned char>(strTurnCnt[0]))) || strTurnCnt ==
					 "10") {
				intMsgCnt += strTurnCnt.length() + 1;
				intTurnCnt = stoi(strTurnCnt);
			}
		}
		string strMainDice = "D100";
		string strSkillModify;
		//���ѵȼ�
		string strDifficulty;
		int intDifficulty = 1;
		int intSkillModify = 0;
		//����
		int intSkillMultiple = 1;
		//����
		int intSkillDivisor = 1;
		//�Զ��ɹ�
		bool isAutomatic = false;
		//D100���н�ɫ��ʱ����ͳ��
		bool isStatic = PList.count(fromQQ);
		CharaCard* pc{ isStatic ? &PList[fromQQ][fromGroup] : nullptr };
		if ((strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b') && strLowerMessage[intMsgCnt - 1] != ' ') {
			isStatic = false;
			strMainDice = strLowerMessage[intMsgCnt];
			intMsgCnt++;
			while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
				strMainDice += strLowerMessage[intMsgCnt];
				intMsgCnt++;
			}
		}
		readSkipSpace();
		if (strMsg[intMsgCnt] == '_') {
			isHidden = true;
			++intMsgCnt;
		}
		if (strMsg.length() == intMsgCnt) {
			strVar["attr"] = getMsg("strEnDefaultName");
			reply(getMsg("strUnknownPropErr"), { strVar["attr"] });
			return 1;
		}
		strVar["attr"] = strMsg.substr(intMsgCnt);
		if (strVar["attr"].find("�Զ��ɹ�") == 0) {
			strDifficulty = strVar["attr"].substr(0, 8);
			strVar["attr"] = strVar["attr"].substr(8);
			isAutomatic = true;
		}
		if (strVar["attr"].find("����") == 0 || strVar["attr"].find("����") == 0) {
			strDifficulty += strVar["attr"].substr(0, 4);
			intDifficulty = (strVar["attr"].substr(0, 4) == "����") ? 2 : 5;
			strVar["attr"] = strVar["attr"].substr(4);
		}
		if (pc && pc->count(strVar["attr"]))intMsgCnt = strMsg.length();
		else strVar["attr"] = readAttrName();
		if (strLowerMessage[intMsgCnt] == '*' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			intMsgCnt++;
			readNum(intSkillMultiple);
		}
		while ((strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') && isdigit(
			strLowerMessage[intMsgCnt + 1])) {
			if (!readNum(intSkillModify))strSkillModify = to_signed_string(intSkillModify);
		}
		if (strLowerMessage[intMsgCnt] == '/' && isdigit(strLowerMessage[intMsgCnt + 1])) {
			intMsgCnt++;
			readNum(intSkillDivisor);
			if (intSkillDivisor == 0) {
				reply(getMsg("strValueErr"));
				return 1;
			}
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' ||
			   strLowerMessage[intMsgCnt] ==
			   ':')
			intMsgCnt++;
		string strSkillVal;
		while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			strSkillVal += strLowerMessage[intMsgCnt];
			intMsgCnt++;
		}
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))) {
			intMsgCnt++;
		}
		strVar["reason"] = readRest();
		int intSkillVal;
		if (strSkillVal.empty()) {
			if (pc && pc->count(strVar["attr"])) {
				intSkillVal = PList[fromQQ][fromGroup].call(strVar["attr"]);
			}
			else {
				if (!pc && SkillNameReplace.count(strVar["attr"])) {
					strVar["attr"] = SkillNameReplace[strVar["attr"]];
				}
				if (!pc && SkillDefaultVal.count(strVar["attr"])) {
					intSkillVal = SkillDefaultVal[strVar["attr"]];
				}
				else {
					reply(getMsg("strUnknownPropErr"), { strVar["attr"] });
					return 1;
				}
			}
		}
		else if (strSkillVal.length() > 3) {
			reply(getMsg("strPropErr"));
			return 1;
		}
		else {
			intSkillVal = stoi(strSkillVal);
		}
		//���ճɹ��ʼ���춨ͳ��
		int intFianlSkillVal = (intSkillVal * intSkillMultiple + intSkillModify) / intSkillDivisor / intDifficulty;
		if (intFianlSkillVal < 0 || intFianlSkillVal > 1000) {
			reply(getMsg("strSuccessRateErr"));
			return 1;
		}
		RD rdMainDice(strMainDice);
		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes == ZeroDice_Err) {
			reply(getMsg("strZeroDiceErr"));
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err) {
			reply(getMsg("strDiceTooBigErr"));
			return 1;
		}
		strVar["attr"] = strDifficulty + strVar["attr"] + (
			(intSkillMultiple != 1) ? "��" + to_string(intSkillMultiple) : "") + strSkillModify + ((intSkillDivisor != 1)
																								  ? "/" + to_string(
																									  intSkillDivisor)
																								  : "");
		strVar["nick"] = getName(fromQQ, fromGroup);
	    getPCName(*this);																						 
		if (strVar["reason"].empty()) {
			strReply = format(getMsg("strRollSkill"), { strVar["pc"], strVar["attr"] });
		}
		else strReply = format(getMsg("strRollSkillReason"), { strVar["pc"], strVar["attr"], strVar["reason"] });
		ResList Res;
		string strAns;
		if (intTurnCnt == 1) {
			rdMainDice.Roll();
			if (isStatic) {
				pc->cntRollStat(rdMainDice.intTotal, 100);
				pc->cntRcStat(rdMainDice.intTotal, intFianlSkillVal);
			}
			strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
			int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
			switch (intRes) {
			case 0: strAns += getMsg("strRollFumble");
				break;
			case 1: strAns += isAutomatic ? getMsg("strRollRegularSuccess") : getMsg("strRollFailure");
				break;
			case 5: strAns += getMsg("strRollCriticalSuccess");
				break;
			case 4: if (intDifficulty == 1) {
				strAns += getMsg("strRollExtremeSuccess");
				break;
			}
			case 3: if (intDifficulty == 1) {
				strAns += getMsg("strRollHardSuccess");
				break;
			}
			case 2: strAns += getMsg("strRollRegularSuccess");
				break;
			}
			strReply += strAns;
		}
		else {
			Res.dot("\n");
			while (intTurnCnt--) {
				rdMainDice.Roll();
				if (isStatic) {
					pc->cntRollStat(rdMainDice.intTotal, 100);
					pc->cntRcStat(rdMainDice.intTotal, intFianlSkillVal);
				}
				strAns = rdMainDice.FormCompleteString() + "/" + to_string(intFianlSkillVal) + " ";
				int intRes = RollSuccessLevel(rdMainDice.intTotal, intFianlSkillVal, intRule);
				switch (intRes) {
				case 0: strAns += getMsg("strFumble");
					break;
				case 1: strAns += isAutomatic ? getMsg("strSuccess") : getMsg("strFailure");
					break;
				case 5: strAns += getMsg("strCriticalSuccess");
					break;
				case 4: if (intDifficulty == 1) {
					strAns += getMsg("strExtremeSuccess");
					break;
				}
				case 3: if (intDifficulty == 1) {
					strAns += getMsg("strHardSuccess");
					break;
				}
				case 2: strAns += getMsg("strSuccess");
					break;
				}
				Res << strAns;
			}
			strReply += Res.show();
		}
		if (isHidden) {
			replyHidden();
			reply(getMsg("strRollSkillHidden"));
		}
		else
			reply();
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ri") {
		if (fromChat.second == msgtype::Private) {
			reply(fmt->get_help("ri"));
			return 1;
		}
		intMsgCnt += 2;
		string strinit = "D20";
		if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-') {
			strinit += readDice();
		}
		else if (isRollDice()) {
			strinit = readDice();
		}
		readSkipSpace();
		string strname = strip(strMsg.substr(intMsgCnt));
		if (strname.empty()) {
			
			if (!strVar.count("pc") || strVar["pc"].empty()) {
				strVar["nick"] = getName(fromQQ, fromGroup);
				getPCName(*this);
			}
			strname = strVar["pc"];
		}
		RD initdice(strinit, 20);
		const int intFirstTimeRes = initdice.Roll();
		if (intFirstTimeRes == Value_Err) {
			reply(getMsg("strValueErr"));
			return 1;
		}
		if (intFirstTimeRes == Input_Err) {
			reply(getMsg("strInputErr"));
			return 1;
		}
		if (intFirstTimeRes == ZeroDice_Err) {
			reply(getMsg("strZeroDiceErr"));
			return 1;
		}
		if (intFirstTimeRes == ZeroType_Err) {
			reply(getMsg("strZeroTypeErr"));
			return 1;
		}
		if (intFirstTimeRes == DiceTooBig_Err) {
			reply(getMsg("strDiceTooBigErr"));
			return 1;
		}
		if (intFirstTimeRes == TypeTooBig_Err) {
			reply(getMsg("strTypeTooBigErr"));
			return 1;
		}
		if (intFirstTimeRes == AddDiceVal_Err) {
			reply(getMsg("strAddDiceValErr"));
			return 1;
		}
		if (intFirstTimeRes != 0) {
			reply(getMsg("strUnknownErr"));
			return 1;
		}
		gm->session(fromSession).table_add("�ȹ�", initdice.intTotal, strname);
		const string strReply = strname + "���ȹ����㣺" + initdice.FormCompleteString();
		reply(strReply);
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "sc") {
		intMsgCnt += 2;
		string SanCost = readUntilSpace();
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (SanCost.empty()) {
			reply(fmt->get_help("sc"));
			return 1;
		}
		if (SanCost.find('/') == string::npos) {
			reply(getMsg("strSanCostInvalid"));
			return 1;
		}
		string attr = "����";
		int intSan = 0;
		CharaCard* pc{ PList.count(fromQQ) ? &getPlayer(fromQQ)[fromGroup] : nullptr };
		if (readNum(intSan)) {
			if (pc && pc->count(attr)) {
				intSan = pc->call(attr);
			}
			else {
				reply(getMsg("strSanEmpty"));
				return 1;
			}
		}
		string strSanCostSuc = SanCost.substr(0, SanCost.find('/'));
		string strSanCostFail = SanCost.substr(SanCost.find('/') + 1);
		for (const auto& character : strSanCostSuc) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				reply(getMsg("strSanCostInvalid"));
				return 1;
			}
		}
		for (const auto& character : SanCost.substr(SanCost.find('/') + 1)) {
			if (!isdigit(static_cast<unsigned char>(character)) && character != 'D' && character != 'd' && character !=
				'+' && character != '-') {
				reply(getMsg("strSanCostInvalid"));
				return 1;
			}
		}
		RD rdSuc(strSanCostSuc);
		RD rdFail(strSanCostFail);
		if (rdSuc.Roll() != 0 || rdFail.Roll() != 0) {
			reply(getMsg("strSanCostInvalid"));
			return 1;
		}
		if (intSan <= 0) {
			reply(getMsg("strSanInvalid"));
			return 1;
		}
		const int intTmpRollRes = RandomGenerator::Randint(1, 100);
		//���Ǽ춨����ͳ��
		if (pc) {
			pc->cntRollStat(intTmpRollRes, 100);
			pc->cntRcStat(intTmpRollRes, intSan);
		}
		strVar["res"] = "1D100=" + to_string(intTmpRollRes) + "/" + to_string(intSan) + " ";
		//���÷���
		int intRule = fromGroup
			? get(chat(fromGroup).intConf, string("rc����"), 0)
			: get(getUser(fromQQ).intConf, string("rc����"), 0);
		switch (RollSuccessLevel(intTmpRollRes, intSan, intRule)) {
		case 5:
		case 4:
		case 3:
		case 2:
			strVar["res"] += getMsg("strSuccess");
			strVar["change"] = rdSuc.FormCompleteString();
			intSan = max(0, intSan - rdSuc.intTotal);
			break;
		case 1:
			strVar["res"] += getMsg("strFailure");
			strVar["change"] = rdFail.FormCompleteString();
			intSan = max(0, intSan - rdFail.intTotal);
			break;
		case 0:
			strVar["res"] += getMsg("strFumble");
			rdFail.Max();
			strVar["change"] = rdFail.strDice + "���ֵ=" + to_string(rdFail.intTotal);
			intSan = max(0, intSan - rdFail.intTotal);
			break;
		}
		strVar["final"] = to_string(intSan);
		if (pc)pc->set(attr, intSan);
		reply(getMsg("strSanRollRes"));
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "st") {
		intMsgCnt += 2;
		while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			intMsgCnt++;
		if (intMsgCnt == strLowerMessage.length()) {
			reply(fmt->get_help("st"));
			return 1;
		}
		strVar["nick"] = getName(fromQQ, fromGroup);
		getPCName(*this);
		if (strLowerMessage.substr(intMsgCnt, 3) == "clr") {
			if (!PList.count(fromQQ)) {
				reply(getMsg("strPcNotExistErr"));
				return 1;
			}
			getPlayer(fromQQ)[fromGroup].clear();
			strVar["char"] = getPlayer(fromQQ)[fromGroup].getName();
			reply(getMsg("strPropCleared"), { strVar["char"] });
			return 1;
		}
		if (strLowerMessage.substr(intMsgCnt, 3) == "del") {
			if (!PList.count(fromQQ)) {
				reply(getMsg("strPcNotExistErr"));
				return 1;
			}
			intMsgCnt += 3;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			if (strMsg[intMsgCnt] == '&') {
				intMsgCnt++;
			}
			strVar["attr"] = readAttrName();
			if (getPlayer(fromQQ)[fromGroup].erase(strVar["attr"])) {
				reply(getMsg("strPropDeleted"), { strVar["pc"], strVar["attr"] });
			}
			else {
				reply(getMsg("strPropNotFound"), { strVar["attr"] });
			}
			return 1;
		}
		CharaCard& pc = getPlayer(fromQQ)[fromGroup];
		if (strLowerMessage.substr(intMsgCnt, 4) == "show") {
			intMsgCnt += 4;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				intMsgCnt++;
			strVar["attr"] = readAttrName();
			if (strVar["attr"].empty()) {
				strVar["char"] = pc.getName();
				strVar["type"] = pc.Attr["__Type"].to_str();
				strVar["show"] = pc.show(false);
				reply(getMsg("strPropList"));
				return 1;
			}
			if (pc.show(strVar["attr"], strVar["val"]) > -1) {
				reply(format(getMsg("strProp"), { strVar["pc"], strVar["attr"], strVar["val"] }));
			}
			else {
				reply(getMsg("strPropNotFound"), { strVar["attr"] });
			}
			return 1;
		}
		bool boolError = false;
		bool isDetail = false;
		bool isModify = false;
		//ѭ��¼��
		int cntInput{ 0 };
		while (intMsgCnt != strLowerMessage.length()) {
			readSkipSpace();
			//�ж�¼����ʽ
			if (strMsg[intMsgCnt] == '&') {
				strVar["attr"] = readToColon(); 
				if (strVar["attr"].empty()) {
					continue;
				}
				if (pc.set(strVar["attr"], readExp())) {
					reply(getMsg("strPcTextTooLong"));
					return 1;
				}
				++cntInput;
				continue;
			}
			//��ȡ������
			string strSkillName = readAttrName();
			if (strSkillName.empty()) {
				readSkipSpace();
				while (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '+' ||
			           strMsg[intMsgCnt] == '-' || strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/')
				{
					intMsgCnt++;
				}
				readDigit(false);
				continue;
			}
			strSkillName = pc.standard(strSkillName);
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
				'=' || strLowerMessage[intMsgCnt] == ':')intMsgCnt++;
			//�ж���¼��Ϊ�ı�
			if (bool isSqr{ strMsg.substr(intMsgCnt, 2) == "��" }; pc.pTemplet->sInfoList.count(strSkillName) || isSqr) {
				string strVal;
				if (auto pos{ strMsg.find("��",intMsgCnt) }; pos != string::npos) {
					strVal = strMsg.substr(intMsgCnt + 2, pos - intMsgCnt - 2);
					intMsgCnt = pos + 2;
				}
				else {
					strVal = readUntilTab();
				}
				if (pc.set(strSkillName, strVal)) {
					reply(getMsg("strPcTextTooLong"));
					return 1;
				}
				++cntInput;
				continue;
			}
			if (strSkillName == "note") {
				if (pc.setNote(readRest())) {
					reply(getMsg("strPcNoteTooLong"));
					return 1;
				}
				++cntInput;
				break;
			}
			readSkipSpace();
			//�ж���ֵ�޸�
			if ((strLowerMessage[intMsgCnt] == '-' || strLowerMessage[intMsgCnt] == '+')) {
				isDetail = true;
				isModify = true;
				AttrVar& nVal{ pc[strSkillName] };
				RD Mod((nVal.to_int() == 0 ? "" : nVal.to_str()) + readDice());
				if (Mod.Roll()) {
					reply(getMsg("strValueErr"));
					return 1;
				}
				strReply += "\n" + strSkillName + "��" + Mod.FormCompleteString();
				if (Mod.intTotal < -32767) {
					strReply += "��-32767";
					nVal = -32767;
				}
				else if (Mod.intTotal > 32767) {
					strReply += "��32767";
					nVal = 32767;
				}
				else nVal = Mod.intTotal;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] ==
					   '|')intMsgCnt++;
				++cntInput;
				continue;
			}
			string strSkillVal = readDigit();
			if (strSkillName.empty() || strSkillVal.empty() || strSkillVal.length() > 5) {
				boolError = true;
				break;
			}
			int intSkillVal = std::clamp(stoi(strSkillVal), -32767, 32767);
			//¼�봿��ֵ
			pc.set(strSkillName, intSkillVal);
			++cntInput;
			while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
				intMsgCnt++;
		}
		if (boolError) {
			reply(getMsg("strPropErr"));
		}
		else if (isModify) {
			reply(format(getMsg("strStModify"), { strVar["pc"] }) + strReply);
		}
		else if(cntInput){
			strVar["cnt"] = to_string(cntInput);
			reply(getMsg("strSetPropSuccess"));
		}
		else {
			reply(fmt->get_help("st"));
		}
		return 1;
	}
	else if (strLowerMessage.substr(intMsgCnt, 2) == "ti") {
		string strAns = "{pc}�ķ����-��ʱ֢״:\n";
		TempInsane(strAns);
		reply(strAns);
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'w') {
		intMsgCnt++;
		bool boolDetail = false;
		if (strLowerMessage[intMsgCnt] == 'w') {
			intMsgCnt++;
			boolDetail = true;
		}
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h') {
			isHidden = true;
			intMsgCnt += 1;
		}
		readSkipSpace();
		const unsigned int len{ (unsigned int)strMsg.length() };
		if (intMsgCnt == len) {
			reply(fmt->get_help("ww"));
			return 1;
		}
		strVar["nick"] = getName(fromQQ, fromGroup);
		getPCName(*this);
		if (!fromGroup)isHidden = false;
		CharaCard* pc{ PList.count(fromQQ) ? &getPlayer(fromQQ)[fromGroup] : nullptr };
		string strMainDice;
		string& strReason{ strVar["reason"] };
		string strAttr;
		if (pc) {	//���ý�ɫ�����Ի���ʽ
			while (intMsgCnt < len && !isspace(static_cast<unsigned char>(strMsg[intMsgCnt]))) {
				if (isdigit(static_cast<unsigned char>(strMsg[intMsgCnt]))
					|| strMsg[intMsgCnt] == 'a'
					|| strMsg[intMsgCnt] == '+' || strMsg[intMsgCnt] == '-'
					|| strMsg[intMsgCnt] == '*' || strMsg[intMsgCnt] == '/') {
					strMainDice += strMsg[intMsgCnt++];
				}
				else if (strMsg[intMsgCnt] == '=' || strMsg[intMsgCnt] == ':') {
					intMsgCnt++;
				}
				else {
					strAttr = readAttrName();
					strMainDice += pc->getExp(strAttr);
					if (!pc->count("&" + strAttr) && (*pc)[strAttr].type == AttrVar::AttrType::Integer)strMainDice += 'a';
				}
			}
		}
		else {
			strMainDice = readDice(); 	//ww�ı��ʽ�����Ǵ�����
		}
		strReason = readRest();
		int intTurnCnt = 1;
		const int intDefaultDice = get(getUser(fromQQ).intConf, string("Ĭ����"), 100);
		//����.ww[����]#[���ʽ]
		if (size_t pos{ strMainDice.find('#') }; pos != string::npos) {
			string strTurnCnt = strMainDice.substr(0, pos);
			if (strTurnCnt.empty())
				strTurnCnt = "1";
			strMainDice = strMainDice.substr(pos + 1);
			RD rdTurnCnt(strTurnCnt, intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			if (intRdTurnCntRes != 0) {
				if (intRdTurnCntRes == Value_Err) {
					reply(getMsg("strValueErr"));
					return 1;
				}
				if (intRdTurnCntRes == Input_Err) {
					reply(getMsg("strInputErr"));
					return 1;
				}
				if (intRdTurnCntRes == ZeroDice_Err) {
					reply(getMsg("strZeroDiceErr"));
					return 1;
				}
				if (intRdTurnCntRes == ZeroType_Err) {
					reply(getMsg("strZeroTypeErr"));
					return 1;
				}
				if (intRdTurnCntRes == DiceTooBig_Err) {
					reply(getMsg("strDiceTooBigErr"));
					return 1;
				}
				if (intRdTurnCntRes == TypeTooBig_Err) {
					reply(getMsg("strTypeTooBigErr"));
					return 1;
				}
				if (intRdTurnCntRes == AddDiceVal_Err) {
					reply(getMsg("strAddDiceValErr"));
					return 1;
				}
				reply(getMsg("strUnknownErr"));
				return 1;
			}
			if (rdTurnCnt.intTotal > 10) {
				reply(getMsg("strRollTimeExceeded"));
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0) {
				reply(getMsg("strRollTimeErr"));
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strTurnCnt.find('d') != string::npos) {
				string strTurnNotice = strVar["pc"] + "����������: " + rdTurnCnt.FormShortString() + "��";
				replyHidden(strTurnNotice);
			}
		}
		if (strMainDice.empty()) {
			reply(fmt->get_help("ww"));
			return 1;
		}
		string strFirstDice = strMainDice.substr(0, strMainDice.find('+') < strMainDice.find('-')
												 ? strMainDice.find('+')
												 : strMainDice.find('-'));
		strFirstDice = strFirstDice.substr(0, strFirstDice.find('x') < strFirstDice.find('*')
										   ? strFirstDice.find('x')
										   : strFirstDice.find('*'));
		bool boolAdda10 = true;
		for (auto i : strFirstDice) {
			if (!isdigit(static_cast<unsigned char>(i))) {
				boolAdda10 = false;
				break;
			}
		}
		if (boolAdda10)
			strMainDice.insert(strFirstDice.length(), "a10");
		RD rdMainDice(strMainDice, intDefaultDice);

		const int intFirstTimeRes = rdMainDice.Roll();
		if (intFirstTimeRes != 0) {
			if (intFirstTimeRes == Value_Err) {
				reply(getMsg("strValueErr"));
				return 1;
			}
			if (intFirstTimeRes == Input_Err) {
				reply(getMsg("strInputErr"));
				return 1;
			}
			if (intFirstTimeRes == ZeroDice_Err) {
				reply(getMsg("strZeroDiceErr"));
				return 1;
			}
			if (intFirstTimeRes == ZeroType_Err) {
				reply(getMsg("strZeroTypeErr"));
				return 1;
			}
			if (intFirstTimeRes == DiceTooBig_Err) {
				reply(getMsg("strDiceTooBigErr"));
				return 1;
			}
			if (intFirstTimeRes == TypeTooBig_Err) {
				reply(getMsg("strTypeTooBigErr"));
				return 1;
			}
			if (intFirstTimeRes == AddDiceVal_Err) {
				reply(getMsg("strAddDiceValErr"));
				return 1;
			}
			reply(getMsg("strUnknownErr"));
			return 1;
		}
		if (!boolDetail && intTurnCnt != 1) {
			if (strReason.empty())strReply = getMsg("strRollMuiltDice");
			else strReply = getMsg("strRollMuiltDiceReason");
			vector<int> vintExVal;
			strVar["res"] = "{ ";
			while (intTurnCnt--) {
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strVar["res"] += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strVar["res"] = ",";
			}
			strVar["res"] += " }";
			if (!vintExVal.empty()) {
				strVar["res"] += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					strVar["res"] += to_string(*it);
					if (it != vintExVal.cend() - 1)strVar["res"] += ",";
				}
			}
			if (!isHidden) {
				reply();
			}
			else {
				strReply = format(strReply, GlobalMsg, strVar);
				strReply = "��" + printChat(fromChat) + "�� " + strReply;
				AddMsgToQueue(strReply, fromQQ, msgtype::Private);
				for (auto qq : gm->session(fromSession).get_ob()) {
					if (qq != fromQQ) {
						AddMsgToQueue(strReply, qq, msgtype::Private);
					}
				}
			}
		}
		else {
			while (intTurnCnt--) {
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				strVar["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
				if (strReason.empty())
					strReply = format(getMsg("strRollDice"), { strVar["pc"], strVar["res"] });
				else strReply = format(getMsg("strRollDiceReason"), { strVar["pc"], strVar["res"], strReason });
				if (!isHidden) {
					reply();
				}
				else {
					strReply = format(strReply, GlobalMsg, strVar);
					strReply = "��" + printChat(fromChat) + "�� " + strReply;
					AddMsgToQueue(strReply, fromQQ, msgtype::Private);
					for (auto qq : gm->session(fromSession).get_ob()) {
						if (qq != fromQQ) {
							AddMsgToQueue(strReply, qq, msgtype::Private);
						}
					}
				}
			}
		}
		if (isHidden) {
			reply(getMsg("strRollHidden"), { strVar["pc"] });
		}
		return 1;
	}
	else if (strLowerMessage[intMsgCnt] == 'r' || strLowerMessage[intMsgCnt] == 'h') {
		strVar["nick"] = getName(fromQQ, fromGroup);
		getPCName(*this);
		bool isHidden = false;
		if (strLowerMessage[intMsgCnt] == 'h')
			isHidden = true;
		intMsgCnt += 1;
		bool boolDetail = true;
		if (strMsg[intMsgCnt] == 's') {
			boolDetail = false;
			intMsgCnt++;
		}
		if (strLowerMessage[intMsgCnt] == 'h') {
			isHidden = true;
			intMsgCnt += 1;
		}
		if (!fromGroup)isHidden = false;
		while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])))
			intMsgCnt++;
		string strMainDice;
		CharaCard* pc{ PList.count(fromQQ) ? &getPlayer(fromQQ)[fromGroup] : nullptr };
		string& strReason{ strVar["reason"] = strMsg.substr(intMsgCnt) };
		if (strReason.empty()) {
			string key{ "__DefaultDiceExp" };
			if (pc && pc->countExp(strVar[key])) {
				strMainDice = pc->getExp(key);
			}
		}
		if (pc && pc->countExp(strReason)) {
			strMainDice = pc->getExp(strReason);
		}
		else {
			strMainDice = readDice();
			bool isExp = false;
			for (auto ch : strMainDice) {
				if (!isdigit(ch)) {
					isExp = true;
					break;
				}
			}
			if (isExp)strVar["reason"] = readRest();
			else strMainDice.clear();
		}
		int intTurnCnt = 1;
		const int intDefaultDice = (pc && pc->count("__DefaultDice")) 
			? (*pc)["__DefaultDice"].to_int()
			: get(getUser(fromQQ).intConf, string("Ĭ����"), 100);
		if (strMainDice.find('#') != string::npos) {
			strVar["turn"] = strMainDice.substr(0, strMainDice.find('#'));
			if (strVar["turn"].empty())
				strVar["turn"] = "1";
			strMainDice = strMainDice.substr(strMainDice.find('#') + 1);
			RD rdTurnCnt(strVar["turn"], intDefaultDice);
			const int intRdTurnCntRes = rdTurnCnt.Roll();
			switch (intRdTurnCntRes) {
			case 0: break;
			case Value_Err:
				reply(getMsg("strValueErr"));
				return 1;
			case Input_Err:
				reply(getMsg("strInputErr"));
				return 1;
			case ZeroDice_Err:
				reply(getMsg("strZeroDiceErr"));
				return 1;
			case ZeroType_Err:
				reply(getMsg("strZeroTypeErr"));
				return 1;
			case DiceTooBig_Err:
				reply(getMsg("strDiceTooBigErr"));
				return 1;
			case TypeTooBig_Err:
				reply(getMsg("strTypeTooBigErr"));
				return 1;
			case AddDiceVal_Err:
				reply(getMsg("strAddDiceValErr"));
				return 1;
			default:
				reply(getMsg("strUnknownErr"));
				return 1;
			}
			if (rdTurnCnt.intTotal > 10) {
				reply(getMsg("strRollTimeExceeded"));
				return 1;
			}
			if (rdTurnCnt.intTotal <= 0) {
				reply(getMsg("strRollTimeErr"));
				return 1;
			}
			intTurnCnt = rdTurnCnt.intTotal;
			if (strVar["turn"].find('d') != string::npos) {
				strVar["turn"] = rdTurnCnt.FormShortString();
				if (!isHidden) {
					reply(getMsg("strRollTurn"), { strVar["pc"], strVar["turn"] });
				}
				else {
					replyHidden(getMsg("strRollTurn"));
				}
			}
		}
		if (strMainDice.empty() && pc && pc->countExp(strReason)) {
			strMainDice = pc->getExp(strReason);
		}
		RD rdMainDice(strMainDice, intDefaultDice);
		const int intFirstTimeRes = rdMainDice.Roll();
		switch (intFirstTimeRes) {
		case 0: break;
		case Value_Err:
			reply(getMsg("strValueErr"));
			return 1;
		case Input_Err:
			reply(getMsg("strInputErr"));
			return 1;
		case ZeroDice_Err:
			reply(getMsg("strZeroDiceErr"));
			return 1;
		case ZeroType_Err:
			reply(getMsg("strZeroTypeErr"));
			return 1;
		case DiceTooBig_Err:
			reply(getMsg("strDiceTooBigErr"));
			return 1;
		case TypeTooBig_Err:
			reply(getMsg("strTypeTooBigErr"));
			return 1;
		case AddDiceVal_Err:
			reply(getMsg("strAddDiceValErr"));
			return 1;
		default:
			reply(getMsg("strUnknownErr"));
			return 1;
		}
		strVar["dice_exp"] = rdMainDice.strDice;
		//��ͳ����Ĭ����һ�µ�����
		bool isStatic{ intDefaultDice <= 100 && pc && rdMainDice.strDice == ("D" + to_string(intDefaultDice)) };
		string strType = (intTurnCnt != 1
						  ? (strVar["reason"].empty() ? "strRollMultiDice" : "strRollMultiDiceReason")
						  : (strVar["reason"].empty() ? "strRollDice" : "strRollDiceReason"));
		if (!boolDetail && intTurnCnt != 1) {
			strReply = getMsg(strType);
			vector<int> vintExVal;
			strVar["res"] = "{ ";
			while (intTurnCnt--) {
				// �˴�����ֵ����
				// ReSharper disable once CppExpressionWithoutSideEffects
				rdMainDice.Roll();
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				strVar["res"] += to_string(rdMainDice.intTotal);
				if (intTurnCnt != 0)
					strVar["res"] += ",";
				if ((rdMainDice.strDice == "D100" || rdMainDice.strDice == "1D100") && (rdMainDice.intTotal <= 5 ||
																						rdMainDice.intTotal >= 96))
					vintExVal.push_back(rdMainDice.intTotal);
			}
			strVar["res"] += " }";
			if (!vintExVal.empty()) {
				strVar["res"] += ",��ֵ: ";
				for (auto it = vintExVal.cbegin(); it != vintExVal.cend(); ++it) {
					strVar["res"] += to_string(*it);
					if (it != vintExVal.cend() - 1)
						strVar["res"] += ",";
				}
			}
			if (!isHidden) {
				reply();
			}
			else {
				replyHidden(strReply);
			}
		}
		else {
			ResList dices;
			if (intTurnCnt > 1) {
				while (intTurnCnt--) {
					rdMainDice.Roll();
					if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
					string strForm = to_string(rdMainDice.intTotal);
					if (boolDetail) {
						string strCombined = rdMainDice.FormStringCombined();
						string strSeparate = rdMainDice.FormStringSeparate();
						if (strCombined != strForm)strForm = strCombined + "=" + strForm;
						if (strSeparate != strMainDice && strSeparate != strCombined)strForm = strSeparate + "=" +
							strForm;
					}
					dices << strForm;
				}
				strVar["res"] = dices.dot(", ").line(7).show();
			}
			else {
				if (isStatic)pc->cntRollStat(rdMainDice.intTotal, intDefaultDice);
				strVar["res"] = boolDetail ? rdMainDice.FormCompleteString() : rdMainDice.FormShortString();
			}
			strReply = format(getMsg(strType), { strVar["pc"], strVar["res"], strVar["reason"] });
			if (!isHidden) {
				reply();
			}
			else {
				replyHidden(strReply);
			}
		}
		if (isHidden) {
			reply(getMsg("strRollHidden"), { strVar["pc"] });
		}
		return 1;
	}
	return 0;
}

//�ж��Ƿ���Ӧ
bool FromMsg::DiceFilter()
{
	while (isspace(static_cast<unsigned char>(strMsg[0])))
		strMsg.erase(strMsg.begin());
	init(strMsg);
	bool isOtherCalled = false;
	string strAt = CQ_AT + to_string(DD::getLoginQQ()) + "]";
	while (strMsg.find(CQ_AT) == 0)
	{
		if (strMsg.find(strAt) == 0)
		{
			strMsg = strMsg.substr(strAt.length());
			isCalled = true;
		}
		else if (strMsg.find("[CQ:at,qq=all]") == 0) 
		{
			strMsg = strMsg.substr(14);
			isCalled = true;
		}
		else if (strMsg.find(']') != string::npos)
		{
			strMsg = strMsg.substr(strMsg.find(']') + 1);
			isOtherCalled = true;
		}
		while (isspace(static_cast<unsigned char>(strMsg[0])))
			strMsg.erase(strMsg.begin());
	}
	init2(strMsg);
	strLowerMessage = strMsg;
	std::transform(strLowerMessage.begin(), strLowerMessage.end(), strLowerMessage.begin(),
				   [](unsigned char c) { return tolower(c); });
	trusted = trustedQQ(fromQQ);
	fwdMsg();
	if (isOtherCalled && !isCalled)return false;
	if (fromChat.second == msgtype::Private) isCalled = true;
	isDisabled = ((console["DisabledGlobal"] && trusted < 4) || groupset(fromGroup, "Э����Ч") > 0);
	if (BasicOrder()) 
	{
		if (isAns) {
			if (!isVirtual) {
				AddFrq(fromQQ, fromTime, fromChat, strMsg);
				getUser(fromQQ).update(fromTime);
				if (fromChat.second != msgtype::Private)chat(fromGroup).update(fromTime);
			}
			else {
				AddFrq(0, fromTime, fromChat, strMsg);
			}
		}
		return 1;
	}
	if (fromChat.second == msgtype::Group && ((console["CheckGroupLicense"] > 0 && pGrp->isset("δ���"))
											  || (console["CheckGroupLicense"] == 2 && !pGrp->isset("���ʹ��")) 
											  || blacklist->get_group_danger(fromGroup))) {
		isDisabled = true;
	}
	if (blacklist->get_qq_danger(fromQQ))isDisabled = true;
	if (!isDisabled && (isCalled || !pGrp->isset("ͣ��ָ��"))) {
		if (fmt->listen_order(this) || InnerOrder()) {
			if (!isVirtual) {
				AddFrq(fromQQ, fromTime, fromChat, strMsg);
				getUser(fromQQ).update(fromTime);
				if (fromChat.second != msgtype::Private)chat(fromGroup).update(fromTime);
			}
			else {
				AddFrq(0, fromTime, fromChat, strMsg);
			}
			return true;
		}
	}
	if (!isDisabled && fmt->listen_reply(this))return true;
	if (isDisabled)return console["DisabledBlock"];
	return false;
}
bool FromMsg::WordCensor() {
	//����С��4���û��������дʼ��
	if (trusted < 4) {
		vector<string>sens_words;
		switch (int danger = censor.search(strMsg, sens_words) - 1) {
		case 3:
			if (trusted < danger++) {
				console.log("����:" + printQQ(fromQQ) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 0b1000,
							printTTime(fromTime));
				reply(getMsg("strCensorDanger"));
				return 1;
			}
		case 2:
			if (trusted < danger++) {
				console.log("����:" + printQQ(fromQQ) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 0b10,
							printTTime(fromTime));
				reply(getMsg("strCensorWarning"));
				break;
			}
		case 1:
			if (trusted < danger++) {
				console.log("����:" + printQQ(fromQQ) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 0b10,
							printTTime(fromTime));
				reply(getMsg("strCensorCaution"));
				break;
			}
		case 0:
			console.log("����:" + printQQ(fromQQ) + "��" + getMsg("strSelfName") + "�����˺����д�ָ��:\n" + strMsg, 1,
						printTTime(fromTime));
			break;
		default:
			break;
		}
	}
	return false;
}

void FromMsg::virtualCall() {
	isVirtual = true;
	isCalled = true;
	DiceFilter();
}

int FromMsg::getGroupAuth(long long group) {
	if (trusted > 0)return trusted;
	if (ChatList.count(group)) {
		return DD::isGroupAdmin(group, fromQQ, true) ? 0 : -1;
	}
	return -2;
}
void FromMsg::readSkipColon() {
	readSkipSpace();
	while (intMsgCnt < strMsg.length() && (strMsg[intMsgCnt] == ':' || strMsg[intMsgCnt] == '='))intMsgCnt++;
}

int FromMsg::readNum(int& num)
{
	string strNum;
	while (intMsgCnt < strMsg.length() && !isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])) && strMsg[intMsgCnt] != '-')intMsgCnt++;
	if (strMsg[intMsgCnt] == '-')
	{
		strNum += '-';
		intMsgCnt++;
	}
	if (intMsgCnt >= strMsg.length())return -1;
	while (intMsgCnt < strMsg.length() && isdigit(static_cast<unsigned char>(strMsg[intMsgCnt])))
	{
		strNum += strMsg[intMsgCnt];
		intMsgCnt++;
	}
	if (strNum.length() > 9)return -2;
	if (strNum.empty() || strNum == "-")return -3;
	num = stoi(strNum);
	return 0;
}

int FromMsg::readChat(chatType& ct, bool isReroll)
{
	const int intFormor = intMsgCnt;
	if (const string strT = readPara(); strT == "me")
	{
		ct = {fromQQ, msgtype::Private};
		return 0;
	}
	else if (strT == "this")
	{
		ct = fromChat;
		return 0;
	}
	else if (strT == "qq") 
	{
		ct.second = msgtype::Private;
	}
	else if (strT == "group")
	{
		ct.second = msgtype::Group;
	}
	else if (strT == "discuss")
	{
		ct.second = msgtype::Discuss;
	}
	else
	{
		if (isReroll)intMsgCnt = intFormor;
		return -1;
	}
	if (const long long llID = readID(); llID)
	{
		ct.first = llID;
		return 0;
	}
	if (isReroll)intMsgCnt = intFormor;
	return -2;
}

string FromMsg::readItem()
{
	while (isspace(static_cast<unsigned char>(strMsg[intMsgCnt])) || strMsg[intMsgCnt] == '|')intMsgCnt++;
	unsigned int intBegin{ intMsgCnt };
	unsigned int intEnd{ intMsgCnt };
	do{
		if (!isspace(static_cast<unsigned char>(strMsg[++intMsgCnt])))intEnd = intMsgCnt;
	} while (strMsg[intMsgCnt] != '|' && intMsgCnt != strMsg.length());
	return strMsg.substr(intBegin, intEnd - intBegin);
}
void FromMsg::readItems(vector<string>& vItem) {
	string strItem;
	while (!(strItem = readItem()).empty()) {
		vItem.push_back(strItem);
	}
}