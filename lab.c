#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>


int f(int x){
	printf("f(x) started...\n");
	sleep(15);
	printf("f(x) done!\n");
	return x*x;
}

int g(int x){
	printf("g(x) started...\n");
	sleep(10);
	printf("g(x) done!\n");
	return x-3;
}

bool read_int(int *x, int readfd){
	if (read(readfd, (void* )x, sizeof(int)) == sizeof(int)){
		return true;
	} else {
		return false;
	}
}

bool write_int(int x, int writefd){
	bool res = write(writefd, &x, sizeof(int)) == sizeof(int);
	return res;
}

void call_func(int (*func)(int),int readfd, int writefd){
	int x;
	while (!read_int(&x, readfd)){}
	x = func(x);
	write_int(x, writefd);
}

void open_pipe(int pipefds[2]){
	if (pipe(pipefds) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	fcntl(pipefds[0], F_SETFL, O_NONBLOCK);
}

int main(void){
	int pipefds_f_to_main[2], pipefds_main_to_f[2], pipefds_g_to_main[2], pipefds_main_to_g[2];

	open_pipe(pipefds_f_to_main);
	open_pipe(pipefds_main_to_f);
	pid_t pid_f = fork();

	if(pid_f == -1){
		exit(EXIT_FAILURE);
	} else if (pid_f == 0){ // f child process
		call_func(&f, pipefds_main_to_f[0], pipefds_f_to_main[1]);
	} else {
		open_pipe(pipefds_g_to_main);
		open_pipe(pipefds_main_to_g);

		pid_t pid_g = fork();
		if (pid_g == -1){
			exit(EXIT_FAILURE);
		} else if (pid_g == 0){ // g child process
			call_func(&g, pipefds_main_to_g[0], pipefds_g_to_main[1]);
		} else { //main
			int x, fres, gres, fret = false, gret = false;
			char ch;
			printf("Enter x: \n");
			while (scanf("%d", &x) != 1){
				printf("invalid input, try again\n");
			}
			getchar();

			write_int(x, pipefds_main_to_f[1]);
			write_int(x, pipefds_main_to_g[1]);

			double timer = 0;
			bool stop = false;
			while (!stop) {
				if (timer > 0.5d){
					timer = 0;
					do{
						printf("Stop? [y/n] \n");
						ch = getchar();
					getchar();
					} while(ch != 'y' && ch != 'n');

					if(ch == 'y'){
						stop = true;
						continue;
					} else { timer = 0; }
				}

				fret = fret || read_int(&fres, pipefds_f_to_main[0]);
				gret = gret || read_int(&gres, pipefds_g_to_main[0]);
				if ((fret && fres == 0) || (gret && gres == 0)){
					printf("Result: 0\n");
					exit(0);
					stop = true;
				} else if (fret && gret){
					printf("Result: %d \n", fres*gres);
					exit(0);
					stop = true;
				} else {
					usleep(1000);
					timer += 1e-3d;
				}
			}

			}
		close(pipefds_g_to_main[0]);
		close(pipefds_g_to_main[1]);
		close(pipefds_main_to_g[0]);
		close(pipefds_main_to_g[1]);
		}
		close(pipefds_f_to_main[0]);
		close(pipefds_f_to_main[1]);
		close(pipefds_main_to_f[0]);
		close(pipefds_main_to_f[1]);
		return 0;
}

