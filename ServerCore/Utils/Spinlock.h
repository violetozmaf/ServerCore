#pragma once

class Spinlock
{
public:
	Spinlock();
	~Spinlock();

	void EnterLock();
	void LeaveLock();

private:
	Spinlock(const Spinlock& rhs);
	Spinlock& operator=(const Spinlock& rhs);

	volatile long mLockFlag;
};

class SpinlockGuard
{
public:
	SpinlockGuard(Spinlock& lock) : mLock(lock)
	{
		mLock.EnterLock();
	}

	~SpinlockGuard()
	{
		mLock.LeaveLock();
	}

private:
	Spinlock& mLock;
};

template <class TargetClass>
class ClassTypeLock
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			TargetClass::mLock.EnterLock();
		}

		~LockGuard()
		{
			TargetClass:mLock.LeaveLock();
		}
	};

private:
	static Spinlock mLock;
};


template <class TargetClass>
Spinlock ClassTypeLock<TargetClass>::mLock;