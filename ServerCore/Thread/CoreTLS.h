#pragma once
#include <stack>
#include "../Utils/Types.h"

extern thread_local uint32					LThreadId;
extern thread_local std::stack<int32>		LLockStack;


class Thread
{

};