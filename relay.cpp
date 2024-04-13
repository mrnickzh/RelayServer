#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <thread>
#include <errno.h>

struct sockaddr_in client, server;
socklen_t client_size, server_size;
struct addrinfo hints, *resr, *resw;
int readsocket, writesocket;
int bytes;

int main(int argc, char *argv[])
{
	if (argc != 2){
		std::cout << "usage: relay {protocol}" << std::endl;
		exit(1);
	}
	
	if (std::string(argv[1]) == "udp"){
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;
		
		getaddrinfo(NULL, "50501", &hints, &resr);
		getaddrinfo(NULL, "50502", &hints, &resw);
		
		readsocket = socket(resr->ai_family, resr->ai_socktype, resr->ai_protocol);
		writesocket = socket(resw->ai_family, resw->ai_socktype, resw->ai_protocol);
		
		if (readsocket < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "read socket ok" << std::endl;
		
		if (writesocket < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "write socket ok" << std::endl;
		
		if (bind(readsocket, resr->ai_addr, resr->ai_addrlen) < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "read bind ok" << std::endl;
		
		if (bind(writesocket, resw->ai_addr, resw->ai_addrlen) < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "write bind ok" << std::endl;
		
		std::thread thrd([&]()
		{
			while (true){
				char buff[16384];
				
				server_size = sizeof(server);
				recvfrom(writesocket, buff, sizeof(buff), 0, (struct sockaddr*)&server, &server_size);
			}
		});
		
		
		while (true){
			char buff[16384];
			
			client_size = sizeof(client);
			recvfrom(readsocket, buff, sizeof(buff), 0, (struct sockaddr*)&client, &client_size);
			
			if (strlen(buff) > 0){
				std::cout << buff << std::endl;
				bytes = sendto(writesocket, buff, sizeof(buff), 0, (struct sockaddr*)&server, server_size);
				std::cout << bytes << std::endl;
			}
				
		}
	}
	
	else if (std::string(argv[1]) == "tcp"){
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		
		getaddrinfo(NULL, "50501", &hints, &resr);
		getaddrinfo(NULL, "50502", &hints, &resw);
		
		readsocket = socket(resr->ai_family, resr->ai_socktype, resr->ai_protocol);
		writesocket = socket(resw->ai_family, resw->ai_socktype, resw->ai_protocol);
		
		if (readsocket < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "read socket ok" << std::endl;
		
		if (writesocket < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "write socket ok" << std::endl;
		
		if (bind(readsocket, resr->ai_addr, resr->ai_addrlen) < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "read bind ok" << std::endl;
		
		if (bind(writesocket, resw->ai_addr, resw->ai_addrlen) < 0)
			std::cerr << strerror(errno);
		else
			std::cout << "write bind ok" << std::endl;
		
		char buff[16384];
			
		server_size = sizeof(server);
		listen(writesocket, SOMAXCONN);
		int write_accept_socket = accept(writesocket, (struct sockaddr*)&server, &server_size);
		
		std::cout << "host connected" << std::endl;
		
		while (true){
			client_size = sizeof(client);
			listen(readsocket, SOMAXCONN);
			int accept_socket = accept(readsocket, (struct sockaddr*)&client, &client_size);
			recv(accept_socket, buff, sizeof(buff), 0);
			
			if (strlen(buff) > 0){
				std::cout << buff << std::endl;
				bytes = send(write_accept_socket, buff, sizeof(buff), 0);
				std::cout << bytes << std::endl;
			}
		}
	}

	return 0;
}