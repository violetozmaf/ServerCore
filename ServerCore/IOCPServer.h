#pragma once
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <vector>

#define MAX_SOCKBUF 1024					// ��Ŷ ũ��
#define MAX_WORKERTHREAD 8					// ������ Ǯ�� ���� �������

enum class IOOperation
{
	RECV,
	SEND
};

// WSAOVERLAPPED ����ü Ȯ��
struct stOverlappedEx
{
	WSAOVERLAPPED		m_wsaOverlapped;			// Overlapped I/O ����ü
	SOCKET				m_socketClient;				// Ŭ���̾�Ʈ ����
	WSABUF				m_wsaBuf;					// Overlapped I/O �۾�����
	char				m_szBuf[MAX_SOCKBUF];		// ������ ����
	IOOperation			m_eOperation;				// �۾� ���� ����
};

// Ŭ���̾�Ʈ ������ ������� ����ü
struct stClientInfo
{
	SOCKET				m_socketClient;				// Ŭ���̾�Ʈ�� ����Ǵ� ����
	stOverlappedEx		m_stRecvOverlappedEx;		// Recv Overlapped I/O �۾�
	stOverlappedEx		m_stSendOverlappedEx;		// Send Overlapped I/O �۾�

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_socketClient = INVALID_SOCKET;
	}
};

class IOCPServer
{
public:
	IOCPServer(void) {};
	~IOCPServer(void) 
	{
		WSACleanup();
	}

	bool InitSocket(const uint32_t);					// �����ʱ�ȭ
	bool BindListen(uint32_t);							// Bind & Listen
	bool StartServer(const uint32_t);					// 
	

private:
	stClientInfo* GetEmptyClientInfo()
	{
		for (auto& client : mClientInfos)
		{
			if (client.m_socketClient == INVALID_SOCKET)
			{
				return &client;
			}
		}
		return nullptr;
	}

	// WaitingThread Queue ���� ����� ������ ����
	bool CreateWorkerThread()
	{
		// WaitingThread Queue���� ����� ��Ŀ ������ ����
		uint32_t uiThreadId = 0;

		try 
		{
			// WaitingTHread Queue �� ��� ���·� ���� ������� ���� (cpu���� * 2 + 1)
			for (int32_t i = 0; i < MAX_WORKERTHREAD; i++)
			{
				mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
			}
		}
		catch(const std::exception& e)
		{
			printf("[Exception] - exception message : %s\n", e.what());
			return false;
		}

		printf("WorkerThread ����\n");
		return true;
	}

	// accept��û�� ó���ϴ� ������ ����
	bool CreateAcceptThread()
	{
		try
		{
			mAcceptThread = std::thread([this]() { AcceptThread(); });
		}
		catch (const std::exception& e)
		{
			printf("[Exception] exception message : %s\n", e.what());
			return false;
		}
		
		printf("AcceptThread ����\n");
		return true;
	}

	void AcceptThread()
	{

	}

	// Overlapped I/O �۾��� ���� �Ϸ� �뺸�� �޾� �׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WorkerThread()
	{
		// CompletionKey �� ���� ������ ����
		stClientInfo* pClientInfo = nullptr;

		// �Լ� ȣ�� ���� ����
		bool bSuccess = true;

		// Overlapped I/O �۾����� ���۵� ������ ũ��
		DWORD dwIoSize = 0;

		// I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
		LPOVERLAPPED lpOverlapped = nullptr;

		while (mIsWorkerRun)
		{
			// ��������� WaitingThread Queue �� �����·� ���� ��
			// �Ϸ�� Overlapped I/O�۾��� �߻��ϸ� IOCP Queue���� �Ϸ�� �۾��� ������ ó��
			// �׸��� PostQueuedCompletionStatus() �Լ��� ���� ����� �޽����� �����Ǹ� ������ ����
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
				&dwIoSize,										// ������ ���۵� ����Ʈ
				(PULONG_PTR)&pClientInfo,						// CompletionKey
				&lpOverlapped,									// Overlapped IO ��ü
				INFINITE										// ����� �ð�
			);

			// ����� ������ ���� �޽��� ó��
			if (bSuccess == true && dwIoSize == 0 && lpOverlapped == nullptr)
			{
				mIsWorkerRun = false;
				continue;
			}

			if (lpOverlapped == nullptr)
			{
				continue;
			}

			// Client �� ������ ������
			if (bSuccess == false || (dwIoSize == 0 && bSuccess == true))
			{
				printf("socket(%d) ���� ����\n", (uint32_t)pClientInfo->m_socketClient);
				CloseSocket(pClientInfo);
				continue;
			}

			stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			// Overlapped I/O Recv �۾� ��� ó��
			if (IOOperation::RECV == pOverlappedEx->m_eOperation)
			{
				pOverlappedEx->m_szBuf[dwIoSize] = NULL;
				printf("[Recv] bytes : %d, msg : %s\n", dwIoSize, pOverlappedEx->m_szBuf);
				BindRecv(pClientInfo);
			}
			// Overlapped I/O Send �۾� ��� ó��
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				printf("[Send] bytes : %d, msg : %s\n", dwIoSize, pOverlappedEx->m_szBuf);
			}
			else
			{
				printf("socket(%d) ���� ���ܹ߻�\n", (int)pClientInfo->m_socketClient);
			}
		}
	};

private:
	SOCKET						mListenSocket = INVALID_SOCKET;			// Ŭ���̾�Ʈ �ޱ����� ����
	std::vector<stClientInfo>	mClientInfos;							// Ŭ���̾�Ʈ ���� ����ü
	uint32_t					mClientCount = 0;						// ���ӵǾ� �ִ� Ŭ���̾�Ʈ ��
	std::vector<std::thread>	mIOWorkerThreads;						// IO Worker ������
	std::thread					mAcceptThread;							// Accept ������
	HANDLE						mIOCPHandle = INVALID_HANDLE_VALUE;		// CompletionPort ��ü �ڵ�
	bool						mIsWorkerRun = true;					// �۾� ������ ���� �÷���
	bool						mIsAcceptRun = true;					// ���� ������ ���� �÷���
	char						mSocketBuffer[1024];					// ���� ����
};

