/*
 * Copyright (C) 2019 String.Empty
 */
#pragma once
#ifndef Dice_Console
#define Dice_Console
#include <string>
#include <vector>
#include <map>
#include <set>
#include <time.h>
#include <Windows.h>
#include "GlobalVar.h"
#include "DiceMsgSend.h"
#include "CQEVE_ALL.h"
#include "BlackMark.hpp"

	//Masterģʽ
	extern bool boolMasterMode;
	//Master��QQ������ʱΪ0
	extern long long masterQQ;
	extern long long DiceMaid;
	//����Ա�б�
	extern std::set<long long> AdminQQ;
	//��־�����б�
	extern std::set<chatType> RecorderList;
	//��ش����б�
	extern std::set<std::pair<long long, CQ::msgtype>> MonitorList;
	//����ȫ�ֿ���
	extern std::map<std::string, bool>boolConsole;
	//�����б�
	extern std::map<long long, long long> mDiceList;
	//���Ի����
	extern std::map<std::string, std::string> PersonalMsg;
	//botoff��Ⱥ
	extern std::set<long long> DisabledGroup;
	//botoff��������
	extern std::set<long long> DisabledDiscuss;
	//������Ⱥ��˽��ģʽ����
	extern std::set<long long> WhiteGroup;
	//������Ⱥ������������
	extern std::set<long long> BlackGroup;
	//�������û�������˽������
	extern std::set<long long> WhiteQQ;
	//�������û�������������
	extern std::set<long long> BlackQQ;
	extern std::map<long long, BlackMark>mBlackQQMark;
	extern std::map<long long, BlackMark>mBlackGroupMark;
//��������
void loadData();
//��������
void dataBackUp(); 
void loadBlackMark(std::string strPath);
void saveBlackMark(std::string strPath);
	//��ȡ�����б�
	void getDiceList();
	//֪ͨ����Ա 
	void sendAdmin(std::string strMsg, long long fromQQ = 0);
	//�����־
	void addRecord(std::string strMsg);
	//֪ͨ��ش��� 
	void NotifyMonitor(std::string strMsg);
	//һ������
	extern int clearGroup(std::string strPara = "unpower", long long fromQQ = 0);
	//�����Ϣ��¼
	extern std::map<chatType, time_t> mLastMsgList;
	//���ӵ����촰��
	extern std::map<chatType, chatType> mLinkedList;
	//����ת���б�
	extern std::multimap<chatType, chatType> mFwdList;
	//Ⱥ������
	extern std::map<long long,long long> mGroupInviter;
	//��������ʱ��
	extern long long llStartTime;
	//��ǰʱ��
	extern SYSTEMTIME stNow;
	//�ϰ�ʱ��
	extern std::pair<int, int> ClockToWork;
	//�°�ʱ��
	extern std::pair<int, int> ClockOffWork;
	std::string printClock(std::pair<int, int> clock);
	std::string printSTime(SYSTEMTIME st);
	std::string printQQ(long long);
	std::string printGroup(long long);
	std::string printChat(chatType);
//�����û�����Ⱥ
void checkBlackQQ(BlackMark &mark);
//�����û�
bool addBlackQQ(BlackMark mark);
bool addBlackGroup(BlackMark &mark);
void rmBlackQQ(long long llQQ, long long operateQQ = 0);
void rmBlackGroup(long long llQQ, long long operateQQ = 0);
extern std::set<std::string> strWarningList;
void AddWarning(const std::string& msg, long long DiceQQ, long long fromGroup);
void warningHandler();
	extern void ConsoleTimer();
#endif /*Dice_Console*/


