#include <string.h>
#include <ncurses.h>
#include <sys/types.h>
#include "snake_game.h"


void setup();
void teardown();

typedef struct {
  char* label;
  void (*invoke_on_select)(void);
} option_t;

typedef struct {
  int currently_selected, size;
  option_t options[];
} options_menu_t;

void opa();

options_menu_t menu = {
  .currently_selected=0,
  .size=2,
  .options={{.label="snake_game", .invoke_on_select=&snake_game_play},{.label="opa2", .invoke_on_select=&opa}}
};

void opa() {
  static int count=0;
  count++;
  mvprintw(10, 0, "contagem: %d", count);
}

int max_y2, max_x2;

int main() {
  setup();

  char ch=0;
  while (1) {
    for (int i=0;i<max_y2; i++) {
      for (int j=0; j<max_x2; j++) {
        mvaddch(i, j, ' ');
      }
    }

    if (ch == 'w') {
      menu.currently_selected++;
      if (menu.currently_selected >= menu.size) {
        menu.currently_selected = 0;
      }
    }

    if (ch == 'e') {
      void (*func)(void) = menu.options[menu.currently_selected].invoke_on_select;
      func();
    }

    if (ch == 'q') {
      break;
    }

    for (size_t i=0; i < menu.size; i++) {
      if (menu.currently_selected == i) {
        mvprintw(max_y2/2, (max_x2 - 10)/2, menu.options[i].label);
      }
    }


    mvprintw(0, 0, "Press 'Q' to exit.");
    refresh();
    
    ch = getchar();
  }

  teardown();

  return 0;
}

void setup() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  timeout(0);
  getmaxyx(stdscr, max_y2, max_x2);
}

void teardown() {
  endwin();
}

