#pragma once
#include <unordered_map>

#include "../Utils/ErrorCode.h"
#include "User.h"

class UserManager
{
public:
	UserManager() = default;
	~UserManager() = default;

	void Init(const INT32 maxUserCount_)
	{
		mMaxUserCount = maxUserCount_;
		mUserObjPool = std::vector<User*>(mMaxUserCount);

		for (auto i = 0; i < mMaxUserCount; i++)
		{
			mUserObjPool[i] = new User();
			mUserObjPool[i]->Init(i);
		}
	}

	INT32 GetCurrentUserCount() { return mCurrentUserCount; }
	INT32 GetMaxUserCount() { return mMaxUserCount; }

	void IncreaseUserCount() { mCurrentUserCount++; }
	void DecreaseUserCount()
	{
		if (mCurrentUserCount > 0)
		{
			mCurrentUserCount--;
		}
	}

	ERROR_CODE AddUser(char* userID_, int clientIndex_)
	{
		// 유저 중복 체크
		auto useridx = clientIndex_;
		mUserObjPool[useridx]->SetLogin(userID_);
		mUserIDDictionary.insert(std::pair<char*, int>(userID_, clientIndex_));

		return ERROR_CODE::NONE;
	}

	INT32 FindUserIndexByID(char* userID_)
	{
		if (auto res = mUserIDDictionary.find(userID_); res != mUserIDDictionary.end())
		{
			return (*res).second;
		}
		return -1;
	}

	void DeleteUserInfo(User* user_)
	{
		mUserIDDictionary.erase(user_->GetUserId());
		user_->Clear();
	}

	User* GetUserByConnIdx(INT32 clientIndex_)
	{
		return mUserObjPool[clientIndex_];
	}

private:
	INT32 mMaxUserCount = 0;
	INT32 mCurrentUserCount = 0;

	std::vector<User*> mUserObjPool;
	std::unordered_map<std::string, int> mUserIDDictionary;

};