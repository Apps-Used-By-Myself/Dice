/*
 *  _______     ________    ________    ________    __
 * |   __  \   |__    __|  |   _____|  |   _____|  |  |
 * |  |  |  |     |  |     |  |        |  |_____   |  |
 * |  |  |  |     |  |     |  |        |   _____|  |__|
 * |  |__|  |   __|  |__   |  |_____   |  |_____    __
 * |_______/   |________|  |________|  |________|  |__|
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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <thread>
#include <chrono>
#include <mutex>

#include "APPINFO.h"
#include "jsonio.h"
#include "DiceFile.hpp"
#include "RandomGenerator.h"
#include "RD.h"
#include "CQEVE_ALL.h"
#include "InitList.h"
#include "GlobalVar.h"
#include "NameStorage.h"
#include "GetRule.h"
#include "DiceMsgSend.h"
#include "CustomMsg.h"
#include "NameGenerator.h"
#include "MsgFormat.h"
#include "DiceCloud.h"
#include "CardDeck.h"
#include "DiceConsole.h"
#include "EncodingConvert.h"
#include "DiceEvent.h"

using namespace std;
using namespace CQ;


//Masterģʽ
bool boolMasterMode = false;
//����ģʽ
bool boolStandByMe = false;
//�����������ʺ�
long long IdentityQQ = 0;
long long StandQQ = 0;
map<long long, int> DefaultDice;
map<chatType, int> mDefaultCOC;
map<long long, string> WelcomeMsg;
map<long long, string> DefaultRule;
set<long long> DisabledJRRPGroup;
set<long long> DisabledJRRPDiscuss;
set<long long> DisabledMEGroup;
set<long long> DisabledMEDiscuss;
set<long long> DisabledHELPGroup;
set<long long> DisabledHELPDiscuss;
set<long long> DisabledOBGroup;
set<long long> DisabledOBDiscuss;

unique_ptr<Initlist> ilInitList;

map<chatType, time_t> mLastMsgList;
map<chatType, chatType> mLinkedList;
multimap<chatType, chatType> mFwdList;

using PropType = map<string, int>;
map<SourceType, PropType> CharacterProp;
multimap<long long, long long> ObserveGroup;
multimap<long long, long long> ObserveDiscuss;
string strFileLoc;

//��������
void dataBackUp() {
	//����MasterQQ
	if (!boolStandByMe) {
		ofstream ofstreamMaster(strFileLoc + "Master.RDconf", ios::out | ios::trunc);
		ofstreamMaster << masterQQ << std::endl << boolMasterMode << std::endl << boolConsole["DisabledGlobal"] << std::endl << boolConsole["DisabledMe"] << std::endl << boolConsole["Private"] << std::endl << boolConsole["DisabledJrrp"] << std::endl << boolConsole["LeaveDiscuss"] << std::endl;
		ofstreamMaster << ClockToWork.first << " " << ClockToWork.second << endl
			<< ClockOffWork.first << " " << ClockOffWork.second << endl;
		ofstreamMaster.close();
	}
	//���ݹ���Ա�б�
	saveFile(strFileLoc + "AdminQQ.RDconf", AdminQQ);
	//���ݼ�ش����б�
	ofstream ofstreamMonitorList(strFileLoc + "MonitorList.RDconf");
	for (auto it : MonitorList) {
		if (it.first)ofstreamMonitorList << it.first << " " << it.second << std::endl;
	}
	ofstreamMonitorList.close();
	saveJMap(strFileLoc + "boolConsole.json", boolConsole);
	//���ݸ��Ի����
	ofstream ofstreamPersonalMsg(strFileLoc + "PersonalMsg.RDconf", ios::out | ios::trunc);
	for (auto it = PersonalMsg.begin(); it != PersonalMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamPersonalMsg << it->first << std::endl << it->second << std::endl;
	}
	//����CustomMsg
	//����Ĭ�Ϲ���
	ofstream ofstreamDefaultRule(strFileLoc + "DefaultRule.RDconf", ios::out | ios::trunc);
	for (auto it = DefaultRule.begin(); it != DefaultRule.end(); ++it)
	{
		ofstreamDefaultRule << it->first << std::endl << it->second << std::endl;
	}
	//���ݺڰ�����
	saveFile(strFileLoc + "WhiteGroup.RDconf", WhiteGroup);
	saveFile(strFileLoc + "BlackGroup.RDconf", BlackGroup);
	saveFile(strFileLoc + "WhiteQQ.RDconf", WhiteQQ);
	saveFile(strFileLoc + "BlackQQ.RDconf", BlackQQ);
	//saveBlackMark(strFileLoc + "BlackMarks.json");
	//���������б�
	ofstream ofstreamLastMsgList(strFileLoc + "LastMsgList.MYmap", ios::out | ios::trunc);
	for (auto it : mLastMsgList)
	{
		ofstreamLastMsgList << it.first.first << " " << it.first.second << " "<< it.second << std::endl;
	}
	ofstreamLastMsgList.close();
	//�����������б�
	saveFile(strFileLoc + "GroupInviter.RDconf", mGroupInviter);
	//����Ĭ��COC����
	ofstream ofstreamDefaultCOC(strFileLoc + "DefaultCOC.MYmap", ios::out | ios::trunc);
	for (auto it : mDefaultCOC)
	{
		ofstreamDefaultCOC << it.first.first << " " << it.first.second << " " << it.second << std::endl;
	}
	ofstreamDefaultCOC.close();
	saveFile(strFileLoc + "Default.RDconf", DefaultDice);
	//���濨��
	saveJMap(strFileLoc + "GroupDeck.json", CardDeck::mGroupDeck);
	saveJMap(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	saveJMap(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	saveJMap(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	saveJMap(strFileLoc + "ReplyDeck.json", CardDeck::mReplyDeck);
	//����Ⱥ������
	ilInitList.reset();
	Name.reset();
	saveFile(strFileLoc + "DisabledGroup.RDconf", DisabledGroup);
	saveFile(strFileLoc + "DisabledDiscuss.RDconf", DisabledDiscuss);
	saveFile(strFileLoc + "DisabledJRRPGroup.RDconf", DisabledJRRPGroup);
	saveFile(strFileLoc + "DisabledJRRPDiscuss.RDconf", DisabledJRRPDiscuss);
	saveFile(strFileLoc + "DisabledMEGroup.RDconf", DisabledMEGroup);
	saveFile(strFileLoc + "DisabledMEDiscuss.RDconf", DisabledMEDiscuss);
	saveFile(strFileLoc + "DisabledHELPGroup.RDconf", DisabledHELPGroup);
	saveFile(strFileLoc + "DisabledHELPDiscuss.RDconf", DisabledHELPDiscuss);
	saveFile(strFileLoc + "DisabledOBGroup.RDconf", DisabledOBGroup);
	saveFile(strFileLoc + "DisabledOBDiscuss.RDconf", DisabledOBDiscuss);
	saveFile(strFileLoc + "ObserveGroup.RDconf", ObserveGroup);
	saveFile(strFileLoc + "ObserveDiscuss.RDconf", ObserveDiscuss);

	ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
	for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
	{
		for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
		{
			ofstreamCharacterProp << it->first.QQ << " " << it->first.Type << " " << it->first.GrouporDiscussID << " "
				<< it1->first << " " << it1->second << std::endl;
		}
	}
	ofstreamCharacterProp.close();

	ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
	for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
	{
		while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
		while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
		while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
		while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
		ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
	}
	ofstreamWelcomeMsg.close();
}
EVE_Enable(eventEnable)
{
	//Wait until the thread terminates
	while (msgSendThreadRunning)
		Sleep(10);

	thread msgSendThread(SendMsg);
	msgSendThread.detach();
	thread threadConsoleTimer(ConsoleTimer);
	threadConsoleTimer.detach();
	thread threadWarning(warningHandler);
	threadWarning.detach();
	thread threadFrq(frqHandler);
	threadFrq.detach();
	strFileLoc = getAppDirectory();
	DiceMaid = getLoginQQ();
	/*
	* ���ƴ洢-�������ȡ
	*/
	Name = make_unique<NameStorage>(strFileLoc + "Name.dicedb");
	//��ȡMasterMode
	ifstream ifstreamMaster(strFileLoc + "Master.RDconf");
	if (ifstreamMaster)
	{
		ifstreamMaster >> masterQQ >> boolMasterMode >> boolConsole["DisabledGlobal"] >> boolConsole["DisabledMe"] >> boolConsole["Private"] >> boolConsole["DisabledJrrp"] >> boolConsole["LeaveDiscuss"]
			>> ClockToWork.first >> ClockToWork.second >> ClockOffWork.first >> ClockOffWork.second;
	}
	ifstreamMaster.close();
	//��ȡ����Ա�б�
	loadFile(strFileLoc + "AdminQQ.RDconf", AdminQQ);
	if(AdminQQ.upper_bound(0)!= AdminQQ.begin()) {
		AdminQQ.erase(AdminQQ.begin(), AdminQQ.upper_bound(0));
	}
	AdminQQ.insert(masterQQ);
	//��ȡ��ش����б�
	ifstream ifstreamMonitorList(strFileLoc + "MonitorList.RDconf");
	while (ifstreamMonitorList)
	{
		long long llMonitor = 0;
		int t = 0;
		ifstreamMonitorList >> llMonitor >> t;
		if (llMonitor)MonitorList.insert({ llMonitor ,(msgtype)t });
	}
	if (MonitorList.size() == 0) {
		for (auto it : AdminQQ) {
			MonitorList.insert({ it ,Private });
		}
		MonitorList.insert({ 863062599 ,Group });
		MonitorList.insert({ 192499947 ,Group });
		MonitorList.insert({ 754494359 ,Group });
	}
	ifstreamMonitorList.close();
	//��ȡboolConsole
	loadJMap(strFileLoc + "boolConsole.json", boolConsole);
	ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
	if (ifstreamCharacterProp)
	{
		long long QQ, GrouporDiscussID;
		int Type, Value;
		string SkillName;
		while (ifstreamCharacterProp >> QQ >> Type >> GrouporDiscussID >> SkillName >> Value)
		{
			CharacterProp[SourceType(QQ, Type, GrouporDiscussID)][SkillName] = Value;
		}
	}
	ifstreamCharacterProp.close();
	loadFile(strFileLoc + "WhiteGroup.RDconf", WhiteGroup);
	loadFile(strFileLoc + "WhiteQQ.RDconf", WhiteQQ);
	loadFile(strFileLoc + "BlackGroup.RDconf", BlackGroup);
	loadFile(strFileLoc + "BlackQQ.RDconf", BlackQQ);
	loadBlackMark(strFileLoc + "BlackMarks.json");
	loadFile(strFileLoc + "DisabledGroup.RDconf", DisabledGroup);
	loadFile(strFileLoc + "DisabledDiscuss.RDconf", DisabledDiscuss);
	loadFile(strFileLoc + "DisabledJRRPGroup.RDconf", DisabledJRRPGroup);
	loadFile(strFileLoc + "DisabledJRRPDiscuss.RDconf", DisabledJRRPDiscuss);
	loadFile(strFileLoc + "DisabledMEGroup.RDconf", DisabledMEGroup);
	loadFile(strFileLoc + "DisabledMEDiscuss.RDconf", DisabledMEDiscuss);
	loadFile(strFileLoc + "DisabledHELPGroup.RDconf", DisabledHELPGroup);
	loadFile(strFileLoc + "DisabledHELPDiscuss.RDconf", DisabledHELPDiscuss);
	loadFile(strFileLoc + "DisabledOBGroup.RDconf", DisabledOBGroup);
	loadFile(strFileLoc + "DisabledOBDiscuss.RDconf", DisabledOBDiscuss);
	loadFile(strFileLoc + "ObserveGroup.RDconf", ObserveGroup);
	loadFile(strFileLoc + "ObserveDiscuss.RDconf", ObserveDiscuss);
	loadFile(strFileLoc + "Default.RDconf", DefaultDice);
	loadFile(strFileLoc + "DefaultRule.RDconf", DefaultRule);

	ifstream ifstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf");
	if (ifstreamWelcomeMsg)
	{
		long long GroupID;
		string Msg;
		while (ifstreamWelcomeMsg >> GroupID >> Msg)
		{
			while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
			while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
			while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
			while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
			WelcomeMsg[GroupID] = Msg;
		}
	}
	ifstreamWelcomeMsg.close();
	//��ȡ�����ĵ�
	ifstream ifstreamHelpDoc(strFileLoc + "HelpDoc.txt");
	if (ifstreamHelpDoc)
	{
		string strName, strMsg ,strDebug;
		while (ifstreamHelpDoc) {
			getline(ifstreamHelpDoc, strName);
			getline(ifstreamHelpDoc, strMsg);
			while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
			while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\s"), 2, " ");
			while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
			EditedHelpDoc[strName] = strMsg;
			HelpDoc[strName] = strMsg;
		}
	}
	ifstreamHelpDoc.close();
	
	//��ȡ�����б�
	ifstream ifstreamLastMsgList(strFileLoc + "LastMsgList.MYmap");
	if (ifstreamLastMsgList)
	{
		long long llID;
		int intT;
		chatType ct;
		time_t tLast;
		while (ifstreamLastMsgList >> llID >> intT >> tLast)
		{
			ct = { llID,(msgtype)intT };
			mLastMsgList[ct] = tLast;
		}
	}
	ifstreamLastMsgList.close();
	//��ȡ�������б�
	ifstream ifstreamGroupInviter(strFileLoc + "GroupInviter.RDconf");
	if (ifstreamGroupInviter)
	{
		long long llGroup;
		long long llQQ;
		while (ifstreamGroupInviter >> llGroup >> llQQ)
		{
			if (llGroup&&llQQ)mGroupInviter[llGroup] = llQQ;
		}
	}
	ifstreamGroupInviter.close();
	//��ȡCOC����
	ifstream ifstreamDefaultCOC(strFileLoc + "DefaultCOC.MYmap");
	if (ifstreamDefaultCOC)
	{
		long long llID;
		int intT;
		chatType ct;
		int intRule;
		while (ifstreamDefaultCOC >> llID >> intT >> intRule)
		{
			ct = { llID,(msgtype)intT };
			mDefaultCOC[ct] = intRule;
		}
	}
	ifstreamDefaultCOC.close();
	ilInitList = make_unique<Initlist>(strFileLoc + "INIT.DiceDB");
	GlobalMsg["strSelfName"] = getLoginNick();
	ifstream ifstreamCustomMsg(strFileLoc + "CustomMsg.json");
	if (ifstreamCustomMsg)
	{
		ReadCustomMsg(ifstreamCustomMsg);
	}
	ifstreamCustomMsg.close();
	//Ԥ�޸ĳ����ظ��ı�
	for (auto it : GlobalMsg) {
		string strMsg = it.second;
		while (strMsg.find("��������") != string::npos) {
			strMsg.replace(strMsg.find("��������"), 8, GlobalMsg["strSelfName"]);
		}
		GlobalMsg[it.first] = strMsg;
	}
	//��ȡ����
	loadJMap(strFileLoc + "GroupDeck.json",CardDeck::mGroupDeck);
	loadJMap(strFileLoc + "GroupDeckTmp.json", CardDeck::mGroupDeckTmp);
	loadJMap(strFileLoc + "PrivateDeck.json", CardDeck::mPrivateDeck);
	loadJMap(strFileLoc + "PrivateDeckTmp.json", CardDeck::mPrivateDeckTmp);
	loadJMap(strFileLoc + "PublicDeck.json", CardDeck::mPublicDeck);
	loadJMap(strFileLoc + "ExternDeck.json", CardDeck::mPublicDeck);
	loadJMap(strFileLoc + "ReplyDeck.json", CardDeck::mReplyDeck);
	//��ȡ����ģʽ
	ifstream ifstreamStandByMe(strFileLoc + "StandByMe.RDconf");
	if (ifstreamStandByMe)
	{
		ifstreamStandByMe >> IdentityQQ >> StandQQ;
		if (getLoginQQ() == StandQQ) {
			boolStandByMe = true;
			masterQQ = IdentityQQ;
			string strName,strMsg;
			while (ifstreamStandByMe >> strName) {
				getline(ifstreamStandByMe, strMsg);
				while (strMsg.find("\\n") != string::npos)strMsg.replace(strMsg.find("\\n"), 2, "\n");
				while (strMsg.find("\\s") != string::npos)strMsg.replace(strMsg.find("\\s"), 2, " ");
				while (strMsg.find("\\t") != string::npos)strMsg.replace(strMsg.find("\\t"), 2, "	");
				GlobalMsg[strName] = strMsg;
			}
		}
	}
	ifstreamStandByMe.close();
	//��������
	getDiceList();
	Cloud::update();
	return 0;
}

