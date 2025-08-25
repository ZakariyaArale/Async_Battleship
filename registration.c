#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "registration.h"
#include "broadcast.h"


user* in_server_cfd(user *list_user, int cfd)
{
    for(user *traversal = list_user; traversal != NULL; traversal = traversal->next)
    {
        if(traversal->cfd == cfd) return traversal;
    }
    return NULL;
}

int in_server_name(user *list_user, char* user_name)
{
    for(user *traversal = list_user; traversal != NULL; traversal = traversal->next)
    {
        if(strcmp(traversal->name, user_name) == 0) return 1;
    }
    return 0;
}

user *create_user(user *list_user, int cfd)
{
    user *new_user = calloc(1, sizeof(user));
    if(new_user == NULL) 
    {
        fprintf(stderr, "Failed data allocation #REG :(\n");
        return list_user;
    }
    new_user->start_read = 0;
    new_user->num_hits = -1; //This will also be used to indicate once the user has been registered
    new_user->cfd = cfd;
    new_user->next = list_user;
    return new_user;
}

int read_user_data(int ep, user **list_user, user *target, char *buffer, int cfd)
{
    char *newline_char;
    if((newline_char = memchr(buffer, '\n', target->start_read)) != NULL) //We have at least one full message!
    {
        return newline_char - buffer;
    }
    int bytes_remaining = 101 - target->start_read; 
    while(bytes_remaining > 0)
    {
        int num_bytes = read(cfd, buffer + target->start_read, bytes_remaining);
        if(num_bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) //Reading is blocked, we need to try again/process the msg we may have
        {
            fprintf(stderr, "Trying reading again #REG\n");
            return -1; 
        }
        if(num_bytes <= 0)
        {
            fprintf(stderr, "Can no longer read from client #REG\n");
            (*list_user) = remove_client(ep, (*list_user), cfd);
            return -1;
        }
        bytes_remaining -= num_bytes;
        target->start_read += num_bytes;
        //Verify if we have a full msg and follow msg limits
        if((newline_char = memchr(buffer, '\n', target->start_read)) != NULL) //We have at least one full message!
        {
            return newline_char - buffer;
        }
    }
    //If we reached here, it must be too much data
    fprintf(stderr, "User gave way too much data #REG\n");
    (*list_user) = remove_client(ep, (*list_user), cfd);
    return -1;
}


void add_user(int ep, user **list_user, user *target, int cfd, char* user_msg)
{
    //Store user info
    if(target->num_hits >= 0)
    {
        fprintf(stderr, "User already registered #REG :(\n");
        (*list_user) = invalid_data(ep, (*list_user), cfd);
        return;
    }

    char name[21];
    memset(name, 0, 21);
    int x,y;
    char d;

    //Read/import client data
    if(sscanf(user_msg, "REG %20s %d %d %c", name, &x, &y, &d) != 4) 
    {
        fprintf(stderr, "User gave invalid data #REG :(\n");
        (*list_user) = invalid_data(ep, (*list_user), cfd);
        return;
    }

    //Check if the name is valid
    for(int i = 0; i < strlen(name); i++)
    {
        if(!isalnum(name[i]) && name[i] != '-')
        {
            fprintf(stderr, "User gave invalid name #REG :(\n");
            (*list_user) = invalid_data(ep, (*list_user), cfd);
            return;
        }
    }

    //Check if user name is in the list
    if(in_server_name((*list_user), name))
    {
        fprintf(stderr, "Name taken #REG :(\n");
        (*list_user) = taken_data(ep, (*list_user), cfd);
        return;
    }

    //Store name 
    strcpy(target->name, name);

    //Store ship position
    if(d == '-')
    {
        for(int i = 0; i < 5; i++)
        {
            int new_x = x - 2 + i;
            if((new_x >= 0 && new_x <= 9) && (y >= 0 && y <= 9))
            {
                target->ship_pos[i][0] = new_x;
                target->ship_pos[i][1] = y;
            }
            else
            {
                fprintf(stderr, "User gave invalid ship position #REG :(\n");
                (*list_user) = invalid_data(ep, (*list_user), cfd);
                return;
            }
        }
    }
    else if(d == '|')
    {
        for(int i = 0; i < 5; i++)
        {
            int new_y = y - 2 + i;
            if((x >= 0 && x <= 9) && (new_y >= 0 && new_y <= 9))
            {
                target->ship_pos[i][0] = x;
                target->ship_pos[i][1] = new_y;
            }
            else
            {
                fprintf(stderr, "User gave invalid ship position #REG :(\n");
                (*list_user) = invalid_data(ep, (*list_user), cfd);
                return;
            }
        }
    }
    else
    {
        fprintf(stderr, "User gave invalid ship direction #REG :(\n");
        (*list_user) = invalid_data(ep, (*list_user), cfd);
        return;
    }
    target->num_hits = 0; //Registration complete
    if(write(cfd, "WELCOME\n", 8) == -1)
    {
        fprintf(stderr, "Cant send data to user #REG :(\n");
        (*list_user) = remove_client(ep, (*list_user), cfd);
        return;
    }
    memset(target->attacked, 0, 5 * sizeof(int));
    join_broadcast(ep, list_user, target);
}



user *delete_user(user *list_user, int cfd) //Again we assume 1 registration per client
{
    if(list_user == NULL) return list_user;
    if(list_user->cfd  == cfd)
    {
        user* temp_list = list_user->next;
        free(list_user);
        close(cfd);
        return temp_list;
    }
    
    user *traversal = list_user->next;
    user *previous = list_user;
    while(traversal != NULL)
    {
        if(traversal->cfd == cfd)
        {
            previous->next = traversal->next;
            free(traversal);
            close(cfd);
            break;
        }
        traversal = traversal->next;
        previous = previous->next;
    }
    return list_user;
}

