#pragma once

#include <stdio.h>
#include "CmdSock.h"
class UdpExample {
public:
	int udpMain(int argc, char **argv);
	void stopWrite();

private:
	CCmdSock *pCmdSock;
};