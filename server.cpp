//325CA Dragan Dan-Stefan

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <stdlib.h>
#include <cstdio>
#include <fcntl.h>
#include <string.h>
#include <ctime>
#include <map>

#define CLIENTS 100
#define BUFLEN 256

using namespace std;

// Retine informatii despre clientii conectati la server
class Client {
	
	public:
	
	char name[BUFLEN];
	sockaddr_in sockaddr;
	int fd;
	time_t timer;
};

// Afiseaza informatii despre fiecare client
void status (map<char*, Client> client_list) {
	
	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
	 	cout << it->second.name << "\t" 
	 		<< inet_ntoa(it->second.sockaddr.sin_addr) << "\t" 
	 		<< it->second.sockaddr.sin_port << endl;	
	}
}	

// Trimite mesaj de deconectare clientului si il elimina din baza de date
void kick(char *client, map<char*, Client> client_list,
		fd_set read_fds) {
	
	char buf[6];
	int fd;
	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
	 	if(strncmp(it->first, client, sizeof(client)) == 0) {
			client_list.erase(it);
			fd = it->second.fd;
		}
	}
	
	strcpy(buf, "KICK");
	send(fd, buf, strlen(buf), 0);
	FD_CLR(fd, &read_fds);	
}

// Trimite unui client numele tuturor clientilor serverului
void listclients(map<char*, Client> client_list, int fd) {

	char buffer[BUFLEN];
	int cx = 0, n;	
	
	sprintf(buffer, "Lista clientilor:\n");
	cx = strlen("Lista clientilor:\n");
	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
		sprintf(buffer + cx, "%s\n", it->second.name);
		cx += strlen(it->second.name) + 1;	 	 
	}
	
	if((n = send(fd, buffer, strlen(buffer), 0)) <	0) {
		cerr << "Nu a fost trimis mesajul\n";
	}
}

// Trimite unui client informatii despre clientul solicitat
void infoclient(map<char*, Client> client_list, char client[], int fd) {

	char buffer[BUFLEN];
	int cx = 0, n;	
	
	sprintf(buffer, "Informatii client %s:\n", client);
	cx = strlen(buffer);
	Client clnt;
	time_t timer;
	time(&timer); 
	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
	 	if(strncmp(it->first, client, sizeof(client)) == 0) {
			sprintf(buffer + cx, "nume: %s\tport: %d\ttimp scurs: %d\n",
			 	it->second.name, it->second.sockaddr.sin_port,
			 		timer - it->second.timer);
		}
	}

	if((n = send(fd, buffer, strlen(buffer), 0)) <	0) {
		cerr << "Nu a fost trimis mesajul\n";
	}	
}

// Trimite unui client ip-ul si portul clientului solicitat
void message(map<char*, Client> client_list, char client[], int fd) {
	
	char buffer[BUFLEN];
	int n, cx = 0;
	
	bool ok = false;
	sprintf(buffer, "message ");
	cx = strlen(buffer);
	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
	 	if(strncmp(it->first, client, sizeof(client)) == 0) {
	 		sprintf(buffer + cx, "%s %d",
	 			inet_ntoa(it->second.sockaddr.sin_addr),
			 	it->second.sockaddr.sin_port);
			ok = true;			 
		}
	}
	
	if(ok == false) {
		sprintf(buffer, "Nu exista clientul %s", client);
	}
		
	if((n = send(fd, buffer, strlen(buffer), 0)) <	0) {
		cerr << "Nu a fost trimis mesajul\n";
	}	
}

// Trimite ip-ul si portul tuturor clientilor
void broadcast(map<char*, Client> client_list, int fd) {

	char buffer[BUFLEN];
	int n, cx = 0;
	
	sprintf(buffer, "broadcast");

	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
	 	if(it->second.fd != fd) {
	 		sprintf(buffer + strlen(buffer), " %s %d",
	 			inet_ntoa(it->second.sockaddr.sin_addr),
		 		it->second.sockaddr.sin_port);
		}
	}
		
	if((n = send(fd, buffer, strlen(buffer), 0)) <	0) {
		cerr << "Nu a fost trimis mesajul\n";
	}	
}

// Trimite ip-ul si portul unui client
void sendfile(map<char*, Client> client_list, char client[], int fd) {
	
	char buffer[BUFLEN];
	int n, cx = 0;
	
	bool ok = false;
	sprintf(buffer, "sendfile ");
	cx = strlen(buffer);
	for (map<char*, Client>::iterator it = client_list.begin();
	 	it != client_list.end(); ++it) {
	 	
	 	if(strncmp(it->first, client, sizeof(client)) == 0) {
	 		sprintf(buffer + cx, "%s %d",
	 			inet_ntoa(it->second.sockaddr.sin_addr),
			 	it->second.sockaddr.sin_port);
			ok = true;			 
		}
	}
	
	if(ok == false) {
		sprintf(buffer, "Nu exista clientul %s", client);
	}
		
	if((n = send(fd, buffer, strlen(buffer), 0)) <	0) {
		cerr << "Nu a fost trimis mesajul\n";
	}	
}

