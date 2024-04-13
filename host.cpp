#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

const char* ip = "213.159.209.12";

int main(int argc, char** argv)
{
	
	if (argc != 3){
		std::cout << "usage: host.exe {port} {protocol}" << std::endl;
		exit(1);
	}
	
	WSADATA wsa_data;
	struct sockaddr_in server, client, localhost;
	struct addrinfo hints, *resr;
	socklen_t client_size, server_size, localhost_size;
	int readsocket;
	int bytes = 0;

	WSAStartup(MAKEWORD(2, 2), &wsa_data);
	
	std::cout << argv[2] << std::endl;
	
	if (std::string(argv[2]) == "udp"){
	
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = AI_PASSIVE;
		
		getaddrinfo(NULL, "80808", &hints, &resr);
		
		readsocket = socket(resr->ai_family, resr->ai_socktype, resr->ai_protocol);
		
		std::thread thrd([&]()
		{
			server.sin_family = AF_INET;
			server.sin_port = htons(50502);
			inet_pton(AF_INET, ip, &server.sin_addr);
			
			while (true){
				server_size = sizeof(server);
				char* data = "keep-alive";
				bytes = sendto(readsocket, data, sizeof(data), 0, (struct sockaddr*)&server, server_size);
				std::cout << bytes << std::endl;
				Sleep(10000);
			}
		});
		
		std::cout << "Listening for incoming connections..." << std::endl;
		
		while (true){
			char buff[16384];
			
			client_size = sizeof(client);
			recvfrom(readsocket, buff, sizeof(buff), 0, (struct sockaddr*)&client, &client_size);
			
			localhost.sin_family = AF_INET;
			localhost.sin_port = htons(std::stoi(argv[1]));
			inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr);
			
			if (strlen(buff) > 0){
				std::cout << buff << std::endl;
				
				localhost_size = sizeof(localhost);
				sendto(readsocket, buff, sizeof(buff), 0, (struct sockaddr*)&localhost, localhost_size);
			}
				
		}
	}
	
	else if (std::string(argv[2]) == "tcp"){
	
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		
		struct addrinfo *resl;
		
		getaddrinfo(NULL, "80808", &hints, &resr);
		getaddrinfo(NULL, "90909", &hints, &resl);
		
		readsocket = socket(resr->ai_family, resr->ai_socktype, resr->ai_protocol);
		int localsocket = socket(resl->ai_family, resl->ai_socktype, resl->ai_protocol);
		
		server.sin_family = AF_INET;
		server.sin_port = htons(50502);
		inet_pton(AF_INET, ip, &server.sin_addr);
		
		bind(readsocket, resr->ai_addr, resr->ai_addrlen);
		
		server_size = sizeof(server);
		connect(readsocket, (struct sockaddr *)&server, server_size);
		
		std::cout << "Listening for incoming connections..." << std::endl;
		
		while (true){
			char buff[16384];
			
			recv(readsocket, buff, sizeof(buff), 0);
			
			localhost.sin_family = AF_INET;
			localhost.sin_port = htons(std::stoi(argv[1]));
			inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr);
			
			if (strlen(buff) > 0){
				std::cout << buff << std::endl;
				
				localhost_size = sizeof(localhost);
				connect(localsocket, (struct sockaddr *)&localhost, localhost_size);
				send(localsocket, buff, sizeof(buff), 0);
			}
		}
	}
}