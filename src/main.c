#include <ncurses.h>
#include <sys/types.h>
#include "snake_game.h"


void setup();
void teardown();

int main() {
  setup();

  snake_game_play();

  teardown();

  return 0;
}

void setup() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  timeout(0);
}

void teardown() {
  endwin();
}

