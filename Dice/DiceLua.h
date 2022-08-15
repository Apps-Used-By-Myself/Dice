/**
 * lua�ű�Ƕ��
 * �����Զ���ǰ׺ָ���
 * Copyright (C) 2019-2021 String.Empty
 */
#pragma once
#include <string>
#include <unordered_map>

class Lua_State;
class FromMsg;
bool lua_msg_call(FromMsg*, const AttrVar&);
bool lua_call_event(AttrObject eve, const AttrVar&);
bool lua_call_task(const AttrVars&);
int lua_readStringTable(const char*, const char*, std::unordered_map<std::string, AttrVar>&);