#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<time.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio_ext.h>

#define _XOPEN_SOURCE 500
#define BUF_SIZE 100
#define NORMAL_SIZE 30

typedef struct
{
	int team;		// 1 이면 RED 팀, 2 이면 BLUE 팀,3 면 아직 팀 안 정해진 상태, else 이면 안 들어온 상태
	int ready_player;	// 1이면 READY, 0 이면 NO READY
	int check;
	int flag;
	char nickname[30];
	char message[50];
}State;

typedef struct
{
	int map_map[50][50];
	int P_score[4];
}pp;

//input자체를 주고받고 함. 나의 자리에 넣고 그것을 보낸다.
char input[4]= {0};


int getch();
int kbhit();
char getKey();

void* send_msg(void* arg);
void* recv_msg(void* arg);
void* send_game(void* arg);
void* recv_game(void* arg);
void error_handling(char* msg);
void present_state();
void printf_map();
void ending_credit();

char name[NORMAL_SIZE]; 
char mssg[BUF_SIZE];                    
char clnt_ip[NORMAL_SIZE];            

State s[4];



int temp;
char chat[16][100]={0,};//{"",};


int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	pthread_t snd_game_thread, rcv_game_thread;
	void* thread_return;
	void* thread_game_return;




	if (argc!=4)
	{
		printf(" Usage : %s <ip> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "[%s]", argv[3]);
	sprintf(clnt_ip, "%s", argv[1]);
	sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling(" conncet() error");

	read(sock, &s, sizeof(s));		// user number 받아 오는 read  	

	for(int i=0;i<4;i++)
	{
		if(s[i].check==0)
		{
			temp=i;
			break;
		}
	}


	strcpy(s[temp].nickname,name);
	s[temp].team=3;
	s[temp].ready_player=0;
	s[temp].check=1;

	write(sock,&s, sizeof(s));		// 내정보 입력하고 구조체배열전송
	read(sock, &s, sizeof(s));		// 이제부터 여기구조체에 입력받음  	
	present_state();	//연결되면 정보창 보이기	

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);
	return 0;
}

void* send_msg(void* arg)
{
	int sock=*((int*)arg);
	char name_msg[100];
	char *ptr;
	char msg[50];


	while(s[0].flag==0)
	{


		fgets(msg,sizeof(msg),stdin);		//게임으로 전환되면 엔터쳐야댐.... 조건걸기어려워

		ptr=strstr(msg,"//");	// //가 있으면 해당 주소값 반환, 없으면 NULL반환

		if(strcmp(msg,"/ready\n")==0)
		{
			s[2].message[0]='\0';
			s[1].message[0]='\0';
			s[0].message[0]='\0';
			s[temp].ready_player=1;
		}
		else if(strcmp(msg,"/unready\n")==0)
		{
			s[2].message[0]='\0';
			s[1].message[0]='\0';
			s[0].message[0]='\0';
			s[temp].ready_player=0;
		}
		else if(strcmp(msg,"/red\n")==0)
		{
			s[2].message[0]='\0';
			s[1].message[0]='\0';
			s[0].message[0]='\0';
			s[temp].team=1;
		}
		else if(strcmp(msg,"/blue\n")==0)
		{
			s[2].message[0]='\0';
			s[1].message[0]='\0';
			s[0].message[0]='\0';
			s[temp].team=2;
		}
		else if(ptr!=NULL&&s[temp].team==1)	//레드팀 채팅
		{
			sprintf(name_msg,"%s %s",name,msg);
			strcpy(s[1].message,name_msg);
			s[2].message[0]='\0';
			s[0].message[0]='\0';
		}
		else if(ptr!=NULL&&s[temp].team==2)	//블루팀 채팅
		{
			sprintf(name_msg,"%s %s",name,msg);
			strcpy(s[2].message,name_msg);
			s[1].message[0]='\0';
			s[0].message[0]='\0';
		}
		else if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			s[2].message[0]='\0';
			s[1].message[0]='\0';
			s[0].message[0]='\0';

			close(sock);	//소켓종료시키기	
			exit(0);		//프로세스종료
		}

		else
		{	
			sprintf(name_msg,"%s %s",name,msg);
			strcpy(s[0].message,name_msg);
			s[1].message[0]='\0';
			s[2].message[0]='\0';
		}

		write(sock, &s, sizeof(s));   
		if(strcmp(msg,"/ready\n")==0)
			break;
	}

	while(1){
		if(s[0].flag==1)
		{
			//커멘드 입력후 연산. 후 배열전송

			while(1)
			{
				if(kbhit())
					input[temp]=getch();
				write(sock, &input, sizeof(input));   	
				input[temp]=0;
				usleep(50000);
			}
		}
	}
	return NULL;
}
void* recv_msg(void* arg)
{

	int sock=*((int*)arg);		
	int str_len;				//read길이
	pp mp;

	while(s[0].flag==0)
	{
		str_len=read(sock, &s, sizeof(s));	// 서버에서 보낸 데이터 받기
		if (str_len==-1)
			return (void*)-1;
		present_state();
	}
	while(1){
		if(s[0].flag==1)
		{
			while(1)
			{
				str_len=read(sock, &mp, sizeof(mp));	// 서버에서 보낸 데이터 받기
				if (str_len==-1)
					return (void*)-1;
				printf("\n");
				system("clear");

				if(mp.P_score[0]>10000||mp.P_score[1]>10000||mp.P_score[2]>10000||mp.P_score[3]>10000) ending_credit(mp);
				else printf_map(mp);


			}
		}
	}
	return NULL;
}


