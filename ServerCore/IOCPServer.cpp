#include "IOCPServer.h"

bool IOCPServer::InitSocket(const uint32_t port)
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[Error] - WSAStartup() ���� : %d\n", WSAGetLastError());
		return false;
	}

	//TCP, Overlapped I/O ���� ����
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (mListenSocket == INVALID_SOCKET)
	{
		printf("[Error] - WSASocket() ���� : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}


bool IOCPServer::BindListen(uint32_t nBindPort)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(nBindPort);
	stServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// IOCompletionPort ���� ����
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		printf("[Error] - Bind() ���� : %d\n", WSAGetLastError());
		return false;
	}

	// ���� ��û �޾� ���̱����� IOCompletionPort ���� ����ϰ� ��� ť ����
	nRet = listen(mListenSocket, 5);
	if (nRet != 0)
	{
		printf("[Error] - Listen() ���� : %d\n", WSAGetLastError());
		return false;
	}

	printf("BindListen ��� ����!!\n");
	return true;
}

bool IOCPServer::StartServer(const uint32_t maxClientCount)
{
	for (uint32_t i = 0; i < maxClientCount; i++)
	{
		mClientInfos.emplace_back();
	}

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (mIOCPHandle == NULL)
	{
		printf("[Error] - CreateIoCompletionPort() ���� : %d\n", WSAGetLastError());
		return false;
	}

	bool bRet = CreateWorkerThread();

	return true;
}
