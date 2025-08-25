#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>  
#include <signal.h>
#include "registration.h"
#include "broadcast.h"
#include "bombing.h"

#define MAX_SIZE_BUFF 101

//Prevent server from being killed from client disconnection
void ignore_sigpipe(void) 
{
    struct sigaction myaction; myaction.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &myaction, NULL);
}

void shift_data(char *buffer, user* target_user, int new_line_index)
{
    //Shift the buffer down to the next possible msg
    memmove(buffer, buffer + new_line_index + 1, target_user->start_read - (new_line_index + 1)); 
    //Update the position for reading due to shift
    target_user->start_read -= new_line_index + 1; 
}

void process_data(int ep, user** list_user, int cfd)
{
    //Read the data that the user has sent
    user *temp_user;
    int position_idx;

    //Add new client to the LL
    if((temp_user = in_server_cfd((*list_user),cfd)) == NULL) 
    {
        temp_user = (*list_user);
        (*list_user) = create_user((*list_user), cfd);

        //Verify if node creation was successful
        if(temp_user == (*list_user)) return; 
        temp_user = (*list_user);
    }
    
    //Keep processing our data untill we cant read anymore (hanging) and there no more full msg
    while((position_idx = read_user_data(ep, list_user, temp_user, temp_user->user_msg, cfd)) != -1)
    {
        temp_user->user_msg[position_idx] = '\0'; //Ensure we parse only the msg
        
        //User most likely wants to register, we'll verfify this in registration.c, assume register once
        if(temp_user->user_msg[0] == 'R') 
        {
            add_user(ep, list_user, temp_user, cfd, temp_user->user_msg);
        }
        //User most likely wants to Bomb, we'll verfify this in bombing.c
        else if(temp_user->user_msg[0] == 'B') 
        {
            process_bombing(ep, list_user, temp_user, cfd, temp_user->user_msg);
        }
        else (*list_user) = invalid_data(ep, (*list_user), cfd); 
        //Shift buffer to the next possible msg
        shift_data(temp_user->user_msg, temp_user, position_idx);
    }
}


int main(int argc, char **argv)
{
    if(argc < 2)
    {
        fprintf(stderr, "Please give a port number!\n");
        return 1;
    }
    //Read port num
    uint16_t port;
    if(sscanf(argv[1], "%hu", &port) != 1)
    {
        fprintf(stderr, "Can't processes port num!\n");
        return 1;
    }
    //Ensure SIGPIPE doesn't terminate our server
    ignore_sigpipe(); 

    //Create socket
    int sfd;
    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Unsuccessfull Socket Creation :(\n");
        return 1;
    }

    //Set Address
    struct sockaddr_in a;
    memset(&a, 0, sizeof(struct sockaddr_in));
    a.sin_family = AF_INET; a.sin_port = htons(port); a.sin_addr.s_addr = htonl(INADDR_ANY);

    //Bind
    if(bind(sfd, (struct sockaddr*) &a, sizeof(struct sockaddr_in)) == -1)
    {
        fprintf(stderr, "Socket failed to bind :(\n");
        return 1;
    }

    //Listen
    if(listen(sfd, 100) == -1)
    {
        fprintf(stderr, "Cannot make waiting room via listen :(\n");
        return 1;
    }

    //Make Epoll for clients and server
    int ep;
    struct epoll_event ev;
    if((ep = epoll_create1(0)) == -1)
    {
        fprintf(stderr, "Failed to create epoll :(\n");
        return 1;
    }

    //Add socket to epoll to indicate when new user are available
    ev.events = EPOLLIN; ev.data.fd = sfd;
    if(epoll_ctl(ep, EPOLL_CTL_ADD, sfd, &ev) == -1)
    {
        fprintf(stderr, "Failed to add socket to epoll :(\n");
        return 1;
    }

    //Actual Implementation
    user* list_user = NULL;
    while(1)
    {
        int i, n, cfd;
        struct epoll_event es[100];

        n = epoll_wait(ep, es, 100, -1);
        for(i = 0; i < n; i++)
        {
            //If sfd pops in our event we have a new client :)
            if(es[i].data.fd == sfd) 
            {
                struct sockaddr_in ca; 
                socklen_t sinlen;
                sinlen = sizeof(struct sockaddr_in);
                if((cfd = accept(sfd, (struct sockaddr *) &ca, &sinlen)) != -1)
                {
                    //Add the user for reading to epoll 
                    ev.events = EPOLLIN; ev.data.fd = cfd;
                    if(epoll_ctl(ep, EPOLL_CTL_ADD, cfd, &ev) == -1)
                    {
                        fprintf(stderr, "Failed to add client to epoll :(\n");
                        return 1;
                    }
                    int flags = fcntl(cfd, F_GETFL);
                    fcntl(cfd, F_SETFL, flags | O_NONBLOCK); //We dont want any hanging 
                }
            }
            else process_data(ep, &list_user, es[i].data.fd); //We process our current users
        }
    }
}