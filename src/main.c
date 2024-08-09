#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/types.h>
#include "snake_game.h"
#include "menu.h"


void setup();
void teardown();

void opa();

menu_t *menu;

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
      menu->hover_i++;
      if (menu->hover_i >= menu->n_options) {
        menu->hover_i = 0;
      }
    }

    if (ch == 'e') {
      void (*func)(void) = menu->options[menu->hover_i].on_select;
      func();
    }

    if (ch == 'q') {
      break;
    }
    void (*current_on_hover)(void) = menu->options[menu->hover_i].on_hover;
    current_on_hover();

    for (size_t i=0; i < menu->n_options; i++) {
      if (menu->hover_i == i) {
        mvprintw(max_y2/2, (max_x2 - 10)/2, menu->options[i].title);
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
  menu = malloc(sizeof(menu_t));

  menu->hover_i=0,
  menu->n_options=2,
  menu->options=malloc(sizeof(menu_option_t) * menu->n_options);
  menu->options[0] = snake_game_menu_option;
  menu->options[1] =(menu_option_t){.title="opa2", .on_select=&opa, .on_hover=&opa};

  initscr();
  start_color();

  init_pair(1, COLOR_BLUE, COLOR_BLACK);
  
  raw();
  noecho();
  keypad(stdscr, TRUE);
  getmaxyx(stdscr, max_y2, max_x2);
}

void teardown() {
  endwin();
}

