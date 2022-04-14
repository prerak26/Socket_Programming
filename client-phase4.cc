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

#define BACKLOG 10
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
    // string filename;
    string msg=to_string(bytesleft)+' ';
    long l=512;
    int a=min(l,bytesleft);
    for(long i=0;i<a;i++){
        msg+=buf[total+i];
    }
    //cout<<msg<<endl;
        n=send(s,msg.c_str(),msg.length(),0);
        if (n == -1) { return n;}
        total += a;
        bytesleft -= a;
    // cout<<n<<" "<<a<<endl;
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
    vector<char *> files;

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

    for (auto file : files)
        cout << file << endl;

    vector<string> search_files;

    int OWN_CLIENT_NO;
    int OWN_UNIQUE_ID;
    int NO_FILES;
    int NO_NEIGHBOUR;

    // Create a text string, which is used to output the text file
    std::string myText;

    // Read from the text file
    ifstream MyReadFile(argv[1]);

    getline(MyReadFile, myText);
    std::string delimiter = " ";
    size_t pos = 0;
    std::string token;
    int i = 0;
    while ((pos = myText.find(delimiter)) != std::string::npos)
    {
        int x = int(i / 2);
        token = myText.substr(0, pos);
        if (i == 0)
        {
            OWN_CLIENT_NO = stoi(token);
        }
        if (i == 1)
        {
            PORT_LIST_ON = stoi(token);
        }
        myText.erase(0, pos + delimiter.length());
        i++;
    }
    OWN_UNIQUE_ID = stoi(myText);

    getline(MyReadFile, myText);
    NO_NEIGHBOUR = stoi(myText);

    getline(MyReadFile, myText);

    vector<pair<int, int>> neighbours_accept;
    vector<pair<int, int>> neighbours_connect;
    pos = 0;
    i = 0;
    int ff, ss;
    while ((pos = myText.find(delimiter)) != std::string::npos)
    {
        token = myText.substr(0, pos);
        if (i % 2 == 0)
        {
            ff = stoi(token);
        }
        else
        {
            ss = stoi(token);
            if (ff > OWN_CLIENT_NO)
            {
                neighbours_connect.push_back(make_pair(ff, ss));
            }
            else
            {
                neighbours_accept.push_back(make_pair(ff, ss));
            }
        }
        myText.erase(0, pos + delimiter.length());
        i++;
    }
    ss = stoi(myText);
    if (ff > OWN_CLIENT_NO)
    {
        neighbours_connect.push_back(make_pair(ff, ss));
    }
    else
    {
        neighbours_accept.push_back(make_pair(ff, ss));
    }

    getline(MyReadFile, myText);
    NO_FILES = stoi(myText);
    for (int i = 0; i < NO_FILES-1; i++)
    {
        getline(MyReadFile, myText);
        myText = myText.substr(0, myText.length() - 1);
        // cout<<myText<<endl;
        // cout<<myText.length()<<endl;
        // for(int j=0;j<myText.length();j++){
        // cout<<myText[j]<<' ';
        // }
        // cout<<endl;
        search_files.push_back(myText);
    }
    getline(MyReadFile, myText);
    search_files.push_back(myText);

    // Close the file
    MyReadFile.close();

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

    mkdir(path.c_str(), 0777);
   
    map<int,int> neighbours_2;
    int phase_4_6r=0,phase_4_ncount=NO_NEIGHBOUR;
    vector<pair<int, pair<int, int>>> n_sockfd;
    set<pair<int, int>> n_connect(neighbours_connect.begin(), neighbours_connect.end());
    set<pair<int, int>> pandu = n_connect;
    int n_accept = neighbours_accept.size();
    int phase2_counter_1=0;
    
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
            // printf("client: connecting to %s\n", theirIP);
            freeaddrinfo(res); // all done with this structure

            if (getsockname(new_fd, (struct sockaddr *)&sin, &len) == -1)
            {
                perror("getsockname");
            }
            else
            {
                x = ntohs(sin.sin_port);
            }
            string msg = "Connected to " + to_string(OWN_CLIENT_NO) + " with unique-ID " + to_string(OWN_UNIQUE_ID) + " on port " + to_string(x) + to_string(1) + "#";
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
                        // int y = ntohs(((struct sockaddr_in *)&their_addr)->sin_port);
                        // cout<<y<<endl;
                        n_accept--;
                        if (getsockname(new_fd, (struct sockaddr *)&sin, &len) == -1)
                        {
                            perror("getsockname");
                        }
                        else
                        {
                            x = ntohs(sin.sin_port);
                        }
                        string msg = "Connected to " + to_string(OWN_CLIENT_NO) + " with unique-ID " + to_string(OWN_UNIQUE_ID) + " on port " + to_string(x) + to_string(1) + "#";
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
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(pfds[i].fd); // Bye!
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        // cout<<recvbuf[nbytes-1]<<recvbuf[nbytes-2]<<endl;
                        vector<string> r_msgs = tokenize(recvbuf);
                        vector<string> r_msg[9];
                        for(auto it: r_msgs){
                            int l = it.size(); 
                            int x = it[l-1] - '0';
                            r_msg[x].push_back(it);
                        }

                        for (auto it : r_msg[1]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                cout << it << endl;
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
                                // cout<<sender_fd<<endl;
                                // cout<<pfds[i].fd<<endl;
                                n_sockfd.push_back(make_pair(stoi(temp_2), make_pair(stoi(temp_1), sender_fd)));
                        }
                        
                        string t_msg="";
                        int check_2_send = 0;
                        for (auto it : r_msg[2]){
                            int l = it.size();
                            // Question asked: reply to it
                                    it[l - 1] = '\0';
                                    //cout << it+"phase2case2" << endl;
                                    string filetoscan = it.substr(0,it.length()-1);
                                    //cout<<filetoscan<<" "<<filetoscan.length()<<endl;
                                    string msg = filetoscan + " " + to_string(0) + " 3#";
                                    for (auto file : files)
                                    {
                                        if (strcmp(file, filetoscan.c_str()) == 0)
                                        {
                                            msg = filetoscan + " " + to_string(OWN_UNIQUE_ID) + " 3#";
                                            continue;
                                        }
                                    }
                                t_msg+=msg;
                                check_2_send = 1;
                        }
                        if(check_2_send == 1){
                           if (send(sender_fd, (t_msg.c_str()), t_msg.length(), 0) == -1)
                                    {
                                        perror("send");
                                    } 
                            check_2_send = 0; 
                            phase2_counter_1++;           
                        }
                        
                        
                    } // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!
    }

    // start of phase 2

    string temp_msg=""; 

    for(int i = 0; i < search_files.size(); i++){
        temp_msg += search_files[i] + "2#";
        //cout<<temp_msg<<endl;      
    }
    for (auto neighbour : n_sockfd){
            if ((send(neighbour.second.second, (temp_msg).c_str(), temp_msg.length(), 0)) == -1)
            {
                perror("send");
            }
    }
    map<string, int> files_to_be_found;
    map<string,pair<int,int>> files_to_be_found_2;
    
    int phase2cnt = (2 * (n_sockfd.size())) - phase2_counter_1;
    int check = 1;
    int ackcntphase3=0;
    vector<string> phase3_ans;
    while ((phase2cnt)!=(-1))
    {   
        if(phase2cnt == 0){
            //For outputting the searched files in sorted order
            // sort(search_files.begin(),search_files.end());
            // for (auto it : search_files)
            // {
            //     if (files_to_be_found[it] == 0)
            //     {
            //         cout << "Found " + it + " at " + to_string(files_to_be_found[it]) + " with MD5 0 at depth 0"<<endl;
            //     }
            //     else
            //     {
            //         cout << "Found " + it + " at " + to_string(files_to_be_found[it]) + " with MD5 0 at depth 1"<<endl;
            //     }
            // }
            phase2cnt = -1;
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
                    cout<<"idhar kese mc"<<endl;
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
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(pfds[i].fd); // Bye!
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        vector<string> r_msgs = tokenize(recvbuf);
                        vector<string> r_msg[9];
                        for(auto it: r_msgs){
                            int l = it.size(); 
                            int x = it[l-1] - '0';
                            r_msg[x].push_back(it);
                        }

                        for (auto it : r_msg[1]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                cout << it << endl;
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
                                n_sockfd.push_back(make_pair(stoi(temp_2), make_pair(stoi(temp_1), sender_fd)));
                        }
                        
                        string t_msg="";
                        int check_2_send = 0;
                        for (auto it : r_msg[2]){
                            int l = it.size();
                            // Question asked: reply to it
                                    it[l - 1] = '\0';
                                    //cout << it+"phase2case2" << endl;
                                    string filetoscan = it.substr(0,it.length()-1);
                                    //cout<<filetoscan<<" "<<filetoscan.length()<<endl;
                                    string msg = filetoscan + " " + to_string(0) + " 3#";
                                    for (auto file : files)
                                    {
                                        if (strcmp(file, filetoscan.c_str()) == 0)
                                        {
                                            msg = filetoscan + " " + to_string(OWN_UNIQUE_ID) + " 3#";
                                            continue;
                                        }
                                    }
                                t_msg+=msg;
                                check_2_send = 1;
                        }
                        if(check_2_send == 1){
                           if (send(sender_fd, (t_msg.c_str()), t_msg.length(), 0) == -1)
                                    {
                                        perror("send");
                                    }
                            phase2cnt--;         
                            check_2_send = 0;            
                        }
                        int reply_check = 0;
                        for(auto it:r_msg[3]){
                                int l = it.size();
                                it[l - 1] = '\0';
                                string temp_1 = ""; // file name
                                int i = 0;
                                while (it[i] != ' ')
                                {
                                    temp_1 = temp_1 + it[i];
                                    i++;
                                }
                                //cout<<it<<endl;
                                //cout<<temp_1<<endl;
                                string temp_2; // samne wale ka unique id
                                int j = 1 + temp_1.size();
                                while (it[j] != ' ')
                                {
                                    temp_2 = temp_2 + it[j];
                                    j++;
                                }
                                //cout<<temp_2<<endl;
                                if (temp_2 != "0")
                                {
                                    if (files_to_be_found[temp_1] == 0)
                                    {
                                        files_to_be_found[temp_1] = stoi(temp_2);
                                    }
                                    else
                                    {
                                        files_to_be_found[temp_1] = min(stoi(temp_2), files_to_be_found[temp_1]);
                                    }
                                }
                                reply_check = 1;
                                
                        }
                        if(reply_check==1){
                                phase2cnt--;
                                reply_check = 0;
                        }
                        for(auto it :r_msg[4]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                it=it.substr(0,it.length()-1);
                                // cout<<it<<endl;
                            
                                char * buffer;     //buffer to store file contents
                                long size;     //file size
                                ifstream file (argv[2]+it, ios::in|ios::binary|ios::ate);     //open file in binary mode, get pointer at the end of the file (ios::ate)
                                size = file.tellg();     //retrieve get pointer position
                                file.seekg (0, ios::beg);     //position get pointer at the begining of the file
                                buffer = new char [size];     //initialize the buffer
                                file.read (buffer, size);     //read file to buffer
                                file.close();     //close file
                                sendall(sender_fd,it, buffer, &size);
                                // cout<<send<<endl;
                        }
                        for( auto it: r_msg[5]){
                            int l = it.size();
                            it[l - 1] = '\0';
                            int k=0;
                            string temp="";
                            while(it[k]!=' '){
                                   temp+=it[k];
                                   k++; 
                            }
                            
                            int peer_id = stoi(temp);

                                for(auto it3: search_files){
                                    if(files_to_be_found[it3] == peer_id ){ 

                                                string msg=it3+"4#";
                                                
                                                if (send(pfds[i].fd, (msg.c_str()), msg.length(), 0) == -1){
                                                    perror("send");
                                                }
                                            // cout<<msg<<endl;
                                                char recvbuf[numbytes];
                                                int done=1;
                                                long file_size;
                                                long rec_size_tn ;
                                                ofstream wf((path+"/"+it3).c_str(), ios::out | ios::binary);
                                                        if(!wf) {
                                                            cout << "Cannot open file!" << endl;
                                                            return 1;
                                                        }

                                                    //cout<<"waiting_1"<<endl;
                                                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                                                    rec_size_tn = nbytes;
                                                    //cout<<"waiting done_1"<<endl;
                                                    recvbuf[nbytes] = '\0';
                                                    //cout<<nbytes;
                                                    //cout<<recvbuf<<endl;
                                                    int sender_fd = pfds[i].fd;
                                                    if (nbytes <= 0)
                                                    {
                                                        // Got error or connection closed by client
                                                        if (nbytes == 0)
                                                        {
                                                            // Connection closed
                                                            printf("pollserver: socket %d hung up\n", sender_fd);
                                                        }
                                                        else
                                                        {
                                                            perror("recv");
                                                        }
                                                        close(sender_fd); // Bye!
                                                  
                                                        del_from_pfds(pfds, i, &fd_count);
                                                    }
                                                    else{
                                                        //cout<< "not entering" <<endl;   
                                                        int k=0;
                                                        string temp="";
                                                        while(recvbuf[k]!=' '){
                                                            temp+=recvbuf[k]; 
                                                            k++ ;
                                                        }
                                                        k++;
                                                        cout<<temp<<" "<<temp.size()<<"check"<<endl;
                                                        // cout<<temp.size()<<endl;
                                                        file_size= stoi(temp);
                                                        rec_size_tn -= k;
                                                        char *ptr=recvbuf+k;
                                                        wf.write((char *) ptr,nbytes-k);       
                                                    }

                                                while((rec_size_tn)<file_size){
                                                    //cout<<"waiting"<<endl;
                                                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                                                    rec_size_tn+=nbytes;
                                                    //cout<<"waiting done"<<endl;
                                                    recvbuf[nbytes] = '\0';
                                                    //cout<<nbytes;
                                                    //cout<<recvbuf<<endl;
                                                    int sender_fd =pfds[i].fd;
                                                    if (nbytes <= 0)
                                                    {
                                                        // Got error or connection closed by client
                                                        if (nbytes == 0)
                                                        {
                                                            // Connection closed
                                                            printf("pollserver: socket %d hung up\n", sender_fd);
                                                        }
                                                        else
                                                        {
                                                            perror("recv");
                                                        }
                                                        close(sender_fd); // Bye!

                                                        del_from_pfds(pfds, i, &fd_count);
                                                    }
                                                    else{
                                                        wf.write((char *) recvbuf,nbytes);
                                                    }
                                                }  
                                                wf.close();
                                                unsigned char result[MD5_DIGEST_LENGTH];
                                                string orig=argv[2];
                                                orig+="Downloaded/";
                                                FILE *inFile = fopen ((orig+it3).c_str(), "rb");
                                                MD5_CTX mdContext;
                                                int bytes;
                                                unsigned char data[1024];
                                                if (inFile == NULL) {
                                                    printf ("%s can't be opened.\n", it3.c_str());
                                                    return 0;
                                                }
                                                MD5_Init (&mdContext);
                                                while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                                                    MD5_Update (&mdContext, data, bytes);
                                                MD5_Final (result,&mdContext);
                                                string ans_here="Found "+it3+" at "+to_string(files_to_be_found[it3])+" with MD5 ";
                                                for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
                                                    std::string thisone( int_to_hex((int)result[i]) );
                                                    ans_here+=thisone;
                                                } 
                                                ans_here+=" at depth 1";
                                                // cout<<endl;
                                                phase3_ans.push_back(ans_here);
                                                ackcntphase3++; 
                                                
                                    }    
                                }
                                
                        }  
                        for(auto it: r_msg[6]){
                            int l = it.size();
                            // it[l - 1] = '\0';
                            if(it[0]=='S'){
                                string r_msg_1=it;//the recvmsg
                                //break msg here
                                int ptr=2;
                                string temp="";//for storing the client ID from which it came
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                //send here
                                cout<<temp<<" "<<temp.size()<<endl;
                                for(auto it2: n_sockfd){
                                    assert(temp.size()>0);
                                    if(it2.first==stoi(temp)){
                                        string msg="T "+to_string(n_sockfd.size()-1)+" 6#";
                                        if (send(it2.second.second, (msg.c_str()), msg.length(), 0) == -1){
                                            perror("send");
                                        }
                                    }
                                    else{
                                        r_msg_1[r_msg_1.size()-1]='7';
                                        // string msg=r_msg;
                                        r_msg_1+="#";
                                        cout<<r_msg<<endl;
                                        if (send(it2.second.second, (r_msg_1.c_str()), r_msg_1.length(), 0) == -1){
                                            perror("send");
                                        }
                                    }
                                }
                            }
                            else if(it[0]=='T'){
                                // phase_4_ncount--;
                                // int ptr=2;
                                // string temp="";//for storing the no. of R msgs we will receive from this neighbour
                                // while(it[ptr]!=' '){
                                //     temp+=it[ptr];
                                // }
                                // phase_4_6r+=stoi(temp);
                                cout<<"mc"<<endl;
                            }
                            else if(it[0]=='R'){
                                cout<<"Not here"<<endl;
                            }else{
                                cout<<"wrong"<<endl;
                            }
                        }
                        for(auto it: r_msg[7]){
                            int l = it.size();
                            if(it[0]=='S'){
                                int ptr=2;
                                int client_no=0;
                                
                                string temp="";//UNIQUE ID of the ask person
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                string temp2="";//name of the file to be searched
                                while(it[ptr]!=' '){
                                    temp2+=it[ptr];ptr++;
                                }
                                // while(it[ptr]==' '){
                                //     ptr++;
                                // }
                                for(auto file:files){
                                    if(file==temp){
                                        client_no=OWN_UNIQUE_ID;
                                        break;
                                    }
                                }

                                string msg="R "+temp+ " "+temp2+" "+to_string(client_no)+" "+to_string(PORT_LIST_ON)+" 7#";
                                if (send(sender_fd, (msg.c_str()), msg.length(), 0) == -1){
                                    perror("send");
                                }
                            }
                            else if(it[0]=='R'){
                                string r_msg_1=it;//the recvmsg
                                //break msg here
                                int ptr=2;
                                string temp="";//for storing the client ID from which it came
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                r_msg_1[r_msg_1.size()-1]='6';
                                // string msg=r_msg;
                                r_msg_1+="#";
                                //send here

                                for(auto it2: n_sockfd){
                                    if(it2.first==stoi(temp)){
                                        if (send(it2.second.second, (r_msg_1.c_str()), r_msg_1.length(), 0) == -1){
                                            perror("send");
                                        }
                                        break;
                                    }
                                }
                                
                            }
                            else{
                                cout<<"Wrong :7#"<<endl;
                            }
                        }  


                        }
                         // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!
    }

    //Phase 2 Completed here

    vector<string> search_files_2;
    for(auto it: search_files){
        if(files_to_be_found[it]==0){
            search_files_2.push_back(it);
        }
    }
    //Start Phase 3 here
    cout<<"enter_1"<<endl;
    //Ask the greater neighbours for the files
    int phase3_check=0;
   
    for(auto it: search_files){
        if(files_to_be_found[it]!=0){ 
            phase3_check++;
            for(auto it2:n_sockfd){
                if((it2.first==files_to_be_found[it]) && (it2.second.first > OWN_CLIENT_NO)){

                    string msg=it+"4#";
                    
                    if (send(it2.second.second, (msg.c_str()), msg.length(), 0) == -1){
                        perror("send");
                    }
                   // cout<<msg<<endl;
                    char recvbuf[numbytes];
                    int done=1;
                    long file_size;
                    long rec_size_tn ;
                    ofstream wf((path+"/"+it).c_str(), ios::out | ios::binary);
                            if(!wf) {
                                cout << "Cannot open file!" << endl;
                                return 1;
                            }

                        //cout<<"waiting_1"<<endl;
                        int nbytes = recv(it2.second.second, recvbuf, sizeof recvbuf, 0);
                        rec_size_tn = nbytes;
                        //cout<<"waiting done_1"<<endl;
                        recvbuf[nbytes] = '\0';
                        //cout<<nbytes;
                        //cout<<recvbuf<<endl;
                        int sender_fd = it2.second.second;
                        if (nbytes <= 0)
                        {
                            // Got error or connection closed by client
                            if (nbytes == 0)
                            {
                                // Connection closed
                                printf("pollserver: socket %d hung up\n", sender_fd);
                            }
                            else
                            {
                                perror("recv");
                            }
                            close(it2.second.second); // Bye!





                        /*
                            Delete pending findm in in pfds and delete;
                        */
                            //del_from_pfds(pfds, i, &fd_count);




                        }
                        else{
                            //cout<< "not entering" <<endl;   
                            int k=0;
                            string temp="";
                            while(recvbuf[k]!=' '){
                                  temp+=recvbuf[k]; 
                                  k++ ;
                            }
                            k++;
                            // cout<<temp<<" "<<temp.size()<<"check"<<endl;
                            file_size= stoi(temp);
                            rec_size_tn -= k;
                            char *ptr=recvbuf+k;
                            wf.write((char *) ptr,nbytes-k);       
                        }

                    while((rec_size_tn)<file_size){
                        //cout<<"waiting"<<endl;
                        int nbytes = recv(it2.second.second, recvbuf, sizeof recvbuf, 0);
                        rec_size_tn+=nbytes;
                        //cout<<"waiting done"<<endl;
                        recvbuf[nbytes] = '\0';
                        //cout<<nbytes;
                        //cout<<recvbuf<<endl;
                        int sender_fd = it2.second.second;
                        if (nbytes <= 0)
                        {
                            // Got error or connection closed by client
                            if (nbytes == 0)
                            {
                                // Connection closed
                                printf("pollserver: socket %d hung up\n", sender_fd);
                            }
                            else
                            {
                                perror("recv");
                            }
                            close(it2.second.second); // Bye!





                        /*
                            Delete pending findm in in pfds and delete;
                        */
                            //del_from_pfds(pfds, i, &fd_count);




                        }
                        else{
                            wf.write((char *) recvbuf,nbytes);
                        }
                    }  
                    wf.close();  
                    string orig=argv[2];
                    orig+="Downloaded/";
                    // file_size = get_size_by_fd(file_descript);
                    // printf("file size:\t%lu\n", file_size);
                    // cout<<it<<" ";
                    unsigned char result[MD5_DIGEST_LENGTH];
                    FILE *inFile = fopen ((orig+it).c_str(), "rb");
                    MD5_CTX mdContext;
                    int bytes;
                    unsigned char data[1024];
                    if (inFile == NULL) {
                        printf ("%s can't be opened.\n", it.c_str());
                        return 0;
                    }
                    MD5_Init (&mdContext);
                    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                        MD5_Update (&mdContext, data, bytes);
                    MD5_Final (result,&mdContext);
                    string ans_here="Found "+it+" at "+to_string(files_to_be_found[it])+" with MD5 ";
                    for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
                        std::string thisone( int_to_hex((int)result[i]) );
                        ans_here+=thisone;
                    }   
                    ans_here+=" at depth 1";
                    phase3_ans.push_back(ans_here);
                    // cout<<endl;
                    ackcntphase3++;
                    break;
                }
            }
        }
        // else{
        //     string thisans="Found "+it+" at 0 with MD5 0 at depth 0";
        //     phase3_ans.push_back(thisans);
        // }    
    }

    cout<<"enter_2"<<endl;

    for(auto it:n_sockfd){
            if(it.second.first>OWN_CLIENT_NO){
                string msg=to_string(OWN_UNIQUE_ID)+" 5#";
                if (send(it.second.second, (msg.c_str()), msg.length(), 0) == -1){
                        perror("send");
                    }
            }
    }
    

    while(ackcntphase3<phase3_check){

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

                if (pfds[i].fd == listener){}
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
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(pfds[i].fd); // Bye!
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        // cout<<recvbuf[nbytes-1]<<recvbuf[nbytes-2]<<endl;
                        vector<string> r_msgs = tokenize(recvbuf);
                        vector<string> r_msg[9];
                        for(auto it: r_msgs){
                            int l = it.size(); 
                            int x = it[l-1] - '0';
                            r_msg[x].push_back(it);
                        }

                        for (auto it : r_msg[1]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                cout << it << endl;
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
                                // cout<<sender_fd<<endl;
                                // cout<<pfds[i].fd<<endl;
                                n_sockfd.push_back(make_pair(stoi(temp_2), make_pair(stoi(temp_1), sender_fd)));
                        }
                        
                        string t_msg="";
                        int check_2_send = 0;
                        for (auto it : r_msg[2]){
                            int l = it.size();
                            // Question asked: reply to it
                                    it[l - 1] = '\0';
                                    //cout << it+"phase2case2" << endl;
                                    string filetoscan = it.substr(0,it.length()-1);
                                    //cout<<filetoscan<<" "<<filetoscan.length()<<endl;
                                    string msg = filetoscan + " " + to_string(0) + " 3#";
                                    for (auto file : files)
                                    {
                                        if (strcmp(file, filetoscan.c_str()) == 0)
                                        {
                                            msg = filetoscan + " " + to_string(OWN_UNIQUE_ID) + " 3#";
                                            continue;
                                        }
                                    }
                                t_msg+=msg;
                                check_2_send = 1;
                        }
                        if(check_2_send == 1){
                           if (send(sender_fd, (t_msg.c_str()), t_msg.length(), 0) == -1)
                                    {
                                        perror("send");
                                    } 
                            check_2_send = 0; 
                            phase2_counter_1++;           
                        }
                        int reply_check = 0;
                        for(auto it:r_msg[3]){
                               int l = it.size();
                               it[l - 1] = '\0';
                                string temp_1 = ""; // file name
                                int i = 0;
                                while (it[i] != ' ')
                                {
                                    temp_1 = temp_1 + it[i];
                                    i++;
                                }
                                //cout<<it<<endl;
                                //cout<<temp_1<<endl;
                                string temp_2; // samne wale ka unique id
                                int j = 1 + temp_1.size();
                                while (it[j] != ' ')
                                {
                                    temp_2 = temp_2 + it[j];
                                    j++;
                                }
                                //cout<<temp_2<<endl;
                                if (temp_2 != "0")
                                {
                                    if (files_to_be_found[temp_1] == 0)
                                    {
                                        files_to_be_found[temp_1] = stoi(temp_2);
                                    }
                                    else
                                    {
                                        files_to_be_found[temp_1] = min(stoi(temp_2), files_to_be_found[temp_1]);
                                    }
                                }
                                reply_check = 1;
                                
                        }
                        if(reply_check==1){
                                phase2cnt--;
                                reply_check = 0;
                        }
                        for(auto it :r_msg[4]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                it=it.substr(0,it.length()-1);
                                // cout<<it<<endl;
                            
                                char * buffer;     //buffer to store file contents
                                long size;     //file size
                                ifstream file (argv[2]+it, ios::in|ios::binary|ios::ate);     //open file in binary mode, get pointer at the end of the file (ios::ate)
                                size = file.tellg();     //retrieve get pointer position
                                file.seekg (0, ios::beg);     //position get pointer at the begining of the file
                                buffer = new char [size];     //initialize the buffer
                                file.read (buffer, size);     //read file to buffer
                                file.close();     //close file
                                sendall(sender_fd,it, buffer, &size);
                                // cout<<"senddone"<<endl;
                        }
                        for(auto it: r_msg[5]){
                            int l = it.size();
                            it[l - 1] = '\0';
                            int k=0;
                            string temp="";
                            while(it[k]!=' '){
                                   temp+=it[k];
                                   k++; 
                            }
                            int peer_id = stoi(temp);

                                for(auto it3: search_files){
                                    if(files_to_be_found[it3] == peer_id ){ 

                                                string msg=it3+"4#";
                                                
                                                if (send(pfds[i].fd, (msg.c_str()), msg.length(), 0) == -1){
                                                    perror("send");
                                                }
                                            // cout<<msg<<endl;
                                                char recvbuf[numbytes];
                                                int done=1;
                                                long file_size;
                                                long rec_size_tn ;
                                                ofstream wf((path+"/"+it3).c_str(), ios::out | ios::binary);
                                                        if(!wf) {
                                                            cout << "Cannot open file!" << endl;
                                                            return 1;
                                                        }

                                                    //cout<<"waiting_1"<<endl;
                                                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                                                    rec_size_tn = nbytes;
                                                    //cout<<"waiting done_1"<<endl;
                                                    recvbuf[nbytes] = '\0';
                                                    //cout<<nbytes;
                                                    //cout<<recvbuf<<endl;
                                                    int sender_fd = pfds[i].fd;
                                                    if (nbytes <= 0)
                                                    {
                                                        // Got error or connection closed by client
                                                        if (nbytes == 0)
                                                        {
                                                            // Connection closed
                                                            printf("pollserver: socket %d hung up\n", sender_fd);
                                                        }
                                                        else
                                                        {
                                                            perror("recv");
                                                        }
                                                        close(sender_fd); // Bye!
                                                  
                                                        del_from_pfds(pfds, i, &fd_count);
                                                    }
                                                    else{
                                                        //cout<< "not entering" <<endl;   
                                                        int k=0;
                                                        string temp="";
                                                        while(recvbuf[k]!=' '){
                                                            temp+=recvbuf[k]; 
                                                            k++ ;
                                                        }
                                                        k++;
                                                        // cout<<temp<<" "<<temp.size()<<"check"<<endl;
                                                        file_size= stoi(temp);
                                                        rec_size_tn -= k;
                                                        char *ptr=recvbuf+k;
                                                        wf.write((char *) ptr,nbytes-k);       
                                                    }

                                                while((rec_size_tn)<file_size){
                                                    //cout<<"waiting"<<endl;
                                                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                                                    rec_size_tn+=nbytes;
                                                    //cout<<"waiting done"<<endl;
                                                    recvbuf[nbytes] = '\0';
                                                    //cout<<nbytes;
                                                    //cout<<recvbuf<<endl;
                                                    int sender_fd =pfds[i].fd;
                                                    if (nbytes <= 0)
                                                    {
                                                        // Got error or connection closed by client
                                                        if (nbytes == 0)
                                                        {
                                                            // Connection closed
                                                            printf("pollserver: socket %d hung up\n", sender_fd);
                                                        }
                                                        else
                                                        {
                                                            perror("recv");
                                                        }
                                                        close(sender_fd); // Bye!

                                                        del_from_pfds(pfds, i, &fd_count);
                                                    }
                                                    else{
                                                        wf.write((char *) recvbuf,nbytes);
                                                    }
                                                }  
                                                wf.close(); 
                                                // cout<<it3<<endl;
                                                unsigned char result[MD5_DIGEST_LENGTH];
                                                string orig=argv[2];
                                                orig+="Downloaded/";
                                                FILE *inFile = fopen ((orig+it3).c_str(), "rb");
                                                MD5_CTX mdContext;
                                                int bytes;
                                                unsigned char data[1024];
                                                if (inFile == NULL) {
                                                    printf ("%s can't be opened.\n", it3.c_str());
                                                    return 0;
                                                }
                                                MD5_Init (&mdContext);
                                                while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                                                    MD5_Update (&mdContext, data, bytes);
                                                MD5_Final (result,&mdContext);
                                                string ans_here="Found "+it3+" at "+to_string(files_to_be_found[it3])+" with MD5 ";
                                                for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
                                                    std::string thisone( int_to_hex((int)result[i]) );
                                                    ans_here+=thisone;
                                                }  
                                                ans_here+=" at depth 1";
                                                phase3_ans.push_back(ans_here);
                                                // cout<<endl;
                                                ackcntphase3++;         
                                    }    
                                }
                                 
                        }  
                        for(auto it: r_msg[6]){
                            int l = it.size();
                            // it[l - 1] = '\0';
                            if(it[0]=='S'){
                                string r_msg_1=it;//the recvmsg
                                //break msg here
                                int ptr=2;
                                string temp="";//for storing the client ID from which it came
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                //send here
                                for(auto it2: n_sockfd){
                                    if(it2.first==stoi(temp)){
                                        string msg="T "+to_string(n_sockfd.size()-1)+" 6#";
                                        if (send(it2.second.second, (msg.c_str()), msg.length(), 0) == -1){
                                            perror("send");
                                        }
                                    }
                                    else{
                                        r_msg_1[r_msg_1.size()-1]='7';
                                        // string msg=r_msg;
                                        r_msg_1+="#";
                                        cout<<r_msg<<endl;
                                        if (send(it2.second.second, (r_msg_1.c_str()), r_msg_1.length(), 0) == -1){
                                            perror("send");
                                        }
                                    }
                                }
                            }
                            else if(it[0]=='T'){
                                // phase_4_ncount--;
                                // int ptr=2;
                                // string temp="";//for storing the no. of R msgs we will receive from this neighbour
                                // while(it[ptr]!=' '){
                                //     temp+=it[ptr];
                                // }
                                // phase_4_6r+=stoi(temp);
                                cout<<"galat"<<endl;
                            }
                            else if(it[0]=='R'){
                                cout<<"Bilkul nahi"<<endl;
                            }else{
                                cout<<"wrong"<<endl;
                            }
                        }
                        for(auto it: r_msg[7]){
                            int l = it.size();
                            if(it[0]=='S'){
                                int ptr=2;
                                int client_no=0;
                                
                                string temp="";//UNIQUE ID of the ask person
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                string temp2="";//name of the file to be searched
                                while(it[ptr]!=' '){
                                    temp2+=it[ptr];ptr++;
                                }
                                // while(it[ptr]==' '){
                                //     ptr++;
                                // }
                                for(auto file:files){
                                    if(file==temp){
                                        client_no=OWN_UNIQUE_ID;
                                        break;
                                    }
                                }

                                string msg="R "+temp+ " "+temp2+" "+to_string(client_no)+" "+to_string(PORT_LIST_ON)+" 7#";
                                if (send(sender_fd, (msg.c_str()), msg.length(), 0) == -1){
                                    perror("send");
                                }
                            }
                            else if(it[0]=='R'){
                                string r_msg_1=it;//the recvmsg
                                //break msg here
                                int ptr=2;
                                string temp="";//for storing the client ID from which it came
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                r_msg_1[r_msg_1.size()-1]='6';
                                // string msg=r_msg;
                                r_msg_1+="#";
                                //send here

                                for(auto it2: n_sockfd){
                                    if(it2.first==stoi(temp)){
                                        if (send(it2.second.second, (r_msg_1.c_str()), r_msg_1.length(), 0) == -1){
                                            perror("send");
                                        }
                                        break;
                                    }
                                }
                            }
                            else{
                                cout<<"Wrong :7#"<<endl;
                            }
                        }
                    } // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!

    }
    for(auto it:search_files){
        if(files_to_be_found[it]==0){
            cout<<"Found "+it+" at 0 with MD5 0 at depth 0"<<endl;
        }
    }
    for(auto it:phase3_ans){
        cout<<it<<endl;
    }

    //phase4 started for me
    // string msg="";
    for(auto it: search_files){
        if(files_to_be_found[it]==0){
            files_to_be_found_2[it]=make_pair(files_to_be_found[it],0);
        }
        else{
            files_to_be_found_2[it]=make_pair(files_to_be_found[it],1);
        }
    }
    phase_4_ncount*=search_files_2.size();
    for(auto it:search_files_2){
        string temp="S "+to_string(OWN_UNIQUE_ID)+" "+it+" 6#";
        // msg+=temp;
        for (auto neighbour : n_sockfd){
                if ((send(neighbour.second.second, (temp).c_str(), temp.length(), 0)) == -1)
                {
                    perror("send");
                }
        }
    }

    
    while(1){
        if(!(phase_4_ncount!=0||phase_4_6r!=0)){
            for(auto it:search_files){
                cout<<"Found "+it+" at "+to_string(files_to_be_found_2[it].first)+" with MD5 0 at depth "+to_string(files_to_be_found_2[it].second)<<endl; 
            }
            phase_4_ncount=-1;
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

                if (pfds[i].fd == listener){}
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
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(pfds[i].fd); // Bye!
                        del_from_pfds(pfds, i, &fd_count);
                    }
                    else
                    {
                        // cout<<recvbuf[nbytes-1]<<recvbuf[nbytes-2]<<endl;
                        vector<string> r_msgs = tokenize(recvbuf);
                        vector<string> r_msg[9];
                        for(auto it: r_msgs){
                            int l = it.size(); 
                            int x = it[l-1] - '0';
                            r_msg[x].push_back(it);
                        }

                        for (auto it : r_msg[1]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                cout << it << endl;
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
                                // cout<<sender_fd<<endl;
                                // cout<<pfds[i].fd<<endl;
                                n_sockfd.push_back(make_pair(stoi(temp_2), make_pair(stoi(temp_1), sender_fd)));
                        }
                        
                        string t_msg="";
                        int check_2_send = 0;
                        for (auto it : r_msg[2]){
                            int l = it.size();
                            // Question asked: reply to it
                                    it[l - 1] = '\0';
                                    //cout << it+"phase2case2" << endl;
                                    string filetoscan = it.substr(0,it.length()-1);
                                    //cout<<filetoscan<<" "<<filetoscan.length()<<endl;
                                    string msg = filetoscan + " " + to_string(0) + " 3#";
                                    for (auto file : files)
                                    {
                                        if (strcmp(file, filetoscan.c_str()) == 0)
                                        {
                                            msg = filetoscan + " " + to_string(OWN_UNIQUE_ID) + " 3#";
                                            continue;
                                        }
                                    }
                                t_msg+=msg;
                                check_2_send = 1;
                        }
                        if(check_2_send == 1){
                           if (send(sender_fd, (t_msg.c_str()), t_msg.length(), 0) == -1)
                                    {
                                        perror("send");
                                    } 
                            check_2_send = 0; 
                            phase2_counter_1++;           
                        }
                        int reply_check = 0;
                        for(auto it:r_msg[3]){
                               int l = it.size();
                               it[l - 1] = '\0';
                                string temp_1 = ""; // file name
                                int i = 0;
                                while (it[i] != ' ')
                                {
                                    temp_1 = temp_1 + it[i];
                                    i++;
                                }
                                //cout<<it<<endl;
                                //cout<<temp_1<<endl;
                                string temp_2; // samne wale ka unique id
                                int j = 1 + temp_1.size();
                                while (it[j] != ' ')
                                {
                                    temp_2 = temp_2 + it[j];
                                    j++;
                                }
                                //cout<<temp_2<<endl;
                                if (temp_2 != "0")
                                {
                                    if (files_to_be_found[temp_1] == 0)
                                    {
                                        files_to_be_found[temp_1] = stoi(temp_2);
                                    }
                                    else
                                    {
                                        files_to_be_found[temp_1] = min(stoi(temp_2), files_to_be_found[temp_1]);
                                    }
                                }
                                reply_check = 1;
                                
                        }
                        if(reply_check==1){
                                phase2cnt--;
                                reply_check = 0;
                        }
                        for(auto it :r_msg[4]){
                            int l = it.size();
                            it[l - 1] = '\0';
                                it=it.substr(0,it.length()-1);
                                // cout<<it<<endl;
                            
                                char * buffer;     //buffer to store file contents
                                long size;     //file size
                                ifstream file (argv[2]+it, ios::in|ios::binary|ios::ate);     //open file in binary mode, get pointer at the end of the file (ios::ate)
                                size = file.tellg();     //retrieve get pointer position
                                file.seekg (0, ios::beg);     //position get pointer at the begining of the file
                                buffer = new char [size];     //initialize the buffer
                                file.read (buffer, size);     //read file to buffer
                                file.close();     //close file
                                sendall(sender_fd,it, buffer, &size);
                                // cout<<"senddone"<<endl;
                        }
                        for(auto it: r_msg[5]){
                            int l = it.size();
                            it[l - 1] = '\0';
                            int k=0;
                            string temp="";
                            while(it[k]!=' '){
                                   temp+=it[k];
                                   k++; 
                            }
                            int peer_id = stoi(temp);

                                for(auto it3: search_files){
                                    if(files_to_be_found[it3] == peer_id ){ 

                                                string msg=it3+"4#";
                                                
                                                if (send(pfds[i].fd, (msg.c_str()), msg.length(), 0) == -1){
                                                    perror("send");
                                                }
                                            // cout<<msg<<endl;
                                                char recvbuf[numbytes];
                                                int done=1;
                                                long file_size;
                                                long rec_size_tn ;
                                                ofstream wf((path+"/"+it3).c_str(), ios::out | ios::binary);
                                                        if(!wf) {
                                                            cout << "Cannot open file!" << endl;
                                                            return 1;
                                                        }

                                                    //cout<<"waiting_1"<<endl;
                                                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                                                    rec_size_tn = nbytes;
                                                    //cout<<"waiting done_1"<<endl;
                                                    recvbuf[nbytes] = '\0';
                                                    //cout<<nbytes;
                                                    //cout<<recvbuf<<endl;
                                                    int sender_fd = pfds[i].fd;
                                                    if (nbytes <= 0)
                                                    {
                                                        // Got error or connection closed by client
                                                        if (nbytes == 0)
                                                        {
                                                            // Connection closed
                                                            printf("pollserver: socket %d hung up\n", sender_fd);
                                                        }
                                                        else
                                                        {
                                                            perror("recv");
                                                        }
                                                        close(sender_fd); // Bye!
                                                  
                                                        del_from_pfds(pfds, i, &fd_count);
                                                    }
                                                    else{
                                                        //cout<< "not entering" <<endl;   
                                                        int k=0;
                                                        string temp="";
                                                        while(recvbuf[k]!=' '){
                                                            temp+=recvbuf[k]; 
                                                            k++ ;
                                                        }
                                                        k++;
                                                        // cout<<temp<<" "<<temp.size()<<"check"<<endl;
                                                        file_size= stoi(temp);
                                                        rec_size_tn -= k;
                                                        char *ptr=recvbuf+k;
                                                        wf.write((char *) ptr,nbytes-k);       
                                                    }

                                                while((rec_size_tn)<file_size){
                                                    //cout<<"waiting"<<endl;
                                                    int nbytes = recv(pfds[i].fd, recvbuf, sizeof recvbuf, 0);
                                                    rec_size_tn+=nbytes;
                                                    //cout<<"waiting done"<<endl;
                                                    recvbuf[nbytes] = '\0';
                                                    //cout<<nbytes;
                                                    //cout<<recvbuf<<endl;
                                                    int sender_fd =pfds[i].fd;
                                                    if (nbytes <= 0)
                                                    {
                                                        // Got error or connection closed by client
                                                        if (nbytes == 0)
                                                        {
                                                            // Connection closed
                                                            printf("pollserver: socket %d hung up\n", sender_fd);
                                                        }
                                                        else
                                                        {
                                                            perror("recv");
                                                        }
                                                        close(sender_fd); // Bye!

                                                        del_from_pfds(pfds, i, &fd_count);
                                                    }
                                                    else{
                                                        wf.write((char *) recvbuf,nbytes);
                                                    }
                                                }  
                                                wf.close(); 
                                                // cout<<it3<<endl;
                                                unsigned char result[MD5_DIGEST_LENGTH];
                                                string orig=argv[2];
                                                orig+="Downloaded/";
                                                FILE *inFile = fopen ((orig+it3).c_str(), "rb");
                                                MD5_CTX mdContext;
                                                int bytes;
                                                unsigned char data[1024];
                                                if (inFile == NULL) {
                                                    printf ("%s can't be opened.\n", it3.c_str());
                                                    return 0;
                                                }
                                                MD5_Init (&mdContext);
                                                while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                                                    MD5_Update (&mdContext, data, bytes);
                                                MD5_Final (result,&mdContext);
                                                string ans_here="Found "+it3+" at "+to_string(files_to_be_found[it3])+" with MD5 ";
                                                for(int i=0; i <MD5_DIGEST_LENGTH; i++) {
                                                    std::string thisone( int_to_hex((int)result[i]) );
                                                    ans_here+=thisone;
                                                }    
                                                ans_here+=" at depth 1";
                                                // cout<<ans_here<<endl;
                                                phase3_ans.push_back(ans_here);
                                                // cout<<endl;
                                                ackcntphase3++;         
                                    }    
                                }
                                 
                        } 
                        for(auto it: r_msg[6]){
                            int l = it.size();
                            // it[l - 1] = '\0';
                            if(it[0]=='S'){
                                string r_msg_1=it;//the recvmsg
                                //break msg here
                                int ptr=2;
                                string temp="";//for storing the client ID from which it came
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                //send here
                                for(auto it2: n_sockfd){
                                    if(it2.first==stoi(temp)){
                                        string msg="T "+to_string(n_sockfd.size()-1)+" 6#";
                                        if (send(it2.second.second, (msg.c_str()), msg.length(), 0) == -1){
                                            perror("send");
                                        }
                                    }
                                    else{
                                        r_msg_1[r_msg_1.size()-1]='7';
                                        // string msg=r_msg;
                                        r_msg_1+="#";
                                        cout<<r_msg<<endl;
                                        if (send(it2.second.second, (r_msg_1.c_str()), r_msg_1.length(), 0) == -1){
                                            perror("send");
                                        }
                                    }
                                }
                            }
                            else if(it[0]=='T'){
                                phase_4_ncount--;
                                int ptr=2;
                                string temp="";//for storing the no. of R msgs we will receive from this neighbour
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                phase_4_6r+=stoi(temp);
                            }
                            else if(it[0]=='R'){
                                int ptr=2;
                                string temp1="";//for storing our ID,
                                while(it[ptr]!=' '){
                                    temp1+=it[ptr];
                                    ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                string temp2="";//for storing the filename for which info came
                                while(it[ptr]!=' '){
                                    temp2+=it[ptr];
                                    ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                string temp3="";//for storing the UNIQUE_NO from which file can be obtained
                                while(it[ptr]!=' '){
                                    temp3+=it[ptr];
                                    ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                string temp4="";//for storing the portno of the client
                                while(it[ptr]!=' '){
                                    temp4+=it[ptr];
                                    ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                //stored everything from msg till here
                                phase_4_6r--;
                                
                                if (temp3 != "0")
                                {
                                    neighbours_2[stoi(temp3)]=stoi(temp4);
                                    if (files_to_be_found_2[temp2].first == 0)
                                    {
                                        files_to_be_found_2[temp2] = make_pair(stoi(temp3),2);
                                    }
                                    else
                                    {
                                        files_to_be_found_2[temp2] = make_pair(min(stoi(temp3), files_to_be_found_2[temp2].first),2);
                                    }
                                }
                            }else{
                                cout<<"wrong"<<endl;
                            }
                        }
                        for(auto it: r_msg[7]){
                            int l = it.size();
                            if(it[0]=='S'){
                                int ptr=2;
                                int client_no=0;
                                
                                string temp="";//UNIQUE ID of the ask person
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];ptr++;
                                }
                                while(it[ptr]==' '){
                                    ptr++;
                                }
                                string temp2="";//name of the file to be searched
                                while(it[ptr]!=' '){
                                    temp2+=it[ptr];ptr++;
                                }
                                // while(it[ptr]==' '){
                                //     ptr++;
                                // }
                                for(auto file:files){
                                    if(file==temp){
                                        client_no=OWN_UNIQUE_ID;
                                        break;
                                    }
                                }

                                string msg="R "+temp+ " "+temp2+" "+to_string(client_no)+" "+to_string(PORT_LIST_ON)+" 7#";
                                if (send(sender_fd, (msg.c_str()), msg.length(), 0) == -1){
                                    perror("send");
                                }
                            }
                            else if(it[0]=='R'){
                                string r_msg_1=it;//the recvmsg
                                //break msg here
                                int ptr=2;
                                string temp="";//for storing the client ID from which it came
                                while(it[ptr]!=' '){
                                    temp+=it[ptr];
                                }
                                r_msg_1[r_msg_1.size()-1]='6';
                                // string msg=r_msg;
                                r_msg_1+="#";
                                //send here

                                for(auto it2: n_sockfd){
                                    if(it2.first==stoi(temp)){
                                        if (send(it2.second.second, (r_msg_1.c_str()), r_msg_1.length(), 0) == -1){
                                            perror("send");
                                        }
                                        break;
                                    }
                                }
                            }
                            else{
                                cout<<"Wrong :7#"<<endl;
                            }
                        } 
                    } // END handle data from client
                }     // END got ready-to-read from poll()
            }         // END looping through file descriptors
        }             // END for(;;)--and you thought it would never end!

    }
    return 0;
}