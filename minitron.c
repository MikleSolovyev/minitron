#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>
#include <string.h>

#define LEN 8
#define WID 5
#define LCAR_COLOR 0x00FF0000
#define RCAR_COLOR 0x000000FF
#define LLINE_COLOR 0x00FFFF00
#define RLINE_COLOR 0x0000FF00
#define BORDER_COLOR 0x00FFFFFF
#define sec 1000000/16

//start positions-------------
#define L_X -1
#define L_Y (WID / 2)

#define R_X x_res
#define R_Y (y_res - 1 - WID / 2)
//############################

#define SEND() if(-1 == send(udp_socket, ch, sizeof(char), 0)){ \
	*ch = 'q';													\
	mvprintw(LINES - 3, 0, "Send error");						\
}

#define CHECK_COLL() if(is_collision == 0 && (i < 0 || i >= y_res || j < 0 || j >= x_res || 																			   \
					 ptr[i * xres_v + j] == LCAR_COLOR || ptr[i * xres_v + j] == RCAR_COLOR || ptr[i * xres_v + j] == LLINE_COLOR || ptr[i * xres_v + j] == RLINE_COLOR))  \
						is_collision = 1;																																   \
					 if(is_collision == 1 && (ptr[i * xres_v + j] == LCAR_COLOR || ptr[i * xres_v + j] == RCAR_COLOR))													   \
						is_collision = 2;

typedef struct{
	int udp_socket;
	char *ch;
	char dir;
} handlerData;

struct vehicle{
	int x;
	int y;
	uint32_t color;
	uint32_t line_color;
	int lose;
};

int work_flag = 1;

void sig_handler(int sig){
	work_flag = 0;
}

void* send_handler(void *arg){
	handlerData *data = (handlerData *)arg;
	int udp_socket = data->udp_socket;
	char *ch = data->ch;
	char dir = data->dir;
	char prev;
	
	while(*ch != 'q'){
		prev = *ch;
		*ch = getc(stdin);
		
		if(prev == 0){
			*ch = dir;
			SEND();
		}
		else{
			switch(*ch){
				case 'w':
					if(prev != 'w' && prev != 's'){
						SEND();
					}
					else{
						*ch = prev;
					}
					break;
					
				case 'a':
					if(prev != 'a' && prev != 'd'){
						SEND();
					}
					else{
						*ch = prev;
					}
					break;
					
				case 's':
					if(prev != 's' && prev != 'w'){
						SEND();
					}
					else{
						*ch = prev;
					}
					break;
					
				case 'd':
					if(prev != 'd' && prev != 'a'){
						SEND();
					}
					else{
						*ch = prev;
					}
					break;
					
				case 'q':
					SEND();
					break;
					
				default:
					*ch = prev;
					break;
			}
		}
	}
	
	return NULL;
}

void* recv_handler(void *arg){
	handlerData *data = (handlerData *)arg;
	int udp_socket = data->udp_socket;
	char *ch = data->ch;
	char dir = data->dir;
	char prev;
	
	while(*ch != 'q'){
		prev = *ch;
		if(-1 == recv(udp_socket, ch, sizeof(char), 0)){
			*ch = 'q';
			mvprintw(LINES - 3, 0, "Recieve error");
		}
		
		if(prev == 0){
			*ch = dir;
		}
		else{
			switch(*ch){
				case 'w':
					if(prev == 'w' || prev == 's')
						*ch = prev;
					break;
					
				case 'a':
					if(prev == 'a' || prev == 'd')
						*ch = prev;
					break;
					
				case 's':
					if(prev == 's' || prev == 'w')
						*ch = prev;
					break;
					
				case 'd':
					if(prev == 'd' || prev == 'a')
						*ch = prev;
					break;
					
				case 'q':
					break;
					
				default:
					*ch = prev;
					break;
			}
		}
	}
	
	return NULL;
}

void clean_prev(uint32_t *ptr, int x_res, int y_res, int xres_v, struct vehicle car){
	for(int i = (car.y - LEN); i <= (car.y + LEN); i++){
		for(int j = (car.x - LEN); j <= (car.x + LEN); j++){
			if(i >= 0 && i < y_res && j >= 0 && j < x_res && ptr[i * xres_v + j] == car.color)
				ptr[i * xres_v + j] = 0;
		}
	}
}