void printf_map(pp mp)
{



	for (int i = 0; i < 50; i++)
	{
		for (int j = 0; j < 50; j++)
		{
			if (mp.map_map[i][j] == 0)
				printf("  ");
			else if (mp.map_map[i][j] == 1) printf("🟥");	// player 1
			else if (mp.map_map[i][j] == 2) printf("🟥");	// player 2
			else if (mp.map_map[i][j] == 3) printf("🟦");	// player 3
			else if (mp.map_map[i][j] == 4) printf("🟦");	// player 4			
			else if (mp.map_map[i][j] == 5) printf("💥");   // 폭탄
			else if (mp.map_map[i][j] == 9) printf("🏾");	//  뚫을 수 있는 벽	
			else if (mp.map_map[i][j] == 10) printf("🏿");	// 못 뚫는 벽

			else if (mp.map_map[i][j] == 11 || mp.map_map[i][j] == 12)
				printf("🦅");
		}

		if(i==2) printf("\t*****************************");
		if(i==4) printf("\t< 승리조건 : 1만점 넘기기>");
		if(i==6) printf("\t*****************************");
		if(i==8) printf("\t%s Player Score : %d",s[0].nickname, mp.P_score[0]);
		if(i==9) printf("\t%s Player Score : %d",s[1].nickname, mp.P_score[1]);
		if(i==10) printf("\t%s Player Score : %d",s[2].nickname, mp.P_score[2]);
		if(i==11) printf("\t%s Player Score : %d",s[3].nickname, mp.P_score[3]);
		if(i==13) printf("\t*****************************");
		if(i==15) printf("\t  플레이어 맞추면 1000점");
		if(i==16) printf("\t  독수리 맞추면 500점");
		if(i==17) printf("\t  벽 맞추면 100점");
		if(i==18) printf("\t  미사일 발사 비용 : -50점");
		if(i==20) printf("\t*****************************");

		printf("\n");
	}

}

