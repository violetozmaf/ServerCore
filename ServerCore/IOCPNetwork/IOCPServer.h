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

	// 소켓초기화
	bool InitSocket(const UINT32 maxIOWorkerThreadCount_)
	{
		WSADATA wsaData;

		int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (0 != nRet)
		{
			printf_s("[Error] - WSAStartup() 실패 : %d\n", WSAGetLastError());
			return false;
		}

		//TCP, Overlapped I/O 소켓 생성
		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

		if (mListenSocket == INVALID_SOCKET)
		{
			printf_s("[Error] - WSASocket() 실패 : %d\n", WSAGetLastError());
			return false;
		}

		mMaxIOWorkerThreadCount = maxIOWorkerThreadCount_;

		printf_s("소켓 초기화 성공\n");
		return true;
	}

	bool BindListen(uint32_t nBindPort)
	{
		SOCKADDR_IN stServerAddr;
		stServerAddr.sin_family = AF_INET;
		stServerAddr.sin_port = htons(nBindPort);
		stServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);		// 모든 접속 받아들임

		// 서버 주소와 IOCompletionPort 소켓 연결
		int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
		if (nRet != 0)
		{
			printf_s("[Error] - Bind() 실패 : %d\n", WSAGetLastError());
			return false;
		}

		// 접속 요청 받아 들이기위해 IOCompletionPort 소켓 등록하고 대기 큐 설정
		nRet = listen(mListenSocket, 5);
		if (nRet != 0)
		{
			printf_s("[Error] - Listen() 실패 : %d\n", WSAGetLastError());
			return false;
		}

		// IOCompletionPort 객체만 생성
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxIOWorkerThreadCount);
		if (mIOCPHandle == NULL)
		{
			printf_s("[Error] - CreateIoCompletionPort() 생성 실패 : %d\n", GetLastError());
			return false;
		}

		// 위에서 생성한 IOCompletionPort 에 소켓 연결
		auto hIOCPHandle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, (UINT32)0, 0);
		if (hIOCPHandle == nullptr)
		{
			printf_s("[Error] - CreateIoCompletionPort() 에 소켓연결 실패 : %d\n", WSAGetLastError());
			return false;
		}

		printf_s("서버 등록 성공!!\n");
		return true;
	}


	bool StartServer(const uint32_t maxClientCount)
	{
		CreateClient(maxClientCount);

		// 접속된 클라이언트 주소 정보 저장할 구조체
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

		printf_s("서버 시작!!\n");
		return true;
	}

	// 생성되어 있는 쓰레드 파괴
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

		// Accept 쓰레드를 종료한다.
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

	// WaitingThread Queue 에서 대기할 쓰레드 생성
	bool CreateWorkerThread()
	{
		// WaitingThread Queue에서 대기할 워커 쓰레드 생성
		uint32_t uiThreadId = 0;

		try
		{
			// WaitingTHread Queue 에 대기 상태로 넣을 쓰레드들 생성 (cpu개수 * 2 + 1)
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

		printf_s("WorkerThread 시작\n");
		return true;
	}

	// accept요청을 처리하는 쓰레드 생성
	bool CreateAcceptThread()
	{
		mAcceptThread = std::thread([this]() { AcceptThread(); });
	
		printf_s("AcceptThread 시작\n");
		return true;
	}

	// 사용하지 않는 클라이언트 정보 구조체 반환
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

	// 클라이언트 세션정보 가져오기
	stClientInfo* GetClientInfo(const UINT32 sessionIndex)
	{
		return mClientInfos[sessionIndex];
	}


	// Overlapped I/O 작업에 대한 완료 통보를 받아 그에 해당하는 처리를 하는 함수
	void WorkerThread()
	{
		// CompletionKey 를 받을 포인터 변수
		stClientInfo* pClientInfo = nullptr;

		// 함수 호출 성공 여부
		bool bSuccess = true;

		// Overlapped I/O 작업에서 전송된 데이터 크기
		DWORD dwIoSize = 0;

		// I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
		LPOVERLAPPED lpOverlapped = nullptr;

		while (mIsWorkerRun)
		{
			// 쓰레드들은 WaitingThread Queue 에 대기상태로 들어가게 됨
			// 완료된 Overlapped I/O작업이 발생하면 IOCP Queue에서 완료된 작업을 가져와 처리
			// 그리고 PostQueuedCompletionStatus() 함수에 의해 사용자 메시지가 도착되면 쓰레드 종료
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
				&dwIoSize,										// 실제로 전송된 바이트
				(PULONG_PTR)&pClientInfo,						// CompletionKey
				&lpOverlapped,									// Overlapped IO 객체
				INFINITE										// 대기할 시간
			);

			// 사용자 쓰레드 종료 메시지 처리
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

			// Client 가 접속을 끊을때
			if (bSuccess == false || (dwIoSize == 0 && IOOperation::ACCEPT != pOverlappedEx->m_eOperation))
			{
				CloseSocket(pClientInfo);
				continue;
			}

			if (pOverlappedEx->m_eOperation == IOOperation::ACCEPT)
			{
				pClientInfo = GetClientInfo(pOverlappedEx->SessionIndex);
				// 클라이언트와 연결이 되면
				if (pClientInfo->AcceptCompletion())
				{
					// 클라이언트 갯수 증가
					mClientCount++;
					OnConnect(pClientInfo->GetIndex());
				}
				else
				{
					CloseSocket(pClientInfo, true);
				}
			}
			// Overlapped I/O Recv 작업 결과 처리
			else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
			{
				OnReceive(pClientInfo->GetIndex(), dwIoSize, pClientInfo->RecvBuffer());;
				pClientInfo->BindRecv();
			}
			// Overlapped I/O Send 작업 결과 처리
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				pClientInfo->SendCompleted(dwIoSize);
			}
			else
			{
				printf_s("socket(%d) 에서 예외발생\n", (int)pClientInfo->GetIndex());
			}
		}
	};


	// 접속 받는 Accept 쓰레드
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

				// 현재 시간보다 Client 의 마지막 종료시간보다 작으면
				if ((UINT64)curTimeSec < client->GetLatestClosedTimeSec())
				{
					continue;
				}

				auto diff = curTimeSec - client->GetLatestClosedTimeSec();
				if (diff <= RE_USE_SESSION_WAIT_TIMESEC)
				{
					continue;
				}

				// 종료되었으면 다시 초기화
				client->PostAccept(mListenSocket, curTimeSec);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	}

	// 소켓의 연결 종료
	void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false)
	{
		auto clientIndex = pClientInfo->GetIndex();
		pClientInfo->Close();
		OnClose(clientIndex);
	}

private:
	SOCKET						mListenSocket = INVALID_SOCKET;			// 클라이언트 받기위한 소켓
	std::vector<stClientInfo*>	mClientInfos;							// 클라이언트 저장 구조체
	uint32_t					mClientCount = 0;						// 접속되어 있는 클라이언트 수
	uint32_t					mMaxIOWorkerThreadCount = 0;
	std::vector<std::thread>	mIOWorkerThreads;						// IO Worker 쓰레드
	std::thread					mAcceptThread;							// Accept 쓰레드
	HANDLE						mIOCPHandle = INVALID_HANDLE_VALUE;		// CompletionPort 객체 핸들
	bool						mIsWorkerRun = true;					// 작업 쓰레드 동작 플래그
	bool						mIsAcceptRun = true;					// 접속 쓰레드 동작 플래그	
};

