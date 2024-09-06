#pragma once

#include <functional>

#include "User/UserManager.h"
#include "Packet/Packet.h"

class Room
{
public:
	Room() = default;
	~Room() = default;

	INT32 GetMaxUserCount() { return mMaxUserCount; }
	INT32 GetCurrentUserCount() { return mCurrentUserCount; }
	INT32 GetRoomNumber() { return mRoomNum; }

	void Init(const INT32 roomNum_, const INT32 maxUserCount_)
	{
		mRoomNum = roomNum_;
		mMaxUserCount = maxUserCount_;
	}

	UINT16 EnterUser(User* user_)
	{
		if (mMaxUserCount <= mCurrentUserCount)
		{
			return (UINT16)ERROR_CODE::ENTER_ROOM_FULL_USER;
		}

		mUserList.push_back(user_);
		++mCurrentUserCount;

		user_->EnterRoom(mRoomNum);
		return (UINT16)ERROR_CODE::NONE;
	}

	void LeaveUser(User* leaveUser_)
	{
		mUserList.remove_if([leaveUserId = leaveUser_->GetUserId()](User* pUser) {
			return leaveUserId == pUser->GetUserId();
		});
	}

	void NotifyChat(INT32 clientIndex_, const char* userID_, const char* msg_)
	{
		ROOM_CHAT_NOTIFY_PACKET roomChatNotifyPacket;
		roomChatNotifyPacket.PacketId = (UINT16)PACKET_ID::ROOM_CHAT_NOTIFY;
		roomChatNotifyPacket.PacketLength = sizeof(roomChatNotifyPacket);

		CopyMemory(roomChatNotifyPacket.Msg, msg_, sizeof(roomChatNotifyPacket.Msg));
		CopyMemory(roomChatNotifyPacket.UserID, userID_, sizeof(roomChatNotifyPacket.UserID));
		SendToAllUser(sizeof(roomChatNotifyPacket), (char*)&roomChatNotifyPacket, clientIndex_, false);
	}

	std::function<void(UINT32, UINT32, char*)> SendPacketFunc;

private:
	void SendToAllUser(const UINT16 dataSize_, char* data_, const INT32 passUserIndex_, bool exceptMe)
	{
		for (auto pUser : mUserList)
		{
			if (pUser == nullptr)
			{
				continue;
			}

			if (exceptMe && pUser->GetNetConnIdx() == passUserIndex_)
			{
				continue;
			}

			SendPacketFunc((UINT32)pUser->GetNetConnIdx(), (UINT32)dataSize_, data_);
		}
	}

private:
	INT32 mMaxUserCount = 0;
	INT32 mCurrentUserCount = 0;
	INT32 mRoomNum = -1;

	std::list<User*> mUserList;
};