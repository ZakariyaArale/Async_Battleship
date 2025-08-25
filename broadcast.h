#ifndef BROAD_H
#define BROAD_H

#include "registration.h"


user* invalid_data(int ep, user *list_user, int cfd);

void join_broadcast(int ep, user **list_user, user *target);

user* taken_data(int ep, user *list_user, int cfd);

user *remove_client(int ep, user* list_user, int cfd);

void hit_broadcast(int ep, user **list_user, char *attacker, int x, int y, char *victim);

void miss_broadcast(int ep, user **list_user, char *attacker, int x, int y);

#endif