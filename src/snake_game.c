#include "snake_game.h"
#include "menu.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

int game_over=0;
int points=0;

sem_t snake_game_dt_sem;

void snake_game_setup();
void snake_game_teardown();
int snake_game_update_state(int dt);
void snake_game_render();

typedef struct {
  int x, y;
} vec2_t;

typedef struct snake_cell {
  vec2_t position;
  struct snake_cell* next;
  struct snake_cell* previous;
} snake_cell_t;

typedef struct {
  snake_cell_t *head, *tail;
  int size;
} snake_t;

snake_t* snake;

vec2_t* generate_food(vec2_t screen_size) {
  vec2_t* food = malloc(sizeof(vec2_t));

  srand(time(NULL));
  food->x = rand()%screen_size.x;
  food->y = rand()%screen_size.y;
  
  return food;
}


void append_to_snake(snake_t* snake) {
  snake->size++;
  snake->tail->next = malloc(sizeof(snake_cell_t));
  snake->tail->next->next = NULL;
  snake->tail->next->previous = snake->tail;
  snake->tail->next->position = snake->tail->position;
  snake->tail = snake->tail->next;
}

void teardown_snake() {
  snake_cell_t *cell = snake->head;
  snake_cell_t *next_cell;

  do {
    next_cell = cell->next;
    free(cell);
    cell = next_cell;
  } while(cell != NULL);

  free(snake);
}

void setup_snake(int begin_size, vec2_t begin_pos) {
  snake = malloc(sizeof(snake_t));
  snake->size = begin_size;

  snake_cell_t* head = malloc(sizeof(snake_cell_t));
  head->next = NULL;
  head->previous = NULL;
  head->position = begin_pos;
  begin_pos.x--;
  snake_cell_t *last, *prev;
  prev=head;
  begin_size--;
  while (begin_size--) {
    last = malloc(sizeof(snake_cell_t));
    last->previous = prev;
    last->next = NULL;
    last->position = begin_pos;
    begin_pos.x--;
    prev->next = last;
    prev = last;
  }

  snake->head = head;
  snake->tail = prev;
}

typedef struct {
  char **tiles;
  int cols, rows;
} grid_t;

grid_t grid;
int max_x, max_y;

void teardown_grid() {
  for (int i = 0; i < grid.rows; i++) {
    free(grid.tiles[i]);
  }
  free(grid.tiles);
}

void setup_grid(
  int columns, int rows
) {
  grid.rows = rows;
  grid.cols = columns;
  grid.tiles = malloc(rows * sizeof(char*));

  // TODO: check if possible to memset entire 2d array with single memset call
  for (int i = 0; i < rows; i++) {
    grid.tiles[i] = malloc(columns * sizeof(char));
    memset(grid.tiles[i], ' ', columns*sizeof(char));
  }
}

void add_ms_to_timespec(struct timespec* t, int ms) {
  t->tv_nsec += ms * 1000000;
  if (t->tv_nsec >= 1000000000) {
    t->tv_nsec -= 1000000000;
    t->tv_sec++;
  }
}

int ms_between_timespecs(struct timespec* t1, struct timespec* t2) {
  return (t2->tv_sec - t1->tv_sec)
    + 1.0e-9 * (t2->tv_nsec - t1->tv_nsec);
}

void snake_game_play() {
  int has_updated = 1;
  float after, before;
  // [TODO]: more consistent way of tracking dt
  // this clock()/CLOCKS_PER_SEC fluctuates, sometimes a lot
  int dt;

  before = ((float)clock())/CLOCKS_PER_SEC;
  snake_game_setup();

  struct timespec t1, t2, next_tick;
  timespec_get(&next_tick, TIME_UTC);
  timespec_get(&t1, TIME_UTC);

  while (!game_over) {
    timespec_get(&t2, TIME_UTC);
    dt = ms_between_timespecs(&t1, &t2);

    t1 = t2;
    next_tick = t2;
    add_ms_to_timespec(&next_tick, 66);
    has_updated = snake_game_update_state(dt);
    before = after;
    snake_game_render();

    sem_timedwait(&snake_game_dt_sem, &next_tick);
  }

  mvprintw(max_y/2, (max_x - 10)/2, "GAME OVER!");
  mvprintw(max_y/2 + 1, (max_x - 10)/2, "Score: %d", points);
  mvprintw(0, 0, "Press 'Q' to go back to the main menu.");
  refresh();
  char c;
  timeout(-1);
  do {
    c =  getch();
  } while(c != 'Q' && c != 'q');

  snake_game_teardown();
}

