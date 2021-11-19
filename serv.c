#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 200
#define MAX_CLNT 256

typedef struct
{
    int team;	// 0 이면 RED 팀, 1 이면 BLUE 팀,2 면 아직 팀 안 정해진 상태,3 이면 안 들어온 상태
    int ready_player;	// 1이면 READY, 0 이면 NO READY
    int check;
    int flag;
    char nickname[30];
    char message[50];
}State;

typedef struct
{
    int xpos;
    int ypos;
}P;


typedef struct
{
    int xpos;
    int ypos;
    int direction;
    int on;
}mi;


typedef struct
{
	int map_map[50][50];
	int P_score[4];
}pp;


char input[4] = {0};


void * handle_clnt(void * arg);
void send_msg();
void error_handling(char * msg);
void send_msg_s(int clnt_sock);   // send to all
void send_game();   // send game to all
void map_initialization();
void missile();
void player_move();
void map_side();



int clnt_cnt=0;
int clnt_socks[MAX_CLNT];


State s[4];	// 데이터 저장 역할
P player[4];
mi bullet[4][6];
pp mp;

pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    srand((unsigned)time(NULL));

    if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }


    for(int i=0;i<4;i++){
        s[i].team=4;		// 1 2 3
        s[i].ready_player=0;	//0 1
        s[i].check=0;		// 0 1 
        s[i].flag=0;		// 0 1 
    }

/*
    map_map[0][0]=1;
    map_map[0][21]=2;
    map_map[21][0]=3;
    map_map[21][21]=4;

    player[0].xpos = 0;	
    player[0].ypos = 0;

    player[1].xpos = 21;	
    player[1].ypos = 0;	

    player[2].xpos = 0;	
    player[2].ypos = 21;	

    player[3].xpos = 21;	
    player[3].ypos = 21;	
*/
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<6;j++){

            bullet[i][j].xpos=0;
            bullet[i][j].ypos=0;
            bullet[i][j].direction=0;
            bullet[i][j].on=0;
        }
    }
	map_initialization();




    pthread_mutex_init(&mutx, NULL);
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET; 
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    while(1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);	//게임시작하면 원래 쓰레드는 여기에 멈춰있음.



        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;	// 들어온 유저 수에 따라 clnt_cnt 증가시키며 clnt_socks 배열에 값 저장
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);	//handle함수로 ㄱㄱ
        pthread_detach(t_id);//47행 쓰레드의 코드들이 모두 진행된뒤 소멸 시키기위해 (t-id라는 쓰레드를)
        printf("새로운 클라이언트가 접속했습니다.\n");

    }


    close(serv_sock);
    return 0;
}

void * handle_clnt(void * arg)
{
    int clnt_sock=*((int*)arg);
    int str_len=0, i;
    int str=1;
    int clnt_cnt_temp=clnt_cnt;
    int count=0;
    int tem=0;
    char msg[BUF_SIZE];

    char last_input[4]={0};
    int bul_count[4] = {0};
    int bullet_con[4][6]={0};


    for(i=0; i<4;i++)
    {
        if(clnt_socks[i]==clnt_sock)
        {
            tem=i;
            break;
        }
    }




    printf("구조체배열을 전송합니다. \n");
    write(clnt_sock, &s, sizeof(s));	// 새로들어온 클라이언트에게 입력하게할 구조체 전송 (구조체전부)

    while(s[0].flag==0&&(str_len=read(clnt_sock, &s, sizeof(s)))!=0){
        if(s[0].ready_player==1&&s[1].ready_player==1&&s[2].ready_player==1&&s[3].ready_player==1) s[0].flag=1;
        //	fflush();
        send_msg();	
    }
    sleep(1);	//클라들어갈 시간

    while(s[0].flag==1&&(str_len=read(clnt_sock, &input, sizeof(input)))!=0)	//버튼읽음.
    {
        send_game(tem, bul_count, last_input,bullet_con);
    }



    //====================================================== 제거
    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++)   
    {
        if(clnt_sock==clnt_socks[i])
        {
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}
void send_msg()   // send to all
{
    int i;
    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++)		// 모든 클라이언트에게 s 구조체 발송
        write(clnt_socks[i], &s , sizeof(s));		//통신할 소켓. s, 크기

    pthread_mutex_unlock(&mutx);
}

