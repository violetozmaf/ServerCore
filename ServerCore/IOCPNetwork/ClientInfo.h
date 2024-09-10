#pragma once

#include <stdio.h>
#include <mutex>
#include <queue>
#include "Define.h"

// Ŭ���̾�Ʈ ������ ������� Ŭ����
class stClientInfo
{
public:
	stClientInfo()
	{
		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));
		mSock = INVALID_SOCKET;
	}

	~stClientInfo() = default;

	void Init(const UINT32 index, HANDLE iocpHandle_)
	{
		mIndex = index;
		mIOCPHandle = iocpHandle_;
	}

	UINT32 GetIndex() { return mIndex; }
	bool IsConnectd() { return mSock != INVALID_SOCKET; }
	SOCKET GetSock() { return mSock; }
	UINT64 GetLatestClosedTimeSec() { return mLatestClosedTimeSec; }
	char* RecvBuffer() { return mRecvBuf; }

	bool OnConnect(HANDLE iocpHandle_, SOCKET socket_)
	{
		mSock = socket_;

		// I/OCompletion Port ��ü�� ������ ����
		if (BindIOCompletionPort(iocpHandle_) == false)
		{
			return false;
		}

		return BindRecv();
	}

	void Close(bool bIsForce = false)
	{
		struct linger stLinger = { 0, 0 };			// SO_DONTLINGER �� ����

		// bIsForce ��  true �� SO_LINGER, timeout = 0 ���� �����Ͽ� ��������. (������ �ս� ����������)
		if (bIsForce == true)
		{
			stLinger.l_onoff = 1;
		}

		// socketClose ������ ������ �ۼ����� ��� �ߴ�
		shutdown(mSock, SD_BOTH);

		// ���Ͽɼ� ����
		setsockopt(mSock, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		// ���� ��������
		closesocket(mSock);
		mSock = INVALID_SOCKET;
	}

	void Clear()
	{

	}

	bool PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_)
	{
		printf_s("PostAccept Client Index: %d\n", GetIndex());
		mLatestClosedTimeSec = UINT32_MAX;

		mSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mSock == INVALID_SOCKET)
		{
			printf_s("[Error] WSASocket ���� : %d\n", GetLastError());
			return false;
		}

		ZeroMemory(&mAcceptOverlappedEx, sizeof(stOverlappedEx));

		DWORD bytes = 0;
		DWORD flags = 0;
		mAcceptOverlappedEx.m_wsaBuf.buf = 0;
		mAcceptOverlappedEx.m_wsaBuf.buf = nullptr;
		mAcceptOverlappedEx.m_eOperation = IOOperation::ACCEPT;
		mAcceptOverlappedEx.SessionIndex = mIndex;

		if (AcceptEx(listenSock_, mSock, mAcceptBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (mAcceptOverlappedEx)))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf_s("[Error] AcceptEx ���� : %d\n", GetLastError());
				return false;
			}
		}

		return true;
	}
	
	bool AcceptCompletion()
	{
		printf_s("AcceptCompletion : SessionIndex(%d)\n", mIndex);

		if (OnConnect(mIOCPHandle, mSock) == false)
		{
			return false;
		}

		SOCKADDR_IN stClientAddr;
		int32_t nAddrLen = sizeof(SOCKADDR_IN);
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		printf_s("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int32_t)mSock);

		return true;
	}

	bool BindIOCompletionPort(HANDLE iocpHandle_)
	{
		auto hIOCP = CreateIoCompletionPort((HANDLE)GetSock(),
			iocpHandle_,
			(ULONG_PTR)(this),
			0
		);

		if (hIOCP == INVALID_HANDLE_VALUE)
		{
			printf_s("[Error] CreateIoCompletionPort ���� : %d\n", GetLastError());
			return false;
		}

		return true;
	}

	bool BindRecv()
	{
		DWORD dwFlag = 0;
		DWORD dwRecvNumBytes = 0;

		ZeroMemory(&mRecvOverlappedEx, sizeof(stOverlappedEx));
		/*ZeroMemory(&mRecvOverlappedEx.m_wsaOverlapped, sizeof(WSAOVERLAPPED));
		mRecvOverlappedEx.m_wsaOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (mRecvOverlappedEx.m_wsaOverlapped.hEvent == NULL)
		{
			printf_s("[Error] CreateEvent Fail - %d\n", WSAGetLastError());
			return false;
		}*/

		mRecvOverlappedEx.m_wsaBuf.len = MAX_SOCK_RECVBUF;
		mRecvOverlappedEx.m_wsaBuf.buf = mRecvBuf;
		mRecvOverlappedEx.m_eOperation = IOOperation::RECV;

		ZeroMemory(mRecvBuf, sizeof(mRecvBuf));
		
		int32_t nRet = WSARecv(
			mSock,
			&(mRecvOverlappedEx.m_wsaBuf),
			1,
			&dwRecvNumBytes,
			&dwFlag,
			(LPWSAOVERLAPPED) & (mRecvOverlappedEx),
			NULL
		);

		// socket_error �̸� client socket �� �������ɷ� ó��
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf_s("[Error] WSARecv() ���� : %d\n", GetLastError());
			return false;
		}

		return true;
	}

	// 1���� �����忡���� ȣ���ؾ���
	bool SendMsg(const UINT32 dataSize_, char* pMsg_)
	{
		// �ʱ�ȭ
		auto sendOverlappedEx = new stOverlappedEx;
		ZeroMemory(sendOverlappedEx, sizeof(stOverlappedEx));

		sendOverlappedEx->m_wsaOverlapped.hEvent = NULL;
		
		sendOverlappedEx->m_wsaBuf.len = dataSize_;
		sendOverlappedEx->m_wsaBuf.buf = new char[dataSize_];
		CopyMemory(sendOverlappedEx->m_wsaBuf.buf, pMsg_, dataSize_);
		sendOverlappedEx->m_eOperation = IOOperation::SEND;

		std::lock_guard<std::mutex> guard(mSendLock);

		mSendDataQueue.push(sendOverlappedEx);

		if (mSendDataQueue.size() > 0)
		{
			SendIO();
		}

		return true;
	}

	void SendCompleted(const UINT32 dataSize_)
	{
		printf_s("[Send] bytes : %d\n", dataSize_);
		std::lock_guard<std::mutex> guard(mSendLock);

		delete[] mSendDataQueue.front()->m_wsaBuf.buf;
		delete mSendDataQueue.front();

		mSendDataQueue.pop();

		if (mSendDataQueue.empty() == false)
		{
			SendIO();
		}
	}

