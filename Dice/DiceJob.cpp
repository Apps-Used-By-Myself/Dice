#include "DiceJob.h"
#include "DiceConsole.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#endif

#include "StrExtern.hpp"
#include "DDAPI.h"
#include "ManagerSystem.h"
#include "DiceCloud.h"
#include "BlackListManager.h"
#include "GlobalVar.h"
#include "CardDeck.h"
#include "DiceMod.h"
#include "DiceNetwork.h"
#include "DiceSession.h"
#include "S3PutObject.h"
#pragma warning(disable:28159)

using namespace std;

int sendSelf(const string& msg) {
	static long long selfQQ = DD::getLoginID();
	DD::sendPrivateMsg(selfQQ, msg);
	return 0;
}

void cq_exit(DiceJob& job) {
#ifdef _WIN32
	job.note("����" + getMsg("self") + "��5�����ɱ", 1);
	std::this_thread::sleep_for(5s);
	dataBackUp();
	DD::killme();
#endif
}

#ifdef _WIN32
inline PROCESSENTRY32 getProcess(int pid) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	Process32First(hParentProcess, &pe32);
	return pe32;
}
#endif

void frame_restart(DiceJob& job) {
#ifdef _WIN32
	if (!job.fromChat.uid) {
		if (console["AutoFrameRemake"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
		else if (int tWait{ console["AutoFrameRemake"] * 60 * 60 - int(time(nullptr) - llStartTime) }; tWait > 0) {
			sch.add_job_for(tWait, job);
			return;
		}
	}
	Enabled = false;
	dataBackUp();
	std::this_thread::sleep_for(3s);
	DD::remake();
#endif
}

void frame_reload(DiceJob& job) {
	if (DD::reload())
		job.note("����" + getMsg("self") + "��ɡ�", 1);
	else
		job.note("����" + getMsg("self") + "ʧ�ܡ�", 0b10);
}

void check_system(DiceJob& job) {
	DD::debugLog(printSTNow() + " ���ϵͳ����");
#ifdef _WIN32
	static int perRAM(0), perLastRAM(0);
	static double  perLastCPU(0), perLastDisk(0),
		perCPU(0), perDisk(0);
	static bool isAlarmRAM(false), isAlarmCPU(false), isAlarmDisk(false);
	static double mbFreeBytes = 0, mbTotalBytes = 0;
	//�ڴ���
	if (console["SystemAlarmRAM"] > 0) {
		perRAM = getRamPort();
		if (perRAM > console["SystemAlarmRAM"] && perRAM > perLastRAM) {
			console.log("���棺" + getMsg("strSelfName") + "����ϵͳ�ڴ�ռ�ô�" + to_string(perRAM) + "%", 0b1000, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = true;
		}
		else if (perLastRAM > console["SystemAlarmRAM"] && perRAM < console["SystemAlarmRAM"]) {
			console.log("���ѣ�" + getMsg("strSelfName") + "����ϵͳ�ڴ�ռ�ý���" + to_string(perRAM) + "%", 0b10, printSTime(stNow));
			perLastRAM = perRAM;
			isAlarmRAM = false;
		}
	}
	//CPU���
	if (console["SystemAlarmCPU"] > 0) {
		perCPU = getWinCpuUsage() / 10.0;
		if (perCPU > 99.9) {
			this_thread::sleep_for(10s);
			perCPU = getWinCpuUsage() / 10.0;
		}
		if (perCPU > console["SystemAlarmCPU"] && (!isAlarmCPU || perCPU > perLastCPU + 1)) {
			console.log("���棺" + getMsg("strSelfName") + "����ϵͳCPUռ�ô�" + toString(perCPU) + "%", 0b1000, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = true;
		}
		else if (perLastCPU > console["SystemAlarmCPU"] && perCPU < console["SystemAlarmCPU"]) {
			console.log("���ѣ�" + getMsg("strSelfName") + "����ϵͳCPUռ�ý���" + toString(perCPU) + "%", 0b10, printSTime(stNow));
			perLastCPU = perCPU;
			isAlarmCPU = false;
		}
	}
	//Ӳ�̼��
	if (console["SystemAlarmRAM"] > 0) {
		perDisk = getDiskUsage(mbFreeBytes, mbTotalBytes) / 10.0;
		if (perDisk > console["SystemAlarmDisk"] && (!isAlarmDisk || perDisk > perLastDisk + 1)) {
			console.log("���棺" + getMsg("strSelfName") + "����ϵͳӲ��ռ�ô�" + toString(perDisk) + "%", 0b1000, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = true;
		}
		else if (perLastDisk > console["SystemAlarmDisk"] && perDisk < console["SystemAlarmDisk"]) {
			console.log("���ѣ�" + getMsg("strSelfName") + "����ϵͳӲ��ռ�ý���" + toString(perDisk) + "%", 0b10, printSTime(stNow));
			perLastDisk = perDisk;
			isAlarmDisk = false;
		}
	}
	if (isAlarmRAM || isAlarmCPU || isAlarmDisk) {
		sch.add_job_for(5 * 60, job);
	}
	else {
		sch.add_job_for(30 * 60, job);
	}
#endif
}



void auto_save(DiceJob& job) {
	if (sch.is_job_cold("autosave"))return;
	DD::debugLog(printSTNow() + " �Զ�����");
	dataBackUp();
	//console.log(getMsg("strSelfName") + "���Զ�����", 0, printSTNow());
	if (console["AutoSaveInterval"] > 0) {
		sch.refresh_cold("autosave", time(NULL) + console["AutoSaveInterval"] * (time_t)60);
		sch.add_job_for(console["AutoSaveInterval"] * 60, "autosave");
	}
}

//�����õ�ͼƬ�б�
void clear_image(DiceJob& job) {
	if (!job.fromChat.uid) {
		if (sch.is_job_cold("clrimage"))return;
		if (console["AutoClearImage"] <= 0) {
			sch.add_job_for(60 * 60, job);
			return;
		}
	}
	scanImage(CardDeck::mPublicDeck, sReferencedImage);
	job.note("����" + getMsg("strSelfName") + "������ͼƬ" + to_string(sReferencedImage.size()) + "��", 0b0);
	int cnt = clrDir("data/image/", sReferencedImage);
	job.note("������image�ļ�" + to_string(cnt) + "��", 1);
	if (console["AutoClearImage"] > 0) {
		sch.refresh_cold("clrimage", time(NULL) + console["AutoClearImage"]);
		sch.add_job_for(console["AutoClearImage"] * 60 * 60, "clrimage");
	}
}

void clear_group(DiceJob& job) {
	console.log("��ʼ���Ⱥ��", 0, printSTNow());
	int intCnt = 0;
	ResList res;
	vector<long long> GrpDelete;
	time_t grpline{ console["InactiveGroupLine"] > 0 ? (tNow - console["InactiveGroupLine"] * (time_t)86400) : 0 };
	if (job.vars["clear_mode"] == "unpower") {
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("����") || grp.isset("Э����Ч"))continue;
			if (grp.isGroup && !DD::isGroupAdmin(id, console.DiceMaid, true)) {
				res << printGroup(id);
				time_t tLast{ grp.tUpdated };
				if (gm->has_session(id) && gm->session(id).tUpdate > grp.tUpdated)tLast = gm->session(id).tUpdate;
				if (tLast < grpline)GrpDelete.push_back(id);
				grp.leave(getMsg("strLeaveNoPower"));
				intCnt++;
				if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"])break;
				this_thread::sleep_for(3s);
			}
		}
		job.note(getMsg("strSelfName") + "ɸ����ȺȨ��Ⱥ��" + to_string(intCnt) + "��:" + res.show(), 0b10);
	}
	else if (isdigit(static_cast<unsigned char>(job.vars["clear_mode"].to_str()[0]))) {
		int intDayLim = stoi(job.vars["clear_mode"].to_str());
		string strDayLim = to_string(intDayLim);
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("����") || grp.isset("����") || grp.isset("Э����Ч"))continue;
			time_t tLast{ grp.tUpdated };
			if (long long tLMT; grp.isGroup && ((tLMT = DD::getGroupLastMsg(id, console.DiceMaid)) > 0 && tLMT > tLast))tLast = tLMT;
			if (gm->has_session(id) && gm->session(id).tUpdate > tLast)tLast = gm->session(id).tUpdate;
			if (!tLast)continue;
			if (tLast < grpline && !grp.isset("���"))GrpDelete.push_back(id);
			if (grp.isset("����") || grp.isset("δ��"))continue;
			int intDay = (int)(tNow - tLast) / 86400;
			if (intDay > intDayLim) {
				job["day"] = to_string(intDay);
				res << printGroup(id) + ":" + to_string(intDay) + "��\n";
				grp.leave(getMsg("strLeaveUnused", job.vars));
				intCnt++;
				if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"])break;
				this_thread::sleep_for(3s);
			}
		}
		job.note(getMsg("strSelfName") + "��ɸ��Ǳˮ" + strDayLim + "��Ⱥ��" + to_string(intCnt) + "����" + res.show(), 0b10);
	}
	else if (job.vars["clear_mode"] == "black") {
		try {
			for (auto id : DD::getGroupIDList()) {
				Chat& grp = chat(id).group().name(DD::getGroupName(id));
				if (grp.isset("����") || grp.isset("����") || grp.isset("���") || grp.isset("Э����Ч"))continue;
				if (blacklist->get_group_danger(id)) {
					time_t tLast{ grp.tUpdated };
					if (gm->has_session(id) && gm->session(id).tUpdate > grp.tUpdated)tLast = gm->session(id).tUpdate;
					if (tLast < grpline)GrpDelete.push_back(id);
					res << printGroup(id) + "��������Ⱥ";
					if (console["LeaveBlackGroup"])grp.leave(getMsg("strBlackGroup"));
				}
				set<long long> MemberList{ DD::getGroupMemberList(id) };
				int authSelf{ DD::getGroupAuth(id, console.DiceMaid, 1) };
				for (auto eachQQ : MemberList) {
					if (blacklist->get_qq_danger(eachQQ) > 1) {
						if (auto authBlack{ DD::getGroupAuth(id, eachQQ, 1) }; authBlack < authSelf) {
							continue;
						}
						else if (authBlack > authSelf) {
							if (grp.tUpdated < grpline)GrpDelete.push_back(id);
							res << printChat(grp) + "��" + printUser(eachQQ) + "�Է�ȺȨ�޽ϸ�";
							grp.leave("���ֺ���������Ա" + printUser(eachQQ) + "\n" + getMsg("strSelfName") + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
						else if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"]) {
							if(intCnt == console["GroupClearLimit"])res << "*���������Ѵ�����*";
							res << printChat(grp) + "��" + printUser(eachQQ);
						}
						else if (console["LeaveBlackQQ"]) {
							if (grp.tUpdated < grpline)GrpDelete.push_back(id);
							res << printChat(grp) + "��" + printUser(eachQQ);
							grp.leave("���ֺ�������Ա" + printUser(eachQQ) + "\n" + getMsg("strSelfName") + "��Ԥ������Ⱥ");
							intCnt++;
							break;
						}
					}
				}
			}
		} 		catch (...) {
			console.log("���ѣ�" + getMsg("strSelfName") + "��������Ⱥ��ʱ����", 0b10, printSTNow());
		}
		if (intCnt) {
			job.note("�Ѱ�" + getMsg("strSelfName") + "���������Ⱥ��" + to_string(intCnt) + "����" + res.show(), 0b10);
		}
		else if (job.fromChat.uid) {
			job.echo(getMsg("strSelfName") + "��������δ���ִ����Ⱥ��");
		}
	}
	else if (job["clear_mode"] == "preserve") {
		for (auto& [id, grp] : ChatList) {
			if (grp.isset("����") || grp.isset("����") || grp.isset("δ��") || grp.isset("���ʹ��") || grp.isset("����") || grp.isset("Э����Ч"))continue;
			if (grp.isGroup && DD::isGroupAdmin(id, console.master(), false)) {
				grp.set("���ʹ��");
				continue;
			}
			time_t tLast{ grp.tUpdated };
			if (gm->has_session(id) && gm->session(id).tUpdate > grp.tUpdated)tLast = gm->session(id).tUpdate;
			if (tLast < grpline)GrpDelete.push_back(id);
			res << printChat(grp);
			grp.leave(getMsg("strPreserve"));
			intCnt++;
			if (console["GroupClearLimit"] > 0 && intCnt >= console["GroupClearLimit"])break;
			this_thread::sleep_for(3s);
		}
		job.note(getMsg("strSelfName") + "ɸ�������Ⱥ��" + to_string(intCnt) + "����" + res.show(), 1);
	}
	else
		job.echo("�޷�ʶ��ɸѡ������");
	if (!GrpDelete.empty()) {
		for (const auto& id : GrpDelete) {
			ChatList.erase(id);
			if (gm->has_session(id))gm->session_end(id);
		}
		job.note("���Ⱥ��ʱ���ղ���Ծ��¼" + to_string(GrpDelete.size()) + "��", 0b1);
	}
}
void list_group(DiceJob& job) {
	console.log("����Ⱥ�б�", 0, printSTNow());
	string mode{ job["list_mode"].to_str() };
	if (mode.empty()) {
		job.reply(fmt->get_help("groups_list"));
	}
	if (mChatConf.count(mode)) {
		ResList res;
		for (auto& [id, grp] : ChatList) {
			if (grp.isset(mode)) {
				res << printChat(grp);
			}
		}
		job.reply("{self}������" + mode + "Ⱥ��¼" + to_string(res.size()) + "��" + res.head(":").show());
	}
	else if (set<long long> grps(DD::getGroupIDList()); mode == "idle") {
		std::priority_queue<std::pair<time_t, string>> qDiver;
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (grp.isGroup && !grps.empty() && !grps.count(id))grp.set("����");
			if (grp.isset("����") || grp.isset("δ��"))continue;
			time_t tLast = grp.tUpdated;
			if (long long tLMT; grp.isGroup && (tLMT = DD::getGroupLastMsg(grp.ID, console.DiceMaid)) > 0 && tLMT > tLast)tLast = tLMT;
			if (!tLast)continue;
			int intDay = (int)(tNow - tLast) / 86400;
			qDiver.emplace(intDay, printGroup(id));
		}
		if (qDiver.empty()) {
			job.reply("{self}��Ⱥ�Ļ�Ⱥ��Ϣ����ʧ�ܣ�");
		}
		size_t intCnt(0);
		ResList res;
		while (!qDiver.empty()) {
			res << qDiver.top().second + to_string(qDiver.top().first) + "��";
			qDiver.pop();
			if (++intCnt > 32 || qDiver.top().first < 7)break;
		}
		job.reply("{self}��������Ⱥ�б�:" + res.show(1));
	}
	else if (job["list_mode"] == "size") {
		std::priority_queue<std::pair<time_t, string>> qSize;
		time_t tNow = time(NULL);
		for (auto& [id, grp] : ChatList) {
			if (grp.isGroup && !grps.empty() && !grps.count(id))grp.set("����");
			if (grp.isset("����") || grp.isset("δ��") || !grp.isGroup)continue;
			GroupSize_t size(DD::getGroupSize(id));
			if (!size.currSize)continue;
			qSize.emplace(size.currSize, DD::printGroupInfo(id));
		}
		if (qSize.empty()) {
			job.reply("{self}��Ⱥ�Ļ�Ⱥ��Ϣ����ʧ�ܣ�");
		}
		size_t intCnt(0);
		ResList res;
		while (!qSize.empty()) {
			res << qSize.top().second;
			qSize.pop();
			if (++intCnt > 32 || qSize.top().first < 7)break;
		}
		job.reply("{self}���ڴ�Ⱥ�б�:" + res.show(1));
	}
}

//�������
void cloud_beat(DiceJob& job) {
	Cloud::heartbeat();
	sch.add_job_for(5 * 60, job);
}

void dice_update(DiceJob& job) {
	job.note("��ʼ����Dice\n�汾:" + job.vars["ver"].to_str(), 1);
	string ret;
	if (DD::updateDice(job.vars["ver"].to_str(), ret)) {
		job.note("����Dice!" + job.vars["ver"].to_str() + "��ɹ���", 1);
	}
	else {
		job.echo("����ʧ��:" + ret);
	}
}

//��ȡ�Ʋ�����¼
void dice_cloudblack(DiceJob& job) {
	bool isSuccess(false);
	job.note("��ʼ��ȡ�Ʋ�����¼", 0);
	string strURL("https://shiki.stringempty.xyz/blacklist/checked.json?" + to_string(job.fromTime));
	switch (Cloud::DownloadFile(strURL.c_str(), DiceDir / "conf" / "CloudBlackList.json")) {
	case -1: {
		string des;
		if (Network::GET("http://shiki.stringempty.xyz/blacklist/checked.json", des)) {
			ofstream fout(DiceDir / "conf" / "CloudBlackList.json");
			fout << des << endl;
			isSuccess = true;
		}
		else
			job.echo("ͬ���Ʋ�����¼ͬ��ʧ��:" + des);
	}
		   break;
	case -2:
		job.echo("ͬ���Ʋ�����¼ͬ��ʧ��!�ļ�δ�ҵ�");
		break;
	case 0:
		isSuccess = true;
		break;
	default:
		break;
	}
	if (isSuccess) {
		if (job["uid"])job.note("ͬ���Ʋ�����¼�ɹ���" + getMsg("self") + "��ʼ��ȡ", 1);
		blacklist->loadJson(DiceDir / "conf" / "CloudBlackList.json", true);
	}
	if (console["CloudBlackShare"])
		sch.add_job_for(24 * 60 * 60, "cloudblack");
}

void log_put(DiceJob& job) {
	if (!job.cntExec) {
		DD::debugLog("����log�ļ�:" + job.vars["log_path"].to_str());
		if ((!job.fromChat.gid || !DD::uploadGroupFile(job.fromChat.gid, job.vars["log_path"].to_str()))
			&& job.fromChat.uid) {
			DD::sendFriendFile(job.fromChat.uid, job.vars["log_path"].to_str());
		}
	}
	job["ret"] = put_s3_object("dicelogger",
							   job.vars["log_file"].to_str().c_str(),
							   job.vars["log_path"].to_str().c_str(),
							   "ap-southeast-1");
	if (job["ret"] == "SUCCESS") {
		job.echo(getMsg("strLogUpSuccess", job.vars));
	}
	else if (job.cntExec++ > 2) {
		job.echo(getMsg("strLogUpFailureEnd", job.vars));
	}
	else {
		job["retry"] = to_string(job.cntExec);
		job.echo(getMsg("strLogUpFailure", job.vars));
		console.log(getMsg("strLogUpFailure", job.vars), 1);
		sch.add_job_for(2 * 60, job);
	}
}


string print_master() {
	if (!console.master())return "��������";
	return printUser(console.master());
}

string list_deck() {
	return listKey(CardDeck::mPublicDeck);
}
string list_extern_deck() {
	return listKey(CardDeck::mExternPublicDeck);
}
string list_order_ex() {
	return fmt->list_order();
}
string list_dice_sister() {
	std::set<long long>list{ DD::getDiceSisters() };
	if (list.size() <= 1)return {};
	else {
		list.erase(console.DiceMaid);
		ResList li;
		li << printUser(console.DiceMaid) + "�Ľ�����:";
		for (auto dice : list) {
			li << printUser(dice);
		}
		return li.show();
	}
}