#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <thread>
#include <errno.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <shared_mutex> 
#include <mutex>

struct packet {
	uint16_t size;
	uint16_t port;
	char data[16384];
};

struct sockaddr_in client, server;
socklen_t client_size, server_size;
struct addrinfo hints, *resr, *resw;
int readsocket, writesocket;
int bytes;

std::shared_mutex mutx;
std::vector<std::vector<int>> clients;

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

int main(int argc, char *argv[])
{
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
		
	server_size = sizeof(server);
	listen(writesocket, SOMAXCONN);
	int write_accept_socket = accept(writesocket, (struct sockaddr*)&server, &server_size);
	
	std::thread receiver([&](){
		while (true){
			char buff[16384];
			memset(&buff, 0, sizeof(buff));
			
			int recv_bytes = recv(write_accept_socket, buff, sizeof(buff), 0);
			
			if (recv_bytes > 0){
				std::cout << recv_bytes << " recv write" << std::endl;
				
				std::cout << buff << " write buff" << std::endl;
				
				std::vector pkts = parse_packet(buff, recv_bytes);
				
				for (unsigned int j = 0; j < pkts.size(); j++){
					packet recvpacket = pkts[j];
					std::cout << j << " packet id" << std::endl;
					std::shared_lock<std::shared_mutex> lock(mutx);
					for (unsigned int i = 0; i < clients.size(); i++){
						if (clients[i][1] == recvpacket.port){
							std::cout << recvpacket.data << " data" << std::endl;
							
							bytes = send(clients[i][0], recvpacket.data, recvpacket.size, 0);
							std::cout << bytes << std::endl;
						}
					}
				}
				
				// packet recvpacket;
				// memcpy(&recvpacket, buff, sizeof(buff));
				// uint16_t port = recvpacket.port;
				// uint16_t datasize = recvpacket.size;
				
				// std::cout << recv_bytes << " recv write" << std::endl;
				
				// std::cout << buff << " write buff" << std::endl;
				
				// std::shared_lock<std::shared_mutex> lock(mutx);
				// for (unsigned int i = 0; i < clients.size(); i++){
					// if (clients[i][1] == port){
						// std::cout << recvpacket.data << " data" << std::endl;
						
						// bytes = send(clients[i][0], recvpacket.data, recv_bytes-(sizeof(uint16_t)*2), 0);
						// std::cout << bytes << std::endl;
					// }
				// }
			}
		}
	});
	
	std::cout << "host connected" << std::endl;
	
	std::thread clientreceiver([&](){
		while (true){
			std::shared_lock<std::shared_mutex> lock(mutx);
			for (int i = 0; i < clients.size(); i++){
				char buff[16384];
				memset(&buff, 0, sizeof(buff));
				
				int recv_bytes = recv(clients[i][0], buff, sizeof(buff), 0);
				
				if (recv_bytes > 0){
					uint16_t port = (uint16_t)clients[i][1];
					
					uint16_t datasize = recv_bytes;
					struct packet testpacket;
					testpacket.size = datasize;
					
					testpacket.port = port;
					memcpy(testpacket.data, buff, sizeof(buff));
					
					std::cout << port << " port" << std::endl;
					std::cout << buff << " client buff" << std::endl;
					std::cout << recv_bytes << " recv client" << std::endl;
					
					char sendbuffer[sizeof(packet)];
    
					memcpy(sendbuffer, &testpacket, sizeof(sendbuffer));
					
					bytes = send(write_accept_socket, sendbuffer, recv_bytes+sizeof(uint16_t)*2, 0);
					std::cout << bytes << std::endl;
				}
			}
		}
	});
	
	while (true){
		client_size = sizeof(client);
		listen(readsocket, SOMAXCONN);
		int accept_socket = accept(readsocket, (struct sockaddr*)&client, &client_size);
		
		uint16_t port = ntohs(client.sin_port);
		fcntl(accept_socket, F_SETFL, O_NONBLOCK);

		std::unique_lock<std::shared_mutex> lock(mutx);
		vector_push_new(clients, std::vector<int>({accept_socket, port}));
		std::cout << clients.size() << std::endl;
	}

	return 0;
}