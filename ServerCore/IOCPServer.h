#pragma once
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <vector>

#define MAX_SOCKBUF 1024					// 패킷 크기
#define MAX_WORKERTHREAD 8					// 쓰레드 풀에 넣을 쓰레드수

enum class IOOperation
{
	RECV,
	SEND
};

// WSAOVERLAPPED 구조체 확장
struct stOverlappedEx
{
	WSAOVERLAPPED		m_wsaOverlapped;			// Overlapped I/O 구조체
	SOCKET				m_socketClient;				// 클라이언트 소켓
	WSABUF				m_wsaBuf;					// Overlapped I/O 작업버퍼
	char				m_szBuf[MAX_SOCKBUF];		// 데이터 버퍼
	IOOperation			m_eOperation;				// 작업 동작 종류
};

// 클라이언트 정보를 담기위한 구조체
struct stClientInfo
{
	SOCKET				m_socketClient;				// 클라이언트와 연결되는 소켓
	stOverlappedEx		m_stRecvOverlappedEx;		// Recv Overlapped I/O 작업
	stOverlappedEx		m_stSendOverlappedEx;		// Send Overlapped I/O 작업

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

	bool InitSocket(const uint32_t);					// 소켓초기화
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

	// WaitingThread Queue 에서 대기할 쓰레드 생성
	bool CreateWorkerThread()
	{
		// WaitingThread Queue에서 대기할 워커 쓰레드 생성
		uint32_t uiThreadId = 0;

		try 
		{
			// WaitingTHread Queue 에 대기 상태로 넣을 쓰레드들 생성 (cpu개수 * 2 + 1)
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

		printf("WorkerThread 시작\n");
		return true;
	}

	// accept요청을 처리하는 쓰레드 생성
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
		
		printf("AcceptThread 시작\n");
		return true;
	}

	void AcceptThread()
	{

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

			// Client 가 접속을 끊을때
			if (bSuccess == false || (dwIoSize == 0 && bSuccess == true))
			{
				printf("socket(%d) 접속 끊김\n", (uint32_t)pClientInfo->m_socketClient);
				CloseSocket(pClientInfo);
				continue;
			}

			stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;

			// Overlapped I/O Recv 작업 결과 처리
			if (IOOperation::RECV == pOverlappedEx->m_eOperation)
			{
				pOverlappedEx->m_szBuf[dwIoSize] = NULL;
				printf("[Recv] bytes : %d, msg : %s\n", dwIoSize, pOverlappedEx->m_szBuf);
				BindRecv(pClientInfo);
			}
			// Overlapped I/O Send 작업 결과 처리
			else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
			{
				printf("[Send] bytes : %d, msg : %s\n", dwIoSize, pOverlappedEx->m_szBuf);
			}
			else
			{
				printf("socket(%d) 에서 예외발생\n", (int)pClientInfo->m_socketClient);
			}
		}
	};

private:
	SOCKET						mListenSocket = INVALID_SOCKET;			// 클라이언트 받기위한 소켓
	std::vector<stClientInfo>	mClientInfos;							// 클라이언트 저장 구조체
	uint32_t					mClientCount = 0;						// 접속되어 있는 클라이언트 수
	std::vector<std::thread>	mIOWorkerThreads;						// IO Worker 쓰레드
	std::thread					mAcceptThread;							// Accept 쓰레드
	HANDLE						mIOCPHandle = INVALID_HANDLE_VALUE;		// CompletionPort 객체 핸들
	bool						mIsWorkerRun = true;					// 작업 쓰레드 동작 플래그
	bool						mIsAcceptRun = true;					// 접속 쓰레드 동작 플래그
	char						mSocketBuffer[1024];					// 소켓 버퍼
};

