#pragma once

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "PacketDefine.h"


class Client
{
public:
    
    Client(boost::asio::io_service& io_, const std::string& host_, const std::string& port_) : mSock(io_), io(io_)
    {
        boost::asio::ip::tcp::resolver resolver(io_);
        boost::asio::ip::tcp::resolver::query query(host_, port_);
        endpoint_iterator = resolver.resolve(query);
    }

    void Start()
    {
        mClientThread = boost::thread(boost::bind((&Client::WorkerThread, this)));

        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));

        mClientThread.join();
    }

    void Connection()
    {
        
        // 비동기 상태로 서버와 접속
        boost::asio::async_connect(mSock, endpoint_iterator, boost::bind(&Client::ConnectHandle, this, boost::asio::placeholders::error));
    }

    void ConnectHandle(const boost::system::error_code& e)
    {
        if (!e)
        {
            std::cout << "연결 성공!!" << std::endl;
            std::string msg = "클라이언트 접속";

            // Server  로부터 비동기 write
            boost::asio::async_write(mSock, 
                boost::asio::buffer(msg, msg.length()), 
                boost::bind(&Client::WriteHandle, this,
                boost::asio::placeholders::error));

        }
    }

    void WriteHandle(const boost::system::error_code& e_)
    {
        if (!e_)
        {
            std::cout << "완료!" << std::endl;
        }
        else
        {
            std::cout << e_.message() << std::endl;
        }
    }

    void Send()
    {
        getline(std::cin, mSendBuf);
        mSock.async_write_some(boost::asio::buffer(mSendBuf), bind(&Client::SendHandle, this, _1));
    }

    void Recv()
    {
        mSock.async_read_some(boost::asio::buffer(mBuf, 80), bind(&Client::RecvHandle, this, _1, _2));
    }
        
    void SendHandle(const boost::system::error_code& e_)
    {
        if (e_)
        {
            std::cout << "Error : " << e_.message() << std::endl;
            StopAll();
            return;
        }

        Send();
    }

    void RecvHandle(const boost::system::error_code& e, size_t size_)
    {
        if (e)
        {
            std::cout << "Error : " << e.message() << std::endl;
            StopAll();
            return;
        }

        if (size_)
        {
            std::cout << "서버에서 세션 닫음" << std::endl;
            StopAll();
            return;
        }

        mBuf[size_] = '\0';
        mRecvBuf = mBuf;

        boost::lock_guard<boost::mutex> lock(mLock);

        std::cout << mRecvBuf << std::endl;

        Recv();

    }

    void StopAll()
    {
        mSock.close();
    }

    void ErrorMessage(std::string message)
    {
        std::cout << "[오류발생] : " << message << std::endl;
        system("pause");
        exit(1);
    }

private:
    void WorkerThread()
    {
        boost::lock_guard<boost::mutex> lock(mLock);
        std::cout << "ThreadID : " << boost::this_thread::get_id() << std::endl;

        io.run();

    }
private:
    boost::asio::io_service& io;
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator;

    boost::asio::ip::tcp::socket mSock;
    boost::mutex mLock;
    boost::thread mClientThread;

    char mData[MAX_LENGTH] = { 0, };
    char mBuf[80];
    std::string mSendBuf;
    std::string mRecvBuf;
};
