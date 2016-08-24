#include <WinSock2.h>
#include "Controller.h"

#pragma comment(lib, "ws2_32.lib")

void Controller::regulatingRevevierFrequency(char* frequence, const char* receiverIp, int receiverPort) {
	//加载套接字
	WSADATA wsaData;//WSADATA结构体中主要包含了系统所支持的Winsock版本信息
	char buff[1024];
	memset(buff, 0, sizeof(buff));

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)//初始化Winsock 2.2
	{
		printf("Failed to load Winsock");
		return;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(receiverPort);
	addrSrv.sin_addr.S_un.S_addr = inet_addr(receiverIp);

	//创建套接字
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == sockClient) {
		printf("Socket() error:%d", WSAGetLastError());
		return;
	}

	//向服务器发出连接请求
	if (connect(sockClient, (struct  sockaddr*)&addrSrv, sizeof(addrSrv)) == INVALID_SOCKET) {
		printf("Connect failed:%d", WSAGetLastError());
		return;
	}
	else
	{
		int i = 1;
		setsockopt(sockClient, IPPROTO_TCP, TCP_NODELAY, (char*)&i, sizeof(i));
		//发送数据
		//char sendBuff[] = "";
		//send(sockClient, sendBuff, sizeof(sendBuff), 0);
		char fre[1024] = {0};
		sprintf(fre, "FREQ %s\n", frequence);
		printf("%s", fre);
		send(sockClient, fre, strlen(fre), 0);
		send(sockClient, "BAND 150 khz\n", 13, 0);
		send(sockClient, "DEM AM\n", 7, 0);

		////接收数据
		//recv(sockClient, buff, sizeof(buff), 0);
		//printf("%s\n", buff);
	}

	//关闭套接字
	closesocket(sockClient);
	WSACleanup();
}