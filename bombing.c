#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "registration.h"
#include "broadcast.h"

user *traverse_ships(int ep, user *list_user, char *attacker, int x, int y)
{
    user* traversal = list_user;
    int tag = 1;
    while(traversal != NULL)
    {
        //Verify if a hit has been made, if not we'll broadcast a miss
        for(int i = 0; i < 5; i++)
        {
            if(traversal->ship_pos[i][0] == x && traversal->ship_pos[i][1] == y)
            {
                hit_broadcast(ep, &list_user, attacker, x, y, traversal->name);
                if(traversal->attacked[i] == 0)
                {
                    traversal->num_hits++;
                    traversal->attacked[i] = 1;
                } 
                tag = 0;
            }
        }
        traversal = traversal->next;
    }
    if(tag) miss_broadcast(ep, &list_user, attacker, x, y);
    return list_user;
}

void process_bombing(int ep, user **list_user, user* target, int cfd, char* user_msg)
{
    if(target == NULL) return;
    int x, y, cfd_delete;

    //Process bomb request
    if(sscanf(user_msg, "BOMB %d %d", &x, &y) != 2)
    {
        fprintf(stderr, "User gave invalid data #BOMB :(\n");
        (*list_user) = invalid_data(ep, (*list_user), cfd);
        return;
    }

    //Check if the target is valid
    if(x < 0 || x > 9 || y < 0 || y > 9) 
    {
        fprintf(stderr, "User gave invalid bomb pos #BOMB :(\n");
        miss_broadcast(ep, list_user, target->name, x, y);
        return;
    }
    else (*list_user) = traverse_ships(ep, (*list_user), target->name, x, y);

    //Delete the users that have their ship destroyed
    user *temp = (*list_user);
    while(temp != NULL)
    {
        if(temp->num_hits == 5) //Broadcast first, then delete according to Albert's server
        {
            //Prepare the writing msg
            fprintf(stderr, "CFD: %d\n", temp->cfd);
            char temp_buff[26];
            int num_char = snprintf(temp_buff, 26, "GG %s\n", temp->name);

            user *temp2 = (*list_user);
            while(temp2 != NULL)
            {
                //Verify if we can write to the user otherwise well delete that user too
                if(write(temp2->cfd, temp_buff, num_char) == -1) 
                {
                    cfd_delete = temp2->cfd;
                    temp2 = temp2->next;
                    (*list_user) = remove_client(ep, (*list_user), cfd_delete);
                }
                else temp2 = temp2->next;
            }
        }
        temp = temp->next;
    }
    temp = (*list_user);
    while(temp != NULL)
    {
        if(temp->num_hits == 5)
        {
            cfd_delete = temp->cfd;
            temp = temp->next;
            epoll_ctl(ep, EPOLL_CTL_DEL, cfd_delete, NULL);
            (*list_user) = delete_user((*list_user), cfd_delete);
        }
        else temp = temp->next;
    }
}