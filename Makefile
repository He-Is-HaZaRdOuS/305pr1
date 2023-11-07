all: process threads

process: game_process_group11.c
	gcc game_process_group11.c -o game_process -Wall

threads: game_threads_group11.c
	gcc game_threads_group11.c -o game_threads -lpthread -Wall

unicode: u_process u_threads

u_process: game_process_group11.c
	gcc game_process_group11.c -o game_process -Wall -D USE_UNICODE

u_threads: game_threads_group11.c
	gcc game_threads_group11.c -o game_threads -lpthread -Wall -D USE_UNICODE


