#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <stdio_ext.h>

#define ANSI_COLOR_BLACK   "\x1b[30m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_BG_BLACK   "\x1b[40m"
#define ANSI_BG_RED     "\x1b[41m"
#define ANSI_BG_GREEN   "\x1b[42m"
#define ANSI_BG_YELLOW  "\x1b[43m"
#define ANSI_BG_BLUE    "\x1b[44m"
#define ANSI_BG_BAGNETA "\x1b[45m"
#define ANSI_BG_CYAN    "\x1b[46m"
#define ANSI_BG_WHITE   "\x1b[47m"

typedef struct
{
	int player_xy[2];
	int player_direction;
	int shot_xy[5][4]; // x, y, player, direction
	int shot_count;
	int point;
	int death;

}Player;

int getch();
int kbhit();
char getKey();
void move_player(Player *p, int kbhit, int map_array[][22], int);

int main(void)
{
	int map_base[22][22]={
		//{0,1,2,3,4,5,6,7,8,9,0,0,1,2,3,4,5,6,7,8,9,0},//
		{9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9},//1
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//2
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//3
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//4
		{9,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,9},//5
		{9,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,9},//6
		{9,1,1,1,1,1,1,1,1,1,2,2,1,1,1,2,1,1,1,1,1,9},//7
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//8
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//9
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//10
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//11
		{9,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,9},//12
		{9,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,9},//13
		{9,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,9},//14
		{9,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,9},//15
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//16
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//17
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//18
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//19
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//20
		{9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9},//21
		{9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9} //22
	};

	int init_col = sizeof(map_base[0]) / sizeof(int); 
	int init_row = sizeof(map_base) / sizeof(map_base[0]);

	//int map_base[22][22] = {0,};
	int map_buffer1[22][22] = {0,};
	int map_buffer2[22][22] = {0,};
	int map_present[22][22] = {0,};
	int input_move = 0;

	Player player[4] = {0, };

	//{{2, 2, 3}, {2, 19, 2}, {19, 2, 1}, {19, 19, 4}}; 
	// x, y, player_direction
	player[0].player_xy[0] = 2;
	player[0].player_xy[1] = 2;
	player[0].player_direction = 3;
	player[1].player_xy[0] = 2;
	player[1].player_xy[1] = 19;
	player[1].player_direction = 2;
	player[2].player_xy[0] = 19;
	player[2].player_xy[1] = 2;
	player[2].player_direction = 1;
	player[3].player_xy[0] = 19;
	player[3].player_xy[1] = 19;
	player[3].player_direction = 4;

	srand(time(NULL));

	char emoji[20][5] = {"  ", "⠐ ", "🧱", "⛔", "🟨", "🟩", "🟦", "🟪", "🟧", "🟫", 
		"🔥", "💥", "🔴", "🟥", "😱", "🤢", "🥶", "👾", "🎃", "💩"};

	int x = 0; // player x
	int y = 0; // player x
	int w = 0; // player direction
	int d = 0; // player death
	int shot_x = 0; // shot x
	int shot_y = 0; // shot y
	
	int buf1_xy = 0;
	int buf2_xy = 0;
	int pre_xy = 0;
	

	for(;;)
	{
		system("clear");

		// build map_buffer1 with map_base + player xy 
		for(int map_y = 0; map_y < init_col; map_y++) // for map y length 
		{
			for(int map_x = 0; map_x < init_row; map_x++) // for map x length
			{
				for(int i = 0; i < 4; i++) // find buf1_xy value by 4 players' identifier
				{
					x = player[i].player_xy[0]; // iterate each player x
					y = player[i].player_xy[1]; // iterate each player y
					w = player[i].player_direction; // iterate each player direction

					if(((y - 1) <= map_y && map_y <= (y + 1)) && ((x - 1) <= map_x && map_x <= (x + 1))) // if mapx & map_y is around player (3x3)
					{
						if((w == 1) && (map_x == (x - 1) && map_y == y))     // show a ◀direction
							buf1_xy = 0;
						else if((w == 2) && (map_x == (x + 1) && map_y == y)) // show d ▶ direction
							buf1_xy = 0;
						else if((w == 3) && (map_x == x && map_y == (y + 1))) // show s ▼ direction
							buf1_xy = 0;
						else if((w == 4) && (map_x == x && map_y == (y - 1))) // show w ▲  direction
							buf1_xy = 0;
						else if(map_y == y && map_x == x)		   // show center == missile
							buf1_xy = (14 + i);
						else
							buf1_xy = (4 + i); 			   // show yellow square around player
						break;
					}
					else
						buf1_xy = map_base[map_y][map_x];

				}
				map_buffer1[map_y][map_x] = buf1_xy; // or copy map_base
			}
		}

		// build map_buffer2 with dead ruins
		for(int map_y = 0; map_y < init_col; map_y++) // for map y length 
		{
			for(int map_x = 0; map_x < init_row; map_x++) // for map x length
			{
				for(int i = 0; i < 4; i++) // check 4 players
				{
					x = player[i].player_xy[0]; // iterate each player x
					y = player[i].player_xy[1]; // iterate each player y
					d = player[i].death;

					if(d && ((y - 1) <= map_y && map_y <= (y + 1)) && ((x - 1) <= map_x && map_x <= (x + 1))) // if xy is around player (3x3)
					{
						buf2_xy = 10; // fire 🔥
						break;
					}
					else
						buf2_xy = map_buffer1[map_y][map_x];
					//break;

				}
				map_buffer2[map_y][map_x] = buf2_xy;
			}
		}


		// remove ruins timer
		for(int i = 0; i < 4; i++) // 4 players
		{
			if(player[i].death > 1) // if death value over 1
				player[i].death--; 
			else if(player[i].death == 1) // if death value is 1
			{
				player[i].death--;

				if(i == 0)
				{
					player[0].player_xy[0] = 2;
					player[0].player_xy[1] = 2;
					player[0].player_direction = 3;
				}
				else if(i == 1)
				{
					player[1].player_xy[0] = 2;
					player[1].player_xy[1] = 19;
					player[1].player_direction = 2;
				}
				else if(i == 2)
				{
					player[2].player_xy[0] = 19;
					player[2].player_xy[1] = 2;
					player[2].player_direction = 1;
				}
				else if(i == 3)
				{
					player[3].player_xy[0] = 19;
					player[3].player_xy[1] = 19;
					player[3].player_direction = 4;
				}
			}
			else 
				continue; // 0, check next player
		}


		// build present map with map_buffer2 & shot xy
		for(int map_y = 0; map_y < init_col; map_y++) // for map y length 
		{
			for(int map_x = 0; map_x < init_row; map_x++) // for map x length 
			{
				for(int i = 0; i < 4; i++) // 4 players
				{
					for(int j = 0; j < 4; j++) // shot max 4
					{
						shot_x = player[i].shot_xy[j][0]; // iterate each shot x
						shot_y = player[i].shot_xy[j][1]; // iterate each shot y

						if(map_y == shot_y && map_x == shot_x)
						{
							pre_xy = (14 + i); // xy, 0,0 == 14,
							break;
						}
						else
							pre_xy = map_buffer2[map_y][map_x];
					}
					break;

				}
				map_present[map_y][map_x] = pre_xy;
			}
		}

		// print result
		printf("\n");
		for(int line_y = 0; line_y < 22; line_y++) // line y
		{
			// printf("%2d", line_y); // print y line number

			for(int line_x = 0; line_x < 22 ; line_x++) // line x
			{
			
				//printf(" %d ", map_present[line_y][line_x]);
				for(int x = 0; x < 20; x++) // print all map array
				{
					if(map_present[line_y][line_x] == x)
						//printf("%2d ", x);
						printf("%s", emoji[x]);
				}
			}
			printf("\n"); // go next line 
		}


		// input
		printf("    🔼 w            Missile %d/5\n", player[0].shot_count);
		printf("🔚 a🔽 s🔜 d >> \n");

		if(player[0].death == 0)
			input_move = getKey();

		// control next input
		move_player(&player[0], input_move, map_present, 5);

		usleep(100 * 1000);

		// control enemies' move
		for(int d = 1; d < 4; d++)
		{
			input_move = rand() % 5;
			move_player(&player[d], input_move, map_present, 5);
		}


		// shot xy++;
		int add_shot_x = 0;
		int add_shot_y = 0;
		int add_shot_w = 0; // 총알의 방향을 파악해서 

		for(int t = 0; t < 4; t++) // 플레이어들의
		{
			for(int f = 0; f < 5; f++) // 미사일 파악해서 해서 
			{
				if(player[t].shot_xy[f][0] != 0) // 비어있지 않다면, 
				{

					add_shot_x = player[t].shot_xy[f][0];
					add_shot_y = player[t].shot_xy[f][1];
					add_shot_w = player[t].shot_xy[f][3]; // 총알의 방향을 파악해서 

					if(add_shot_w == 1) // a 방향
					{
						player[t].shot_xy[f][0] -= 1; // 방향 좌표값 증가
					}
					else if(add_shot_w == 2)   // d 방향
					{
						player[t].shot_xy[f][0] += 1; // 방향 좌표값 증가
					}
					else if(add_shot_w == 3)   // s 방향
					{
						player[t].shot_xy[f][1] += 1; // 방향 좌표값 증가
					}
					else if(add_shot_w == 4)   // w 방향
					{
						player[t].shot_xy[f][1] -= 1; // 방향 좌표값 증가
					}

				}

				// shot_

				//usleep(500 * 1000);

			}
		}

		// 증가한 총알의 좌표값을 파악해, 이벤트 해결

		int next = 0;
		int zero = 0;
		int kill = 0;

		for(int t = 0; t < 4; t++) // 플레이어들의
		{
			for(int f = 0; f < 5; f++) // 미사일 파악해서 해서 
			{
				if(player[t].shot_xy[f][0] != 0) // 비어있지 않다면, 
				{


					add_shot_x = player[t].shot_xy[f][0];
					add_shot_y = player[t].shot_xy[f][1];
					next = map_base[add_shot_y][add_shot_x];

					//if(next == 1) // 움직이는 길이면,
					//	continue;
					if(map_base[add_shot_y][add_shot_x] == 2) // 부술 수 있는 벽을 만나면, 
					{
						map_base[add_shot_y][add_shot_x] = 1; // 움직이는 길로 표시하고(==벽을 부수고)
						zero = 1; // 총알 리셋
					}
					else if(map_base[add_shot_y][add_shot_x] == 9) // 총알이 부술 수 없는 벽을 만나면, 
					{
						map_base[add_shot_y][add_shot_x] = 19; // 똥으로
						zero = 1; // 총알 리셋
					}
					else if(map_base[add_shot_y][add_shot_x] == 19) // 똥이면, 
					{
						map_base[add_shot_y][add_shot_x] = 9; // 벽으로
						zero = 1; // 총알 리셋
					}
					else if(map_present[add_shot_y][add_shot_x] == 14 || map_present[add_shot_y][add_shot_x] == 4) // player 1번이거나, 1번 총알이면,
						{
							player[0].death = 5;
							zero = 1; // 총알 리셋
							kill = 1;

						}
					else if(map_present[add_shot_y][add_shot_x]== 15 || map_present[add_shot_y][add_shot_x] == 5) // player 2번이거나, 2번 총알이면,
						{
							player[1].death = 5;
							zero = 1; // 총알 리셋
							kill = 1;

						}
					else if(map_present[add_shot_y][add_shot_x] == 16 || map_present[add_shot_y][add_shot_x] == 6) // player 3번이거나, 3번 총알이면,
						{
							player[2].death = 5;
							zero = 1; // 총알 리셋
							kill = 1;

						}
					else if(map_present[add_shot_y][add_shot_x] == 17 || map_present[add_shot_y][add_shot_x] == 7) // player 4번이거나, 4번 총알이면,
						{
							player[3].death = 5;
							zero = 1; // 총알 리셋
							kill = 1;
						}
					if(zero)
					{
						player[t].shot_xy[f][0] = 0; // 0번 총알의 x값
						player[t].shot_xy[f][1] = 0; // 0번 총알의 y값
						player[t].shot_xy[f][2] = 0; // 0번 총알의 주인, 0 == player 0
						player[t].shot_xy[f][3] = 0; // 0번 총알 방향
						player[t].shot_count--; // shot 갯수 - 1
					}
					if(kill)
					{
						player[t].point += 1;
					}
				}
			}
		}
	}
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


void move_player(Player *p, int kbhit, int map[][22], int shot_MAX)
{

	//int col = sizeof(map_array[0]) / sizeof(int); 
	//int row = sizeof(map_array) / sizeof(map_array[0]);

	int map_check = 1; // move, cuz nth next

	int x = p->player_xy[0];
	int y = p->player_xy[1];
	int w = p->player_direction;
	int x_add = 0;
	int y_add = 0;


	int range_srt = 0;
	int range_end = 0;
	int range_fix = 0;

	int shot_max = p->shot_count; // n번째 총알의 (0부터 시작 ~ max까지)

	if(kbhit == 65 || kbhit == 97 || kbhit == 1) // if press a,
		{
			range_srt = p->player_xy[1] - 1;
			range_end = p->player_xy[1] + 1;
			range_fix = p->player_xy[0] - 2;
			x_add = -1;
			y_add = 0;
			w = 1;
		}
	else if(kbhit == 68 || kbhit == 100 || kbhit == 2) // d 누르면, -> 방향의 지형과, 적을 확인해야 한다.
	{
		range_srt = p->player_xy[1] - 1;
		range_end = p->player_xy[1] + 1;
		range_fix = p->player_xy[0] + 2;
		x_add = 1;
		y_add = 0;
		w = 2;
	}
	else if(kbhit == 83 || kbhit == 115 || kbhit == 3) // s 를 누르면, ▽ 방향 지형과, 적을 확인해야 한다.
	{
		range_srt = p->player_xy[0] - 1;
		range_end = p->player_xy[0] + 1;
		range_fix = p->player_xy[1] + 2;
		x_add = 0;
		y_add = 1;
		w = 3;
	}
	else if(kbhit == 87 || kbhit == 119 || kbhit == 4) // w 를 누르면, △ 방향 지형과, 적을 확인해야 한다.
	{
		range_srt = p->player_xy[0] - 1;
		range_end = p->player_xy[0] + 1;
		range_fix = p->player_xy[1] - 2;
		x_add = 0;
		y_add = -1;
		w = 4;
	}
	else if(kbhit == 32 && shot_max < shot_MAX) // 총알을 쐈다면, 플레이어 xy의 다음 방향 + 1 무조건 가능
	{		
		for(int t = 0; t < 5; t++) // 비어있는 총알 배열, 플레이어 xy 기준으로 초기값 추가
		{
			if(p->shot_xy[t][0] == 0)
			{
				if(w == 1) // a 방향
				{
					p->shot_xy[t][0] = x - 1;
					p->shot_xy[t][1] = y;
					p->shot_xy[t][2] = 0; // shot 주인
					p->shot_xy[t][3] = w;
				}
				else if(w == 2)   // d 방향
				{
					p->shot_xy[t][0] = x + 1;
					p->shot_xy[t][1] = y;
					p->shot_xy[t][2] = 0; // shot 주인
					p->shot_xy[t][3] = w;
				}
				else if(w == 3)   // s 방향
				{
					p->shot_xy[t][0] = x;
					p->shot_xy[t][1] = y + 1;
					p->shot_xy[t][2] = 0; // shot 주인
					p->shot_xy[t][3] = w;
				}
				else if(w == 4)   // w 방향
				{
					p->shot_xy[t][0] = x;
					p->shot_xy[t][1] = y - 1;
					p->shot_xy[t][2] = 0; // shot 주인
					p->shot_xy[t][3] = w;
				}
				p->shot_count += 1;
				break;
			}
		}


	}

	// check map
	if(kbhit != 32)
	{
		for(int i = range_srt; i <= range_end; i++) // check range from y +- 1
		{
			int j = i;
			int k = range_fix;

			if(w == 3 || w == 4)
			{
				j = range_fix;
				k = i;
			}

			if(map[j][k] != 1) // to fix range x - 2, then not 1
			{
				map_check = 0; // don't move
				break;
			}
		}
	}

	// move, cuz nth next
	if(map_check) 
	{
		p->player_xy[0] += x_add; // player x
		p->player_xy[1] += y_add; // player y
		p->player_direction = w;  // update player direction
	}


}