vec2_t *food;

void snake_game_setup() {
  timeout(0);
  game_over=0;
  points=0;
  getmaxyx(stdscr, max_y, max_x);
  setup_snake(1, (vec2_t){ max_x/2, max_y/2 });
  setup_grid(max_x, max_y);
  food = generate_food((vec2_t){.x=max_x, .y=max_y});
}

void snake_game_teardown() {
  free(food);
  teardown_grid();
  teardown_snake();
}

typedef enum {
  UP, DOWN, LEFT, RIGHT
} DIRECTION;

void move_snake(snake_t *snake, DIRECTION dir) {
  if (snake->size != 1) {
    snake_cell_t *next_head = snake->tail;
    snake_cell_t *next_tail = snake->tail->previous;

    next_head->previous = NULL;
    next_head->next = snake->head;
    snake->head->previous = next_head;
    next_head->position = snake->head->position;

    next_tail->next = NULL;

    snake->head = next_head;
    snake->tail = next_tail;
  }

  switch (dir) {
    case UP:
      snake->head->position.y--;
      if (snake->head->position.y < 0) snake->head->position.y += max_y;
      break;
    case DOWN:
      snake->head->position.y++;
      if (snake->head->position.y >= max_y) snake->head->position.y -= max_y;
      break;
    case LEFT:
      snake->head->position.x--;
      if (snake->head->position.x < 0) snake->head->position.x += max_x;
      break;
    case RIGHT:
      snake->head->position.x++;
      if (snake->head->position.x >= max_x) snake->head->position.x -= max_x;
      break;
  }
}

int check_snake_collision(snake_t *snake) {
  snake_cell_t *cell = snake->head->next;

  while (cell != NULL) {
    if (snake->head->position.x == cell->position.x && snake->head->position.y == cell->position.y) {
      return 1;
    }
    cell = cell->next;
  }

  return 0;
}

int snake_game_update_state(int dt) {
  char ch;
  ch = getch();

  if (ch == 'C' || ch == 'c' || ch == 'q' || ch == 'Q') {
    game_over = 1;
  }
  static DIRECTION dir = RIGHT;
  static DIRECTION previous_dir = RIGHT;

  if (ch == 'W' || ch == 'w') {
    if (previous_dir != DOWN)
      dir = UP;
  }

  if (ch == 'S' || ch == 's') {
    if (previous_dir != UP)
      dir = DOWN;
  }

  if (ch == 'A' || ch == 'a') {
    if (previous_dir != RIGHT)
      dir = LEFT;
  }

  if (ch == 'D' || ch == 'd') {
    if (previous_dir != LEFT)
      dir = RIGHT;
  }

  previous_dir = dir;
  // mvprintw(0, 0, "tick = %d", (int)tick);
  move_snake(snake, dir);

  if (check_snake_collision(snake)) {
    game_over = 1;
  }

  if (snake->head->position.x == food->x && snake->head->position.y == food->y) {
    append_to_snake(snake);
    free(food);
    food = generate_food((vec2_t){max_x, max_y});
    points++;
  }

  return 1;
}

void snake_game_render() {
  for (int i=0;i<grid.cols;i++) {
    for (int j=0;j<grid.rows;j++) {
      mvaddch(j, i, grid.tiles[j][i]);
    }
  }

  mvaddch(food->y, food->x, 'a');

  snake_cell_t *cell_to_render = snake->head;

  do {
    mvaddch(cell_to_render->position.y, cell_to_render->position.x, 'o');
    if (cell_to_render->position.x > 400) cell_to_render->position.x = 0;
  } while ((cell_to_render = cell_to_render->next) != NULL);

  // mvprintw(0, 0, "size=%d,x=%d,y=%d", snake->size,snake->head->position.x, snake->head->position.y);
  mvprintw(0, 0, "%d", points);

  refresh();
}

void snake_game_preview();

menu_option_t snake_game_menu_option = {
  .on_select=&snake_game_play,
  .title="Snek",
  .on_hover=&snake_game_preview,
};

void snake_game_preview() {
  attron(COLOR_PAIR(1));
  getmaxyx(stdscr, max_y, max_x);

  for (int i=0;i<max_x;i++) {
    for (int j=0;j<max_y;j++) {
      mvaddch(j, i, ' ');
    }
  }

  mvaddch(20, 30, 'f');
  mvaddch(20, 31, 'o');
  mvaddch(21, 31, 'o');
  mvaddch(22, 31, 'o');
  mvaddch(22, 32, 'o');
  attroff(COLOR_PAIR(1));

  refresh();
}

