#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <netdb.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include "game.h"

#define MAX_CLIENTS 6
#define BUFFER_SZ 256
#define PORTNUM  1590 /* the port number the server will listen to*/
#define DEFAULT_PROTOCOL 0  /*constant for default protocol*/

static int cli_count = 0;
static int uid = 0;

/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;


//declarations
int ready[5];
int ready_flag = 0;
int ready_flag2 = 0;
char deck[18];
char symbolDeck[18];
char value[5][2];
int game_start = 0;
int game_start_alt = 0;
int score[5];
int player = 0; //which client's turn it is
int turn = 0; //0 and 1
int alt;


void checkReady(char*,client_t*);
void printBoard(client_t*);
int userInput(char, client_t*);
void restart();

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void send_all(char *s){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(write(clients[i]->sockfd, s, strlen(s)) < 0){
				perror("ERROR: write to descriptor failed");
				break;
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to the sender */
void sender_message(char *buffer,int uid){
	int status = write(clients[uid]->sockfd,buffer,BUFFER_SZ);
	if(status < 0){
		perror("Error writing to socket");
		exit(1);
	}
}

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} 
  else{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);
	}

	bzero(buff_out, BUFFER_SZ);
	while(1){
		if (leave_flag) {
			break;
		}
		//need to change here *********************************************************
		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0){   
			if(strlen(buff_out) > 0){
        if(gameOver() == 0){
          if(ready_flag == 0)
            checkReady(buff_out, cli);
            
          if(ready_flag == 1){
          
            //not alternating
            if(alt == 0){
              if(userInput(buff_out[0],cli) != 1){
                if(gameOver() == 0)          
                  printBoard(cli);
                else
                  restart();
              }
            }
            //alternating
            else{
              if(cli->uid == player && ready_flag2 == 1){
                int tmp = userInput(buff_out[0],cli);
                //turn 2
                if(tmp != 1 && turn == 1){
                  if(tmp == 0){
                    turn = 0;
                    player +=1;
                    if(player == cli_count)
                      player = 0;
                  }
                  if(gameOver() == 0)          
                    printBoard(cli);
                  else
                    restart();
                }
                //turn 1
                else if(tmp != 1 && turn == 0){
                  if(tmp == 0)
                    turn = 1;
                  if(gameOver() == 0)          
                    printBoard(cli);
                  else
                    restart();
                }
              }
              else if(cli->uid != player && ready_flag2 == 0){
                printBoard(cli);
                ready_flag2 = 1;
                
              }
                
              else if(cli->uid != player && ready_flag2 == 1){
                bzero(buff_out,BUFFER_SZ);
                sprintf(buff_out,"It's not your turn.\n");
                sender_message(buff_out,cli->uid);
              }
              
            }
          }
  				//send_message(buff_out, cli->uid);
  				//str_trim_lf(buff_out, strlen(buff_out));
  				printf("%s -> %s\n", cli->name, buff_out);
        }
        else{
          //game is over reset
          printf("game over\n");
        }
			}
		} else if (receive == 0 || strcmp(buff_out, "exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

  /* Delete client from queue and yield thread */
	close(cli->sockfd);
  uid = cli->uid;
  queue_remove(cli->uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self());

	return NULL;
}

void checkReady(char* check, client_t *cli){
  int comp = strncmp(check, "ready to play",13);
  int flag = 0;
  
  if(comp == 0)
    ready[cli->uid] = 1;
  else if(comp != 0 && ready_flag == 0)
  {
    bzero(check,BUFFER_SZ);
    sprintf(check,"Type \"ready to play\" when ready\n");
    sender_message(check,cli->uid);
    bzero(check,BUFFER_SZ);
  }
  if(cli_count >1){
    int i;
    for(i = 0;i < cli_count; i++){
      if(ready[i] != 1){
        flag = 1;
        break;
      }
    }

    bzero(check,BUFFER_SZ);
    if(flag == 1 && comp == 0){
      sprintf(check, "Waiting for other players.\nGame will start when other player is ready.\n");
      sender_message(check,cli->uid);
      bzero(check,BUFFER_SZ);
    }
    else if(flag == 0){
      ready_flag = 1;
      printf("\nSymbols\n");
      printArray(symbolDeck,18,check);
      printf("%s\n",check);
      bzero(check,BUFFER_SZ);
    }
    if(comp==0){
      sprintf(check,"%s is ready\n", cli->name);
      send_message(check,cli->uid);
      printf("%s\n",check);
      bzero(check,BUFFER_SZ);
    }
  }
  else{
    bzero(check,BUFFER_SZ);
    sprintf(check, "Waiting for more players to join\n");
    sender_message(check,cli->uid);
    bzero(check,BUFFER_SZ);
  }
}

void printBoard(client_t* cli){
  char buffer[BUFFER_SZ];
  bzero(buffer,BUFFER_SZ);

  strcat(buffer,"Current Board\n");
	printArray(deck,18,buffer);
  strcat(buffer,"\n");
  printf("%s",buffer);
  
  
  if(alt == 0){
    strcat(buffer,"Select your card.\n");
    send_all(buffer);
  } 
  else{
    send_all(buffer);
    bzero(buffer,BUFFER_SZ);
    if(ready_flag2 == 1){
      if(game_start_alt == 0){
        sprintf(buffer,"Select your card.\n");
        sender_message(buffer,cli->uid);
        game_start_alt += 1;
      }
      else{
        sprintf(buffer,"Select your card.\n");
        if(turn == 1){
          sender_message(buffer,cli->uid);
        }
        else if(turn == 0){
          int i = cli->uid;
          if((i+1) != cli_count)
            sender_message(buffer,(cli->uid+1));
          else if((i+1) == cli_count)
            sender_message(buffer,0);
        }
      }
    }
    else{
      sprintf(buffer,"Select your card.\n");
      sender_message(buffer,0);
    }
  }
}

void updateBoard(){
  char buffer[BUFFER_SZ];
  bzero(buffer,BUFFER_SZ);
	printArray(deck,18,buffer);
  strcat(buffer,"\n\n");
  send_all(buffer); 
}


int userInput(char card, client_t* cli){
  char buffer[BUFFER_SZ];
  char buffer2[2];
  bzero(buffer2,2);
  bzero(buffer,BUFFER_SZ);

  if(deck[check(card)] == 'X'){
    strcat(buffer,"\nCard already matched. Choose another card.\n");
    sender_message(buffer,cli->uid);
    bzero(buffer,BUFFER_SZ);  
    return 1;
  }
  
  char choice[18]= {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r'};
  int i, correct = 0;
  
  for(i=0;i<18;i++)
  {
    if(card == choice[i]){
      correct = 1;
      break;
    }
  }
  
  if(correct == 0){
    return -1;
  }

  if(game_start != 0){
    strcat(buffer, "\n");
    strcat(buffer, cli->name);
    strcat(buffer," made the move.\n");
    send_message(buffer,cli->uid);
    bzero(buffer,BUFFER_SZ);
  }
  else
    game_start = 1;
   
   
  if(value[cli->uid][0] == '\0'){
    value[cli->uid][0] = card;
    int pos = check(value[cli->uid][0]);
    deck[pos] = symbolDeck[pos];
  }
  else{ 
    value[cli->uid][1] = card;
    int pos = check(value[cli->uid][1]);
    deck[pos] = symbolDeck[pos]; 
    updateBoard();
    
    if(symbolDeck[check(value[cli->uid][0])] == symbolDeck[check(value[cli->uid][1])]){
      score[cli->uid] += 1;
      
      strcat(buffer, cli->name);
      strcat(buffer," made a match.\nTheir current points: ");
      sprintf(buffer2,"%d",score[cli->uid]);
      strcat(buffer,buffer2);
      strcat(buffer,"\n");
      send_all(buffer);
      
      deck[check(value[cli->uid][0])] = 'X';
      deck[check(value[cli->uid][1])] = 'X';
    }
    else{
      deck[check(value[cli->uid][0])] = value[cli->uid][0];
      deck[check(value[cli->uid][1])] = value[cli->uid][1];
    }
    bzero(value[cli->uid],2);
  }
  return 0;
}

int gameOver(){
  if(strncmp(deck,"XXXXXXXXXXXXXXXXXX",18) == 0){
    return 1;
  }
  return 0;
}

void restart(){
  char buffer[BUFFER_SZ];
  char buffer2[BUFFER_SZ];
  bzero(buffer2,BUFFER_SZ);
  bzero(buffer,BUFFER_SZ);
  
  sprintf(buffer,"All cards have been matched.\n");
  int i;
  int highest = -1;
  for(i=0;i<cli_count;i++){
    if(score[i] > highest)
      highest = i;
  }
  strcat(buffer,clients[highest]->name);
  strcat(buffer," won with the score of ");
  sprintf(buffer2,"%d",score[highest]);
  strcat(buffer,buffer2);
  send_all(buffer);
  
  bzero(buffer,BUFFER_SZ);
  sprintf(buffer,"\nType \"ready to play\" to continue or \"exit\" to exit\n");
  send_all(buffer);
  
  //reset everything
  ready_flag = 0;
  game_start = 0;
  game_start_alt = 0;
  player = 0;
  turn = 0;

  for(i=0; i< 5; i++){
    bzero(value[i],2);
  }
  bzero(score,5);
  bzero(ready,5);
  bzero(symbolDeck,18);
  bzero(deck,18);
  game(deck,symbolDeck);
}


int main(int argc, char **argv){
   
   int listenfd = 0, connfd = 0, option = 1, port, clilen;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int status, pid;
   pthread_t tid;
   
   
   //declaration
   game(deck,symbolDeck);
   int i;
   for(i=0; i< 5; i++){
     bzero(value[i],2);
   }
   bzero(score,5);

   char tmpA;
   printf("Choose if you want to alternate between players(y/n)");
   scanf(" %c",&tmpA);
   if(tmpA == 'y'){
       alt = 1;
   }
   else if(tmpA == 'n'){
       alt = 0;
   }
   
   bzero(buffer,BUFFER_SZ);
   
   /* First call to socket() function */
   listenfd = socket(AF_INET, SOCK_STREAM,DEFAULT_PROTOCOL );

   
   if (listenfd < 0) {
      perror("ERROR opening socket");
      exit(1);
   }
   
   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   port = PORTNUM;
   
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(port);
   
   
   /* Ignore pipe signals */
 	 signal(SIGPIPE, SIG_IGN);

	 if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
 		 perror("ERROR: setsockopt failed");
     return EXIT_FAILURE;
	 }
   
   /* Now bind the host address using bind() call.*/

   status =  bind(listenfd, (struct sockaddr *) &serv_addr, sizeof	(serv_addr)); 

   if (status < 0) {
      perror("ERROR on binding");
      exit(1);
   }



  /* Listen */
  if (listen(listenfd, 6) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
	}

	printf("Server Created.\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
    if(clients[uid+1] == NULL)
		  cli->uid = uid++;
    else{
      while(uid != cli_count - 1 && clients[uid + 1] == NULL )
      {
        uid++;
      }
      cli->uid = uid++;  
    }
		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
