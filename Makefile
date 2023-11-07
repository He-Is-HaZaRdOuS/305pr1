all: process threads

process: game_process_group11.c
	gcc game_process_group11.c -o game_process -Wall

threads: game_threads_group11.c
	gcc game_threads_group11.c -o game_threads -lpthread -Wall
