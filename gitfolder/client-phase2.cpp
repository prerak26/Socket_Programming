#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
// #include<crypto++/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>

#define BACKLOG 25
#define SIZE 1024

int PORT_LIST_ON;

using namespace std;
using std::cin;
using std::cout;
using std::endl;
using std::vector;

// get sockaddr, IPv4 or
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_listener_socket(void)
{
    // Port we're listening on
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;
    struct addrinfo hints, *ai, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // Get us a socket and bind it
    if ((rv = getaddrinfo(NULL, (to_string(PORT_LIST_ON)).c_str(), &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    // Lose the pesky "address already in use" error message
    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL)
    {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1)
    {
        return -1;
    }

    return listener;
}

// Add a new file descriptor to the set

void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // // If we don't have room, add more space in the pfds array
    // if (*fd_count == *fd_size) {
    // *fd_size *= 2; // Double it

    // pfds = realloc(*pfds, sizeof(*pfds) * (*fd_size));
    // }

    (*pfds)[*fd_count].fd = newfd;

    // need to change
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

vector<string> tokenize(string s, string del = "#")
{
    vector<string> ans;
    int start = 0;
    int end = s.find(del);
    while (end != -1)
    {
        ans.push_back(s.substr(start, end - start));
        start = end + del.size();
        end = s.find(del, start);
    }
    return ans;
}

int sendall(int s,string filename, char *buf, long *len)
{
    long total = 0;        // how many bytes we've sent
    long bytesleft = *len; // how many we have left to send
    long n;
    
    string msg=to_string(bytesleft)+' ';
    long l=512;
    int a=min(l,bytesleft);
    for(long i=0;i<a;i++){
        msg+=buf[total+i];
    }
    
        n=send(s,msg.c_str(),msg.length(),0);
        if (n == -1) { return n;}
        total += a;
        bytesleft -= a;
    
    msg = "";    

    while(total < *len) {
        msg="";
        a=min(l,bytesleft);
        //string msg=filename+" "+to_string(bytesleft)+' ';
        long l=512;
        for(long i=0;i<a;i++){
            msg+=buf[total+i];
        }
        //msg+="5#";
        // n = send(s, buf+total, bytesleft, 0);
        //cout<<msg<<endl;
        n=send(s,msg.c_str(),msg.length(),0);
        // cout<<n<<" "<<a<<endl;
        if (n == -1) { break; }
        total += a;
        bytesleft -= a;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

void send_file(FILE *fp, int sockfd){
    int n;
    char data[SIZE] = {0};
    
    while(fgets(data, SIZE, fp) != NULL) {
        if (send(sockfd, data, sizeof(data), 0) == -1) {
        perror("[-]Error in sending file.");
        exit(1);
        }
        bzero(data, SIZE);
    }
}



void write_file(string new_rfile,int sockfd){
    int n;
    FILE *fp;
    // string filename = new_rfile;
    char buffer[SIZE];
    
    fp = fopen(new_rfile.c_str(), "w");
    while (1) {
        n = recv(sockfd, buffer, SIZE, 0);
        if (n <= 0){
            break;
            return;
        }
        fprintf(fp, "%s", buffer);
        bzero(buffer, SIZE);
    }
    return;
}

void string2hexString(unsigned char * input,unsigned char * output){
    int loop=0;
    int i=0;
    while(input[loop]!='\0'){
        sprintf((char*)(output+i),"%02x",input[loop]);
        loop++;
        i+=2;
    }
    output[i++]='\0';
}
template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream  
         << std::setfill ('0') << std::setw(2) 
         << std::hex << i;
  return stream.str();
}


int main(int argc, char *argv[])
{
    DIR *dir;
    struct dirent *diread;
    vector<string> files;

    if ((dir = opendir(argv[2])) != nullptr)
    {
        while ((diread = readdir(dir)) != nullptr)
        {
            if (string(diread->d_name) != "." && string(diread->d_name) != ".." && string(diread->d_name)!="Downloaded")
            {
                files.push_back(diread->d_name);
            }
        }
        closedir(dir);
    }
    else
    {
        perror("opendir");
        return EXIT_FAILURE;
    }

    sort(files.begin(),files.end());
    for (auto file : files)
        cout << file << endl;

    vector<string> search_files;

    int OWN_CLIENT_NO;
    int OWN_UNIQUE_ID;
    int NO_FILES;
    int NO_NEIGHBOUR;

    freopen(argv[1], "r", stdin);
    vector<pair<int, int>> neighbours_accept;
    vector<pair<int, int>> neighbours_connect;
    cin>>OWN_CLIENT_NO>>PORT_LIST_ON>>OWN_UNIQUE_ID;
    cin>>NO_NEIGHBOUR;
    for(int i=0;i<NO_NEIGHBOUR;i++){
        int c_no;int p;
        cin>>c_no>>p;
        if(c_no>OWN_CLIENT_NO){
            neighbours_connect.push_back(make_pair(c_no,p));
        }
        else{
            neighbours_accept.push_back(make_pair(c_no,p));
        }
    }
    cin>>NO_FILES;
    for(int i=0;i<NO_FILES;i++){
        string a;cin>>a;
        search_files.push_back(a);
    }

    // listening on port
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    int x;

    struct addrinfo hints, *res;
    int status, numbytes = 1024;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int new_fd, listener;

    char buf[256];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char theirIP[INET6_ADDRSTRLEN];
    int numberofaccept = 0;

    int fd_count = 0;
    int fd_size = 25;
    struct pollfd *pfds = new pollfd[fd_size];
    listener = get_listener_socket();
    if (listener == -1)
    {
        fprintf(stderr, "error getting a listening socket\n");
        exit(1);
    }
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;
    fd_count++;

    string r_path = argv[2];
    string path = r_path+"Downloaded";

    // mkdir(path.c_str(), 0777);
   
    map<int,int> neighbours_2;
    vector<pair<int, pair<int, int>>> n_sockfd;
    set<pair<int, int>> n_connect(neighbours_connect.begin(), neighbours_connect.end());
    set<pair<int, int>> pandu = n_connect;
    int n_accept = neighbours_accept.size();
    map<string,pair<int,int>> files_to_be_found_2;

    int LOOP_NUMBER = 1;
    int B_3_M = 0;
    
    vector<pair<int,string>> phase1_output;
    while (n_sockfd.size() != (neighbours_connect.size() + neighbours_accept.size()))
    {
        n_connect = pandu;
        for (auto it : n_connect)
        {

            if ((status = getaddrinfo(NULL, (to_string(it.second)).c_str(), &hints, &res)) != 0)
            {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
                return 2;
            }

            // make a socket:
            if ((new_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
            {
                perror("server: socket");
            }

            if (connect(new_fd, res->ai_addr, res->ai_addrlen) < 0)
            {
                close(new_fd);
                // perror("client connect");
                continue;
            }

            add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);
            pandu.erase(it);
            inet_ntop(res->ai_family, get_in_addr((struct sockaddr *)res->ai_addr), theirIP, sizeof theirIP);
            freeaddrinfo(res); // all done with this structure

            string msg = "Connected to " + to_string(OWN_CLIENT_NO) + " with unique-ID " + to_string(OWN_UNIQUE_ID) + " on port " + to_string(PORT_LIST_ON) +" "+ to_string(1) + "#";
            if (send(new_fd, (msg).c_str(), msg.length(), 0) == -1)
            {
                perror("send");
            }
        }

        int poll_count = poll(pfds, fd_count, 1000);

        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        for (int i = 0; i < fd_count; i++)
        {
            // Check if someone's ready to read
            if (pfds[i].revents & POLLIN)
            { // We got one!!

                if (pfds[i].fd == listener)
                {
                    // If listener is ready to read, handle new connection
                    addr_len = sizeof their_addr;
                    new_fd = accept(listener, (struct sockaddr *)&their_addr, &addr_len);

                    if (new_fd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        add_to_pfds(&pfds, new_fd, &fd_count, &fd_size);
                        n_accept--;
                        string msg = "Connected to " + to_string(OWN_CLIENT_NO) + " with unique-ID " + to_string(OWN_UNIQUE_ID) + " on port " + to_string(PORT_LIST_ON) +" "+to_string(1) + "#";
                        if (send(new_fd, (msg).c_str(), msg.length(), 0) == -1)
                        {
                            perror("send");
                        }
                    }
                }
                else
                {
                    // If not the listener, we're just a regular client
                    char recvbuf[numbytes];
                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                    recvbuf[nbytes] = '\0';
                    int sender_fd = pfds[i].fd;
                    if (nbytes <= 0)
                    {
                        // Got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // Connection closed
                            // printf("pollserver: socket %d hung up\n", sender_fd);
                        }
                        else
                        {
                            // perror("recv");
                        }
                        close(pfds[i].fd); // Bye!
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        vector<string> r_msgs = tokenize(recvbuf);
                        vector<string> r_msg[10];
                        for(auto it: r_msgs){
                            int l = it.size(); 
                            int x = it[l-1] - '0';
                            r_msg[x].push_back(it);
                        }
                        for (auto it : r_msg[1]){
                            int l = it.size();
                            it=it.substr(0,it.size()-2);
                                // cout << it << endl;
                                string temp_1;
                                int i = 13;
                                while (it[i] != ' ')
                                {
                                    temp_1 = temp_1 + it[i];
                                    i++;
                                }
                                string temp_2;
                                int j = 29 + temp_1.size();
                                while (it[j] != ' ')
                                {
                                    temp_2 = temp_2 + it[j];
                                    j++;
                                }
                                phase1_output.push_back(make_pair(stoi(temp_1),it));
                                n_sockfd.push_back(make_pair(stoi(temp_2), make_pair(stoi(temp_1), sender_fd)));
                        }
                        
                        for(auto it:r_msg[9]){
                            if ((send(sender_fd, "1 8#", 4, 0)) == -1)
                                {
                                    perror("send");
                                }
                        }
                        int reply_check = 0;
                        for(auto it:r_msg[3]){
                            int l = it.size();
                            // Question asked: reply to it
                            it[l - 1] = '\0';
                            int k = 0;
                            string fname="";
                            while(it[k]!=' '){
                               fname+=it[k];k++;     
                            }
                            k++;
                            string UID="";
                            while(it[k]!=' '){
                               UID+=it[k];k++;     
                            }
                            k++;
                            string peer_port="";
                            while(it[k]!=' '){
                               peer_port+=it[k];k++;     
                            }
                            neighbours_2[stoi(UID)]=stoi(peer_port);
                            for(auto file:search_files){
                                if(fname==file){
                                    if (files_to_be_found_2[fname].first == 0)
                                    {
                                        files_to_be_found_2[fname] = make_pair(stoi(UID),2);
                                    }
                                    else if (files_to_be_found_2[fname].second == 2){
                                        files_to_be_found_2[fname] = make_pair(min(stoi(UID), files_to_be_found_2[fname].first),2);
                                    }
                                    else
                                    {
                                    }
                                }
                            } 
                            reply_check = 1;
                                
                        }
                        if(reply_check==1){
                            B_3_M--;
                            reply_check = 0;
                        }
                    } // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!
    }

    // start of phase 2
    
    sort(phase1_output.begin(),phase1_output.end());
    for(auto it:phase1_output){
        cout<<it.second<<endl;
    }

    string broad_cast_msg_2=""; 
    for(int i = 0; i < files.size(); i++){
        string a = files[i];
        a+=" "+to_string(OWN_UNIQUE_ID)+" "+to_string(PORT_LIST_ON);
        broad_cast_msg_2+= a + " 2#";     
    }
    
    LOOP_NUMBER = 2;
    for (auto neighbour : n_sockfd){
            if ((send(neighbour.second.second, "N 9#", 4, 0)) == -1)
            {
                perror("send");
            }
    }
    
    int Number_B_Send_1 = n_sockfd.size();
    int Number_B_Rec_1 = n_sockfd.size();
    int Reply_T = n_sockfd.size();
    
    while (Number_B_Rec_1!=0 || Number_B_Send_1!=0 || Reply_T !=0 || B_3_M !=0)
    {   
        int poll_count = poll(pfds, fd_count, 1000);

        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        for (int i = 0; i < fd_count; i++)
        {
            // Check if someone's ready to read
            if (pfds[i].revents & POLLIN)
            { // We got one!!

                if (pfds[i].fd == listener)
                {
                }
                else
                {
                    // If not the listener, we're just a regular client
                    char recvbuf[numbytes];
                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                    recvbuf[nbytes] = '\0';
                    int sender_fd = pfds[i].fd;
                    if (nbytes <= 0)
                    {
                        // Got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // Connection closed
                            // printf("pollserver: socket %d hung up\n", sender_fd);
                        }
                        else
                        {
                            // perror("recv");
                        }
                        close(pfds[i].fd); // Bye!
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        vector<string> r_msgs = tokenize(recvbuf);
                        vector<string> r_msg[10];
                        for(auto it: r_msgs){
                            int l = it.size(); 
                            int x = it[l-1] - '0';
                            r_msg[x].push_back(it);
                        }

                        for(auto it:r_msg[9]){
                            if ((send(sender_fd, "2 8#", 4, 0)) == -1)
                                {
                                    perror("send");
                                }
                        }
                        

                        for(auto it:r_msg[8]){
                            if(it[0]=='2'){
                                   if(send(sender_fd,(broad_cast_msg_2).c_str(),broad_cast_msg_2.length(),0) == -1){
                                       perror("send");
                                   }
                                   Number_B_Send_1--; 
                            }
                            else {
                                if ((send(sender_fd, "N 9#", 4, 0)) == -1)
                                {
                                    perror("send");
                                }
                            }
                        }


                        string t_msg="";
                        int check_2_send = 0;
                        for (auto it : r_msg[2]){
                            int l = it.size();
                            // Question asked: reply to it
                            it[l - 1] = '\0';
                            int k = 0;
                            string fname="";
                            while(it[k]!=' '){
                               fname+=it[k];k++;     
                            }
                            k++;
                            string UID="";
                            while(it[k]!=' '){
                               UID+=it[k];k++;     
                            }
                            k++;
                            string peer_port="";
                            while(it[k]!=' '){
                               peer_port+=it[k];k++;     
                            }

                            for(auto file:search_files){
                                if(fname==file){
                                    if (files_to_be_found_2[fname].first == 0 || files_to_be_found_2[fname].second == 2 )
                                    {
                                        files_to_be_found_2[fname] = make_pair(stoi(UID),1);
                                    }
                                    else
                                    {
                                        files_to_be_found_2[fname] = make_pair(min(stoi(UID), files_to_be_found_2[fname].first),1);
                                    }
                                }
                            }
                            string msg = fname + " " + UID + " " + peer_port + " 3#";
                            t_msg+=msg;
                            check_2_send = 1;
                        }
                        if(check_2_send == 1){
                            Number_B_Rec_1--;
                            for(auto it2: n_sockfd){
                                if(it2.second.second==sender_fd){
                                    string msg=to_string(n_sockfd.size()-1)+" 4#";
                                    if (send(sender_fd, (msg.c_str()), msg.length(), 0) == -1){
                                        perror("send");
                                    }
                                }
                                else{
                                    if (send(it2.second.second, (t_msg.c_str()), t_msg.length(), 0) == -1){
                                        perror("send");
                                    }
                                }
                            }         
                            check_2_send = 0;            
                        }



                        int reply_check = 0;
                        for(auto it:r_msg[3]){

                            int l = it.size();
                            // Question asked: reply to it
                            it[l - 1] = '\0';
                            int k = 0;
                            string fname="";
                            while(it[k]!=' '){
                               fname+=it[k];k++;     
                            }
                            k++;
                            string UID="";
                            while(it[k]!=' '){
                               UID+=it[k];k++;     
                            }
                            k++;
                            string peer_port="";
                            while(it[k]!=' '){
                               peer_port+=it[k];k++;     
                            }
                            neighbours_2[stoi(UID)]=stoi(peer_port);
                            for(auto file:search_files){
                                if(fname==file){
                                    if (files_to_be_found_2[fname].first == 0)
                                    {
                                        files_to_be_found_2[fname] = make_pair(stoi(UID),2);
                                    }
                                    else if (files_to_be_found_2[fname].second == 2){
                                        files_to_be_found_2[fname] = make_pair(min(stoi(UID), files_to_be_found_2[fname].first),2);
                                    }
                                    else
                                    {
                                    }
                                }
                            } 
                            reply_check = 1;
                                
                        }
                        if(reply_check==1){
                            B_3_M--;                                                                                                                                                                                                                                                                                                                                                                                                                                    
                            reply_check = 0;
                        }

                        for(auto it: r_msg[4]){
                            int k = 0;
                            string n_2="";
                            while(it[k]!=' '){
                               n_2+=it[k];k++;     
                            }
                            Reply_T--;
                            B_3_M+=stoi(n_2);
                        }
                    }
                      // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!
    }


    //For outputting the searched files in sorted order
    sort(search_files.begin(),search_files.end());
    for (auto it : search_files)
    {
        if(files_to_be_found_2[it].second==2){
            cout<<"Found "+it+" at "+to_string(0)+" with MD5 0 at depth "+to_string(0)<<endl;
        }
        else{
            cout<<"Found "+it+" at "+to_string(files_to_be_found_2[it].first)+" with MD5 0 at depth "+to_string(files_to_be_found_2[it].second)<<endl;
        }
    }


   return 0;
}   