void send_game(int it, int bul_count[], char last_input[], int bullet_con[][6])   // send game to all
{
    int i;

    pthread_mutex_lock(&mutx);
    //클라이언트ID넣기



    player_move(it, last_input); //플레이어 이동후 흔적이지워짐


if((input[it] == 'q') && (bul_count[it] < 6))    //반복문 돌리고 break포함시켜야할듯
		{		

			mp.P_score[it] -= 50;

			input[it]='0';  // input초기화
			for(int i=0;i<6;i++)
			{
				if(bullet[it][i].on==0)
				{                        

					if(last_input[it] == 'w')
					{
						bullet[it][i].xpos = player[it].xpos;			//고친부분=================================
						bullet[it][i].ypos = player[it].ypos;    //한칸위
						bullet[it][i].direction = 1; 
						bullet[it][i].on=1;
						bul_count[it]+=1;
						break;
					}

					else if(last_input[it] == 's')
					{
						bullet[it][i].xpos = player[it].xpos;
						bullet[it][i].ypos = player[it].ypos;    //아래
						bullet[it][i].direction = 2; 
						bullet[it][i].on=1;
						bul_count[it]+=1;
						break;
					}
					else if(last_input[it] == 'd')
					{
						bullet[it][i].xpos = player[it].xpos;
						bullet[it][i].ypos = player[it].ypos;    //오른쪽
						bullet[it][i].direction = 3; 
						bullet[it][i].on=1;
						bul_count[it]+=1;
						break;
					}
					else if(last_input[it] == 97)
					{
						bullet[it][i].xpos = player[it].xpos;
						bullet[it][i].ypos = player[it].ypos;    //왼쪽
						bullet[it][i].direction = 4; 
						bullet[it][i].on=1;
						bul_count[it]+=1;
						break;
					}

				}
			}
		}


		for(int i=0;i<6;i++)			//포탄이동
		{

			if(bullet[it][i].on == 1 && bullet[it][i].direction==1)   //위쪽       
			{


				bullet[it][i].ypos -=1;						//다음위치 시작.


/*
	player[0].xpos=41;
	player[0].ypos=4;
	player[1].xpos=45;
	player[1].ypos=8;
	
	player[2].xpos=4;
	player[2].ypos=41;
	
	player[3].xpos=8;
	player[3].ypos=45;

   */



				//총알이 다음위치로 가기전 검사를 한다. 만약 구조물이 있다면 다음턴은 위치이동이아닌 총알과 구조물이 동시에 사라지게한다 
				if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=0 &&mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=10)
				{	mp.P_score[it]+=100;		//일반벽일때
					if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==11|| mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==12)	mp.P_score[it]+=400; //독수리맞추면
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==1) {mp.map_map[4][41]=1; player[0].xpos=41; player[0].ypos=4;  mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==2) {mp.map_map[8][45]=2; player[1].xpos=45; player[1].ypos=8; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==3) {mp.map_map[41][4]=3; player[2].xpos=4; player[2].ypos=41; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==4) {mp.map_map[45][8]=4; player[3].xpos=8; player[3].ypos=45; mp.P_score[it]+=900;}	//플레이어맞추면  4개로 나눠서 좌표 로가게 만들어야함
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==10) mp.P_score[it]-=100;	//스코어를 밑에다두면 좌표가 0으로 되므로 0으로 되기전에 스코어를 올려야한다.

					bullet[it][i].on=0;
					bul_count[it]-=1;
					mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 0;
					mp.map_map[bullet[it][i].ypos+1][bullet[it][i].xpos] = 0;
				mp.map_map[player[it].ypos][player[it].xpos]=it+1;			//없어진 플레이어 다시만들기 포탄이 이동할때 마다 내캐릭턱값을 다시 생성.
					bullet_con[it][i]=1;

				}



				if(bullet_con[it][i]==1)
				{
					bullet_con[it][i]=0;
					continue;
				}


				mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 5;
				mp.map_map[bullet[it][i].ypos+1][bullet[it][i].xpos] = 0;
				mp.map_map[player[it].ypos][player[it].xpos]=it+1;			//없어진 플레이어 다시만들기===============================



				if(bullet[it][i].ypos==1)		
				{

					bullet[it][i].on=0;
					bul_count[it]-=1;
				}
			}
			if(bullet[it][i].on == 1 && bullet[it][i].direction==2)   //아래쪽      
			{

				bullet[it][i].ypos +=1;

				if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=0 &&mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=10)		 
				{
					mp.P_score[it]+=100;		//일반벽일때
					if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==11|| mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==12)	mp.P_score[it]+=400; //독수리맞추면
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==1) {mp.map_map[4][41]=1; player[0].xpos=41; player[0].ypos=4;  mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==2) {mp.map_map[8][45]=2; player[1].xpos=45; player[1].ypos=8; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==3) {mp.map_map[41][4]=3; player[2].xpos=4; player[2].ypos=41; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==4) {mp.map_map[45][8]=4; player[3].xpos=8; player[3].ypos=45; mp.P_score[it]+=900;}	
					bullet[it][i].on=0;
					bul_count[it]-=1;
					mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 0;
					mp.map_map[bullet[it][i].ypos-1][bullet[it][i].xpos] = 0;
				mp.map_map[player[it].ypos][player[it].xpos]=it+1;			//없어진 플레이어 다시만들기 포탄이 이동할때 마다 내캐릭턱값을 다시 생성.
					bullet_con[it][i]=1;

				}	

				if(bullet_con[it][i]==1)
				{
					bullet_con[it][i]=0;
					continue;
				}


				mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 5;
				mp.map_map[bullet[it][i].ypos-1][bullet[it][i].xpos] = 0;
				mp.map_map[player[it].ypos][player[it].xpos]=it+1;

				if(bullet[it][i].ypos==48)
				{
					bullet[it][i].on=0;
					bul_count[it]-=1;
				}
			}
			if(bullet[it][i].on == 1 && bullet[it][i].direction==3)    //오른쪽     
			{

				bullet[it][i].xpos +=1;
				if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=0&& mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=10)		 
				{
					if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==11|| mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==12)	mp.P_score[it]+=400; //독수리맞추면
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==1) {mp.map_map[4][41]=1; player[0].xpos=41; player[0].ypos=4;  mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==2) {mp.map_map[8][45]=2; player[1].xpos=45; player[1].ypos=8; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==3) {mp.map_map[41][4]=3; player[2].xpos=4; player[2].ypos=41; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==4) {mp.map_map[45][8]=4; player[3].xpos=8; player[3].ypos=45; mp.P_score[it]+=900;}	
					bullet[it][i].on=0;
					bul_count[it]-=1;
					mp.P_score[it]+=100;		//일반벽일때
					mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 0;
					mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos-1] = 0;
					mp.map_map[player[it].ypos][player[it].xpos]=it+1;			//없어진 플레이어 다시만들기 포탄이 이동할때 마다 내캐릭턱값을 다시 생성.
					bullet_con[it][i]=1;

				}	

				if(bullet_con[it][i]==1)
				{
					bullet_con[it][i]=0;
					continue;
				}

				mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 5;
				mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos-1] = 0;
				mp.map_map[player[it].ypos][player[it].xpos]=it+1;

				if(bullet[it][i].xpos==48)
				{
					bullet[it][i].on=0;
					bul_count[it]-=1;
				}
			}
			if(bullet[it][i].on == 1 && bullet[it][i].direction==4)    //왼쪽     
			{

				bullet[it][i].xpos -=1;

				if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=0 && mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]!=10)		 
				{
					mp.P_score[it]+=100;		//일반벽일때
					if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==11|| mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==12)	mp.P_score[it]+=400; //독수리맞추면
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==1) {mp.map_map[4][41]=1; player[0].xpos=41; player[0].ypos=4;  mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==2) {mp.map_map[8][45]=2; player[1].xpos=45; player[1].ypos=8; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==3) {mp.map_map[41][4]=3; player[2].xpos=4; player[2].ypos=41; mp.P_score[it]+=900;}
					else if(mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos]==4) {mp.map_map[45][8]=4; player[3].xpos=8; player[3].ypos=45; mp.P_score[it]+=900;}	
					bullet[it][i].on=0;
					bul_count[it]-=1;
					mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 0;
					mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos+1] = 0;
					mp.map_map[player[it].ypos][player[it].xpos]=it+1;			//없어진 플레이어 다시만들기 포탄이 이동할때 마다 내캐릭턱값을 다시 생성.
					bullet_con[it][i]=1;

				}	

				if(bullet_con[it][i]==1)
				{
					bullet_con[it][i]=0;
					continue;
				}


				mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos] = 5;
				mp.map_map[bullet[it][i].ypos][bullet[it][i].xpos+1] = 0;
				mp.map_map[player[it].ypos][player[it].xpos]=it+1;

				if(bullet[it][i].xpos==1)
				{
					bullet[it][i].on=0;
					bul_count[it]-=1;

				}
			}

		}

		map_side();


	




    for(i=0; i<clnt_cnt; i++)		// 모든 클라이언트에게 s 구조체 발송
        write(clnt_socks[i],&mp , sizeof(mp));		//통신할 소켓. s, 크기


    pthread_mutex_unlock(&mutx);
}


