#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include <time.h>

#define READ_END 0
#define WRITE_END 1
#define ROUND_COUNT 5

typedef struct Player {
    int x, y;
    int id;
    int closestGuess;
} Player;

enum State {
    ONGOING,
    P1WON,
    P2WON,
    TIE
};

enum State state = ONGOING;
int map_size;
int currentRound = 1;

void printBox(int width, int height, Player *players, int player_count);

void guess(int *x, int *y, int manhattan);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./game_process <map_size>\n");
        return 1;
    }

    map_size = atoi(argv[1]);
    if (map_size < 2) {
        printf("Map size cannot be less than 2\n");
        return 1;
    }

    printf("Map size: %dx%d\n", map_size, map_size);
    printf("Coordinates of players are chosen randomly\n");

    Player players[2]; // fixed
    srand(time(NULL));
    // assign both players to the same random position
    players[0].x = players[1].x = rand() % map_size;
    players[0].y = players[1].y = rand() % map_size;
    // assign player2 a new random position until they are not at the same
    // position
    while (players[0].x == players[1].x && players[0].y == players[1].y) {
        players[1].x = rand() % map_size;
        players[1].y = rand() % map_size;
    }
    players[0].id = 1;
    players[1].id = 2;
    players[0].closestGuess = INT_MAX;
    players[1].closestGuess = INT_MAX;

    printf("player1: [%d,%d], player2: [%d,%d]\n", players[0].x, players[0].y,
           players[1].x, players[1].y);
    printBox(map_size, map_size, players, 2);

    int fd1[2], fd2[2], fd3[2], fd4[2];
    pid_t pid;

    if (pipe(fd1) == -1) {
        printf("Pipe1 failed!\n");
        return -1;
    }
    if (pipe(fd2) == -1) {
        printf("Pipe2 failed!\n");
        return -2;
    }

    if (pipe(fd3) == -1) {
        printf("Pipe3 failed!\n");
        return -3;
    }

    if (pipe(fd4) == -1) {
        printf("Pipe4 failed!\n");
        return -4;
    }

    printf("Game launches -->\n");

    pid = fork();
    if (pid < 0) {
        printf("Fork failed!\n");
        return -5;
    }

    if (pid == 0) {
        srand(getpid());
        close(fd1[READ_END]);
        close(fd2[WRITE_END]);

        int sentx, senty, getx, gety, distance, manhattan, minDistance;
        sentx = senty = getx = gety = distance = minDistance = manhattan = 0;
        for (int i = 1; i <= ROUND_COUNT; i++) {
            if(i != 1) {
                close(fd4[WRITE_END]);
                read(fd4[READ_END], &state, sizeof(state));
            }
            if(state == ONGOING && currentRound == i) {
                guess(&sentx, &senty, manhattan);

                read(fd2[READ_END], &getx, sizeof(getx));
                read(fd2[READ_END], &gety, sizeof(gety));

                printf("---------- Round-%d ----------\n", i);
                write(fd1[WRITE_END], &sentx, sizeof(sentx));
                write(fd1[WRITE_END], &senty, sizeof(senty));
                printf("%d.guess of player1: [%d,%d]\n", i, sentx, senty);
                read(fd2[READ_END], &distance, sizeof(distance));
                printf("the distance with player2: %d\n", distance);
                manhattan = distance;
                write(fd3[WRITE_END], &distance, sizeof(distance));

                distance = abs(players[0].x - getx) + abs(players[0].y - gety);
                write(fd1[WRITE_END], &distance, sizeof(distance));

            }

            currentRound++;
        }

        close(fd2[READ_END]);
        close(fd1[WRITE_END]);
        close(fd3[WRITE_END]);

    } else {
        srand(getpid());
        int sentx, senty, getx, gety, distance, manhattan, minDistance;
        sentx = senty = getx = gety = distance = minDistance = manhattan = 0;
        close(fd2[READ_END]);
        close(fd1[WRITE_END]);
        for (int i = 1; i <= ROUND_COUNT; i++) {
            if(state == ONGOING && currentRound == i) {
                guess(&sentx, &senty, manhattan);

                write(fd2[WRITE_END], &sentx, sizeof(sentx));
                write(fd2[WRITE_END], &senty, sizeof(senty));
                read(fd1[READ_END], &getx, sizeof(getx));
                read(fd1[READ_END], &gety, sizeof(gety));
                distance = abs(players[1].x - getx) + abs(players[1].y - gety);
                write(fd2[WRITE_END], &distance, sizeof(distance));
                read(fd1[READ_END], &distance, sizeof(distance));
                manhattan = distance;

                if(distance < players[1].closestGuess)
                    players[1].closestGuess = distance;

                read(fd3[READ_END], &minDistance, sizeof(minDistance));

                if(minDistance < players[0].closestGuess)
                    players[0].closestGuess = minDistance;
                //printf("\n%d\n", players[0].closestGuess);

                if (minDistance == 0) {
                    if(state == P2WON)
                        state = TIE;
                    else
                        state = P1WON;
                }

                printf("%d.guess of player2: [%d,%d]\n", i, sentx, senty);
                printf("the distance with player1: %d\n", distance);

                if (distance == 0) {
                    if(state == P1WON)
                        state = TIE;
                    else
                        state = P2WON;
                }
                close(fd4[READ_END]);
                write(fd4[WRITE_END], &state, sizeof(state));

            }

            currentRound++;
        }

        if(state == P1WON) {
            printf("Player1 won the game \n");
        }
        else if(state == P2WON) {
            printf("Player2 won the game \n");
        }
        else if(state == TIE) {
            printf("Players 1 and 2 tied the game");
        }
        else if (players[0].closestGuess < players[1].closestGuess) {
            printf("Player1 won the game with closest distance guess of %d \n", players[0].closestGuess);
        } else if (players[0].closestGuess > players[1].closestGuess) {
            printf("Player2 won the game with closest distance guess of %d \n", players[1].closestGuess);
        }
        else
            printf("Players 1 and 2 tied the game with closest distance guess of %d \n", players[0].closestGuess);


        close(fd1[READ_END]);
        close(fd2[WRITE_END]);
        close(fd3[READ_END]);
    }

    return 0;
}

