#pragma once
#ifndef __BOTENVIRONMENT__
#define __BOTENVIRONMENT__

// �������ʹ��Mirai����������ȫ��__MIRAI__���ȡ������һ���ע��
// #define __MIRAI__


#ifdef __MIRAI__
#pragma comment(lib, "CQP_Mirai.lib")
#else
#pragma comment(lib, "CQP.lib")
#endif

#endif