#include <stdio.h> 
#include <stdlib.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER 200


int totalClient;
key_t key; 
int msgid; 
int wordCount[26] = {0};
char final_result[BUFFER];
int count = 0;

pthread_mutex_t array_lock = PTHREAD_MUTEX_INITIALIZER;



void createMessageQueue()
{
	key = 1234; //generating unique key
	msgid = msgget(key, 0666 | IPC_CREAT); 
}

//update data
void increaseCharCount(char ch, int c)
{
	pthread_mutex_lock(&array_lock);
	wordCount[ch-'a']++;
	pthread_mutex_unlock(&array_lock);
}


struct message_buffer { 
	long type; 
	char text[BUFFER]; 
};


//To generate string from array data
void generateResult()
{
	strcpy(final_result, "");
	char buffer[BUFFER];
	for(int i=0;i<26;i++)
	{
		sprintf(buffer,"%d", wordCount[i]);
		strcat(final_result, buffer);
		if(i != 25)
			strcat(final_result,"#");
	}
}

//send final result to all client process
void *sendFinalResult(void *arg)
{
	int c = *((int*) arg);
	struct message_buffer global_data;
	global_data.type = totalClient * 2 + c + 1;
	strcpy(global_data.text, final_result);
	msgsnd(msgid, &global_data, sizeof(global_data), 0);
}

//handle client request
void *handleClient(void *arg)
{
	int c = *((int*) arg);
	int conversationOver = 0;
	struct message_buffer global_data;
	while(!conversationOver)
	{
		struct message_buffer message, ack; 
		msgrcv(msgid, &message, sizeof(message), c+1, 0);
		printf("Thread %d received %s from client process %d\n", c, message.text, c);
		FILE *file;
		char buffer[BUFFER];
		if(strcmp(message.text, "END") != 0)
		{
		
			file = fopen(message.text, "r");
			
			
			while(fscanf(file, "%s", buffer)!=-1)
			{
				char ch = buffer[0];
				increaseCharCount(ch, c);
				
			}
			fclose(file);
			
			strcpy(ack.text, "ACK");
			ack.type = totalClient+c+1;
			printf("Sending %s to client %d for %s\n", ack.text, c, message.text);
			msgsnd(msgid, &ack, sizeof(ack), 0);	
		}
		else
		{
			conversationOver = 1;
			printf("Thread %d received %s from client process %d", c, message.text, c);	
		}
	}

}

int main(int argc, char* argv[]) 
{ 
	printf("Server starts...\n");
	totalClient = atoi(argv[1]);
	pthread_t tid[totalClient];
	createMessageQueue();
	for(int i=0;i<totalClient;i++)
	{
		int *temp = malloc(sizeof(int));
		*temp = i;
		pthread_create(&tid[i], NULL, handleClient,(void *)temp);
	}
	for(int i=0;i<totalClient;i++)
	{
		pthread_join(tid[i], NULL);
	}
	generateResult();
	for(int i=0;i<totalClient;i++)
	{
		int *temp = malloc(sizeof(int));
		*temp = i;
		pthread_create(&tid[i], NULL, sendFinalResult,(void *)temp);
	}
	for(int i=0;i<totalClient;i++)
	{
		pthread_join(tid[i], NULL);
	}

	printf("Server ends...\n");
	return 0; 
	
} 

