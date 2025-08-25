#ifndef BOMB_H
#define BOMB_H

#include "registration.h"

void process_bombing(int ep, user **list_user, user* target, int cfd, char* user_msg);

user *traverse_ships(int ep, user *list_user, char *attacker, int x, int y);

#endif