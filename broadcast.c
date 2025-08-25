#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "broadcast.h"
#include "registration.h"

//Write to the user the given input was invalid, delete to user if we cant write to the user
user* invalid_data(int ep, user *list_user, int cfd)
{
    if(write(cfd, "INVALID\n", 8) == -1)
    {
        list_user = remove_client(ep, list_user, cfd);
    } 
    return list_user;
}


//Write to all users that the user had join the server, delete the user with unsuccessful writes
void join_broadcast(int ep, user **list_user, user *target)
{
    user *traversal = (*list_user);
    while(traversal != NULL)
    {
        char temp_buff[27];
        int num_char = snprintf(temp_buff, 27, "JOIN %s\n", target->name);
        if(write(traversal->cfd, temp_buff, num_char) == -1)
        {
            fprintf(stderr, "Cant write to user #BROAD:(\n");
            int cfd_delete = traversal->cfd;
            traversal = traversal->next;
            (*list_user) = remove_client(ep, (*list_user), cfd_delete);
        }
        else traversal = traversal->next;
    }
}

//Write to all users that the user had a successful hit, delete the user with unsuccessful writes
void hit_broadcast(int ep, user **list_user, char *attacker, int x, int y, char *victim)
{
    user *traversal = (*list_user);
    while(traversal != NULL)
    {
        char temp_buff[57];
        int num_char = snprintf(temp_buff, 57, "HIT %s %d %d %s\n", attacker, x, y, victim);

        if(write(traversal->cfd, temp_buff, num_char) == -1)
        {
            fprintf(stderr, "Cant write to user #BROAD:(\n");
            int cfd_delete = traversal->cfd;
            traversal = traversal->next;
            (*list_user) = remove_client(ep, (*list_user), cfd_delete);
        }
        else traversal = traversal->next;
    }
}

//Write to all users that the user miss, delete the user with unsuccessful writes
void miss_broadcast(int ep, user **list_user, char *attacker, int x, int y)
{
    user *traversal = (*list_user);
    while(traversal != NULL)
    {
        char temp_buff[37];
        int num_char = snprintf(temp_buff, 37, "MISS %s %d %d\n", attacker, x, y);

        if(write(traversal->cfd, temp_buff, num_char) == -1)
        {
            fprintf(stderr, "Cant write to user #BROAD:(\n");
            int cfd_delete = traversal->cfd;
            traversal = traversal->next;
            (*list_user) = remove_client(ep, (*list_user), cfd_delete);
        }

        traversal = traversal->next;
    }
}

//Write to the user the given name exists, delete to user if we cant write to the user
user* taken_data(int ep, user *list_user, int cfd)
{
    if(write(cfd, "TAKEN\n", 6) == -1) 
    {
        list_user = remove_client(ep, list_user, cfd);
    }
    return list_user;
}


user *remove_client(int ep, user* list_user, int cfd) //Assume Client registers once
{ 
    user* temp = in_server_cfd(list_user, cfd);
    if(temp == NULL) return list_user;
    else if(temp->num_hits == -1) //Hasnt been registered yet
    {
        epoll_ctl(ep, EPOLL_CTL_DEL, cfd, NULL);
        list_user = delete_user(list_user, cfd);
        return list_user;
    }

    user *traversal = list_user; int index = 0;

    int cfd_delete[50]; //I dont feel like doing dynamic allocation, would make this more complicated then it has to

    //Broadcast to all users (other than the target) that the user is gone
    while(traversal != NULL)
    {
        if(traversal->cfd == cfd)
        {
            traversal = traversal->next;
            continue;
        }

        //Prepare the writing msg
        fprintf(stderr, "CFD: %d\n", traversal->cfd);
        char temp_buff[26];
        int num_char = snprintf(temp_buff, 26, "GG %s\n", temp->name);

        //Verify if we can write to the user otherwise well delete that user too
        if(write(traversal->cfd, temp_buff, num_char) == -1) 
        {
            fprintf(stderr, "Cant write to user #BROAD:(\n");
            if(index < 50) cfd_delete[index++] = traversal->cfd;
        }

        traversal = traversal->next;
    }

    //Delete that user
    epoll_ctl(ep, EPOLL_CTL_DEL, cfd, NULL);
    list_user = delete_user(list_user, cfd);
   
    //Delete the other users that had trouble writing to
    for(int i = 0; i < index; i++)
    {
        list_user = remove_client(ep, list_user, cfd_delete[i]);
    }
    return list_user;
}