// Rezolva mesajele trimise de clienti
void solicitare(char *comanda, map<char*, Client> client_list, int fd) {

	int sock = fd;
	
	if(strncmp(comanda, "listclients", 11) == 0) {
		listclients(client_list, sock);
	}
	
	if(strncmp(comanda, "infoclient", 10) == 0) {
		char *client;
		client = new char[BUFLEN];
		sscanf(comanda, "%*s %s", client);
		infoclient(client_list, client, sock);
	}

	if(strncmp(comanda, "message", 7) == 0) {
		char *client;
		client = new char[BUFLEN];
		sscanf(comanda, "%*s %s", client);
		message(client_list, client, sock);		
	}
	
	if(strncmp(comanda, "broadcast", 9) == 0) {
		broadcast(client_list, sock);
	}
	
	if(strncmp(comanda, "sendfile", 8) == 0) {
		char *client;
		client = new char[BUFLEN];
		sscanf(comanda, "%*s %s", client);
		sendfile(client_list, client, sock);
	}
	
	if(strncmp(comanda, "quit", 4) == 0) {
		for (map<char*, Client>::iterator it = client_list.begin();
	 		it != client_list.end(); ++it) {
	 		
	 		if(it->second.fd == sock) {
	 			client_list.erase(it);
	 		}
		}
		close(sock);
	}
}

int main (int argc, char **argv) {
	
	map <char*, Client> client_list;
	struct sockaddr_in my_sockaddr, from_station;
	fd_set read_fds;	
    fd_set tmp_fds;	 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds
	
	int port = atoi(argv[1]);
	
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
	
	int newsockfd[CLIENTS], k = 0, cnt = 0;
	unsigned int clilen;
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
        cerr << "ERROR opening socket\n";
	}
	
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_port = port;
	inet_aton("127.0.0.1", &my_sockaddr.sin_addr);
	
	bind(sockfd, (struct sockaddr*)&my_sockaddr, sizeof(struct sockaddr_in));
	
	listen(sockfd, CLIENTS);
	
    FD_SET(sockfd, &read_fds);
    FD_SET(fileno(stdin), &read_fds);
    fdmax = sockfd;
    
    char buffer[BUFLEN];
    int n;
        
    while (1) {
    
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			cerr << "ERROR in select\n";
	
		for(int i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &tmp_fds)) {
			
				if (i == sockfd) {
					// conexiune noua
					clilen = sizeof(from_station);
					if ((newsockfd[k] = accept(sockfd, (struct sockaddr *)&from_station,
					 &clilen)) == -1) {
						cerr << "ERROR in accept\n";
					} 
					else {
						// Inregistrarea clientului in baza de date
						Client clnt[CLIENTS];
						bool ok = true;
						if((n = recv(newsockfd[k], buffer, sizeof(buffer), 0)) > 0) {
							sscanf(buffer, "%s %d", clnt[cnt].name, &clnt[cnt].sockaddr.sin_port);
							
							for (map<char*, Client>::iterator it = client_list.begin();
	 							it != client_list.end(); ++it) {
	 		
	 							if(strncmp(it->second.name, clnt[cnt].name, strlen(clnt[cnt].name)) == 0) {
	 								ok = false;
	 								memset(buffer, 0, BUFLEN);
	 								sprintf(buffer, "Existent name");
	 								if((n = send(newsockfd[k], buffer, strlen(buffer), 0)) < 0) {
										cerr << "Nu a fost trimis mesajul\n";
									}
	 							}
							}
						}
						// Clintul nu mai exista deja in baza de date
						if(ok == true) {
						
							FD_SET(newsockfd[k], &read_fds);
							time_t timer;
							time(&timer);
							inet_aton(inet_ntoa(from_station.sin_addr), &clnt[cnt].sockaddr.sin_addr);
							clnt[cnt].fd = newsockfd[k];
							clnt[cnt].timer = timer;
							memset(buffer, 0, BUFLEN);
							client_list[clnt[cnt].name] = clnt[cnt];
							cnt++;
						}
						
						if (newsockfd[k] > fdmax) { 
							fdmax = newsockfd[k];
						}
					}
					
					k++;
				}
					
				else {
					if(i != fileno(stdin)) {
						// se primesc date pe un socket de comunicare cu clientii
						memset(buffer, 0, 100);
						if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
							if (n == 0) {
								//conexiunea s-a inchis
								printf("server: socket %d hung up\n", i);
								for (map<char*, Client>::iterator it = client_list.begin();
	 								it != client_list.end(); ++it) {
	 		
	 								if(it->second.fd == i) {
	 									client_list.erase(it);
	 								}
								}
								close(i);
							} else {
								cerr << "ERROR in recv\n";
							}
							close(i); 
							FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
						}
						 
					
						else {
							printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n",
							 	i, buffer);
							solicitare(buffer, client_list, i);
						}
					}
					
					else {
									
						memset(buffer, 0 , BUFLEN);
						fgets(buffer, BUFLEN - 1, stdin);
					
						printf ("Am primit de la tastatura, mesajul: %s", buffer);
												
						if(strncmp(buffer, "status", 6) == 0) {

							status(client_list);
						}
												
						if(strncmp(buffer, "kick", 4) == 0) {
							char name[BUFLEN];
							sscanf(buffer, "%*s %s", name);
							kick(name, client_list, read_fds); 
						}

						if(strcmp(buffer, "quit\n") == 0) {
							sprintf(buffer, "quit");
							for (map<char*, Client>::iterator it = client_list.begin();
	 							it != client_list.end(); ++it) {
								if((n = send(it->second.fd, buffer, strlen(buffer), 0)) <	0) {
									cerr << "Nu a fost trimis mesajul\n";
								}
							}
							
							close(sockfd);
							exit(0);
						}
												
					}					
				} 
			}
		}
     }     
	
	return 0;
}
