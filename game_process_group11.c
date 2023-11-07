#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct Player {
  int x, y;
  int id;
} Player;

void printBox(int width, int height, Player *players, int pcount);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: ./game_process <map_size>\n");
    return 1;
  }

  int map_size = atoi(argv[1]);
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

  printf("player1: [%d,%d], player2: [%d,%d]\n", players[0].x, players[0].y,
         players[1].x, players[1].y);
  printBox(map_size, map_size, players, 2);

  // TODO:fork? pipe? make a guess?

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