private:
	bool SendIO()
	{
		auto sendOverlappedEx = mSendDataQueue.front();

		DWORD dwRecvNumBytes = 0;

		//ZeroMemory(&mSendOverlappedEx, sizeof(stOverlappedEx));

		int32_t nRet = WSASend(mSock,
			&(sendOverlappedEx->m_wsaBuf),
			1,
			&dwRecvNumBytes,
			0,
			(LPWSAOVERLAPPED) & (sendOverlappedEx->m_wsaOverlapped),
			NULL
		);

		// socket_error �̸� client socket �� �������ɷ� ó��
		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf_s("[Error] WSASend ���� : %d\n", GetLastError());
			return false;
		}

		return true;
	}

	bool SetSocketOption()
	{
		int32_t opt = 1;
		if (SOCKET_ERROR == setsockopt(mSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int32_t)))
		{
			printf_s("TCP_NODELAY error : %d\n", GetLastError());
			return false;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSock, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(uint32_t)))
		{
			printf_s("SO_RCVBUF change error : %d\n", GetLastError());
			return false;
		}

		return true;
	}


private:
	
	INT32				mIndex = 0;					
	HANDLE				mIOCPHandle = INVALID_HANDLE_VALUE;

	INT64				mIsConnect = 0;
	UINT64				mLatestClosedTimeSec = 0;

	SOCKET				mSock;							// Client �� ����Ǵ� ����
	stOverlappedEx		mRecvOverlappedEx;				// RECV Overlapped I/O �۾��� ����
	char				mRecvBuf[64] = { 0, };

	stOverlappedEx		mSendOverlappedEx;				// Send Overlapped I/O �۾��� ����
	char				mSendBuf[64] = { 0, };
	
	stOverlappedEx		mAcceptOverlappedEx;			// Accept Overlapped I/O �۾���
	char				mAcceptBuf[64] = { 0, };
	
	std::mutex			mSendLock;
	std::queue<stOverlappedEx*> mSendDataQueue;	
	
};