int draw(uint32_t *ptr, int x_res, int y_res, int xres_v, char ch, struct vehicle car){
	int is_collision = 0;
	
	ptr[car.y * xres_v + car.x] = car.line_color;
	
	switch(ch){
		case 'w':
			for(int i = (car.y - LEN); i <= (car.y - 1); i++){
				for(int j = (car.x - WID / 2); j <= (car.x + WID / 2); j++){
					CHECK_COLL();

					ptr[i * xres_v + j] = car.color;
				}
			}
			break;
			
		case 'a':
			for(int i = (car.y - WID / 2); i <= (car.y + WID / 2); i++){
				for(int j = (car.x - LEN); j <= (car.x - 1); j++){
					CHECK_COLL();
					
					ptr[i * xres_v + j] = car.color;
				}
			}
			break;
			
		case 's':
			for(int i = (car.y + 1); i <= (car.y + LEN); i++){
				for(int j = (car.x - WID / 2); j <= (car.x + WID / 2); j++){
					CHECK_COLL();
					
					ptr[i * xres_v + j] = car.color;
				}
			}
			break;
			
		case 'd':
			for(int i = (car.y - WID / 2); i <= (car.y + WID / 2); i++){
				for(int j = (car.x + 1); j <= (car.x + LEN); j++){
					CHECK_COLL();
					
					ptr[i * xres_v + j] = car.color;
				}
			}
			break;
			
		default:
			break;
	}
	
	return is_collision;
}

