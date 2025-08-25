#ifndef REG_H
#define REG_H
typedef struct user {
    //Buffer per user, although this is a bit mem consuming we have to make sure we process
    //data uniquely (as read may give partial or more that 1 msg)
    char user_msg[101];
    //Seperate possible msg form incoming data 
    int start_read;
    int cfd; 
    int attacked[5];
    int num_hits; //Once 5 is reached game over
    //This will be null terminated for simplicity, but we wont write null bytes
    char name[21]; 
    //This will store the coordinate of the users ship
    int ship_pos[5][2];
    struct user* next;
}user;

user *create_user(user *list_user, int cfd);

user* in_server_cfd(user *list_user, int cfd); 

int read_user_data(int ep, user **list_user, user *target, char *buffer, int cfd);

int in_server_name(user *list_user, char* user_name);

void add_user(int ep, user **list_user, user *target, int cfd, char* user_msg);

user *delete_user(user *list_user, int cfd);

#endif