void error_handling(char* mssg)
{
	fputs(mssg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void present_state()
{
	system("clear");
	char ready[4][4];	// Ready 출력용
	int i,j,t;

	printf(" name        : %s \n", name);
	printf(" IP          : %s \n", clnt_ip);
	printf(" Exit 	     : q or Q\n");
	printf(" 팀변경      : /red , /blue\n");
	printf(" 레디        : /ready (레디후 입력불가)\n");
	printf(" 팀채팅      : //할말\n");
	printf("\n\t------------ TANK BATTLEON ------------\n");
	printf("\tTEAM  ||  READY  ||  NICKNAME\n");
	printf("\t───────────────────────────────────────\n");

	for (i = 0; i < 4; i++)
	{
		//----------------------------------------------------------------------------------------
		if (s[i].ready_player == 1)		// ready 여부 감지
			strcpy(ready[i], "yes");	// ready_player[i] == 1 이면 ready : yes 로 표기
		else if (s[i].ready_player == 0)
			strcpy(ready[i], "no");	// ready_player[i] == 0 이면 ready : no 로 표기
		//----------------------------------------------------------------------------------------
		if (s[i].team == 1)	//  RED 팀
			printf("\x1b[31m" "\t%-4s\t   %-4s\t    %-16s\n"  "\x1b[0m\n", "RED", ready[i], s[i].nickname);

		else if (s[i].team == 2)	//  BLUE 팀
			printf("\x1b[34m" "\t%-4s\t   %-4s\t    %-16s\n"  "\x1b[0m\n", "BLUE", ready[i], s[i].nickname);

		else if (s[i].team == 3)	//  팀 안 정해진 상태
			printf("\t%-4s\t   %-4s\t    %-16s\n\n", "NONE", ready[i], s[i].nickname);

		else // 
			printf("\t%-4s\t   %-4s\t    %-16s\n\n", "NONE", "NONE", "NONE");
		//----------------------------------------------------------------------------------------
	}


	printf("\t------------ TANK BATTLEON ------------\n");

	if(s[0].message[0]!='\0')	//전체채팅받음	
		s[0].message[strlen(s[0].message)-1]='\0';	// 문자열 끝 개행문자 \n 제거

	else if(s[temp].team==1&&(s[1].message[0]!='\0'))	//레드팀채팅받음	
		s[1].message[strlen(s[1].message)-1]='\0';

	else if(s[temp].team==2&&(s[2].message[0]!='\0'))	//블루팀 채팅받음
		s[2].message[strlen(s[2].message)-1]='\0';

	printf("\n");					// 위치는 여기가 맞다


	//입력한 채팅 옮겨주는거
	if(s[0].message[0]!='\0')	//전체채팅받음	
		strcpy(chat[15], s[0].message);

	else if(s[temp].team==1&&(s[1].message[0]!='\0'))	//레드팀채팅받음	
		strcpy(chat[15], s[1].message);

	else if(s[temp].team==2&&(s[2].message[0]!='\0'))	//블루팀 채팅받음
		strcpy(chat[15], s[2].message);

	//채팅옮겨주는거
	if(s[0].message[0]!='\0'){	//전체채팅받음	
		for(j=0;j<15;j++){
			strcpy(chat[j],chat[j+1]);
		}

	}
	else if(s[temp].team==1&&(s[1].message[0]!='\0')){	//레드팀채팅받음	
		for(j=0;j<15;j++){
			strcpy(chat[j],chat[j+1]);
		}

	}
	else if(s[temp].team==2&&(s[2].message[0]!='\0')){	//블루팀 채팅받음
		for(j=0;j<15;j++){
			strcpy(chat[j],chat[j+1]);
		}
	}
	//아래서 위
	for(j=0;j<15;j++){
		printf("\t%s\n",chat[j]);
	}

	printf("\t------------------------------------------\n");
	printf("\t입력 : ");
	fflush(stdout);





}


void ending_credit(pp mp)
{

	int a[4];
	int temp[4];
	int first;
	int ary[18];
	int t=0;



	for(int i=0;i<4;i++)
		temp[i]=mp.P_score[i];	

	first=temp[0];
	for(int j=0 ;j<4;j++){

		for(int i=0; i<4;i++)
			if(temp[i]>first)
			{	
				first=temp[i];
				a[j]=i;
			}

		temp[a[j]]=-100;
		first = temp[a[j]];
	}


			
	while(1)
	{
		for(int j=0;j<18;j++)
		{
			if(ary[j]==1)	printf("■ ");
			else if(ary[j]==0)	printf("□ ");
		}
		printf("\n");

		for(int i=0;i<4;i++){
			if(ary[i]==1)	printf("□ ");
			else if(ary[i]==0)	printf("■ ");
			printf("\t%d등  %5s   %4d\t", i+1,s[a[i]].nickname,mp.P_score[a[i]] );	
			if(ary[i]==1)	printf("  ■");
			else if(ary[i]==0)	printf("  □");
			printf("\n");
		}
			
			for(int j=0;j<18;j++)
		{
			if(ary[j]==1)	printf("□ ");
			else if(ary[j]==0)	printf("■ ");
		}
			printf("\n");

			usleep(100000);
			system("clear");
			for(int j=0;j<18;j++)
			{
				if(ary[j]==0)	printf("■ ");
				else if(ary[j]==1)	printf("□ ");
			}
			printf("\n");

			for(int i=0;i<4;i++){
				if(ary[i]==0)	printf("□ ");
				else if(ary[i]==1)	printf("■ ");
				printf("\t%d등  %5s   %4d\t", i+1,s[a[i]].nickname,mp.P_score[a[i]]);
				if(ary[i]==0)	printf("  ■");
				else if(ary[i]==1)	printf("  □");
				printf("\n");
			}

			for(int j=0;j<18;j++)
			{
				if(ary[j]==0)	printf("□ ");
				else if(ary[j]==1)	printf("■ ");
			}
				printf("\n");
				usleep(100000);
				system("clear");
				if(t>=25)	break;
				t++;
	}
			
			

	exit(0);
}


int getch()
{
	int c;
	struct termios oldattr, newattr;

	tcgetattr(STDIN_FILENO, &oldattr);           // 현재 터미널 설정 읽음
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);         // CANONICAL과 ECHO 끔
	newattr.c_cc[VMIN] = 1;                      // 최소 입력 문자 수를 1로 설정
	newattr.c_cc[VTIME] = 0;                     // 최소 읽기 대기 시간을 0으로 설정
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);  // 터미널에 설정 입력
	c = getchar();                               // 키보드 입력 읽음
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);  // 원래의 설정으로 복구
	return c;
}

int kbhit(void)
{
	/* - sleep(1) 1초
	   - usleep(1000 * 1000) 1초, micro초 (1/1000) */

	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

char getKey()
{
	__fpurge(stdin);
	if (kbhit()) {
		return getch();
	}
	return 0;

}