int main(int argc, char *argv[]){
	//socket vars--------------------------
	int udp_socket;
	unsigned long int peer_ip = 0;
    struct sockaddr_in addr;
	int size = sizeof(addr);
	//#####################################
	
	//fb vars------------------------------
	struct fb_var_screeninfo info;
	int fb;
	size_t fb_size, map_size, page_size;
	uint32_t *ptr;
	
	int x_res, y_res;
	sscanf(argv[1], "%d%*c%d", &x_res, &y_res);
	//#####################################
	
	//other vars---------------------------
	char ch1 = 0, ch2 = 0;
	int bump1, bump2;
	
	struct vehicle car1, car2;
	
	handlerData data_send, data_recv;
	data_send.ch = &ch1;
	data_recv.ch = &ch2;
	//#####################################
	
	if(argc != 3){
		fprintf(stderr, "Usage: %s <resolution> <opponent ip>\n", argv[0]);
		return 1;
	}
	
	signal(SIGINT, sig_handler);
	
	//socket init---------------------------------------------------------
	if(0 > (udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))){
        perror("Error calling socket");
        return __LINE__;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(0 > bind(udp_socket, (struct sockaddr *)&addr, sizeof(addr))){
		close(udp_socket);
        perror("Socket bind error");
        return __LINE__;
    }

	if(0 == inet_aton(argv[2], &addr.sin_addr)){
		close(udp_socket);
		perror("Incorrect IP-adress");
		return __LINE__;
	}
	if(0 > connect(udp_socket, (struct sockaddr *)&addr, sizeof(addr))){
		close(udp_socket);
		perror("Socket connect error");
		return __LINE__;
	}
	
	peer_ip = ntohl(addr.sin_addr.s_addr);
	getsockname(udp_socket, (struct sockaddr *)&addr, &size);
	data_send.udp_socket = data_recv.udp_socket = udp_socket;
	//####################################################################
	
	//fb init-------------------------------------------------------------
	page_size = sysconf(_SC_PAGESIZE);
	
	if(0 > (fb = open("/dev/fb0", O_RDWR))){
		close(udp_socket);
		perror("fb open error");
		return __LINE__;
	}

	if((-1) == ioctl(fb, FBIOGET_VSCREENINFO, &info)){
		close(udp_socket);
		close(fb);
		perror("fb ioctl error");
		return __LINE__;
	}
	
	if(x_res > info.xres || y_res > info.yres){
		close(udp_socket);
		close(fb);
		fprintf(stderr, "Incorrect resolution (max resolution = %dx%d)\n", info.xres, info.yres);
		return 2;
	}

	fb_size = sizeof(uint32_t) * info.xres_virtual * info.yres_virtual;
	map_size = (fb_size + (page_size - 1)) & (~(page_size - 1));
	
	ptr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	if(MAP_FAILED == ptr){
		close(udp_socket);
		close(fb);
		perror("mmap error");
		return __LINE__;
	}
	
	if(NULL == initscr()){
		close(udp_socket);
		close(fb);
		munmap(ptr, map_size);
		perror("initscr error");
		return __LINE__;
	}
	
	noecho();
	move(LINES - 1, COLS - 1);
	refresh();
	//####################################################################
	
	//pthread init--------------------------------------------------------
	if(peer_ip < ntohl(addr.sin_addr.s_addr)){
		data_send.dir = 'd';
		data_recv.dir = 'a';
	}
	else{
		data_send.dir = 'a';
		data_recv.dir = 'd';
	}
	
	pthread_t snd_hndlr;
	if(0 != pthread_create(&snd_hndlr, NULL, send_handler, &data_send)){
		close(udp_socket);
		close(fb);
		munmap(ptr, map_size);
		endwin();
		system("reset");
		perror("pthread error");
		return __LINE__;
	}
	
	pthread_t rcv_hndlr;
	if(0 != pthread_create(&rcv_hndlr, NULL, recv_handler, &data_recv)){
		close(udp_socket);
		close(fb);
		munmap(ptr, map_size);
		endwin();
		system("reset");
		perror("pthread error");
		return __LINE__;
	}
	//####################################################################
	
	//clean screen--------------------------------------------------------
	memset(ptr, 0, map_size);
	for(int i = 0; i < info.yres; i++){
		for(int j = 0; j < info.xres; j++){
			if(ptr[i * info.xres_virtual + j] != 0){
				close(udp_socket);
				close(fb);
				munmap(ptr, map_size);
				endwin();
				system("reset");
				fprintf(stderr, "Can`t do blank screen\n");
				return 3;
			}
		}
	}
	//####################################################################
	
	//cars and borders init-----------------------------------------------
	car1.lose = car2.lose = 0;
	if(peer_ip < ntohl(addr.sin_addr.s_addr)){
		car1.x = L_X;
		car1.y = L_Y;
		car1.color = LCAR_COLOR;
		car1.line_color = LLINE_COLOR;
		
		car2.x = R_X;
		car2.y = R_Y;
		car2.color = RCAR_COLOR;
		car2.line_color = RLINE_COLOR;
	}
	else{
		car1.x = R_X;
		car1.y = R_Y;
		car1.color = RCAR_COLOR;
		car1.line_color = RLINE_COLOR;
		
		car2.x = L_X;
		car2.y = L_Y;
		car2.color = LCAR_COLOR;
		car2.line_color = LLINE_COLOR;
	}
	
	for(int i = (L_Y - WID / 2); i <= (L_Y + WID / 2); i++){
		for(int j = (L_X + 1); j <= (L_X + LEN); j++){
			ptr[i * info.xres_virtual + j] = LCAR_COLOR;
		}
	}
	
	for(int i = (R_Y - WID / 2); i <= (R_Y + WID / 2); i++){
		for(int j = (R_X - LEN); j <= (R_X - 1); j++){
			ptr[i * info.xres_virtual + j] = RCAR_COLOR;
		}
	}
	
	if(x_res < info.xres){
		for(int i = 0; i < y_res; i++){
			ptr[i * info.xres_virtual + x_res] = BORDER_COLOR;
		}
	}
	
	if(y_res < info.yres){
		for(int i = 0; i < x_res; i++){
			ptr[y_res * info.xres_virtual + i] = BORDER_COLOR;
		}
	}
	//####################################################################
	
	while(work_flag == 1 && (ch1 == 0 || ch2 == 0)){}
	
	int rendered = 0;
	while(ch1 != 'q' && ch2 != 'q' && work_flag == 1 && car1.lose == 0 && car2.lose == 0){
		curs_set(1);

		rendered++;
		clean_prev(ptr, x_res, y_res, info.xres_virtual, car1);
		clean_prev(ptr, x_res, y_res, info.xres_virtual, car2);
		
		switch(ch1){
			case 'w':
				car1.y -= 1;
				break;
				
			case 'a':
				car1.x -= 1;
				break;
				
			case 's':
				car1.y += 1;
				break;
				
			case 'd':
				car1.x += 1;
				break;
				
			default:
				break;
		}
		
		switch(ch2){
			case 'w':
				car2.y -= 1;
				break;
				
			case 'a':
				car2.x -= 1;
				break;
				
			case 's':
				car2.y += 1;
				break;
				
			case 'd':
				car2.x += 1;
				break;
				
			default:
				break;
		}
		
		curs_set(0);

		bump1 = draw(ptr, x_res, y_res, info.xres_virtual, ch1, car1);
		switch(bump1){
			case 1:
				car1.lose = 1;
				break;
				
			case 2:
				car1.lose = 1;
				car2.lose = 1;
				break;
				
			default:
				break;
		}
		
		bump2 = draw(ptr, x_res, y_res, info.xres_virtual, ch2, car2);
		switch(bump2){
			case 1:
				car2.lose = 1;
				break;
				
			case 2:
				car2.lose = 1;
				car1.lose = 1;
				break;
				
			default:
				break;
		}

		usleep(sec);
	}
	
	close(fb);
	munmap(ptr, map_size);
	pthread_cancel(snd_hndlr);
	pthread_cancel(rcv_hndlr);
	
	if(work_flag == 0){
		ch1 = 'q';
		send(udp_socket, &ch1, sizeof(char), 0);
	}
	close(udp_socket);
	
	//print results-------------------------------------------------------
	if(ch1 == 'q' || ch2 == 'q'){
		mvprintw(LINES - 2, 0, "Stopped!");
	}
	else if(car1.lose == 0 && car2.lose == 1){
		mvprintw(LINES - 2, 0, "You win!");
	}
	else if(car1.lose == 1 && car2.lose == 0){
		mvprintw(LINES - 2, 0, "You lose!");
	}
	else if(car1.lose == 1 && car2.lose == 1){
		mvprintw(LINES - 2, 0, "No one win!");
	}
	mvprintw(LINES - 4, 0, "Rendered %d times", rendered);
	mvprintw(LINES - 1, 0, "Press 'q' to quit...");
	refresh();
	//####################################################################
	
	while(getc(stdin) != 'q'){};
	endwin();
	system("reset");

    return 0;
}