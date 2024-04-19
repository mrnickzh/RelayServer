#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <shared_mutex> 
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

struct packet {
	uint16_t size;
	uint16_t port;
	char data[16384];
};

std::shared_mutex mutx;
std::vector<std::vector<int>> locals;

const char* ip = "213.159.209.12";

void vector_push_new(std::vector<std::vector<int>> &dst, std::vector<int> src){
	if (std::find(dst.begin(), dst.end(), src)==dst.end()){
		dst.push_back(src);
	}
}

std::vector<packet> parse_packet(char* buff, int buffsize){
	int readed = 0;
	std::vector<packet> packets;
	while (readed < buffsize){
		packet rawpacket;
		memcpy(&rawpacket, buff + (buffsize-(buffsize-readed)), buffsize-readed);
		uint16_t port = rawpacket.port;
		uint16_t datasize = rawpacket.size;
		
		std::cout << datasize << " datasize" << std::endl;
		
		packet parsedpacket;
		parsedpacket.port = port;
		parsedpacket.size = datasize;
		
		memcpy(&parsedpacket.data, rawpacket.data, parsedpacket.size);
		
		packets.push_back(parsedpacket);
		readed += parsedpacket.size+(sizeof(uint16_t)*2);
	}
	return packets;
}

int main(int argc, char** argv)
{
	
	if (argc != 2){
		std::cout << "usage: host.exe {port}" << std::endl;
		exit(1);
	}
	
	WSADATA wsa_data;
	struct sockaddr_in server, client, localhost, target;
	struct addrinfo hints, *resr;
	socklen_t client_size, server_size, localhost_size, target_size;
	int readsocket;
	int bytes = 0;

	WSAStartup(MAKEWORD(2, 2), &wsa_data);
	
	std::cout << argv[1] << std::endl;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	struct addrinfo *resl;
	
	getaddrinfo(NULL, "80808", &hints, &resr);
	
	readsocket = socket(resr->ai_family, resr->ai_socktype, resr->ai_protocol);
	
	server.sin_family = AF_INET;
	server.sin_port = htons(50502);
	inet_pton(AF_INET, ip, &server.sin_addr);
	
	bind(readsocket, resr->ai_addr, resr->ai_addrlen);
	
	server_size = sizeof(server);
	connect(readsocket, (struct sockaddr *)&server, server_size);
	
	std::cout << "Listening for incoming connections..." << std::endl;
	
	std::thread localreceiver([&](){
		while (true){
			std::shared_lock<std::shared_mutex> lock(mutx);
			for (unsigned int i = 0; i < locals.size(); i++){
				char buff[16384];
				memset(&buff, 0, sizeof(buff));
				
				int recv_bytes = recv(locals[i][0], buff, sizeof(buff), 0);
			
				if (recv_bytes > 0){
					
					std::cout << buff << " local buff" << std::endl;
					
					uint16_t port = (uint16_t)locals[i][1];
					uint16_t datasize = recv_bytes;
					struct packet testpacket;
					testpacket.size = datasize;
					testpacket.port = port;
					memcpy(testpacket.data, buff, sizeof(buff));
					char sendbuffer[sizeof(packet)];
					memcpy(sendbuffer, &testpacket, sizeof(sendbuffer));
					
					std::cout << recv_bytes << " recv local" << std::endl;
					bytes = send(readsocket, sendbuffer, recv_bytes+(sizeof(uint16_t)*2), 0);
					std::cout << bytes << std::endl;
				}
			}
		}
	});
	
	while (true){
		char buff[16384];
		memset(&buff, 0, sizeof(buff));
		
		int recv_bytes = recv(readsocket, buff, sizeof(buff), 0);
		
		localhost.sin_family = AF_INET;
		localhost.sin_port = htons(std::stoi(argv[1]));
		inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr);
		
		if (recv_bytes > 0){
			std::cout << recv_bytes << " recv read" <<  std::endl;
			
			std::cout << buff << " read buff" <<  std::endl;
			
			std::vector pkts = parse_packet(buff, recv_bytes);
				
			for (unsigned int j = 0; j < pkts.size(); j++){
				packet recvpacket = pkts[j];
				uint16_t port = recvpacket.port;
				uint16_t datasize = recvpacket.size;
				
				std::cout << j << " packet id" << std::endl;
				std::cout << port << " port" << std::endl;
				
				getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &resl);
				int localsocket = socket(resl->ai_family, resl->ai_socktype, resl->ai_protocol);
				
				localhost_size = sizeof(localhost);
				connect(localsocket, (struct sockaddr *)&localhost, localhost_size);
				
				u_long mode = 1;
				ioctlsocket(localsocket, FIONBIO, &mode);
				
				std::unique_lock<std::shared_mutex> lock(mutx);
				vector_push_new(locals, std::vector<int>({localsocket, port}));
				
				std::cout << recvpacket.data << " data" << std::endl;
				
				send(localsocket, recvpacket.data, recvpacket.size, 0);
			}
			
			// packet recvpacket;
			// memcpy(&recvpacket, buff, sizeof(buff));
			// uint16_t port = recvpacket.port;
			// uint16_t datasize = recvpacket.size;
			
			// std::cout << port << " port" << std::endl;
			
			// getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &resl);
			// int localsocket = socket(resl->ai_family, resl->ai_socktype, resl->ai_protocol);
			
			// localhost_size = sizeof(localhost);
			// connect(localsocket, (struct sockaddr *)&localhost, localhost_size);
			
			// u_long mode = 1;
			// ioctlsocket(localsocket, FIONBIO, &mode);
			
			// std::unique_lock<std::shared_mutex> lock(mutx);
			// vector_push_new(locals, std::vector<int>({localsocket, port}));
			
			// std::cout << recvpacket.data << " data" << std::endl;
			
			// send(localsocket, recvpacket.data, recv_bytes-sizeof(uint16_t), 0);
		}
	}
}