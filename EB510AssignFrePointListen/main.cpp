#include <iostream>
#include "Controller.h"
#include "UdpExample.h"

using namespace std;
int main() {
	cout << "start..." << endl;

	Controller* controller = new Controller();
	controller->regulatingRevevierFrequency("140E3", "192.168.10.52", 5555);

	char* argvs[] = { "exe","-am","1", "-af", "d:\\test.wav", "10", "192.168.10.52" };

	for (char * argv : argvs)
	{
		printf("%s\n", argv);
	}

	UdpExample* udpexample = new UdpExample();
	udpexample->udpMain(7, argvs);


	return 0;
}