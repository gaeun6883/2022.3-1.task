#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

//�Ұٹ�ȣ�� �ش� ���Ϲ�ȣ�� ������ �̸��� ���� ���� ����ü
typedef struct{
      char name[BUF_SIZE];
      int num;
}clnt;

void* handle_clnt(void* arg);
void send_msg_all(char* msg, int len);
void send_msg_one(int who, char* msg, int len);
void error_handling(char *message);

int clnt_cnt=0, s=0;//s�� clnt ����ü �����Ҵ翡 �Ҵ�� ������ ���Ѵ�.
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
clnt* c_list[MAX_CLNT];

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	pthread_mutex_init(&mutx, NULL);//mutex ����
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);  //tcp ����� �� ���̴�.
	
    //�ּ� ����
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)//�ּҸ� ���´�.(bind �Լ�)
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");
	
	while(1)
	{
	        clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);//clnt_sock ������ ���� ���� ����� �Ѵ�.
		
		pthread_mutex_lock(&mutx);//�Ӱ迵�� ��ȣ
		clnt_socks[clnt_cnt++]=clnt_sock;//���ӵ� ���� ���� �迭�� �ִ´�.
		pthread_mutex_unlock(&mutx);
		
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);//handle_clnt �Լ��� �����ϴ� ������ ����
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
	}

	close(serv_sock);//socket�� �ݴ´�.
	return 0;
}

void* handle_clnt(void* arg){
        int clnt_sock=*((int*)arg);
        int str_len=0, i;
        char msg[BUF_SIZE]="\0";
        
        while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0){//�޽����� �д´�.
                   char from[BUF_SIZE]="\0", who[BUF_SIZE]="\0";//������ ���� �̸��� ���� ���� �̸��� ����
                   char str[BUF_SIZE]="\0";//���� �޽����� ����
                   int who_n=-1, length=0;//who_n�� ���� ���� ���� ��ȣ, length�� ���� �޽����� ����
                   int idx=0;
                   for(int i=0; i<strlen(msg);i++){//�޽��� ���̰� �� �ɶ�����
                          idx++;
                          if(msg[i]==' ') break;//�޽����� ��ĭ�� ������ ��
                          from[i]=msg[i];//from�� �� �� �ܾ��̴�.
                   }
                   printf("\nfrom: %s\n", from);//from ���
                   clnt* c=(clnt*)malloc(sizeof(clnt));//����ü �Ҵ�
                   memcpy(c->name,&from,strlen(from));//�Ҵ�� ����ü�� �̸�(from)�ֱ�
                   c->num=clnt_sock;//��ȣ�� clnt_sock
                   
                   int flag=1;
                   for(int i=0; i<s; i++)
                        if(strcmp(c_list[i]->name,from)==0)//���� ����ü �迭�� �� �̸��� �̹� �ִٸ� �Ѿ��
                                flag=0;
                   if(flag==1){//���ٸ� �ֱ�
                        c_list[s]= c;
                        s++;
                   }
                   
                   //c_list�� ���¸� ���� ���� ���� ��¹�
                   /*printf("c_list size: %d\n", s);
                   for(int i=0; i<s; i++)
                         printf("c_list[%d] : %s - %d\n",i, c_list[i]->name, c_list[i]->num);*/
                   
                   //���� ��� who ã��
                   idx++;
                   for(int i=0; idx<strlen(msg);i++){
                       if(msg[idx]==' ') break;//��ĭ�� ���� ������
                       who[i]=msg[idx++];//���� ����� �ι�° �ܾ�
                   }
                   printf("to: %s\n", who);
                   for(int i=0; i<s; i++){//c_list Ž��
                       if(strcmp(c_list[i]->name,who)==0)//���� ��� �̸��� �ִٸ�
                              who_n=c_list[i]->num;//who_n�� �� ����� ���� ����, ���ٸ� -1 
                   }
                   //printf("who_n: %d\n", who_n);
                   
                   idx++;
                   str[0]='\0';
                   for(int i=0; idx<strlen(msg); i++)//�޽����� ������ ���� ����
                       str[i]=msg[idx++];
                   printf("send msg: %s\n", str);
                   length=strlen(str);//length�� ���� ������ ����
                       
                   if(strcmp(who,"all")==0) send_msg_all(str, length);//���� ����� all�̶�� ��ο��� ����
                   else {/
                      if (who_n==-1){//���� ����� ���ٸ�
                               char error_msg[BUF_SIZE]="no exist\n";//�������� ����
                               send_msg_one(clnt_sock, error_msg, strlen(error_msg));
                       }
                      else send_msg_one(who_n, str, length);//���� ����� Ư���ȴٸ� �� ������� ����
                   }
                   memset(msg, 0, BUF_SIZE);//�޽��� ���� ����
                   memset(str,0,BUF_SIZE);//str ���� ����
                   
         }
        
        //����� ������ ���� ����� �迭���� ������
        pthread_mutex_lock(&mutx);
        for(i=0; i<clnt_cnt; i++){
                 if(clnt_sock==clnt_socks[i]){
                         while(i<clnt_cnt-1){
                                clnt_socks[i]=clnt_socks[i+1];
                                i++;
                         }
                         break;
                   }
        }
        clnt_cnt--;
        pthread_mutex_unlock(&mutx);
        close(clnt_sock);
        return NULL;
 }
 
//��ο��� �޽��� ����
void send_msg_all(char* msg, int len){
        pthread_mutex_lock(&mutx);
        for(int i=0; i<clnt_cnt; i++){
             //printf("send: %d - %s\n", clnt_socks[i], msg);
             write(clnt_socks[i], msg, len);
        }
        pthread_mutex_unlock(&mutx);
}

//�ѻ�����Ը� �޽��� ����
void send_msg_one(int who, char* msg, int len){
       pthread_mutex_lock(&mutx);
       //printf("send: %d - %s\n", who, msg);
       write(who, msg, len);
       pthread_mutex_unlock(&mutx);
}

//������ �Ͼ�� ��
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
