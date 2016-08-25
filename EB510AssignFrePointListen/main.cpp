#include <iostream>
#include "Controller.h"
#include "UdpExample.h"

using namespace std;
int main() {
	cout << "start..." << endl;

	Controller* controller = new Controller();
	controller->regulatingRevevierFrequency("140E3", "192.168.10.52", 5556);

	char* argvs[] = { "exe","-am","1", "-p","5556", "-af", "d:\\test2.wav", "10", "192.168.10.52" };

	UdpExample* udpexample = new UdpExample();
	udpexample->udpMain(9, argvs);


	return 0;
}