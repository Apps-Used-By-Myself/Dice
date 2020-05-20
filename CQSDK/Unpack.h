#pragma once
#include <vector>
#include <string>

void show(void* t, int len) noexcept;

class Unpack final
{
	std::vector<unsigned char> buff;
public:
	Unpack() noexcept;
	explicit Unpack(const char*) noexcept;
	explicit Unpack(std::vector<unsigned char>) noexcept;
	explicit Unpack(const std::string&) noexcept;

	Unpack& setData(const char* i, int len) noexcept;
	Unpack& clear() noexcept;
	int len() const noexcept;

	Unpack& add(int i) noexcept; //���һ������
	int getInt() noexcept; //����һ������

	Unpack& add(long long i) noexcept; //���һ��������
	long long getLong() noexcept; //����һ��������

	Unpack& add(short i) noexcept; //���һ��������
	short getshort() noexcept; //����һ��������

	Unpack& add(const unsigned char* i, short len) noexcept; //���һ���ֽڼ�(����add(std::string i);)
	std::vector<unsigned char> getchars() noexcept; //����һ���ֽڼ�(����getstring();)

	Unpack& add(std::string i) noexcept; //���һ���ַ���
	std::string getstring() noexcept; //����һ���ַ���

	Unpack& add(Unpack& i) noexcept; //���һ��Unpack
	Unpack getUnpack() noexcept; //����һ��Unpack

	std::string getAll() noexcept; //���ر�������

	void show() noexcept;
};
