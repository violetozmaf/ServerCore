#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

const UINT32 MAX_SOCK_RECVBUF				= 1024;			// ��Ŷ ũ��
const UINT32 MAX_SOCK_SENDBUF				= 4096;			// ���� ������ ũ��
const UINT32 RE_USE_SESSION_WAIT_TIMESEC	= 3;


enum class IOOperation
{
	RECV,
	SEND,
	ACCEPT
};

// WSAOVERLAPPED ����ü Ȯ��
struct stOverlappedEx
{
	WSAOVERLAPPED		m_wsaOverlapped;			// Overlapped I/O ����ü	
	WSABUF				m_wsaBuf;					// Overlapped I/O �۾�����	
	IOOperation			m_eOperation;				// �۾� ���� ����
	UINT32				SessionIndex = 0;
};


 