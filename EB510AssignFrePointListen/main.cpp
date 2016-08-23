#include <iostream>
#include "Controller.h"
#include "UdpExample.h"

using namespace std;
int main() {
	cout << "start..." << endl;

	Controller* controller = new Controller();
	controller->regulatingRevevierFrequency("140E3", "192.168.10.52", 5555);

	char* argvs[] = { "exe","-am","1", "-af", "d:\\test.wav", "192.168.10.52" };
	UdpExample* udpexample = new UdpExample();
	udpexample->udpMain(6, argvs);


	return 0;
}