#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define ROUND_COUNT 5

/* Declare shared variables */
sem_t mutex;
int currentRound = 1;
int tr_counter = 1;
int map_size;
int th_count;

/* Player class */
typedef struct Player {
  int x, y;
  int id;
  int closestGuess;
} Player;

enum State {
  ONGOING,
  FINISHED
};

Player* players;
enum State state;

/* Prototype functions */
void printBox(int x, int y, const Player* players, int player_count);

void assignUniquePositions(Player* players, int playerCount, int mapSize);

int getRandomInRange(int min, int max);

void* threadRoutine(void* args);

int main(const int argc, char* argv[]) {
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

  /* initialize semaphore to be used across threads */
  sem_init(&mutex, 0, 1);

  /* allocate memory to Player objects */
  players = malloc(sizeof(Player) * th_count);
  state = ONGOING;

  srand(time(NULL));

  /* initialize Player objects */
  {
    assignUniquePositions(players, th_count, map_size);
    for (int i = 0; i < th_count; i++) {
      players[i].id = i + 1;
      players[i].closestGuess = INT_MAX;
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

  printf("----------Round %d ----------\n", currentRound);
  /* Allocate memory to thread objects and call them */
  pthread_t* threads = malloc(sizeof(pthread_t) * th_count);
  for (int i = 0; i < th_count; i++) {
    pthread_create(threads + i, NULL, threadRoutine, players + i);
  }

  /* Wait for termination */
  for (int i = 0; i < th_count; i++) {
    pthread_join(threads[i], NULL);
  }

  //printf("Game state: %d", state);
  if(state == FINISHED) {
    /* destroy objects */
    free(threads);
    free(players);
    sem_destroy(&mutex);
    return 0;
  }

  // Checking for the winner(s) with the closest guess
  printf("The game ends!\n");

  /* helper vars */
  int minDistance = INT_MAX;
  int thBool[th_count];
  int thIndex[th_count];
  int winnerCount = 0;
  int winnerIndex = 0;

  /* find minumum distance */
  for (int i = 0; i < th_count; i++) {
    if(players[i].closestGuess < minDistance) {
      minDistance = players[i].closestGuess;
      thBool[i] = 0;
      thIndex[i] = 0;
    }
  }

  /* mark player(s) with minimum distance */
  for (int i = 0; i < th_count; i++) {
    if(players[i].closestGuess == minDistance) {
      thBool[i] = 1;
      thIndex[i] = players[i].id;
      winnerCount++;
      winnerIndex = players[i].id;;
    }
  }

  /* Win/Tie conditions */
  if(winnerCount == 1) {
    printf("Player%d won the game with closest distance guess of %d \n", winnerIndex, minDistance);
  }
  else {
    printf("Players: ");
    for (int i = 0; i < th_count; i++) {
      if(thBool[i] == 1) {
        printf(" %d", thIndex[i]);
        if(i < winnerCount - 1)
          printf(", ");
      }
    }
    printf(". tied the game with a distance guess of %d \n", minDistance);
  }

  printf("\n");

  /* destroy objects */
  free(threads);
  free(players);
  sem_destroy(&mutex);
  return 0;
}

int getRandomInRange(int min, int max) {
  return rand() % (max - min + 1) + min;
}

void* threadRoutine(void* args) {
    Player* p = (Player*)args; /* type-cast voidptr to Player type */

    int minDistance = INT_MAX;
    // Randomly choosing a new guess x and y for the first round
    int guessX = rand() % map_size;
    int guessY = rand() % map_size;

    /* Iterate for ROUND_COUNT times */
    while (currentRound <= ROUND_COUNT && state == ONGOING) {
        sem_wait(&mutex); /* lock */

        printf("%d.guess of player%d: [%d,%d]\n", currentRound, p->id, guessX, guessY);

        /* check player's guess with other players' coordinates */
        for (int i = 0; i < th_count; i++) {
            if (i != p->id - 1) {
                // Calculate the Manhattan distance between the current player's guess and other players' positions
                const int distance = abs(guessX - players[i].x) + abs(guessY - players[i].y);
                printf("the distance with player%d: %d\n", players[i].id, distance);

                // Check if the distance is 0, indicating that the current player has won
                if (distance == 0) {
                  state = FINISHED; /* Skip other threads */
                  printf("**********************************\n");
                  printf("player%d won the game !!!\n", p->id);
                  printf("**********************************\n");
                  return(0);
                }

                // Update minDistance based on the Manhattan distance
                if (distance < minDistance) {
                    minDistance = distance;
                }
                /* update score of player */
                p->closestGuess = minDistance;
            }
        }
        /* increment thread round counter */
        tr_counter++;

        // Calculate the boundaries for the next guess based on the minDistance
        int minX = guessX - minDistance;
        int minY = guessY - minDistance;
        int maxX = guessX + minDistance;
        int maxY = guessY + minDistance;

        // Adjust boundaries to ensure they are within the map limits
        if (minX < 0) { minX = 0; }
        if (minY < 0) { minY = 0; }
        if (maxX >= map_size) { maxX = map_size - 1; }
        if (maxY >= map_size) { maxY = map_size - 1; }

        // Generate a new random guess within the adjusted boundaries
        guessX = getRandomInRange(minX, maxX);
        guessY = getRandomInRange(minY, maxY);

        // Check if all threads have completed their turns for the current round
        if (tr_counter == th_count + 1) {
            currentRound++;
            tr_counter = 1;

            if (currentRound <= ROUND_COUNT) {
                printf("----------Round %d ---------- \n", currentRound);
            }
        }

        sem_post(&mutex); /* unlock */

        // Wait for other threads to complete their turns for the current round
        while (tr_counter != p->id && currentRound <= ROUND_COUNT && state != FINISHED);
    }

    return NULL;
}

void printBox(const int x, const int y, const Player* players, const int player_count) {
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
        printf("  .");
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
 * 1. Create an array of all integers representing possible points
 * 2. Select random index r, assign the position at index r to a player
 * 3. Swap the last item in the array with the item r
 * 4. Decrease the size of array by 1
 *
 * @see https://www.tldraw.com/s/v2_c_BMjYadFUbGcHkvWDuP8GA?viewport=35,15,1443,740&page=page:oOgqbFfUkr671f-oFy9Of
 *
 * @warning Allocates (mapSize * mapSize * 4) bytes of memory during operation
 *
 * @param players Array of Players which will be assigned to a position, must be
 * allocated before passing
 * @param playerCount The number of players to assign positions for.
 * @param mapSize The size of the square map
 */
void assignUniquePositions(Player* players, const int playerCount, const int mapSize) {
  // calculate the total number of positions on the map
  const size_t posCount = mapSize * mapSize;

  // allocate (mapSize * mapSize * 4) bytes for the array of available position indexes
  int* availables = malloc(posCount * sizeof(int));

  int avPosCount = mapSize * mapSize; // number of available positions

  // fill available position indexes array with all possible positions
  for (int i = 0; i < posCount; i++)
    availables[i] = i;

  int p = 0; // player index
  while (avPosCount > 0 && p < playerCount) {
    // select a random index [0, position count)
    const int r = rand() % avPosCount;
    // assign the selected position to the current player
    players[p].x = availables[r] % mapSize;
    players[p].y = availables[r] / mapSize;

    // replace the selected position with the last position in the array
    availables[r] = availables[avPosCount - 1];
    avPosCount--; // decrement the available position counter by 1
    p++; // select next player
  }
  free(availables);
}
