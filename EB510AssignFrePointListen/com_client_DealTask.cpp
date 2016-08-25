#include "com_client_DealTask.h"
#include "Controller.h"
#include "UdpExample.h"

/*
* Class:     com_client_DealTask
* Method:    getUdpExampleAddr
* Signature: ()J
*/
JNIEXPORT jlong JNICALL Java_com_client_DealTask_getUdpExampleAddr(JNIEnv *env, jobject obj) {
	UdpExample* udpexample = new UdpExample();
	return (__int64)udpexample;
}

/*
* Class:     com_client_DealTask
* Method:    stopCurrentTask
* Signature: (J)V
*/
JNIEXPORT void JNICALL Java_com_client_DealTask_stopCurrentTask(JNIEnv *env, jobject obj, jlong udpExampleAddr) {
	UdpExample* udpexample = (UdpExample*)udpExampleAddr;
	udpexample->stopWrite();
}

/*
* Class:     com_client_DealTask
* Method:    doTask
* Signature: (JLjava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V
*/
JNIEXPORT void JNICALL Java_com_client_DealTask_doTask(JNIEnv *env, jobject obj, jlong udpExampleAddr, jstring receiverIp, jint receiverPort, jstring frequence, jstring filePath, jstring fileTotalTime) {
	const char* ip = env->GetStringUTFChars(receiverIp, 0);
	const char* freq = env->GetStringUTFChars(frequence, 0);
	const char* path = env->GetStringUTFChars(filePath, 0);
	const char* fileTime = env->GetStringUTFChars(fileTotalTime, 0);

	char* t_path = new char[strlen(path) + 1];
	char* t_fileTime = new char[strlen(fileTime) + 1];
	char* t_ip = new char[strlen(ip) + 1];
	char* t_port = new char[6];

	strcpy(t_path, path);
	strcpy(t_fileTime, fileTime);
	strcpy(t_ip, ip);
	sprintf(t_port, "%d", receiverPort);
	
	Controller* controller = new Controller();
	controller->regulatingRevevierFrequency(freq, ip, receiverPort);

	//char* argvs[] = { "exe","-am","1", "-p","5556", "-af", "d:\\test2.wav", "10", "192.168.10.52" };
	const int argvsNum = 9;
	char * argvs[argvsNum] = { "exe","-am","1", "-p", t_port, "-af", t_path, t_fileTime, t_ip};

	UdpExample* udpexample = (UdpExample*)udpExampleAddr;
	udpexample->udpMain(argvsNum, argvs);

	//delete
	env->ReleaseStringUTFChars(receiverIp, ip);
	env->ReleaseStringUTFChars(frequence, freq);
	env->ReleaseStringUTFChars(filePath, path);
	env->ReleaseStringUTFChars(fileTotalTime, fileTime);
}
