#pragma once

/*
 * Copyright (C) 2019-2020 String.Empty
 * ����ʱ�¼�
 * �����ܼ�ʱ��ɵ�ָ��
 */

#include <string>
#include <map>
#include <unordered_map>
#include "DiceMsgSend.h"
#include "Jsonio.h"
#include "json.hpp"
#include "DiceAttrVar.h"

using std::string;
using std::map;
using std::unordered_map;
using std::shared_ptr;

struct DiceJobDetail : public std::enable_shared_from_this<DiceJobDetail> {
    chatInfo fromChat;
    string cmd_key;
    string strMsg;
    time_t fromTime = time(nullptr);
    size_t cntExec{ 0 };
    //��ʱ������
    unordered_map<string, AttrVar> vars = {};
    DiceJobDetail(const char* cmd, bool isFromSelf = false, const AttrVars& vars = {});
    DiceJobDetail(const  unordered_map<string, AttrVar>& var, chatInfo ct = {}) :vars(var), fromChat(ct){}
    virtual void reply(const char*, bool = true) {}
    virtual void reply(const string&, bool = true) {}
    AttrVar& operator[](const char* key){
        return vars[key];
    }
    bool operator<(const DiceJobDetail& other)const {
        return cmd_key < other.cmd_key;
    }
};

class DiceJob : public DiceJobDetail {
    enum class Renum { NIL, Retry_For, Retry_Until };
public:
    DiceJob(DiceJobDetail detail) :DiceJobDetail(detail) {}
    Renum ren = Renum::NIL;
    void exec();
    void echo(const std::string&);
    void reply(const std::string&);
    void note(const std::string&, int);
};

class DiceScheduler {
    //�¼���ȴ��
    unordered_map<string, time_t> untilJobs;
public:
    void start();
    void end();
    void push_job(const DiceJobDetail&);
    void push_job(const char*, bool = false, const AttrVars& = {});
    void add_job_for(unsigned int, const DiceJobDetail&);
    void add_job_for(unsigned int, const char*);
    void add_job_until(time_t, const DiceJobDetail&);
    void add_job_until(time_t, const char*);
    bool is_job_cold(const char*);
    void refresh_cold(const char*, time_t);
};
inline DiceScheduler sch;

typedef void (*cmd)(DiceJob&);

//���ռ�¼
class DiceToday {
    tm stToday;
    std::filesystem::path pathFile;
    unordered_map<string, int>cntGlobal;
    unordered_map<long long, unordered_map<string, int>>cntUser;
public:
    DiceToday(const std::filesystem::path& path) :pathFile(path) {
        load();
    }
    void load();
    void save();
    void set(long long qq, const string& key, int cnt) { cntUser[qq][key] = cnt; save(); }
    void inc(const string& key) { cntGlobal[key]++; save(); }
    void inc(long long qq, const string& key, int cnt = 1) { cntUser[qq][key] += cnt; save(); }
    int& get(const string& key) { return cntGlobal[key]; }
    int& get(long long qq, const string& key) { return cntUser[qq][key]; }
    int getJrrp(long long qq);
    size_t cnt(const string& key = "") { return cntUser.size(); }
    void daily_clear();
};
inline std::unique_ptr<DiceToday> today;

string printTTime(time_t tt);