void printBox(int width, int height, Player *players, int player_count) {
    // top border
    printf("+");
    for (int i = 0; i < width; i++) {
        printf("--");
    }
    printf("-+\n");

    // field
    for (int i = 0; i < height; i++) {
        printf("|");
        for (int j = 0; j < width; j++) {
            int is_player = 0;
            for (int p = 0; p < player_count; p++) {
                if (players[p].y == i && players[p].x == j) {
                    is_player = players[p].id;
                }
            }
            if (is_player)
                printf("%2d", is_player);
            else
                printf("  ");
        }
        printf(" |\n");
    }

    // bottom border
    printf("+");
    for (int i = 0; i < width; i++) {
        printf("--");
    }
    printf("-+\n");
}

void guess(int *x, int *y, int manhattan) {
    if (manhattan == 0) {
        *x = rand() % map_size;
        *y = rand() % map_size;
    } else {
        int coords[(manhattan + 1) * 4][2];
        int size = 0;
        for (int i = 0; i <= manhattan; i++) {
            int xDiff = i;
            int yDiff = manhattan - xDiff;
            int x1 = (*x) - xDiff;
            int x2 = (*x) + xDiff;
            int y1 = (*y) - yDiff;
            int y2 = (*y) + yDiff;

            if (xDiff == 0) {
                if (x1 >= 0 && x1 < map_size && y1 >= 0 && y1 < map_size) {
                    coords[size][0] = x1;
                    coords[size][1] = y1;
                    size++;
                }
                if (x1 >= 0 && x1 < map_size && y2 >= 0 && y2 < map_size) {
                    coords[size][0] = x1;
                    coords[size][1] = y2;
                    size++;
                }
            } else if (yDiff == 0) {
                if (x1 >= 0 && x1 < map_size && y1 >= 0 && y1 < map_size) {
                    coords[size][0] = x1;
                    coords[size][1] = y1;
                    size++;
                }
                if (x2 >= 0 && x2 < map_size && y1 >= 0 && y1 < map_size) {
                    coords[size][0] = x2;
                    coords[size][1] = y1;
                    size++;
                }
            } else {
                if (x1 >= 0 && x1 < map_size && y1 >= 0 && y1 < map_size) {
                    coords[size][0] = x1;
                    coords[size][1] = y1;
                    size++;
                }
                if (x1 >= 0 && x1 < map_size && y2 >= 0 && y2 < map_size) {
                    coords[size][0] = x1;
                    coords[size][1] = y2;
                    size++;
                }
                if (x2 >= 0 && x2 < map_size && y1 >= 0 && y1 < map_size) {
                    coords[size][0] = x2;
                    coords[size][1] = y1;
                    size++;
                }
                if (x2 >= 0 && x2 < map_size && y2 >= 0 && y2 < map_size) {
                    coords[size][0] = x2;
                    coords[size][1] = y2;
                    size++;
                }
            }
        }

        int guess = rand() % size;
        *x = coords[guess][0];
        *y = coords[guess][1];
    }
}
