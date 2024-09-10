// DummyClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <boost/asio.hpp>

#include "Client.h"

const std::string IP = "127.0.0.1";
const USHORT PORT = 8600;

int main(int argc, char* argv[])
{
    try
    {
        Client chat(IP, PORT);
        chat.Start();
        /*boost::asio::io_service io_context;
        Client chat(io_context, IP, PORT);
        io_context.run();*/
    }
    catch (std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    
    return 0;
}