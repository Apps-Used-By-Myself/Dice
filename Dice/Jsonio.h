#pragma once
#include <fstream>
#include <map>
#include <vector>
#include "json.hpp"
#include "EncodingConvert.h" 

template<typename T>
typename std::enable_if<!std::is_arithmetic<T>::value, T>::type readJson(std::string strJson) {
	return UTF8toGBK(strJson);
}

template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value, T>::type readJson(std::string strJson) {
	return stoll(strJson);
}

template<typename T>
std::vector<T> readJVec(std::vector<std::string> vJson) {
	std::vector<T> vTmp;
	for (auto it : vJson) {
		vTmp.push_back(readJson<T>(it));
	}
	return vTmp;
}
template<typename T1, typename T2>
int readJson(std::string strJson, std::map<T1, T2> &mapTmp) {
	nlohmann::json j;
	int intCnt;
	try {
		j = nlohmann::json::parse(strJson);
	}
	catch (...) {
		return -1;
	}
	for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it)
	{
		T1 tKey = readJson<T1>(it.key());
		T2 tVal = readJson<T2>(it.value());
		mapTmp[tKey] = tVal;
		intCnt++;
	}
	return intCnt;
}

template<typename T1, typename T2>
int loadJMap(std::string strLoc, std::map<T1, std::vector<T2>> &mapTmp) {
	std::ifstream fin(strLoc);
	if (fin) {
		nlohmann::json j;
		try {
			fin >> j;
		}catch(...){
			return -1;
		}
		for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it)
		{
			T1 tKey = readJson<T1>(it.key());
			std::vector <T2> tVal = readJVec<T2>(it.value());
			mapTmp[tKey] = tVal;
		}
		fin.close();
	}
	return 0;
}
template<typename T>
std::string writeJson(typename std::enable_if<!std::is_arithmetic<T>::value, T>::type strJson) {
	return GBKtoUTF8(strJson);
}

template<typename T>
std::string writeJson(typename std::enable_if<std::is_arithmetic<T>::value, T>::type llJson) {
	return std::to_string(llJson);
}

template<typename T>
std::vector<std::string> writeJVec(std::vector<T> vJson) {
	std::vector<std::string> vTmp;
	for (auto it : vJson) {
		vTmp.push_back(writeJson<T>(it));
	}
	return vTmp;
}

template<typename T1, typename T2>
int saveJMap(std::string strLoc, std::map<T1, std::vector<T2>> mapTmp) {
	if (mapTmp.empty())return 0;
	std::ofstream fout(strLoc);
	if (fout) {
		nlohmann::json j;
		for (auto it : mapTmp) {
			j[writeJson<T1>(it.first)] = writeJVec<T2>(it.second);
		}
		fout << j;
		fout.close();
	}
	return 0;
}