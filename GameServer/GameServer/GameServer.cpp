
#include "GameServer.h"


//
//함수 선언
void Initalize();


int main(int argc, char *argv[])
{
	int retval =1;
	vector <thread> worker;
	Initalize();

	for (int i = 0; i < 4; ++i)
		worker.push_back(thread{ ThreadWorker });
	thread a_thread{ ThreadAccept };


	for (auto &th : worker)
		th.join();
	a_thread.join();

	WSACleanup();
	return 0;
}


void Initalize() {
	for (auto &client : g_clients)
	{
		client.in_use = false;
		client.exover.is_recv = true;
		client.packet_size = 0;
		client.prev_size = 0;
		client.exover.wsabuf.buf = client.exover.io_buffer;
		client.exover.wsabuf.len = sizeof(client.exover.io_buffer);
	}

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}