void player_move(int it, char last_input[])
{


   if(input[it] == 's')
	{
		player[it].ypos+=1;
		if(mp.map_map[player[it].ypos][player[it].xpos]!=0) player[it].ypos-=1;

		if(player[it].ypos>47) player[it].ypos-=1;

		mp.map_map[player[it].ypos][player[it].xpos]=it+1;
		mp.map_map[(player[it].ypos)-1][player[it].xpos]=0;
		last_input[it] = 's';
		input[it]=0;

	}
	else if(input[it] == 'a')
	{
		player[it].xpos-=1;
		if(mp.map_map[player[it].ypos][player[it].xpos]!=0) player[it].xpos+=1;		//장애물 만나면 못지나가게 ==============================
		if(player[it].xpos<2) player[it].xpos+=1;
		mp.map_map[player[it].ypos][player[it].xpos]=it+1;
		mp.map_map[player[it].ypos][(player[it].xpos)+1]=0;
		last_input[it] = 'a';
		input[it]=0;
	}
	else if(input[it] == 'd')
	{
		player[it].xpos+=1;
		if(mp.map_map[player[it].ypos][player[it].xpos]!=0) player[it].xpos-=1;
		if(player[it].xpos>47) player[it].xpos-=1;
		mp.map_map[player[it].ypos][player[it].xpos]=it+1;
		mp.map_map[player[it].ypos][(player[it].xpos)-1]=0;
		last_input[it] = 'd';
		input[it]=0;
	}
	else if(input[it] == 'w')
	{
		player[it].ypos-=1;
		if(mp.map_map[player[it].ypos][player[it].xpos]!=0) player[it].ypos+=1;
		if(player[it].ypos<2) player[it].ypos+=1;
		mp.map_map[player[it].ypos][player[it].xpos]=it+1;
		mp.map_map[(player[it].ypos)+1][player[it].xpos]=0;
		last_input[it] = 'w';
		input[it]=0;
	}



}


