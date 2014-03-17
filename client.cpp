//325CA Dragan Dan-Stefan

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <vector>

#define CLIENTS 100
#define BUFLEN 256
#define SNDMSG 1024

using namespace std;

// Data si ora
char *time_stamp() {

	char *timestamp = new char[25];
	time_t timp;
	timp = time(NULL);
	struct tm *tm;
	tm = localtime(&timp);

	sprintf(timestamp,"%04d.%02d.%02d %02d:%02d:%02d",
		tm->tm_year+1900, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
    	
	return timestamp;
}

// Solicita serverului lista clientilor
void listclients(const char *my_name, int sockfd) {

	char buffer[BUFLEN];
	int n;
	
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "listclients %s", my_name);
	 
	if((n = send(sockfd, buffer, strlen(buffer), 0)) < 0) {
		cerr << "Nu s-a trimis comanda\n";
	}	
}

// Solicita serverului informatii despre un anumit client
void infoclient(char *client, int sockfd) {
	
	char buffer[BUFLEN];
	int n;
	
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "infoclient %s", client);
	 
	if((n = send(sockfd, buffer, strlen(buffer), 0)) < 0) {
		cerr << "Nu s-a trimis comanda\n";
	}
}

// Solicita ip-ul si portul unui client
void mess(char *client, int sockfd) {
	
	char buffer[BUFLEN];
	int n;
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "message %s", client);
	
	if((n = send(sockfd, buffer, strlen(buffer), 0)) < 0) {
		cerr << "Nu s-a trimis comanda\n";
	}	
}

// Trimite un mesaj unui client
void send_msg(char *my_name, char *ip, int port, char *mess) {
	
	char buffer[BUFLEN];
	int n;
	
	memset(buffer, 0, BUFLEN);
		
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = port;
	inet_aton(ip, &sockaddr.sin_addr);
	
	connect(client_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
	
	char *timestamp = new char[25];
	timestamp = time_stamp();
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "%s %s: %s",timestamp, my_name, mess);

	if((n = send(client_fd, buffer, strlen(buffer), 0) < 0)) {
		cerr << "Mesajul nu a fost trimis\n";
	}

	close(client_fd);
}

// Solicita serverului ip-urile si porturile tuturor clientilor
void broad_msg(int sockfd) {
	
	char buffer[BUFLEN];
	int n;
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "broadcast");
	
	if((n = send(sockfd, buffer, strlen(buffer), 0)) < 0) {
		cerr << "Nu s-a trimis comanda\n";
	}	
}

// Trimite un mesaj tuturor clientilor
void broadcast(char *my_name, char *mess, char *clients) {

	char buffer[BUFLEN];
	int n;
	
	memset(buffer, 0, BUFLEN);
	int client_fd[100], k = 0;
	strtok(clients, " ");
	char *ip = strtok(NULL, " ");
	while(ip != NULL) {
		
		int port = atoi(strtok(NULL, " \0"));
		client_fd[k] = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in sockaddr;
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = port;
		inet_aton(ip, &sockaddr.sin_addr);
	
		connect(client_fd[k], (struct sockaddr*) &sockaddr, sizeof(sockaddr));
		char *timestamp = new char[25];
		timestamp = time_stamp();
		memset(buffer, 0, BUFLEN);
		sprintf(buffer, "%s %s: %s",timestamp, my_name, mess);

		if((n = send(client_fd[k], buffer, strlen(buffer), 0) < 0)) {
			cerr << "Mesajul nu a fost trimis\n";
		}

		close(client_fd[k]);
		k++;
		ip = strtok(NULL, " ");		
	}
}

// Solicita informatii despre un client
void send_file(char *client, int sockfd) {
	
	char buffer[BUFLEN];
	int n;
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "sendfile %s", client);
	
	if((n = send(sockfd, buffer, strlen(buffer), 0)) < 0) {
		cerr << "Nu s-a trimis comanda\n";
	}	
}

