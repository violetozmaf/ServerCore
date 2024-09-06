#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock.lib")

#include <thread>
#include <vector>

#include "Define.h"
#include "ClientInfo.h"


class IOCPServer
{
public:
	IOCPServer(void) {}
	~IOCPServer(void) 
	{
		WSACleanup();
	}

	// �����ʱ�ȭ
	bool InitSocket(const UINT32 maxIOWorkerThreadCount_)
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf_s("[Error] - WSAStartup() ���� : %d\n", WSAGetLastError());
			return false;
		}

		//TCP, Overlapped I/O ���� ����
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (mListenSocket == INVALID_SOCKET)
		{
			printf_s("[Error] - WSASocket() ���� : %d\n", WSAGetLastError());
			return false;
		}

		mMaxIOWorkerThreadCount = maxIOWorkerThreadCount_;

		printf_s("���� �ʱ�ȭ ����\n");
		return true;
	}

	bool BindListen(uint32_t nBindPort)
	{
		SOCKADDR_IN stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort);
		stServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);		// ��� ���� �޾Ƶ���

		// ���� �ּҿ� IOCompletionPort ���� ����
		int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (nRet != 0)
		{
			printf_s("[Error] - Bind() ���� : %d\n", WSAGetLastError());
			return false;
		}

		// ���� ��û �޾� ���̱����� IOCompletionPort ���� ����ϰ� ��� ť ����
		nRet = listen(mListenSocket, 5);
		if (nRet != 0)
		{
			printf_s("[Error] - Listen() ���� : %d\n", WSAGetLastError());
			return false;
		}

		// IOCompletionPort ��ü�� ����
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxIOWorkerThreadCount);
		if (mIOCPHandle == NULL)
		{
			printf_s("[Error] - CreateIoCompletionPort() ���� ���� : %d\n", GetLastError());
			return false;
		}

		// ������ ������ IOCompletionPort �� ���� ����
		auto hIOCPHandle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);
		if (hIOCPHandle == nullptr)
		{
			printf_s("[Error] - CreateIoCompletionPort() �� ���Ͽ��� ���� : %d\n", WSAGetLastError());
			return false;
		}

		printf_s("���� ��� ����!!\n");
		return true;
	}


	bool StartServer(const uint32_t maxClientCount)
	{
		CreateClient(maxClientCount);

		// ���ӵ� Ŭ���̾�Ʈ �ּ� ���� ������ ����ü
		bool bRet = CreateWorkerThread();
		if (bRet == false)
		{
			return false;
		}

		bRet = CreateAcceptThread();
		if (bRet == false)
		{
			return false;
		}

		printf_s("���� ����!!\n");
		return true;
	}

	// �����Ǿ� �ִ� ������ �ı�
	void DestroyThread()
	{
		mIsWorkerRun = false;
		CloseHandle(mIOCPHandle);

		for (auto& th : mIOWorkerThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		// Accept �����带 �����Ѵ�.
		mIsAcceptRun = false;
		closesocket(mListenSocket);

		if (mAcceptThread.joinable())
		{
			mAcceptThread.join();
		}
	}

	bool SendMsg(const UINT32 sessionIndex_, const UINT32 dataSize_, char* pData)
	{
		auto pClient = GetClientInfo(sessionIndex_);
		return pClient->SendMsg(dataSize_, pData);
	}

	virtual void OnConnect(const UINT32 clientIndex_) {};
	virtual void OnClose(const UINT32 clientIndex_) {};
	virtual void OnReceive(const UINT32 clientINdex_, const UINT32 size_, char* pData_) {};

private:
	
	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; i++)
		{
			auto client = new stClientInfo;
			client->Init(i, mIOCPHandle);

			mClientInfos.push_back(client);
		}
	}

	// WaitingThread Queue ���� ����� ������ ����
	bool CreateWorkerThread()
	{
		// WaitingThread Queue���� ����� ��Ŀ ������ ����
		uint32_t uiThreadId = 0;

		try
		{
			// WaitingTHread Queue �� ��� ���·� ���� ������� ���� (cpu���� * 2 + 1)
			for (int32_t i = 0; i < (int32_t)mMaxIOWorkerThreadCount; i++)
			{
				mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
			}
		}
		catch (const std::exception& e)
		{
			printf_s("[Exception] - exception message : %s\n", e.what());
			return false;
		}

		printf_s("WorkerThread ����\n");
		return true;
	}

	// accept��û�� ó���ϴ� ������ ����
	bool CreateAcceptThread()
	{
		mAcceptThread = std::thread([this]() { AcceptThread(); });
	
		printf_s("AcceptThread ����\n");
		return true;
	}

	// ������� �ʴ� Ŭ���̾�Ʈ ���� ����ü ��ȯ
	stClientInfo* GetEmptyClientInfo()
	{
		for (auto& client : mClientInfos)
		{
			if (client->IsConnectd() == false)
			{
				return client;
			}
		}
		return nullptr;
	}

	// Ŭ���̾�Ʈ �������� ��������
	stClientInfo* GetClientInfo(const UINT32 sessionIndex)
	{
		return mClientInfos[sessionIndex];
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

			auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			// Client �� ������ ������
			if (bSuccess == false || (dwIoSize == 0 && IOOperation::ACCEPT != pOverlappedEx->m_eOperation))
			{
				CloseSocket(pClientInfo);
				continue;
			}

			if (pOverlappedEx->m_eOperation == IOOperation::ACCEPT)
			{
				pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);
				// Ŭ���̾�Ʈ�� ������ �Ǹ�
				if (pClientInfo->AcceptCompletion())
				{
					// Ŭ���̾�Ʈ ���� ����
					mClientCount++;
					OnConnect(pClientInfo->GetIndex());
				}
				else
				{
					CloseSocket(pClientInfo, true);
				}
			}
			// Overlapped I/O Recv �۾� ��� ó��
			else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
			{
				OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());;
				pClientInfo->BindRecv();
			}
			// Overlapped I/O Send �۾� ��� ó��
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				pClientInfo->SendCompleted(dwIoSize);
			}
			else
			{
				printf_s("socket(%d) ���� ���ܹ߻�\n", (int)pClientInfo->GetIndex());
			}
		}
	};


	// ���� �޴� Accept ������
	void AcceptThread()
	{
		while (mIsAcceptRun)
		{
			auto curTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

			for (auto client : mClientInfos)
			{
				if (client->IsConnectd())
				{ 
					continue;
				}

				// ���� �ð����� Client �� ������ ����ð����� ������
				if ((UINT64)curTimeSec < client->GetLatestClosedTimeSec())
				{
					continue;
				}

				auto diff = curTimeSec - client->GetLatestClosedTimeSec();
				if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
				{
					continue;
				}

				// ����Ǿ����� �ٽ� �ʱ�ȭ
				client->PostAccept(mListenSocket, curTimeSec);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	}

	// ������ ���� ����
	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
	{
		auto clientIndex = pClientInfo->GetIndex();
		pClientInfo->Close();
		OnClose(clientIndex);
	}

private:
	SOCKET						mListenSocket = INVALID_SOCKET;			// Ŭ���̾�Ʈ �ޱ����� ����
	std::vector<stClientInfo*>	mClientInfos;							// Ŭ���̾�Ʈ ���� ����ü
	uint32_t					mClientCount = 0;						// ���ӵǾ� �ִ� Ŭ���̾�Ʈ ��
	uint32_t					mMaxIOWorkerThreadCount = 0;
	std::vector<std::thread>	mIOWorkerThreads;						// IO Worker ������
	std::thread					mAcceptThread;							// Accept ������
	HANDLE						mIOCPHandle = INVALID_HANDLE_VALUE;		// CompletionPort ��ü �ڵ�
	bool						mIsWorkerRun = true;					// �۾� ������ ���� �÷���
	bool						mIsAcceptRun = true;					// ���� ������ ���� �÷���	
};

