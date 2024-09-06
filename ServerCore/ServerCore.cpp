// ServerCore.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <thread>
#include <string>

#include "IOCPNetwork/IOCPServer.h"
#include "EchoServer.h"


const UINT16 SERVER_PORT = 8600;
const UINT16 MAX_CLIENT = 100;			// 총 접속할수 있는 클라이언트
const UINT32 MAX_IO_WORKER_THREAD = 4;	// 쓰레드 풀에 넣을 쓰레드 수 


/*--------------------------------------------------------------------------
* 1. Socket 초기화 WSASocket()
* 2. 설정한 서버주소와 Bind()
* 3. 접속 요청 받아 들이기위해 IOCompletionPort 소켓을 등록하고 대기 Listen()
* 4. IOCP 생성하고 생성된 IOCompletionPort 에 소켓 연결하여 비동기 처리 준비 CreateIoCompletionPort()
* 5. Server 에서 Client 받을 준비 완료됨. 
* 5. 생성된 IOCPHandle 에 접속한 Client정보 할당
* 5. WorkerThread 생성
* 6. AcceptThread 생성
* 7. 서버 시작!!!
--------------------------------------------------------------------------*/


int main()
{
	
	//IOCPServer server;
	EchoServer server;

	// 소켓 초기화
	server.InitSocket(MAX_IO_WORKER_THREAD);

	// 소켓과 서버 주소 연결하고 등록
	server.BindListen(SERVER_PORT);
	
	//server.StartServer(MAX_CLIENT);
	
	server.Run(MAX_CLIENT);

	printf("아무 키나 누를때까지 대기\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "q")
		{
			break;
		}
	}

	server.End();
	return 0;
}