void error_handling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}


void map_initialization()
{

	srand((unsigned)time(NULL));


	int map_data[50][50] =
	{
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,11,11,11,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,11,11,11,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,11,11,11,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,12,12,12,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,12,12,12,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,12,12,12,9,9,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9
	};



	int i, j;
	int ran;
	
	
		
//	mp.map_map[player[it].ypos][player[it].xpos]=1;
	


	ran = rand()%(99)+1;


	for (i = 0; i < 50; i++)
	{
			map_data[0][i] = 10;		// 맵 둘레 경계
			map_data[i][0] = 10;
			map_data[49][i] = 10;
			map_data[i][49] = 10;

			map_data[1][i] = 10;
			map_data[i][1] = 10;
			map_data[48][i] = 10;
			map_data[i][48] = 10;
	//-----------------------------------------------------------------------
	player[0].xpos=41;
	player[0].ypos=4;
	player[1].xpos=45;
	player[1].ypos=8;
	
	player[2].xpos=4;
	player[2].ypos=41;
	
	player[3].xpos=8;
	player[3].ypos=45;



	map_data[4][41] = 1;	// 플레이어 초기 위치
	map_data[8][45] = 2;
	map_data[41][4] = 3;
	map_data[45][8] = 4;

	//-----------------------------------------------------------------------

		for (j = 0; j < 50; j++)
		{	
		//-------------------------------------------------- 맵에서 베이스로 뚫린 길
			if ( 2 <= i && i <= 47  && 24 <= j && j <= 26)		map_data[i][j] = 0;		
			else if (2 <= i && i <= 42 && 2 <= j && j <= 4)		map_data[i][j] = 0;
			else if (8 <= i && i <= 47 && 45 <= j && j <= 47)	map_data[i][j] = 0;	

			else if (2 <= i && i <= 4 && 2 <= j && j <= 40)		map_data[i][j] = 0;	
			else if (45 <= i && i <= 47 && 8 <= j  && j <= 47)	map_data[i][j] = 0;	

		//-------------------------------------------------- 랜덤으로 뚫리는 길 : 좌측

			else if (8 <= i && i <= 10 && 2 <= j && j <= 20 && 40 < ran && ran < 70)		map_data[i][j] = 0;
			else if (14 <= i && i <= 16 && 2 <= j && j <= 20 && 0 < ran && ran < 40)		map_data[i][j] = 0;
			else if (19 <= i && i <= 21 && 2 <= j && j <= 20 && 40 < ran && ran < 100)		map_data[i][j] = 0;
			else if (25 <= i && i <= 27 && 2 <= j && j <= 20 && 40 < ran && ran < 100)		map_data[i][j] = 0;
			else if (30 <= i && i <= 32 && 2 <= j && j <= 20 && 0 < ran && ran < 40)		map_data[i][j] = 0;
			else if (31 <= i && i <= 33 && 2 <= j && j <= 20 && 40 < ran && ran < 100)		map_data[i][j] = 0;

			else if (8 <= i && i <= 10 && 7 <= j && j <= 24 && 0 < ran && ran < 50)			map_data[i][j] = 0;
			else if (14 <= i && i <= 16 && 7 <= j && j <= 24 && 40 < ran && ran < 100)		map_data[i][j] = 0;
			else if (19 <= i && i <= 21 && 7 <= j && j <= 24 && 20 < ran && ran < 100)		map_data[i][j] = 0;
			else if (25 <= i && i <= 27 && 7 <= j && j <= 24 && 0 < ran && ran < 50)		map_data[i][j] = 0;
			else if (30 <= i && i <= 32 && 7 <= j && j <= 24 && 30 < ran && ran < 100)		map_data[i][j] = 0;

			else if (33 <= i && i <= 35 && 12 <= j && j <= 24)		map_data[i][j] = 0;
			else if (39 <= i && i <= 41 && 14 <= j && j <= 24)		map_data[i][j] = 0;

			else if (4 <= i && i <= 9 && 7 <= j && j <= 9 && 0 < ran && ran < 50)			map_data[i][j] = 0;
			else if (4 <= i && i <= 16 && 10 <= j && j <= 12 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (4 <= i && i <= 9 && 13 <= j && j <= 15 && 40 < ran && ran < 90)		map_data[i][j] = 0;
			else if (4 <= i && i <= 16 && 16 <= j && j <= 18 && 0 < ran && ran < 50)		map_data[i][j] = 0;
			else if (4 <= i && i <= 10 && 19 <= j && j <= 21 && 50 < ran && ran < 100)		map_data[i][j] = 0;

			else if (17 <= i && i <= 27 && 7 <= j && j <= 9 && 0 < ran && ran < 70)			map_data[i][j] = 0;
			else if (17 <= i && i <= 27 && 10 <= j && j <= 12 && 60 < ran && ran < 100)		map_data[i][j] = 0;
			else if (17 <= i && i <= 27 && 13 <= j && j <= 15 && 0 < ran && ran < 60)		map_data[i][j] = 0;
			else if (17 <= i && i <= 27 && 18 <= j && j <= 20 && 0 < ran && ran < 70)		map_data[i][j] = 0;
			else if (14 <= i && i <= 27 && 19 <= j && j <= 21 && 50 < ran && ran < 100)		map_data[i][j] = 0;

		//-------------------------------------------------- 랜덤으로 뚫리는 길 : 우측

			else if (8 <= i && i <= 10 && 26 <= j && j <= 36 && 0 < ran && ran < 50)		map_data[i][j] = 0;
			else if (14 <= i && i <= 16 && 26 <= j && j <= 41 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (19 <= i && i <= 21 && 26 <= j && j <= 41 && 40 < ran && ran < 90)		map_data[i][j] = 0;
			else if (25 <= i && i <= 27 && 26 <= j && j <= 41 && 0 < ran && ran < 50)		map_data[i][j] = 0;
			else if (30 <= i && i <= 32 && 26 <= j && j <= 41 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (39 <= i && i <= 41 && 26 <= j && j <= 41 && 20 < ran && ran < 70)		map_data[i][j] = 0;

			
			else if (15 <= i && i <= 17 && 30 <= j && j <= 44 && 0 < ran && ran < 40)		map_data[i][j] = 0;
			else if (21 <= i && i <= 23 && 30 <= j && j <= 44 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (27 <= i && i <= 29 && 30 <= j && j <= 44 && 0 < ran && ran < 60)		map_data[i][j] = 0;
			else if (33 <= i && i <= 35 && 30 <= j && j <= 44 && 20 < ran && ran < 60)		map_data[i][j] = 0;
			else if (39 <= i && i <= 41 && 30 <= j && j <= 44 && 50 < ran && ran < 100)		map_data[i][j] = 0;

			else if (8 <= i && i <= 10 && 24 <= j && j <= 36)		map_data[i][j] = 0;
			else if (14 <= i && i <= 16 && 24 <= j && j <= 36)		map_data[i][j] = 0;

			else if (17 <= i && i <= 24 && 30 <= j && j <= 34 && 40 < ran && ran < 80)		map_data[i][j] = 0;
			else if (15 <= i && i <= 27 && 32 <= j && j <= 34 && 0 < ran && ran < 50)		map_data[i][j] = 0;
			else if (15 <= i && i <= 27 && 34 <= j && j <= 36 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (15 <= i && i <= 27 && 37 <= j && j <= 39 && 40 < ran && ran < 60)		map_data[i][j] = 0;
			else if (15 <= i && i <= 21 && 39 <= j && j <= 41 && 0 < ran && ran < 60)		map_data[i][j] = 0;
			else if (15 <= i && i <= 30 && 39 <= j && j <= 41 && 50 < ran && ran < 100)		map_data[i][j] = 0;

			else if (33 <= i && i <= 37 && 25 <= j && j <= 30 && 30 < ran && ran < 80)		map_data[i][j] = 0;
			else if (33 <= i && i <= 41 && 24 <= j && j <= 30 && 0 < ran && ran < 60)		map_data[i][j] = 0;
			else if (30 <= i && i <= 38 && 30 <= j && j <= 32 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (30 <= i && i <= 41 && 34 <= j && j <= 36 && 0 < ran && ran < 50)		map_data[i][j] = 0;
			else if (33 <= i && i <= 41 && 34 <= j && j <= 36 && 20 < ran && ran < 50)		map_data[i][j] = 0;
			else if (33 <= i && i <= 41 && 35 <= j && j <= 37 && 50 < ran && ran < 100)		map_data[i][j] = 0;
			else if (33 <= i && i <= 45 && 39 <= j && j <= 41 && 0 < ran && ran < 50)		map_data[i][j] = 0;

		//---------------------------------------------------- 여기까지 좌측 맵 랜덤



	
		}
	}


	for (i = 0; i < 50; i++)
	for (j = 0; j < 50; j++)
	mp.map_map[i][j] = map_data[i][j];

}







void map_side()
{
	for (int i = 0; i < 50; i++)
	{
			mp.map_map[0][i] = 10;		// 맵 둘레 경계
			mp.map_map[i][0] = 10;
			mp.map_map[49][i] = 10;
			mp.map_map[i][49] = 10;

			mp.map_map[1][i] = 10;
			mp.map_map[i][1] = 10;
			mp.map_map[48][i] = 10;
			mp.map_map[i][48] = 10;
	}

}