//��������ָ��


EVE_PrivateMsg_EX(eventPrivateMsg)
{
	FromMsg Msg(eve.message, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_GroupMsg_EX(eventGroupMsg)
{
	if (eve.isAnonymous())return;
	if (eve.isSystem()) {
		if (eve.message.find("������Ա����") != string::npos&&eve.message.find(to_string(getLoginQQ())) != string::npos&&boolMasterMode) {
			string strNow = printSTime(stNow);
			long long fromQQ;
			int intAuthCnt = 0;
			string strAuthList;
			for (auto member : getGroupMemberList(eve.fromGroup)) {
				if (member.permissions == 3) {
					//��Ӧ���ľ�����Ⱥ����������
					fromQQ = member.QQID;
				}
				else if (member.permissions == 2) {
					//��¼����Ա
					strAuthList += '\n' + member.Nick + "(" + to_string(member.QQID) + ")";
					intAuthCnt++;
				}
			}
			//Master����
			if (AdminQQ.count(fromQQ))return;
			//�ܷ�����Ⱥ��
			bool isOwner = intAuthCnt == 0 || getGroupMemberInfo(eve.fromGroup, getLoginQQ()).permissions == 2;
			string strNote = strNow + "��" + printGroup(eve.fromGroup) + "��," + eve.message;
			BlackMark mark;
			mark.llMap = { {"fromGroup",eve.fromGroup},{"DiceMaid",getLoginQQ()},{"masterQQ", masterQQ} };
			mark.strMap = { {"type","ban"},{"time",strNow} };
			if (isOwner) {
				strNote.replace(strNote.rfind("����Ա"), 6, printQQ(fromQQ));
				mark.set("fromQQ", fromQQ);
			}
			else if (boolConsole["BannedBanOwner"]) {
				strNote += ";Ⱥ��" + printQQ(fromQQ);
				mark.set("ownerQQ", fromQQ);
			}
			mark.set("note", strNote);
			if (mGroupInviter.count(eve.fromGroup) && !AdminQQ.count(mGroupInviter[eve.fromGroup])) {
				long long llInviter = mGroupInviter[eve.fromGroup];
				strNote += ";��Ⱥ�����ߣ�" + printQQ(llInviter);
				mark.set("inviterQQ", llInviter);
				mark.set("note", strNote);
				if (boolConsole["BannedBanInviter"])addBlackQQ(BlackMark(mark,"inviterQQ"));
			}
			if (isOwner)addBlackQQ(BlackMark(mark, "fromQQ"));
			else {
				NotifyMonitor(strNote + ",���й���Ա" + to_string(intAuthCnt) + "��" + strAuthList);
				if (boolConsole["BannedBanOwner"])addBlackQQ(BlackMark(mark, "ownerQQ"));
			}
			NotifyMonitor(mark.getWarning());
			if (boolConsole["BannedLeave"]){
				setGroupLeave(eve.fromGroup);
				mLastMsgList.erase({ eve.fromGroup ,Group });
			}
			addBlackGroup(mark);
		}
		else return;
	}
	FromMsg Msg(eve.message, eve.fromGroup, Group, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_DiscussMsg_EX(eventDiscussMsg)
{
	time_t tNow = time(NULL);
	if (boolConsole["LeaveDiscuss"]) {
		AddMsgToQueue(GlobalMsg["strNoDiscuss"], eve.fromDiscuss, Discuss);
		Sleep(1000);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	if (BlackQQ.count(eve.fromQQ) && boolConsole["AutoClearBlack"]) {
		string strMsg = "���ֺ������û�" + printQQ(eve.fromQQ) + "���Զ�ִ����Ⱥ";
		AddMsgToQueue(strMsg, eve.fromDiscuss, Discuss);
		Sleep(1000);
		sendAdmin(printChat({ eve.fromDiscuss,Discuss }) + strMsg);
		setDiscussLeave(eve.fromDiscuss);
		return;
	}
	FromMsg Msg(eve.message, eve.fromDiscuss, Discuss, eve.fromQQ);
	if (Msg.DiceFilter())eve.message_block();
	Msg.FwdMsg(eve.message);
	return;
}

EVE_System_GroupMemberIncrease(eventGroupMemberIncrease)
{
	if (beingOperateQQ != getLoginQQ() && WelcomeMsg.count(fromGroup))
	{
		string strReply = WelcomeMsg[fromGroup];
		while (strReply.find("{@}") != string::npos)
		{
			strReply.replace(strReply.find("{@}"), 3, "[CQ:at,qq=" + to_string(beingOperateQQ) + "]");
		}
		while (strReply.find("{nick}") != string::npos)
		{
			strReply.replace(strReply.find("{nick}"), 6, getStrangerInfo(beingOperateQQ).nick);
		}
		while (strReply.find("{age}") != string::npos)
		{
			strReply.replace(strReply.find("{age}"), 5, to_string(getStrangerInfo(beingOperateQQ).age));
		}
		while (strReply.find("{sex}") != string::npos)
		{
			strReply.replace(strReply.find("{sex}"), 5,
			                 getStrangerInfo(beingOperateQQ).sex == 0
				                 ? "��"
				                 : getStrangerInfo(beingOperateQQ).sex == 1
				                 ? "Ů"
				                 : "δ֪");
		}
		while (strReply.find("{qq}") != string::npos)
		{
			strReply.replace(strReply.find("{qq}"), 4, to_string(beingOperateQQ));
		}
		AddMsgToQueue(strReply, fromGroup, Group);
	}
	if (beingOperateQQ != getLoginQQ() && BlackQQ.count(beingOperateQQ)) {
		string strNote = printGroup(fromGroup) + "���ֺ������û�" + printQQ(beingOperateQQ) + "��Ⱥ";
		if (mBlackQQMark.count(beingOperateQQ) && mBlackQQMark[beingOperateQQ].isVal("DiceMaid", getLoginQQ()))AddMsgToQueue(mBlackQQMark[beingOperateQQ].getWarning(), beingOperateQQ, Group);
		if (WhiteGroup.count(fromGroup))strNote += "��Ⱥ�ڰ������У�";
		else if (MonitorList.count({ fromGroup,Group }));
		else if (getGroupMemberInfo(fromGroup, getLoginQQ()).permissions > 1)strNote += "��Ⱥ����Ȩ�ޣ�";
		else if (boolConsole["LeaveBlackQQ"]) {
			sendGroupMsg(fromGroup, "���ֺ������û�" + printQQ(beingOperateQQ) + "��Ⱥ,��Ԥ������Ⱥ");
			strNote += "������Ⱥ��";
			Sleep(100);
			setGroupLeave(fromGroup);
		}
		sendAdmin(strNote);
	}
	else if(beingOperateQQ == getLoginQQ()){
		if (BlackGroup.count(fromGroup)) {
			AddMsgToQueue(GlobalMsg["strBlackGroup"], fromGroup, Group);
			setGroupLeave(fromGroup);
		}
		else if (boolConsole["Private"]&&WhiteGroup.count(fromGroup)==0) 
		{	//����СȺ�ƹ�����û���ϰ�����
			if (fromQQ == masterQQ || WhiteQQ.count(fromQQ) || getGroupMemberInfo(fromGroup, masterQQ).QQID == masterQQ) {
				WhiteGroup.insert(fromGroup);
				return 0;
			}
			AddMsgToQueue(GlobalMsg["strPreserve"], fromGroup, Group);
			setGroupLeave(fromGroup);
			return 0;
		}
		//else if(boolStandByMe&&getGroupMemberInfo(fromGroup, IdentityQQ).QQID != IdentityQQ) {
		//	AddMsgToQueue("�벻Ҫ������������Ⱥ��", fromGroup, Group);
		//	setGroupLeave(fromGroup);
		//	return 0;
		//}
		else if(!GlobalMsg["strAddGroup"].empty()) {
			AddMsgToQueue(GlobalMsg["strAddGroup"], fromGroup, Group);
		}
	}
	return 0;
}

EVE_System_GroupMemberDecrease(eventGroupMemberDecrease) {
	if (beingOperateQQ == getLoginQQ() && boolMasterMode) {
		string strNow = printSTime(stNow);
		mLastMsgList.erase({ fromGroup ,Group });
		if (AdminQQ.count(fromQQ))return 1;
		string strNote = strNow + " " + printQQ(fromQQ) + "��" + GlobalMsg["strSelfName"] + "�Ƴ���Ⱥ" + to_string(fromGroup);
		BlackMark mark;
		mark.llMap = { {"fromGroup",fromGroup},{"fromQQ",fromQQ},{"DiceMaid",getLoginQQ()},{"masterQQ", masterQQ} };
		mark.strMap = { {"type","kick"},{"time",strNow}};
		mark.set("note", strNote);
		if (mGroupInviter.count(fromGroup) && AdminQQ.count(mGroupInviter[fromGroup]) == 0) {
			long long llInviter = mGroupInviter[fromGroup];
			strNote += ";��Ⱥ�����ߣ�" + printQQ(llInviter);
			mark.llMap["inviterQQ"] = llInviter;
			mark.set("note", strNote);
			mark.getWarning();
			if (boolConsole["KickedBanInviter"])addBlackQQ(BlackMark(mark, "inviterQQ"));
		}
		if (WhiteGroup.count(fromGroup)) {
			WhiteGroup.erase(fromGroup);
		}
		BlackGroup.insert(fromGroup);
		NotifyMonitor(mark.getWarning());
		addBlackQQ(BlackMark(mark, "fromQQ"));
	}
	else if (mDiceList.count(beingOperateQQ) && subType == 2) {
		string strNow = printSTime(stNow);
		string strNote = strNow + " " + printQQ(fromQQ) + "��" + printQQ(beingOperateQQ) + "�Ƴ���Ⱥ" + to_string(fromGroup);
		while (strNote.find('\"') != string::npos)strNote.replace(strNote.find('\"'), 1, "\'"); 
		string strWarning = "!warning{\n\"fromGroup\":" + to_string(fromGroup) + ",\n\"type\":\"kick\",\n\"fromQQ\":" + to_string(fromQQ) + ",\n\"time\":\"" + strNow + "\",\n\"DiceMaid\":" + to_string(beingOperateQQ) + ",\n\"masterQQ\":" + to_string(masterQQ) + ",\n\"note\":\"" + strNote + "\"\n}";
		BlackMark mark;
		mark.llMap = { {"fromGroup",fromGroup},{"fromQQ",fromQQ},{"DiceMaid",beingOperateQQ},{"masterQQ", mDiceList[beingOperateQQ]} };
		mark.strMap = { {"type","kick"},{"time",strNow},{"note",strNote} };
		NotifyMonitor(strWarning);
		addBlackGroup(mark);
		addBlackQQ(BlackMark(mark, "fromQQ"));
	}
	return 0;
}

EVE_Request_AddGroup(eventGroupInvited) {
	if (subType == 2) {
		if (masterQQ&&boolMasterMode) {
			string strMsg = "Ⱥ����������ԣ�" + getStrangerInfo(fromQQ).nick +"("+ to_string(fromQQ) + "),Ⱥ��(" + to_string(fromGroup)+")��";
			if (BlackGroup.count(fromGroup)) {
				strMsg += "\n�Ѿܾ���Ⱥ�ں������У�";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else if (BlackQQ.count(fromQQ)) {
				strMsg += "\n�Ѿܾ����û��ں������У�";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else if (WhiteGroup.count(fromGroup)) {
				strMsg += "\n��ͬ�⣨Ⱥ�ڰ������У�";
				setGroupAddRequest(responseFlag, 2, 1, "");
				mGroupInviter[fromGroup] = fromQQ;
			}
			else if (WhiteQQ.count(fromQQ)) {
				strMsg += "\n��ͬ�⣨�û��ڰ������У�";
				WhiteGroup.insert(fromGroup);
				setGroupAddRequest(responseFlag, 2, 1, "");
				mGroupInviter[fromGroup] = fromQQ;
			}
			else if (boolConsole["Private"]) {
				strMsg += "\n�Ѿܾ�����ǰ��˽��ģʽ��";
				setGroupAddRequest(responseFlag, 2, 2, "");
			}
			else{
				strMsg += "��ͬ��";
				setGroupAddRequest(responseFlag, 2, 1, "");
				mGroupInviter[fromGroup] = fromQQ;
			}
			sendAdmin(strMsg);
		}
		return 1;
	}
	return 0;
}

EVE_Menu(eventMasterMode) {
	if (boolMasterMode) {
		boolMasterMode = false;
		MessageBoxA(nullptr, "Masterģʽ�ѹرա�", "Masterģʽ�л�",MB_OK | MB_ICONINFORMATION);
	}else {
		boolMasterMode = true;
		MessageBoxA(nullptr, "Masterģʽ�ѿ�����", "Masterģʽ�л�", MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

EVE_Disable(eventDisable)
{
	Enabled = false;
	dataBackUp();
	EditedMsg.clear(); 
	WhiteGroup.clear();
	BlackGroup.clear();
	WhiteQQ.clear();
	BlackQQ.clear();
	mDefaultCOC.clear();
	DefaultDice.clear();
	DefaultRule.clear();
	DisabledGroup.clear();
	DisabledDiscuss.clear();
	DisabledJRRPGroup.clear();
	DisabledJRRPDiscuss.clear();
	DisabledMEGroup.clear();
	DisabledMEDiscuss.clear();
	DisabledOBGroup.clear();
	DisabledOBDiscuss.clear();
	ObserveGroup.clear();
	ObserveDiscuss.clear();
	return 0;
}

EVE_Exit(eventExit)
{
	if (!Enabled)
		return 0;
	dataBackUp();
	return 0;
}

MUST_AppInfo_RETURN(CQAPPID);
