#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROUND_COUNT 5

/* Declare shared variables */
sem_t mutex[ROUND_COUNT];
int currentRound = 1;
int tr_counter = 1;
int map_size;
int th_count;

/* Player class */
typedef struct Player {
  int x, y;
  int id;
  int score;
} Player;

Player *players;

/* Prototype functions */
void printBox(int x, int y, const Player *players, int player_count);
void assignUniquePositions(Player *players, int playerCount, int mapSize);
void *threadRoutine(void *args);

int main(const int argc, char *argv[]) {
  /* Handle all possible CLA errors */
  if (argc != 3) {
    printf("Usage: ./game_threads <map_size> <thread_count>\n");
    return 1;
  }

  map_size = atoi(argv[1]);
  th_count = atoi(argv[2]);
  if (map_size < 2) {
    printf("Map size cannot be less than 2\n");
    return 1;
  }
  if (th_count < 2) {
    printf("Thread/Player count cannot be less than 2\n");
    return 1;
  }
  if (th_count > map_size * map_size) {
    printf("Cannot put %d player into %dx%d map\n", th_count, map_size,
           map_size);
    return 1;
  }

  printf("Map size: %dx%d\n", map_size, map_size);
  printf("Thread count: %d\n", th_count);

  /* initialize semaphores to be used across threads */
  for(int z = 0; z < ROUND_COUNT; z++) {
    sem_init(&mutex[z], 0, 1);
  }

  /* allocate memory to Player objects */
  players = malloc(sizeof(Player) * th_count);

  srand(time(NULL));

  /* initialize Player objects */
  {
    assignUniquePositions(players, th_count, map_size);
    for (int i = 0; i < th_count; i++) {
      players[i].id = i + 1;
      players[i].score = 0;
    }
  }

  /* print players info */
  for (int i = 0; i < th_count; i++) {
    printf("player%d: [%d,%d]", players[i].id, players[i].x, players[i].y);
    if (i != th_count - 1)
      printf(", ");
  }
  printf("\n");
  printBox(map_size, map_size, players, th_count);

  printf("currentRound: %d\n", currentRound);

  /* Allocate memory to thread objects and call them */
  pthread_t *threads = malloc(sizeof(pthread_t) * th_count);
  for (int i = 0; i < th_count; i++) {
    pthread_create(threads + i, NULL, threadRoutine, players + i);
  }

  /* Wait for termination */
  for (int i = 0; i < th_count; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("currentRound: %d\n", currentRound);

  /* destroy objects */
  free(threads);
  free(players);
  for(int z = 0; z < ROUND_COUNT; z++) {
    sem_destroy(&mutex[z]);
  }
  return 0;
}

void *threadRoutine(void *args) {
  const Player p = *(Player *)args; /* type-cast voidptr to Player type */

  /* need to check if currentRound is not bigger than ROUND_COUNT before entering, use if statement */

  /* lock */
  sem_wait(&mutex[currentRound]);

  /* CRITICAL SECTION */
  printf("I am player #%d at [%d,%d]\n", p.id, p.x, p.y);
  // TODO: Make a guess?
  /*
   * CALCULATE MANHATTAN DISTANCE TO ALL OTHER PLAYERS
   *
   * either do the whole guessing logic here or make
   * a separate function and call it here, you can declare
   * additional global variables to communicate between threads
   * and store information about each round.
   */
  sleep(4); /* remove sleep function when actually testing (placeholder) */

  tr_counter++; /* increment dummy variable */
  /* if dummy variable reaches ROUND_COUNT, transition into new round and reset dummy variable */
  if(tr_counter == ROUND_COUNT) {
    currentRound++;
    tr_counter = 0;
  }

  printf("tr_counter: %d\n", tr_counter);

  /* unlock */
  sem_post(&mutex[currentRound]);


  /* remainder non-critical section */

  return NULL;
}

void printBox(const int x, const int y, const Player *players, const int player_count) {
  // top border
  printf("+");
  for (int i = 0; i < x; i++) {
    printf("---");
  }
  printf("-+\n");

  // field
  for (int i = 0; i < y; i++) {
    printf("|");
    for (int j = 0; j < x; j++) {
      int is_player = 0;
      for (int p = 0; p < player_count; p++) {
        if (players[p].y == i && players[p].x == j) {
          is_player = players[p].id;
        }
      }
      if (is_player)
        printf("%3d", is_player);
      else
        printf("   ");
    }
    printf(" |\n");
  }

  // bottom border
  printf("+");
  for (int i = 0; i < x; i++) {
    printf("---");
  }
  printf("-+\n");
}

/**
 * @brief Assigns unique positions to given players
 *
 * 1. Create an array of all possible points
 * 2. Select random index r, assign the position at index r to a player
 * 3. Swap the last item in the array with the item r
 * 4. Decrease the size of array(k) by 1
 *
 * @warning Allocates (mapSize * mapSize * 8) bytes of memory during operation
 *
 * @param players Array of Players which will be assigned to a position, must be
 * allocated before passing
 * @param playerCount The number of players to assign positions for.
 * @param mapSize The size of the square map
 */
void assignUniquePositions(Player *players, const int playerCount, const int mapSize) {
  struct Point {
    int x;
    int y;
  };

  // calculate the total number of positions on the map
  const size_t posCount = mapSize * mapSize;

  // allocate (mapSize * mapSize * 8) bytes for the array of available positions
  struct Point *availables = malloc(posCount * sizeof(struct Point));

  int avPosCount = 0; // number of available positions

  // fill available positions array with all possible positions
  for (int i = 0; i < mapSize; i++) {
    for (int j = 0; j < mapSize; j++) {
      availables[avPosCount].x = j;
      availables[avPosCount].y = i;
      avPosCount++;
    }
  }

  int p = 0; // player index
  while (avPosCount > 0 && p < playerCount) {
    // select a random index [0, position count)
    int r = rand() % avPosCount;
    // assign the selected position to the current player
    players[p].x = availables[r].x;
    players[p].y = availables[r].y;

    // replace the selected position with the last position in the array
    availables[r] = availables[avPosCount - 1];
    avPosCount--; // decrement the available position counter by 1
    p++;          // select next player
  }
  free(availables);
}