// Trimite un fisier unui client
void sendfile(char *ip, int port, char *mess, char *filename) {
	
	struct stat f_status;
	filename = strtok(filename, "\n");
	int fd = open(filename, O_RDONLY);
	fstat(fd, &f_status);
	
	int filesize = (int) f_status.st_size;
	char buffer[SNDMSG];
	int n, count;
	
	memset(buffer, 0, SNDMSG);
		
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = port;
	inet_aton(ip, &sockaddr.sin_addr);
	
	connect(client_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
	
	memset(buffer, 0, SNDMSG);
	sprintf(buffer, "SENDFILE %s %d", filename, filesize);
	if((n = send(client_fd, buffer, strlen(buffer), 0) < 0)) {
			cerr << "Mesajul nu a fost trimis\n";
	}
	memset(buffer, 0, SNDMSG);
	while((count = read(fd, buffer, SNDMSG - 1)) > 0) {
		if((n = send(client_fd, buffer, strlen(buffer), 0) < 0)) {
			cerr << "Mesajul nu a fost trimis\n";
		}
		memset(buffer, 0, SNDMSG);
	}

	close(fd);
	close(client_fd);
}

int main (int argc, char **argv) {

	char name[BUFLEN];
	strcpy(name, argv[1]);
	int my_port = atoi(argv[2]);
	struct timeval tv = {0, 10};
	struct sockaddr_in my_sockaddr, serv_sockaddr, from_station;
	inet_aton(argv[3], &serv_sockaddr.sin_addr);
	serv_sockaddr.sin_port = atoi(argv[4]);
	serv_sockaddr.sin_family = AF_INET;
	
	inet_aton("127.0.0.1", &my_sockaddr.sin_addr);
	my_sockaddr.sin_port = my_port;
	my_sockaddr.sin_family = AF_INET;
	
	fd_set read_fds;	
    fd_set tmp_fds;	
    int fdmax;		
	
	FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    int newsockfd[CLIENTS], k = 0;
	unsigned int clilen;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
        cerr << "ERROR opening socket\n";
	}
	
	char buffer[SNDMSG];
	int n;
	memset(buffer, 0, BUFLEN);
	connect(sockfd, (struct sockaddr*) &serv_sockaddr, sizeof(serv_sockaddr));
	sprintf(buffer, "%s %d", name, my_port);

	if((n = send(sockfd, buffer, strlen(buffer), 0)) < 0) {
		cerr << "Nu s-a trimis numele si portul\n";
	}
	
	int clients_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (clients_fd < 0) {
        cerr << "ERROR opening socket\n";
	}
	
	bind(clients_fd, (struct sockaddr*)&my_sockaddr, sizeof(struct sockaddr_in));
	
	listen(sockfd, CLIENTS);
	listen(clients_fd, CLIENTS);
	
	FD_SET(clients_fd, &read_fds);
	FD_SET(sockfd, &read_fds);
    FD_SET(fileno(stdin), &read_fds);
    fdmax = clients_fd;
    char *history[100];
    for(int i = 0; i < 100; i++) {
    	history[i] = new char[BUFLEN];
    }
    int c = 0;
    char *message, *copie, *filename, *fname;    
    message = new char[BUFLEN];
    copie = new char[BUFLEN];
    filename = new char[30];
    fname = new char[30];
    int fd, filesize;
    
    while (1) {
    
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, &tv) == -1) 
			cerr << "ERROR in select\n";
	
		for(int i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &tmp_fds)) {

				if (i == clients_fd) {
					// Stabileste conexiune cu un client
					clilen = sizeof(from_station);
					if ((newsockfd[k] = accept(clients_fd, (struct sockaddr *)&from_station,
						&clilen)) == -1) {
						cerr << "ERROR in accept\n";
					} 
					else {

						FD_SET(newsockfd[k], &read_fds);
						if((n = recv(newsockfd[k], buffer, sizeof(buffer), 0)) > 0) {
							// Creaza fisierul
							if(strncmp(buffer, "SENDFILE", 8) == 0) {
								char *name = new char[BUFLEN];
								strtok(buffer, " ");
								filename = strtok(NULL, " ");
								filesize = atoi(strtok(NULL, "/0"));
								sprintf(name, "%s_primit", filename);
								strcpy(history[c++],filename);
								fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							}							
							
							else {
								strcpy(history[c++],buffer);
								cout << buffer << endl;
							}
						}
					
						if (newsockfd[k] > fdmax) { 
							fdmax = newsockfd[k];
						}
					}
					k++;					
				
				}
				// Mesaje de la server
				else if (i == sockfd) {
					memset(buffer, 0, BUFLEN);
					if ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
						cout << buffer << endl;
					}
					
					if(strncmp(buffer, "message", 7) == 0) {
						if(strncmp(buffer, "message Nu", 10) == 0) {
							char *mess = new char[BUFLEN];
							strtok(buffer, " ");
							mess = strtok(NULL, "/0");
							cout << mess << endl;
						}
						else {
							char ip[20];
							int port;
							sscanf(buffer, "%*s %s %d", ip, &port);
							send_msg(name, ip, port, copie);
						}
					}					
					
					if(strncmp(buffer, "broadcast", 9) == 0) {
						broadcast(name, copie, buffer);
					}
					
					if(strncmp(buffer, "sendfile", 8) == 0) {
						if(strncmp(buffer, "sendfile Nu", 11) == 0) {
							char *mess = new char[BUFLEN];
							strtok(buffer, " ");
							mess = strtok(NULL, "/0");
							cout << mess << endl;
						}
						else {
							char ip[20];
							int port;
							sscanf(buffer, "%*s %s %d", ip, &port);
							sendfile(ip, port, copie, fname);
						}
					}
					
					if(strncmp(buffer, "KICK", 4) == 0) {
						close(sockfd);
						close(clients_fd);
						exit(0);
					}
					
					if(strncmp(buffer, "quit", 4) == 0) {
						close(sockfd);
						close(clients_fd);
						exit(0);
					}
					
					if(strncmp(buffer, "Existent", 8) == 0) {
						close(sockfd);
						close(clients_fd);
						exit(0);
					}					
				}
					
				else {
					// Mesaje de la ceilalti clienti
					if(i != fileno(stdin)) {
						
						memset(buffer, 0, SNDMSG);

						if ((n = recv(i, buffer, sizeof(buffer) - 1, 0)) <= 0) {
							if (n == 0) {
								
							} else {
								cerr << "ERROR in recv\n";
							}
							close(i); 
							FD_CLR(i, &read_fds);  
						}
						 
					
						else { 
							
							write(fd, buffer, strlen(buffer));
														
						}
						
					}
					// Mesaje de la tastatura
					else {
									
						memset(buffer, 0 , BUFLEN);
						fgets(buffer, BUFLEN - 1, stdin);
					
						printf ("Am primit de la tastatura, mesajul: %s", buffer);
											
						if(strcmp(buffer, "quit\n") == 0) {
							memset(buffer, 0, BUFLEN);
							printf(buffer, "quit");

							if((n = send(sockfd, buffer, strlen(buffer), 0) < 0)) {
								cerr << "Mesajul nu a fost trimis\n";
							}
							
							close(clients_fd);
							close(sockfd);
							exit(0);
						}
						
						if(strncmp(buffer, "history", 7) == 0) {
							for(int i = 0; i < c; i++) {
								cout << history[i] << endl;								
							}
						}
						
						if(strcmp(buffer, "listclients\n") == 0) {
							listclients(name, sockfd);
						}
						
						if(strncmp(buffer, "infoclient", 10) == 0) {
							char client[BUFLEN];
							sscanf(buffer, "%*s %s", client);
							infoclient(client, sockfd);
						}
						
						if(strncmp(buffer, "message", 7) == 0) {
							char *client;
							client = new char[BUFLEN];
							strtok(buffer, " ");
							client = strtok(NULL, " ");
							message = strtok(NULL, "\0");
							strcpy(copie, message);
							mess(client, sockfd);
						}
						
						if(strncmp(buffer, "broadcast", 9) == 0) {
							strtok(buffer, " ");
							message = strtok(NULL, "\0");
							strcpy(copie, message);
							broad_msg(sockfd);
						}					
						
						if(strncmp(buffer, "sendfile", 8) == 0) {
							char *client;
							client = new char[BUFLEN];
							strtok(buffer, " ");
							client = strtok(NULL, " ");
							filename = strtok(NULL, "\0");
							strcpy(fname, filename);
							send_file(client, sockfd);
						}																
					}					
				} 
			}
		}
     }
    	
	return 0;
}
