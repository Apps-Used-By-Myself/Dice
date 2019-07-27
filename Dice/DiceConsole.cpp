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
#include "MsgFormat.h"
#include "NameStorage.h"
#include "DiceNetwork.h"
#include "jsonio.h"

using namespace std;
using namespace CQ;

	long long masterQQ = 0;
set<long long> AdminQQ = {};
set<chatType> MonitorList = {};
std::map<std::string, bool>boolConsole = { {"DisabledGlobal",false},
{"DisabledMe",false},{"DisabledJrrp",false},{"DisabledDeck",true},{"DisabledDraw",false},{"DisabledSend",true},
{"Private",false},{"LeaveDiscuss",false},
{"LeaveBlackQQ",true},{"AllowStranger",true},
{"BannedBanOwner",true},{"BannedLeave",false},{"BannedBanInviter",true},
{"KickedBanInviter",true} };
//�����б�
std::map<long long, long long> mDiceList;
//Ⱥ������
std::map<long long, long long> mGroupInviter;
	//���Ի����
	std::map<std::string, std::string> PersonalMsg;
	//botoff��Ⱥ
	std::set<long long> DisabledGroup;
	//botoff��������
	std::set<long long> DisabledDiscuss;
	//������Ⱥ��˽��ģʽ����
	std::set<long long> WhiteGroup;
	//������Ⱥ������������
	std::set<long long> BlackGroup;
	//�������û�������˽������
	std::set<long long> WhiteQQ;
	//�������û�������������
	std::set<long long> BlackQQ;
	//��ǰʱ��
	SYSTEMTIME stNow = { 0 };
	SYSTEMTIME stTmp = { 0 };
	//�ϰ�ʱ��
	std::pair<int, int> ClockToWork = { 24,0 };
	//�°�ʱ��
	std::pair<int, int> ClockOffWork = { 24,0 };

	string printClock(std::pair<int, int> clock) {
		string strClock=to_string(clock.first);
		strClock += ":";
		if(clock.second<10)strClock += "0";
		strClock += to_string(clock.second);
		return strClock;
	}
std::string printSTime(SYSTEMTIME st){
	return to_string(st.wYear) + "/" + to_string(st.wMonth) + "/" + to_string(st.wDay) + " " + to_string(st.wHour) + ":" + to_string(st.wMinute) + ":" + to_string(st.wSecond);
}
	//��ӡ�û��ǳ�QQ
	string printQQ(long long llqq) {
		return getStrangerInfo(llqq).nick + "(" + to_string(llqq) + ")";
	}
	//��ӡQQȺ��
	string printGroup(long long llgroup) {
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
	if (!Network::GET("shiki.stringempty.xyz", "/DiceList", 80, list))
	{
		sendAdmin(("��ȡ�����б�ʱ��������: \n" + list).c_str(), 0);
		return;
	}
	readJson(list, mDiceList);
}
	void sendAdmin(std::string strMsg, long long fromQQ) {
		string strName = fromQQ ? getName(fromQQ) : "";
		if(AdminQQ.count(fromQQ)) {
			AddMsgToQueue(strName + strMsg, masterQQ);
			for (auto it : AdminQQ) {
				if (fromQQ == it)AddMsgToQueue(strMsg, it);
				else AddMsgToQueue(strName + strMsg, it);
			}
		}
		else {
			masterQQ == fromQQ ? AddMsgToQueue(strMsg, masterQQ) : AddMsgToQueue(strName + strMsg, masterQQ);
			for (auto it : AdminQQ) {
				AddMsgToQueue(strName + strMsg, it);
			}
		}
	}

void NotifyMonitor(std::string strMsg) {
		if (!boolMasterMode)return;
		for (auto it : MonitorList) {
			AddMsgToQueue(strMsg, it.first, it.second);
			Sleep(1000);
		}
	}
