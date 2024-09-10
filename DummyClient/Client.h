#pragma once

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "PacketDefine.h"


//class Client
//{
//public:
//    
//    Client(const std::string& host_, const USHORT& port_) :
//        end_point(boost::asio::ip::address::from_string(host_), port_),
//        mSock(io_context, end_point.protocol()),
//        resolver(io_context),
//        endpoint_iterator(resolver.resolve(host_, std::to_string(port_)))
//    {
//        
//    }
//
//    void Start()
//    {
//        mClientThread = boost::thread(boost::bind((&Client::WorkerThread, this)));
//
//        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
//
//        mClientThread.join();
//    }
//
//    void Connection()
//    {
//        
//        // 비동기 상태로 서버와 접속
//        boost::asio::async_connect(mSock, endpoint_iterator, boost::bind(&Client::ConnectHandle, this, boost::asio::placeholders::error));
//    }
//
//    void ConnectHandle(const boost::system::error_code& e)
//    {
//        if (!e)
//        {
//            std::cout << "연결 성공!!" << std::endl;
//            std::string msg = "클라이언트 접속";
//
//            // Server  로부터 비동기 write
//            boost::asio::async_write(mSock, 
//                boost::asio::buffer(msg, msg.length()), 
//                boost::bind(&Client::WriteHandle, this,
//                boost::asio::placeholders::error));
//
//        }
//    }
//
//    void WriteHandle(const boost::system::error_code& e_)
//    {
//        if (!e_)
//        {
//            std::cout << "완료!" << std::endl;
//        }
//        else
//        {
//            std::cout << e_.message() << std::endl;
//        }
//    }
//
//    void Send()
//    {
//        getline(std::cin, mSendBuf);
//        mSock.async_write_some(boost::asio::buffer(mSendBuf), bind(&Client::SendHandle, this, _1));
//    }
//
//    void Recv()
//    {
//        mSock.async_read_some(boost::asio::buffer(mBuf, 80), bind(&Client::RecvHandle, this, _1, _2));
//    }
//        
//    void SendHandle(const boost::system::error_code& e_)
//    {
//        if (e_)
//        {
//            std::cout << "Error : " << e_.message() << std::endl;
//            StopAll();
//            return;
//        }
//
//        Send();
//    }
//
//    void RecvHandle(const boost::system::error_code& e, size_t size_)
//    {
//        if (e)
//        {
//            std::cout << "Error : " << e.message() << std::endl;
//            StopAll();
//            return;
//        }
//
//        if (size_)
//        {
//            std::cout << "서버에서 세션 닫음" << std::endl;
//            StopAll();
//            return;
//        }
//
//        mBuf[size_] = '\0';
//        mRecvBuf = mBuf;
//
//        boost::lock_guard<boost::mutex> lock(mLock);
//
//        std::cout << mRecvBuf << std::endl;
//
//        Recv();
//
//    }
//
//    void StopAll()
//    {
//        mSock.close();
//    }
//
//    void ErrorMessage(std::string message)
//    {
//        std::cout << "[오류발생] : " << message << std::endl;
//        system("pause");
//        exit(1);
//    }
//
//private:
//    void WorkerThread()
//    {
//        boost::lock_guard<boost::mutex> lock(mLock);
//        std::cout << "ThreadID : " << boost::this_thread::get_id() << std::endl;
//
//        io_context.run();
//
//    }
//private:
//
//    boost::asio::io_context io_context;
//    boost::asio::ip::tcp::endpoint end_point;
//    boost::asio::ip::tcp::resolver resolver;
//    boost::asio::ip::tcp::resolver::iterator endpoint_iterator;
//
//    boost::asio::ip::tcp::socket mSock;
//    boost::mutex mLock;
//    boost::thread mClientThread;
//
//    char mData[MAX_LENGTH] = { 0, };
//    char mBuf[80];
//    std::string mSendBuf;
//    std::string mRecvBuf;
//};



using namespace boost;
using std::cout;
using std::endl;

