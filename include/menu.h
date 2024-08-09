#ifndef MENU_H
#define MENU_H

typedef struct menu_option {
  char* title;
  void (*on_select)();
  void (*on_hover)();
} menu_option_t;

typedef struct menu {
  menu_option_t *options;
  int n_options;
  int hover_i;
} menu_t;

#endif