//�����û����Ѳ�Ⱥ
void checkBlackQQ(long long llQQ, std::string strWarning) {
	map<long long, string> GroupList = getGroupList();
	string strNotice;
	int intCnt = 0;
	for (auto eachGroup : GroupList) {
		if (getGroupMemberInfo(eachGroup.first, llQQ).QQID == llQQ) {
			strNotice += "\n" + printGroup(eachGroup.first);
			if (getGroupMemberInfo(eachGroup.first, llQQ).permissions < getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
				strNotice += "�Է�ȺȨ�޽ϵ�";
			}
			else if (getGroupMemberInfo(eachGroup.first, llQQ).permissions > getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
				AddMsgToQueue(strWarning, eachGroup.first, Group);
				AddMsgToQueue("����������������Ա" + printQQ(llQQ) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ", eachGroup.first, Group);
				strNotice += "�Է�ȺȨ�޽ϸߣ�����Ⱥ";
				Sleep(1000);
				setGroupLeave(eachGroup.first);
			}
			else if (WhiteGroup.count(eachGroup.first) || MonitorList.count({ eachGroup.first ,Group })) {
				strNotice += "Ⱥ�ڰ�������";
			}
			else if (boolConsole["LeaveBlackQQ"]) {
				AddMsgToQueue(strWarning, eachGroup.first, Group); 
				AddMsgToQueue("����������������Ա" + printQQ(llQQ) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ", eachGroup.first, Group);
				strNotice += "����Ⱥ";
				Sleep(1000);
				setGroupLeave(eachGroup.first);
			}
			else
				AddMsgToQueue(strWarning, eachGroup.first, Group);
			intCnt++;
		}
	}
	if (intCnt) {
		strNotice = "�������" + printQQ(llQQ) + "��ͬȺ��" + to_string(intCnt) + "����" + strNotice;
		sendAdmin(strNotice);
	}
}
//�����û�
void addBlackQQ(long long llQQ, std::string strReason, std::string strNotice) {
	if (llQQ == masterQQ || AdminQQ.count(llQQ) || llQQ == getLoginQQ())return;
	if (WhiteQQ.count(llQQ))WhiteQQ.erase(llQQ);
	if (BlackQQ.count(llQQ) == 0) {
		BlackQQ.insert(llQQ);
		strReason.empty() ? AddMsgToQueue(GlobalMsg["strBlackQQAddNotice"], llQQ)
			: AddMsgToQueue(format(GlobalMsg["strBlackQQAddNoticeReason"], { strReason }), llQQ);
	}
	checkBlackQQ(llQQ, strNotice);
}
struct fromMsg {
	string strMsg;
	long long fromQQ = 0;
	long long fromGroup = 0;
	fromMsg() = default;
	fromMsg(string strMsg, long long QQ, long long Group) :strMsg(strMsg), fromQQ(QQ), fromGroup(Group) {};
};
// ��Ϣ���Ͷ���
std::queue<fromMsg> warningQueue;
// ��Ϣ���Ͷ�����
mutex warningMutex;
void AddWarning(const string& msg, long long DiceQQ, long long fromGroup)
{
	lock_guard<std::mutex> lock_queue(warningMutex);
	warningQueue.emplace(msg, DiceQQ, fromGroup);
}
void warningHandler() {
	while (Enabled)
	{
		fromMsg warning;
		{
			lock_guard<std::mutex> lock_queue(warningMutex);
			if (!warningQueue.empty())
			{
				warning = warningQueue.front();
				warningQueue.pop();
			}
		}
		if (!warning.strMsg.empty()) {
			long long blackQQ, blackGroup;
			string type, time, note;
			nlohmann::json jInfo;
			try {
				jInfo = nlohmann::json::parse(GBKtoUTF8(warning.strMsg));
				jInfo.count("fromQQ") ? blackQQ = jInfo["fromQQ"] : blackQQ = 0;
				jInfo.count("fromGroup") ? blackGroup = jInfo["fromGroup"] : blackGroup = 0;
				jInfo.count("type") ? type = readJKey<string>(jInfo["type"]) : type = "Unknown";
				//jInfo.count("time") ? time = readJKey<string>(jInfo["time"]) : time = "Unknown";
				jInfo.count("note") ? note = readJKey<string>(jInfo["note"]) : note = "";
			}
			catch (...) {
				continue;
			}
			if (type != "ban" && type != "kick" || (!blackGroup || BlackGroup.count(blackGroup)) && (!blackQQ || BlackQQ.count(blackQQ))) {
				continue;
			}
			string strWarning = "!warning" + warning.strMsg;
			if (warning.fromQQ != masterQQ || !AdminQQ.count(warning.fromQQ))sendAdmin("����" + printQQ(warning.fromQQ) + strWarning);
			if (blackGroup) {
				if (!BlackGroup.count(blackGroup)) {
					BlackGroup.insert(blackGroup);
					sendAdmin("��֪ͨ" + GlobalMsg["strSelfName"] + "��" + printGroup(blackGroup) + "����Ⱥ��������", warning.fromQQ);
				}
				if (getGroupList().count(blackGroup)) {
					if (blackGroup != warning.fromGroup)AddMsgToQueue(strWarning, blackGroup, Group);
					setGroupLeave(blackGroup);
				}
			}
			if (blackQQ) {
				if (!BlackQQ.count(blackQQ))sendAdmin("��֪ͨ" + GlobalMsg["strSelfName"] + "��" + printQQ(blackQQ) + "�����û�������", warning.fromQQ);
				addBlackQQ(blackQQ, note, strWarning);
			}
		}
		else
		{
			this_thread::sleep_for(chrono::milliseconds(20));
		}
	}
}
//���׼�ʱ��
	void ConsoleTimer() {
		while (Enabled) {
			GetLocalTime(&stNow);
			//����ʱ��䶯
			if (stTmp.wMinute != stNow.wMinute) {
				stTmp = stNow;
				if (stNow.wHour == ClockOffWork.first&&stNow.wMinute == ClockOffWork.second && !boolConsole["DisabledGlobal"]) {
					boolConsole["DisabledGlobal"] = true;
					NotifyMonitor(GlobalMsg["strSelfName"] + GlobalMsg["strClockOffWork"]);
				}
				if (stNow.wHour == ClockToWork.first&&stNow.wMinute == ClockToWork.second&&boolConsole["DisabledGlobal"]) {
					boolConsole["DisabledGlobal"] = false;
					NotifyMonitor(GlobalMsg["strSelfName"] + GlobalMsg["strClockToWork"]);
				}
				if (stNow.wHour == 5 && stNow.wMinute == 0) {
					getDiceList();
					clearGroup("black");
				}
			}
			Sleep(100);
		}
	}

	//һ������
	int clearGroup(string strPara,long long fromQQ) {
		int intCnt=0;
		string strReply;
		map<long long,string> GroupList=getGroupList();
		if (strPara == "unpower" || strPara.empty()) {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions == 1) {
					AddMsgToQueue(GlobalMsg["strGroupClr"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					intCnt++;
				}
			}
			strReply = GlobalMsg["strSelfName"] + "ɸ����ȺȨ��Ⱥ��" + to_string(intCnt) + "����";
			sendAdmin(strReply);
		}
		else if (isdigit(static_cast<unsigned char>(strPara[0]))) {
			int intDayLim = stoi(strPara);
			string strDayLim = to_string(intDayLim);
			time_t tNow = time(NULL);;
			for (auto eachChat : mLastMsgList) {
				if (eachChat.first.second == Private)continue;
				int intDay = (int)(tNow - eachChat.second) / 86400;
				if (intDay > intDayLim) {
					strReply += printChat(eachChat.first) + ":" + to_string(intDay) + "��\n";
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSelfName"], to_string(intDay) }), eachChat.first.first, eachChat.first.second);
					Sleep(100);
					if (eachChat.first.second == Group) {
						setGroupLeave(eachChat.first.first);
						if (GroupList.count(eachChat.first.first))GroupList.erase(eachChat.first.first);
					}
					else setDiscussLeave(eachChat.first.first);
					mLastMsgList.erase(eachChat.first);
					intCnt++;
				}
			}
			for (auto eachGroup : GroupList) {
				int intDay = (int)(tNow - getGroupMemberInfo(eachGroup.first, getLoginQQ()).LastMsgTime)/86400;
				if (intDay > intDayLim) {
					strReply += printGroup(eachGroup.first) + ":" + to_string(intDay) + "��\n";
					AddMsgToQueue(format(GlobalMsg["strOverdue"], { GlobalMsg["strSelfName"], to_string(intDay) }), eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					mLastMsgList.erase({ eachGroup.first ,CQ::Group });
					intCnt++;
				}
			}
			strReply += GlobalMsg["strSelfName"] + "��ɸ��Ǳˮ" + strDayLim + "��Ⱥ��" + to_string(intCnt) + "����";
			AddMsgToQueue(strReply,masterQQ);
		}
		else if (strPara == "black") {
			for (auto eachGroup : GroupList) {
				if (BlackGroup.count(eachGroup.first)) {
					AddMsgToQueue(GlobalMsg["strBlackGroup"], eachGroup.first, Group);
					strReply += "\n" + printGroup(eachGroup.first) + "��" + "������Ⱥ";
					Sleep(100);
					setGroupLeave(eachGroup.first);
				}
				vector<GroupMemberInfo> MemberList = getGroupMemberList(eachGroup.first);
				for (auto eachQQ : MemberList) {
					if (BlackQQ.count(eachQQ.QQID)) {
						if (getGroupMemberInfo(eachGroup.first, eachQQ.QQID).permissions < getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
							continue;
						}
						else if (getGroupMemberInfo(eachGroup.first, eachQQ.QQID).permissions > getGroupMemberInfo(eachGroup.first, getLoginQQ()).permissions) {
							AddMsgToQueue("���ֺ�������Ա" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ", eachGroup.first, Group);
							strReply += "\n" + printGroup(eachGroup.first) + "��" + printQQ(eachQQ.QQID) + "�Է�ȺȨ�޽ϸ�";
							Sleep(100);
							setGroupLeave(eachGroup.first);
							intCnt++;
							break;
						}
						else if (WhiteGroup.count(eachGroup.first) || MonitorList.count({ eachGroup.first ,Group })) {
							continue;
						}
						else if (boolConsole["LeaveBlackQQ"]) {
							AddMsgToQueue("���ֺ�������Ա" + printQQ(eachQQ.QQID) + "\n" + GlobalMsg["strSelfName"] + "��Ԥ������Ⱥ", eachGroup.first, Group);
							strReply += "\n" + printGroup(eachGroup.first) + "��" + printQQ(eachQQ.QQID);
							Sleep(100);
							setGroupLeave(eachGroup.first);
							intCnt++;
							break;
						}
					}
				}
			}
			if (intCnt) {
				strReply = GlobalMsg["strSelfName"] + "�Ѱ����������Ⱥ��" + to_string(intCnt) + "����" + strReply;
				sendAdmin(strReply);
			}
			else if (fromQQ) {
				sendAdmin(GlobalMsg["strSelfName"] + "δ�������Ⱥ��");
			}
		}
		else if (strPara == "preserve") {
			for (auto eachGroup : GroupList) {
				if (getGroupMemberInfo(eachGroup.first, masterQQ).QQID != masterQQ&&WhiteGroup.count(eachGroup.first)==0) {
					AddMsgToQueue(GlobalMsg["strPreserve"], eachGroup.first, Group);
					Sleep(10);
					setGroupLeave(eachGroup.first);
					mLastMsgList.erase({ eachGroup.first ,Group });
					intCnt++;
				}
			}
			strReply = GlobalMsg["strSelfName"] + "ɸ����������Ⱥ��" + to_string(intCnt) + "����";
			sendAdmin(strReply);
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
	if (boolConsole["DisabledGlobal"]) {
		boolConsole["DisabledGlobal"] = false;
		MessageBoxA(nullptr, "�����ѽ�����Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}
	else {
		boolConsole["DisabledGlobal"] = true;
		MessageBoxA(nullptr, "������ȫ�־�Ĭ��", "ȫ�ֿ���", MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}
EVE_Request_AddFriend(eventAddFriend) {
	string strMsg = "��������������ԣ�" + printQQ(fromQQ);
	if (BlackQQ.count(fromQQ)) {
		strMsg += "���Ѿܾ����û��ں������У�";
		setFriendAddRequest(responseFlag, 2, "");
	}
	else if (WhiteQQ.count(fromQQ)) {
		strMsg += "����ͬ�⣨�û��ڰ������У�";
		setFriendAddRequest(responseFlag, 1, "");
		GlobalMsg["strAddFriendWhiteQQ"].empty() ? AddMsgToQueue(GlobalMsg["strAddFriend"], fromQQ)
			: AddMsgToQueue(GlobalMsg["strAddFriendWhiteQQ"], fromQQ);
	}
	else if (boolConsole["Private"]&& !boolConsole["AllowStranger"]) {
		strMsg += "���Ѿܾ�����ǰ��˽��ģʽ��";
		setFriendAddRequest(responseFlag, 2, "");
	}
	else {
		strMsg += "����ͬ��";
		setFriendAddRequest(responseFlag, 1, "");
		AddMsgToQueue(GlobalMsg["strAddFriend"], fromQQ);
	}
	sendAdmin(strMsg);
	return 1;
}

