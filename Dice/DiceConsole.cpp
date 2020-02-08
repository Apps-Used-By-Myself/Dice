/*
 * _______   ________  ________  ________  __
 * |  __ \  |__  __| |  _____| |  _____| | |
 * | | | |   | |   | |    | |_____  | |
 * | | | |   | |   | |    |  _____| |__|
 * | |__| |  __| |__  | |_____  | |_____  __
 * |_______/  |________| |________| |________| |__|
 *
 * Dice! QQ Dice Robot for TRPG
 * Copyright (C) 2018-2019 w4123���
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
#pragma once
#include <ctime>
#include <queue>
#include <mutex>
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "ManagerSystem.h"
#include "DiceNetwork.h"
#include "DiceCloud.h"
#include "Jsonio.h"

using namespace std;
using namespace CQ;

const std::map<std::string, int>Console::intDefault{
{"DisabledGlobal",0},{"DisabledBlock",0},{"DisabledListenAt",1},
{"DisabledMe",1},{"DisabledJrrp",0},{"DisabledDeck",1},{"DisabledDraw",0},{"DisabledSend",0},
{"Private",0},{"CheckGroupLicense",0},{"LeaveDiscuss",0},
{"ListenGroupRequest",1},{"ListenGroupAdd",1},
{"ListenFriendRequest",1},{"ListenFriendAdd",1},{"AllowStranger",0},
{"AutoClearBlack",1},{"LeaveBlackQQ",0},{"LeaveBlackGroup",1},
{"ListenGroupKick",1},{"ListenGroupBan",1},{"ListenSpam",1},
{"BannedLeave",0},{"BannedBanInviter",0},
{"KickedBanInviter",0},
{"BelieveDiceList",0},{"CloudVisible",0},
{"SystemAlarmCPU",90},{"SystemAlarmRAM",90},
{"SendIntervalIdle",500},{"SendIntervalBusy",100}
};
const enumap<string> Console::mClockEvent { "off", "on", "save", "clear" };
int Console::setClock(Clock c, ClockEvent e) {
	if (c.first > 23 || c.second > 59)return -1;
	if (int(e) > 3)return -2;
	mWorkClock.emplace(c, e);
	save();
	return 0;
}
int Console::rmClock(Clock c, ClockEvent e) {
	if (auto it = match(mWorkClock, c, e); it != mWorkClock.end()) {
		mWorkClock.erase(it);
		save();
		return 0;
	}
	else return -1;
}
ResList Console::listClock()const {
	ResList list;
	string strClock;
	for (auto& [clock, eve] : mWorkClock) {
		strClock = printClock(clock);
		switch (eve) {
		case ClockEvent::on:
			strClock += " ��ʱ����";
			break;
		case ClockEvent::off:
			strClock += " ��ʱ�ر�";
			break;
		case ClockEvent::save:
			strClock += " ��ʱ����";
			break;
		case ClockEvent::clear:
			strClock += " ��ʱ��Ⱥ";
			break;
		default:break;
		}
		list << strClock;
	}
	return list;
}
ResList Console::listNotice()const {
	ResList list;
	for (auto& [ct,lv] : NoticeList) {
		list << printChat(ct) + " " + to_binary(lv);
	}
	return list;
}
int Console::showNotice(chatType ct)const {
	if (auto it = NoticeList.find(ct); it != NoticeList.end())return it->second;
	else return 0;
}
void Console::addNotice(chatType ct, int lv) { NoticeList[ct] |= lv; saveNotice(); }
void Console::redNotice(chatType ct, int lv) { NoticeList[ct] &= (~lv); saveNotice(); }
void Console::setNotice(chatType ct, int lv){
	NoticeList[ct] = lv;
	saveNotice();
}
void Console::rmNotice(chatType ct) {
	NoticeList.erase(ct);
	saveNotice();
}
int Console::log(std::string strMsg, int note_lv, string strTime) {
	ofstream fout(string("DiceData\\audit\\log") + to_string(DiceMaid) + "_" + printDate() + ".txt", ios::out | ios::app);
	fout << strTime << "\t" << note_lv << "\t" << printLine(strMsg) << std::endl;
	int Cnt = 0;
	string note = strTime.empty() ? strMsg : (strTime + " " + strMsg);
	//fout << "list:" << NoticeList.size() << endl;
	for (auto& [ct, level] : NoticeList) {
		int i = level & note_lv;
		//fout << ct << "\t" << i << std::endl;
		if (!(level & note_lv))continue;
		AddMsgToQueue(note, ct);
		Cnt++;
	}
	//fout << "send:" << Cnt << std::endl;
	fout.close();
	return Cnt;
} 
void Console::newMaster(long long qq) {
	masterQQ = qq; 
	getUser(qq).trust(5); 
	setNotice({ qq,CQ::Private }, 0b111111); 
	save(); 
	AddMsgToQueue(getMsg("strNewMaster"), qq); 
}
void Console::reset() {
	intConf.clear();
	mWorkClock.clear();
	NoticeList.clear();
}
void  Console::loadNotice() {
	if (loadFile("DiceData\\conf\\NoticeList.txt", NoticeList) < 1) {
		std::set<chatType>sChat;
		if (loadFile((string)getAppDirectory() + "MonitorList.RDconf", sChat) > 0)
			for (auto& it : sChat) {
				console.setNotice(it, 0b100000);
			}
		sChat.clear();
		if (loadFile("DiceData\\conf\\RecorderList.RDconf", sChat) > 0)
			for (auto& it : sChat) {
				console.setNotice(it, 0b11011);
			}
		console.setNotice({ 863062599, Group }, 0b100000);
		console.setNotice({ 192499947, Group }, 0b100000);
		console.setNotice({ 754494359, Group }, 0b100000);
		for (auto& [ct, lv] : NoticeList) {
			if (ct.second) {
				chat(ct.first).set("���ʹ��").set("����").set("���");
			}
		}
	}
}
void Console::saveNotice() {
	saveFile("DiceData\\conf\\NoticeList.txt", NoticeList);
}
Console console{"DiceData\\conf\\Console.xml"};

//DiceModManager modules{};

//�����б�
std::map<long long, long long> mDiceList;
	//���Ի����
	std::map<std::string, std::string> PersonalMsg;
	//������Ⱥ������������
	std::set<long long> BlackGroup;
	//�������û�������������
	std::set<long long> BlackQQ;

//��������
mutex blackMarkMutex;
std::set<std::string>BlackMarks;
std::map<long long, BlackMark>mBlackQQMark;
std::map<long long, BlackMark>mBlackGroupMark;
BlackMark readMark(nlohmann::json j) {
	BlackMark mark;
	if (j.count("inviterQQ"))mark.set("inviterQQ", j["inviterQQ"].get<long long>());
	if (j.count("ownerQQ"))mark.set("ownerQQ", j["ownerQQ"].get<long long>());
	if (j.count("fromQQ"))mark.set("fromQQ", j["fromQQ"].get<long long>());
	if (j.count("fromGroup"))mark.set("fromGroup", j["fromGroup"].get<long long>());
	if (j.count("DiceMaid"))mark.set("DiceMaid", j["DiceMaid"].get<long long>());
	if (j.count("masterQQ"))mark.set("masterQQ", j["masterQQ"].get<long long>());
	if (j.count("type"))mark.set("type", UTF8toGBK(j["type"].get<string>()));
	if (j.count("time"))mark.set("time", UTF8toGBK(j["time"].get<string>()));
	if (j.count("note"))mark.set("note", UTF8toGBK(j["note"].get<string>()));
	return mark;
}
void loadBlackMark(string strPath) {
	ifstream fin(strPath);
	if (fin) {
		nlohmann::json j;
		BlackMark m;
		fin >> j;
		for (auto it : j) {
			m = readMark(it);
			if (m.count("fromGroup")) {
				mBlackGroupMark[m.fromID] = m;
			}
			else {
				mBlackQQMark[m.fromID] = m;
			}
		}
		fin.close();
	}
}
void saveBlackMark(string strPath) {
	ofstream fout(strPath);
	string sout;
	for (auto it : mBlackGroupMark) {
		sout += it.second.getJson() + ',';
	}
	for (auto it : mBlackQQMark) {
		sout += it.second.getJson() + ',';
	}
	if (sout.empty())sout = ']';
	else sout[sout.length() - 1] = ']';
	fout << "[" << GBKtoUTF8(sout);
	fout.close();
}
//��������ʱ��
long long llStartTime = clock();
	//��ǰʱ��
	SYSTEMTIME stNow = { 0 };
	SYSTEMTIME stTmp = { 0 };
std::string printSTNow() {
	GetLocalTime(&stNow);
	return printSTime(stNow);
}
std::string printDate() {
	return to_string(stNow.wYear) + "-" + (stNow.wMonth < 10 ? "0" : "") + to_string(stNow.wMonth) + "-" + (stNow.wDay < 10 ? "0" : "") + to_string(stNow.wDay);
}
std::string printDate(time_t tt) {
	tm t;
	if (!tt || localtime_s(&t, &tt))return "????-??-??";
	return to_string(t.tm_year+1900) + "-" + to_string(t.tm_mon+1) + "-" + to_string(t.tm_mday);
}
	//�ϰ�ʱ��
	std::pair<int, int> ClockToWork = { 24,0 };
	//�°�ʱ��
	std::pair<int, int> ClockOffWork = { 24,0 };

	string printClock(std::pair<int, int> clock) {
		string strClock = to_string(clock.first);
		strClock += ":";
		if (clock.second < 10)strClock += "0";
		strClock += to_string(clock.second);
		return strClock;
	}
std::string printSTime(SYSTEMTIME st){
	return to_string(st.wYear) + "-" + (st.wMonth < 10 ? "0" : "") + to_string(st.wMonth) + "-" + (st.wDay < 10 ? "0" : "") + to_string(st.wDay) + " " + (st.wHour < 10 ? "0" : "") + to_string(st.wHour) + ":" + (st.wMinute < 10 ? "0" : "") + to_string(st.wMinute) + ":" + (st.wSecond < 10 ? "0" : "") + to_string(st.wSecond);
}
	//��ӡ�û��ǳ�QQ
	string printQQ(long long llqq) {
		string nick = getStrangerInfo(llqq).nick;
		while (nick.find(" ") != string::npos)nick.erase(nick.begin() + nick.find(" "), nick.begin() + nick.find(" ") + strlen(" "));
		while (nick.find(" ") != string::npos)nick.erase(nick.begin() + nick.find(" "), nick.begin() + nick.find(" ") + strlen(" "));
		return getStrangerInfo(llqq).nick + "(" + to_string(llqq) + ")";
	}
	//��ӡQQȺ��
	string printGroup(long long llgroup) {
		if (!llgroup)return"˽��";
		if (getGroupList().count(llgroup))return getGroupList()[llgroup] + "(" + to_string(llgroup) + ")";
		return "Ⱥ��(" + to_string(llgroup) + ")";
	}
	//��ӡ���촰��
	string printChat(chatType ct) {
		switch (ct.second)
		{
		case Private:
			return printQQ(ct.first);
		case Group:
			return printGroup(ct.first);
		case Discuss:
			return "������(" + to_string(ct.first) + ")";
		default:
			break;
		}
		return "";
	}
//��ȡ�����б�
void getDiceList() {
	std::string list;
	if (!Network::GET("shiki.stringempty.xyz", "/DiceList/", 80, list))
	{
		console.log(("��ȡ�����б�ʱ��������: \n" + list).c_str(), 1, printSTNow());
		return;
	}
	readJson(list, mDiceList);
}
int isReliable(long long QQID) {
	if (trustedQQ(QQID) > 2)return trustedQQ(QQID);
	if (mDiceList.count(QQID)) {
		if (trustedQQ(mDiceList[QQID]) > 2)return 2;
		if (console["BelieveDiceList"] || mDiceList[QQID] == QQID)return 1;
	}
	if(BlackQQ.count(QQID))return -1;
	return 0;
}

//�����û����Ѳ�Ⱥ
void checkBlackQQ(BlackMark &mark) {
	long long llQQ = mark.fromID;
	ResList list;
	string strNotice;
	for (auto &[id,grp] : ChatList) {
		if (grp.isset("����") || grp.isset("����") || !grp.isGroup)continue;
		if (getGroupMemberInfo(id, llQQ).QQID == llQQ) {
			strNotice = printGroup(id); 
			if (grp.isset("���")) {
				if (mark.isVal("DiceMaid", console.DiceMaid))sendGroupMsg(id, mark.getWarning());
				strNotice += "Ⱥ���";
			}
			else if (getGroupMemberInfo(id, llQQ).permissions < getGroupMemberInfo(id, getLoginQQ()).permissions) {
				if (mark.isVal("DiceMaid", console.DiceMaid))AddMsgToQueue(mark.getWarning(), id, Group);
				strNotice += "�Է�ȺȨ�޽ϵ�";
			}
			else if (getGroupMemberInfo(id, llQQ).permissions > getGroupMemberInfo(id, getLoginQQ()).permissions) {
				sendGroupMsg(id,mark.getWarning());
				grp.leave("������������������Ա" + printQQ(llQQ) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ");
				strNotice += "�Է�ȺȨ�޽ϸߣ�����Ⱥ";
				this_thread::sleep_for(1s);
			}
			else if (grp.isset("����")) {
				if(mark.isVal("DiceMaid", console.DiceMaid))AddMsgToQueue(mark.getWarning(), id, Group);
				strNotice += "Ⱥ����";
			}
			else if (console["LeaveBlackQQ"]) {
				sendGroupMsg(id, mark.getWarning());
				grp.leave("����������������Ա" + printQQ(llQQ) + "��ͬ��ȺȨ�ޣ�\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ");
				strNotice += "����Ⱥ";
				this_thread::sleep_for(1s);
			}
			else if (mark.isVal("DiceMaid", console.DiceMaid))AddMsgToQueue(mark.getWarning(), id, Group);
			list << strNotice;
		}
	}
	if (!list.empty()) {
		strNotice = "�������" + printQQ(llQQ) + "��ͬȺ��" + to_string(list.size()) + "����" + list.show();
		console.log(strNotice, 0b100, printSTNow());
	}
}
//�����û�
bool addBlackQQ(BlackMark mark) {
	long long llQQ = mark.fromID;
	if (llQQ == console.DiceMaid || trustedQQ(llQQ) > 3)return 0;
	if (trustedQQ(llQQ))getUser(llQQ).trust(0);
	if (BlackQQ.count(llQQ))return 0;
	BlackQQ.insert(llQQ);
	if(UserList.count(llQQ))
		mark.count("note") ? AddMsgToQueue(format(GlobalMsg["strBlackQQAddNoticeReason"], GlobalMsg, { {"0",mark.strMap["note"]},{"reason",mark.strMap["note"]},{"nick",getName(llQQ)} }), llQQ)
			: AddMsgToQueue(format(GlobalMsg["strBlackQQAddNotice"], GlobalMsg, { {"nick",getName(llQQ)} }), llQQ);
	if (console["AutoClearBlack"])checkBlackQQ(mark);
	mark.strWarning.clear();
	if (!mBlackQQMark.count(llQQ) || mBlackQQMark[llQQ].isErased()) {
		lock_guard<std::mutex> lock_queue(blackMarkMutex);
		mBlackQQMark[llQQ] = mark;
		saveBlackMark(string(getAppDirectory()) + "BlackMarks.json");
	}
	return 1;
}
bool addBlackGroup(const BlackMark &mark) {
	if (!mark.count("fromGroup"))return 0;
	long long llGroup = mark.llMap.find("fromGroup")->second;
	if (groupset(llGroup, "���") > 0)return 0;
	if (groupset(llGroup, "���ʹ��") > 0)chat(llGroup).reset("���ʹ��").reset("����");
	if (BlackGroup.count(llGroup))return 0;
	BlackGroup.insert(llGroup);
	if (ChatList.count(llGroup) && console["LeaveBlackGroup"]) {
		chat(llGroup).leave(mark.strWarning);
	}
	if (!mBlackGroupMark.count(llGroup) || mBlackGroupMark[llGroup].isErased()) {
		lock_guard<std::mutex> lock_queue(blackMarkMutex);
		mBlackGroupMark[llGroup] = BlackMark(mark, "fromGroup");
		saveBlackMark(string(getAppDirectory()) + "BlackMarks.json");
	}
	return 1;
}
bool rmBlackQQ(long long llQQ, long long operateQQ) {
	bool flag = false;
	if (BlackQQ.count(llQQ)) {
		BlackQQ.erase(llQQ);
		if(UserList.count(llQQ))AddMsgToQueue(format(GlobalMsg["strBlackQQDelNotice"], GlobalMsg, { {"nick",getName(llQQ)} }), llQQ);
		console.log("�ѽ�" + printQQ(llQQ) + "�Ƴ�" + GlobalMsg["strSelfName"] + "���û���������", 1, printSTNow());
		flag = true;
	}
	if (mBlackQQMark.count(llQQ) && !mBlackQQMark[llQQ].isErased()) {
		lock_guard<std::mutex> lock_queue(blackMarkMutex);
		mBlackQQMark[llQQ].erase();
		saveBlackMark(string(getAppDirectory()) + "BlackMarks.json");
		flag = true;
	}
	return flag;
}
bool rmBlackGroup(long long llGroup, long long operateQQ) {
	bool flag = false;
	if (BlackGroup.count(llGroup)) {
		BlackGroup.erase(llGroup);
		flag = true;
		console.log("�ѽ�" + printGroup(llGroup) + "�Ƴ�" + GlobalMsg["strSelfName"] + "��Ⱥ��������", 1, printSTNow());
	}
	if (mBlackGroupMark.count(llGroup) && !mBlackGroupMark[llGroup].isErased()) {
		lock_guard<std::mutex> lock_queue(blackMarkMutex);
		mBlackGroupMark[llGroup].erase();
		saveBlackMark(string(getAppDirectory()) + "BlackMarks.json");
		flag = true;
	}
	return flag;
}
// warning�������
std::queue<fromMsg> warningQueue;
// ��Ϣ���Ͷ�����
mutex warningMutex;
void AddWarning(const string &msg, long long DiceQQ, long long fromGroup)
{
	lock_guard<std::mutex> lock_queue(warningMutex);
	warningQueue.emplace(msg, DiceQQ, fromGroup);
}
bool setQQWarning(BlackMark &mark_full, const char* strType, long long fromQQ) {
	long long blackQQ = mark_full.llMap[strType];
	if (mark_full.isErased()) {
		if (!mBlackQQMark.count(blackQQ) || mBlackQQMark[blackQQ] == mark_full) {
			return rmBlackQQ(blackQQ, fromQQ);
		}
	}
	else {
		if (addBlackQQ(BlackMark(mark_full, strType))) {
			console.log(printQQ(fromQQ) + "��֪ͨ" + GlobalMsg["strSelfName"] + "��" + printQQ(blackQQ) + "�����û���������", 3, printSTNow());
			return true;
		}
	}
	return false;
}
bool setGroupWarning(const BlackMark &mark_full, long long fromQQ) {
	long long blackGroup = mark_full.llMap.find("fromGroup")->second;
	if (mark_full.isErased()) {
		if (!mBlackGroupMark.count(blackGroup) || mBlackGroupMark[blackGroup] == mark_full){
			return rmBlackGroup(blackGroup, fromQQ);
		}
	}
	else {
		if (addBlackGroup(mark_full)) {
			console.log("��֪ͨ" + GlobalMsg["strSelfName"] + "��" + printGroup(blackGroup) + "����Ⱥ��������", 3, printSTNow());
			return true;
		}
	}
	return false;
}
void warningHandler() {
	fromMsg warning;
	bool isAns = false;
	while (Enabled){
		if (isAns) {
			console.log(getName(warning.fromQQ) + "��֪ͨ" + GlobalMsg["strSelfName"] + ":\n!warning" + warning.strMsg, 1, printSTNow());
			isAns = false;
		}
		if (!warningQueue.empty()) {
			{
				lock_guard<std::mutex> lock_queue(warningMutex);
				warning = warningQueue.front();
				warningQueue.pop();
			}
			if (!warning.strMsg.empty()) {
				nlohmann::json jInfo;
				try {
					jInfo = nlohmann::json::parse(GBKtoUTF8(warning.strMsg));
				}
				catch (...) {
					continue;
				}
				BlackMark mark = readMark(jInfo);
				if (mark.strMap["type"] == "spam" && !console["ListenSpam"] || mark.strMap["type"] == "ban" && !console["ListenGroupBan"] || mark.strMap["type"] == "kick" && !console["ListenGroupKick"])continue;
				mark.strWarning = "!warning" + warning.strMsg;
				int intLevel = isReliable(warning.fromQQ);
				if (intLevel > 0) {
					if (intLevel < 2 && !mark.hasType()) {
						isAns = true;
						continue;
					}
					else if (intLevel < 4 && mark.isVal("type", "local")) {
						isAns = true;
						continue;
					}
				}
				else if (intLevel == 0) {
					if (mark.isNoteEmpty()) { continue; }
					int res = Cloud::checkWarning(mark.getData());
					if (!mark.isErased() && res < 1)continue;
					if (mark.isErased() && res > -1 && !mark.isVal("DiceMaid", warning.fromQQ) && !mark.isVal("masterQQ", warning.fromQQ))continue;
				}
				else continue;
				if (mark.count("fromGroup")) {
					isAns |= setGroupWarning(mark, warning.fromQQ);
				}
				if (mark.count("fromQQ")) {
					isAns |= setQQWarning(mark, "fromQQ", warning.fromQQ);
				}
				if (intLevel == 0 && !mark.isVal("DiceMaid", warning.fromQQ))continue;
				if (mark.count("inviterQQ") && ((mark.strMap["type"] == "kick" && console["KickedBanInviter"]) || (mark.strMap["type"] == "ban" && console["BannedBanInviter"]) || mark.strMap["type"] == "erase")) {
					isAns |= setQQWarning(mark, "inviterQQ", warning.fromQQ);
				}
				if (mark.count("ownerQQ") && ((mark.strMap["type"] == "ban" && console["BannedBanOwner"]) || mark.strMap["type"] == "erase")) {
					isAns |= setQQWarning(mark, "ownerQQ", warning.fromQQ);
				}
			}
			else
			{
				this_thread::sleep_for(100ms);
			}
		}
		else std::this_thread::sleep_for(200ms);
	}
}

bool operator==(const SYSTEMTIME& st, const Console::Clock clock) {
	return st.wHour == clock.first && st.wHour == clock.second;
}
bool operator<(const Console::Clock clock, const SYSTEMTIME& st) {
	return st.wHour == clock.first && st.wHour == clock.second;
}
//���׼�ʱ��
	void ConsoleTimer() {
		Console::Clock clockNow{ stNow.wHour,stNow.wMinute };
		long long perLastCPU = 0;
		long long perLastRAM = 0;
		while (Enabled) {
			GetLocalTime(&stNow);
			//����ʱ��䶯
			if (stTmp.wMinute != stNow.wMinute) {
				stTmp = stNow;
				clockNow = { stNow.wHour,stNow.wMinute };
				for (auto &[clock,eve_type] : multi_range(console.mWorkClock, clockNow)) {
					switch (eve_type) {
					case ClockEvent::on:
						if (console["DisabledGlobal"]) {
							console.set("DisabledGlobal", 0);
							console.log(getMsg("strClockToWork"), 0b10000, "");
						}
						break;
					case ClockEvent::off:
						if (!console["DisabledGlobal"]) {
							console.set("DisabledGlobal", 1);
							console.log(getMsg("strClockOffWork"), 0b10000, "");
						}
						break;
					case ClockEvent::save:
						dataBackUp();
						console.log(GlobalMsg["strSelfName"] + "��ʱ������ɡ�", 1, printSTime(stTmp));
						break;
					case ClockEvent::clear:
						if (console && console["AutoClearBlack"] && clearGroup("black"))
							console.log(GlobalMsg["strSelfName"] + "��ʱ��Ⱥ��ɡ�", 1, printSTNow());
						break;
					default:break;
					}
				}
				//�����¼�
				if (stNow.wMinute % 30 == 0) {
					if (console["SystemAlarmCPU"]) {
						long long perCPU = getWinCpuUsage();
						if (perCPU > console["SystemAlarmCPU"] && perCPU > perLastCPU)console.log("���棺" + GlobalMsg["strSelfName"] + "����ϵͳCPUռ�ô�" + to_string(perCPU) + "%", 0b1001, printSTime(stNow));
						else if (perLastCPU > console["SystemAlarmCPU"] && perCPU < console["SystemAlarmCPU"])console.log("���ѣ�" + GlobalMsg["strSelfName"] + "����ϵͳCPUռ�ý���" + to_string(perCPU) + "%", 0b11, printSTime(stNow));
						perLastCPU = perCPU;
					}
					if (console["SystemAlarmRAM"]) {
						long long perRAM = getRamPort();
						if(perRAM > console["SystemAlarmRAM"])console.log("���棺" + GlobalMsg["strSelfName"] + "����ϵͳ�ڴ�ռ�ô�" + to_string(perRAM) + "%", 0b1001, printSTime(stNow));
						else if (perLastRAM > console["SystemAlarmRAM"] && perRAM < console["SystemAlarmRAM"])console.log("���ѣ�" + GlobalMsg["strSelfName"] + "����ϵͳ�ڴ�ռ�ý���" + to_string(perRAM) + "%", 0b11, printSTime(stNow));
						perLastRAM = perRAM;
					}
				}
			}
			this_thread::sleep_for(100ms);
		}
	}

	//һ������
	int clearGroup(string strPara,long long fromQQ) {
		int intCnt = 0;
		string strReply;
		ResList res;
		std::map<string, string>strVar;
		if (strPara == "unpower" || strPara.empty()) {
			for (auto& [id, grp] : ChatList) {
				if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����"))continue;
				if (grp.isGroup && getGroupMemberInfo(id, console.DiceMaid).permissions == 1) {
					res << printGroup(id);
					grp.leave(getMsg("strLeaveNoPower"));
					intCnt++;
					this_thread::sleep_for(3s);
				}
			}
			strReply = GlobalMsg["strSelfName"] + "ɸ����ȺȨ��Ⱥ��" + to_string(intCnt) + "��:" + res.show();
			console.log(strReply, 3, printSTNow());
		}
		else if (isdigit(static_cast<unsigned char>(strPara[0]))) {
			int intDayLim = stoi(strPara);
			string strDayLim = to_string(intDayLim);
			time_t tNow = time(NULL);
			for (auto& [id, grp] : ChatList) {
				if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����"))continue;
				time_t tLast = grp.tLastMsg;
				if (int tLMT; grp.isGroup && (tLMT = getGroupMemberInfo(id, console.DiceMaid).LastMsgTime) > 0)tLast = tLMT;
				if (!tLast)continue;
				int intDay = (int)(tNow - tLast) / 86400;
				if (intDay > intDayLim) {
					strVar["day"] = to_string(intDay);
					res << printGroup(id) + ":" + to_string(intDay) + "��\n";
					grp.leave(getMsg("strLeaveUnused", GlobalMsg, strVar));
					intCnt++;
					this_thread::sleep_for(3s);
				}
			}
			strReply += GlobalMsg["strSelfName"] + "��ɸ��Ǳˮ" + strDayLim + "��Ⱥ��" + to_string(intCnt) + "����" + res.show();
			console.log(strReply, 3, printSTNow());
		}
		else if (strPara == "black") {
			std::map<long long, string>mGroupList = getGroupList();
			for (auto &[id,grp] : ChatList) {
				if (!mGroupList.count(id)||grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����") || grp.isset("���"))continue;
				if (BlackGroup.count(id)) {
					res << printGroup(id) + "��" + "������Ⱥ";
					if(console["LeaveBlackGroup"])grp.leave(getMsg("strBlackGroup"));
				}
				if (!grp.isGroup)continue;
				vector<GroupMemberInfo> MemberList = getGroupMemberList(id);
				for (auto eachQQ : MemberList) {
					if (BlackQQ.count(eachQQ.QQID)) {
						if (getGroupMemberInfo(id, eachQQ.QQID).permissions < getGroupMemberInfo(id, getLoginQQ()).permissions) {
							continue;
						}
						else if (getGroupMemberInfo(id, eachQQ.QQID).permissions > getGroupMemberInfo(id, getLoginQQ()).permissions) {
							res << printChat(grp) + "��" + printQQ(eachQQ.QQID) + "�Է�ȺȨ�޽ϸ�";
							grp.leave("���ֺ���������Ա" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
						else if (console["LeaveBlackQQ"]) {
							res << printChat(grp) + "��" + printQQ(eachQQ.QQID);
							grp.leave("���ֺ�������Ա" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
					}
				}
			}
			if (intCnt) {
				strReply = GlobalMsg["strSelfName"] + "�Ѱ����������Ⱥ��" + to_string(intCnt) + "����" + strReply;
				console.log(strReply, 3, printSTNow());
			}
			else if (fromQQ) {
				console.log(strReply, 1, printSTNow());
			}
		}
		else if (strPara == "preserve") {
			for (auto &[id,grp] : ChatList) {
				if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("ʹ�����") || grp.isset("����"))continue;
				if (grp.isGroup && getGroupMemberInfo(id, console.master()).permissions) {
					grp.set("ʹ�����");
					continue;
				}
				res << printChat(grp);
				grp.leave(getMsg("strPreserve"));
				intCnt++;
				this_thread::sleep_for(3s);
			}
			strReply = GlobalMsg["strSelfName"] + "ɸ�������Ⱥ��" + to_string(intCnt) + "����";
			console.log(strReply, 1, printSTNow());
		}
		else
			AddMsgToQueue("�޷�ʶ��ɸѡ������", fromQQ);
		return intCnt;
	}

EVE_Menu(eventClearGroupUnpower) {
	int intGroupCnt = clearGroup("unpower");
	string strReply = "��������Ȩ��Ⱥ��" + to_string(intGroupCnt) + "����";
	MessageBoxA(nullptr, strReply.c_str(), "һ������", MB_OK | MB_ICONINFORMATION);
	return 0;
}
EVE_Menu(eventClearGroup30) {
	int intGroupCnt = clearGroup("30");
	string strReply = "������30��δʹ��Ⱥ��" + to_string(intGroupCnt) + "����";
	MessageBoxA(nullptr, strReply.c_str(), "һ������", MB_OK | MB_ICONINFORMATION);
	return 0;
}
EVE_Menu(eventGlobalSwitch) {
	if (console["DisabledGlobal"]) {
		console.set("DisabledGlobal", 0);
		MessageBoxA(nullptr, "�����ѽ�����Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}
	else {
		console.set("DisabledGlobal", 1);
		MessageBoxA(nullptr, "������ȫ�־�Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}
EVE_Request_AddFriend(eventAddFriend) {
	if (!console["ListenFriendRequest"])return 0;
	string strMsg = "��������������ԣ�" + printQQ(fromQQ);
	if (BlackQQ.count(fromQQ)) {
		strMsg += "���Ѿܾ����û��ں������У�";
		setFriendAddRequest(responseFlag, 2, "");
		console.log(strMsg, 3, printSTNow());
	}
	else if (trustedQQ(fromQQ)) {
		strMsg += "����ͬ�⣨�������û���";
		setFriendAddRequest(responseFlag, 1, "");
		GlobalMsg["strAddFriendWhiteQQ"].empty() ? AddMsgToQueue(getMsg("strAddFriend"), fromQQ)
			: AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromQQ);
		console.log(strMsg, 1, printSTNow());
	}
	else if (console["Private"] && !console["AllowStranger"]) {
		strMsg += "���Ѿܾ�����ǰ��˽��ģʽ��";
		setFriendAddRequest(responseFlag, 2, "");
		console.log(strMsg, 1, printSTNow());
	}
	else {
		strMsg += "����ͬ��";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(getMsg("strAddFriend"), fromQQ);
		console.log(strMsg, 1, printSTNow());
	}
	return 1;
}
EVE_Friend_Add(eventFriendAdd) {
	if (!console["ListenFriendAdd"])return 0;
	GlobalMsg["strAddFriendWhiteQQ"].empty() ? AddMsgToQueue(getMsg("strAddFriend"), fromQQ)
		: AddMsgToQueue(getMsg("strAddFriendWhiteQQ"), fromQQ);
	return 0;
}

