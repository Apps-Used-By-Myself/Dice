/*
 * ������￨
 * Copyright (C) 2019-2020 String.Empty
 */
#include "CharacterCard.h"

Player& getPlayer(long long qq) {
	if (!PList.count(qq))PList[qq] = {};
	return PList[qq];
}

string getPCName(long long qq, long long group) {
	if (PList.count(qq) && PList[qq][group].Name != "��ɫ��")return PList[qq][group].Name;
	return getName(qq, group);
}