class Client
{
	asio::ip::tcp::endpoint ep;
	asio::io_context ios;
	asio::ip::tcp::socket sock;
	boost::shared_ptr<asio::io_service::work> work;
	boost::thread_group thread_group;
	std::string sbuf;
	std::string rbuf;
	char buf[80];
	boost::mutex lock;

public:
	Client(std::string ip_address, unsigned short port_num) :
		ep(asio::ip::address::from_string(ip_address), port_num),
		sock(ios, ep.protocol()),
		work(new asio::io_service::work(ios))
	{}

	void Start()
	{
		for (int i = 0; i < 3; i++)
			thread_group.create_thread(bind(&Client::WorkerThread, this));

		// thread 잘 만들어질때까지 잠시 기다리는 부분
		this_thread::sleep_for(chrono::milliseconds(100));

		ios.post(bind(&Client::TryConnect, this));

		thread_group.join_all();
	}

private:
	void WorkerThread()
	{
		lock.lock();
		cout << "[" << boost::this_thread::get_id() << "]" << " Thread Start" << endl;
		lock.unlock();

		ios.run();

		lock.lock();
		cout << "[" << boost::this_thread::get_id() << "]" << " Thread End" << endl;
		lock.unlock();
	}

	void TryConnect()
	{
		cout << "[" << boost::this_thread::get_id() << "]" << " TryConnect" << endl;

		sock.async_connect(ep, boost::bind(&Client::OnConnect, this, _1));
	}

	void OnConnect(const system::error_code& ec)
	{
		cout << "[" << boost::this_thread::get_id() << "]" << " OnConnect" << endl;
		if (ec)
		{
			cout << "connect failed: " << ec.message() << endl;
			StopAll();
			return;
		}

		ios.post(bind(&Client::Send, this));
		ios.post(bind(&Client::Recieve, this));
	}

	void Send()
	{
		sbuf.clear();
		getline(std::cin, sbuf);

		sock.async_write_some(asio::buffer(sbuf), bind(&Client::SendHandle, this, _1));

	}

	void Recieve()
	{
		sock.async_read_some(asio::buffer(buf, 80), bind(&Client::ReceiveHandle, this, _1, _2));
	}

	void SendHandle(const system::error_code& ec)
	{
		if (ec)
		{
			cout << "async_read_some error: " << ec.message() << endl;
			StopAll();
			return;
		}

		Send();
	}

	void ReceiveHandle(const system::error_code& ec, size_t size)
	{
		if (ec)
		{
			cout << "async_write_some error: " << ec.message() << endl;
			StopAll();
			return;
		}

		if (size == 0)
		{
			cout << "Server wants to close this session" << endl;
			StopAll();
			return;
		}

		buf[size] = '\0';
		rbuf = buf;

		lock.lock();
		cout << rbuf << endl;
		lock.unlock();

		Recieve();
	}

	void StopAll()
	{
		sock.close();
		work.reset();
	}
};



//class Client
//{
//public:
//	Client(boost::asio::io_service& io_context, const std::string& host_, const std::string& port_) : mSock(io_context)
//	{
//		boost::asio::ip::tcp::resolver resolver(io_context);
//		boost::asio::ip::tcp::resolver::query query(host_, port_);
//		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
//
//		// 비동기상태로 서버와 접속
//		boost::asio::async_connect(mSock, endpoint_iterator,
//			boost::bind(&Client::HandleConnect, this, boost::asio::placeholders::error));
//	}
//
//	void HandleConnect(const boost::system::error_code& e)
//	{
//		if (!e) 
//		{
//			std::cout << "Connect" << std::endl;
//			std::string msg = "Server Connect";
//			boost::asio::async_write(mSock, boost::asio::buffer(msg, msg.length()),
//				boost::bind(&Client::HandleWrite, this, boost::asio::placeholders::error));
//		}
//	}
//
//	void HandleWrite(const boost::system::error_code& e)
//	{
//		if (!e)
//		{
//			std::cout << "Done" << std::endl;
//		}
//	}
//
//private:
//	boost::asio::ip::tcp::socket mSock;
//	char mData[MAX_LENGTH];
//};