#pragma once

#include <windows.h>

#define WIN32_LEAN_AND_MEAN

// 클라이언트가 보낸 패킷을 저장하는 구조체
struct PacketData
{
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	char* pPacketData = nullptr;

	void Set(PacketData& value)
	{
		SessionIndex = value.SessionIndex;
		DataSize = value.DataSize;

		pPacketData = new char[value.DataSize];
		CopyMemory(pPacketData, value.pPacketData, value.DataSize);
	}

	void Set(UINT32 sessionIndex_, UINT32 dataSize_, char* pData)
	{
		SessionIndex = sessionIndex_;

		DataSize = dataSize_;
		pPacketData = new char[DataSize];
		memset(pPacketData, 0, DataSize);
		CopyMemory(pPacketData, pData, DataSize);
	}

	void Release()
	{
		delete pPacketData;
	}
};

struct PacketInfo
{
	UINT32 ClientIndex = 0;
	UINT16 PacketId = 0;
	UINT16 DataSize = 0;
	char* pDataPtr = nullptr;
};

enum class PACKET_ID : UINT16
{
	//SYSTEM
	SYS_USER_CONNECT = 11,
	SYS_USER_DISCONNECT = 12,
	SYS_END = 30,

	//DB
	DB_END = 199,

	//Client
	LOGIN_REQUEST = 201,
	LOGIN_RESPONSE = 202,

	ROOM_ENTER_REQUEST = 210,
	ROOM_ENTER_RESPONSE = 211,

	ROOM_LEAVE_REQUEST = 220,
	ROOM_LEAVE_RESPONSE = 221,

	ROOM_CHAT_REQUEST = 230,
	ROOM_CHAT_RESPONSE = 231,
	ROOM_CHAT_NOTIFY = 232,
};


// 메모리 정렬 전처리문 1바이트단위로 처리
#pragma pack(push, 1)
struct PACKET_HEADER
{
	UINT16 PacketLength;
	UINT16 PacketId;
	UINT8 Type;			//압축 여부 암호화 여부 등 속성 알아내는 값
};

const UINT32 PACKET_HEADER_LENGTH = sizeof(PACKET_HEADER);

// 로그인 요청
const INT32 MAX_USER_ID_LEN = 32;
const INT32 MAX_USER_PW_LEN = 32;

struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1];
	char UserPW[MAX_USER_PW_LEN + 1];
};
const size_t LOGIN_REQUEST_PACKET_SIZE = sizeof(LOGIN_REQUEST_PACKET);

struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	UINT16 Result;
};

// 룸에 들어가기 요청
struct ROOM_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	INT32 RoomNumber;
};

struct ROOM_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	INT16 Result;
};

// 룸 나가기 요청
struct ROOM_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{

};

struct ROOM_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	INT16 Result;
};

// 룸 채팅
const int MAX_CHAT_MSG_SIZE = 256;
struct ROOM_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char Message[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};

struct ROOM_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	INT16 Result;
};

struct ROOM_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char UserID[MAX_USER_ID_LEN + 1] = { 0, };
	char Msg[MAX_CHAT_MSG_SIZE + 1] = { 0, };
};
#pragma pack(pop)					// 위에 설정한 메모리정렬 전처리문 삭제