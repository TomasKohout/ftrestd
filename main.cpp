#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <sys/socket.h>
#include<stdio.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>

#define OK 0
#define PUT 1
#define DEL 2
#define GET 3
#define LST 4
#define MKD 5
#define RMD 6
using namespace std;
string arr[] = { "PUT", "DEL", "GET", "LST", "MKD", "RMD"};
vector<string> commands (arr, arr + 6) ;
string arr2[] = {"-p", "-r"};
vector<string> args (arr2, arr2+2);

string ok = "HTTP/1.1 200 OK\n";
string notF = "HTTP/1.1 404 Not Found\n";
string badR = "HTTP/1.1 400 Bad Request\n";

void errMsg(int err,const char *msg)
{
    perror(msg);
}

struct arg{
    string rootDir;
    uint16_t portNum;
};

struct arg* getParams (int argc, char *argv[])
{
    //if (argc != 5 && argc != 3)
      //  errMsg(42, "ERROR: spatne argumetny");
    char temp[PATH_MAX];
    arg *str = new arg;
    str->portNum = 6677;
    str->rootDir = getcwd(temp, PATH_MAX);
    if (argc == 3)
    {
        if ( args[0].compare(argv[1]) == 0 && isdigit(*argv[2]))
        {
            char *t = NULL;
            str->portNum =   (uint16_t) strtol(argv[2], &t, 10);

            if (*t != '\0')
                errMsg(42, "ERROR port neni numericky");
        }
    }
    else if (argc == 5)
    {
        if ( args[0].compare(argv[1]) == 0 && args[1].compare(argv[3]) == 0)
        {
            if ( args[0].compare(argv[1]) == 0 && isdigit(*argv[2]))
            {
                char *t = NULL;
                str->portNum =   (uint16_t) strtol(argv[2], &t, 10);

                if (*t != '\0')
                    errMsg(42, "ERROR port neni numericky");

            } else errMsg(42, "ERROR port neni numericky");
            str->rootDir = "";
            str->rootDir = argv[4];
        }
        else if (args[0].compare(argv[3]) == 0 && args[1].compare(argv[1]) == 0)
        {
            if ( args[0].compare(argv[3]) == 0 && isdigit(*argv[4]))
            {
                char *t = NULL;
                str->portNum =   (uint16_t) strtol(argv[4], &t, 10);

                if (*t != '\0')
                    errMsg(42, "ERROR port neni numericky");

            } else errMsg(42, "ERROR port neni numericky");
            str->rootDir = "";
            str->rootDir = argv[2];
        }
    }
    //cout << str->rootDir<< end;
    return str;

}
//0 == folder , 1 == file, ERR == 2
int fileOrFolder(string path)
{
    struct stat s;

    if( stat(path.c_str(),&s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
        {
            return 0;
            //typeOfMsg = "?type=folder HTTP/1.1";
            // cout << localPath + "dir" << endl;
        }
        else if( s.st_mode & S_IFREG )
        {
            return 1;
        } else
            return 2;

    } else
        return 2;
}

int reqForPut(int newsockfd, string type, string path, string fsize)
{
    char buff[1024];
    int size = 0;
    int n;
    FILE *file;
    DIR * dir;
    //cout << path << endl;

    if (type == "file") {
        file = fopen(path.c_str(), "wb");
        if (file == NULL) errMsg(42, "ERROR could not open the path."); //todo error handling to the client

        n = (int) write(newsockfd, ok.c_str(), 1024);//todo send head
        if (n < 0) {
            errMsg(42, "ERROR neodeslano");
        }

        while (atoi(fsize.c_str()) > size) {
            bzero(buff, 1024);
            int rec = (int) recv(newsockfd, buff, 1024, 0);
            if (rec < 0) {
                close(newsockfd);
                errMsg(42, "ERROR nic neprijato nebo disconnect.");
            }

            if (fwrite(buff, rec, 1, file) != 1) {
                close(newsockfd);
                errMsg(42, "ERROR nelze zapsat do souboru.");
            }
            size += rec;

        }

        if (atoi(fsize.c_str()) != size) errMsg(42, "ERROR wrong file");
        n = (int) write(newsockfd, "HTTP/1.1 200 OK", 1024);
        if (n < 0) errMsg(42, "ERROR neodeslano");
        if (file != NULL)
            fclose(file);
        close(newsockfd);
        return OK;
    }
    else if ( type == "folder")
    {
        //todo mkdir, close socket, free?, exit, user handling
        struct stat sb;

        if (fileOrFolder(path) != 0)
        {
            n = (int) write(newsockfd, "Already exists.", 1024);//todo send head
            if (n < 0) {
                close(newsockfd);
                errMsg(42, "ERROR neodeslano");
            }
            close(newsockfd);
            return OK;
        } else
        {
            mkdir(path.c_str(), S_IRWXU | S_IRWXG );
            n = (int) write(newsockfd, "HTTP/1.1 200 OK", 1024);//todo send head
            if (n < 0) {
                close(newsockfd);
                errMsg(42, "ERROR neodeslano");
            }
            close(newsockfd);
            return OK;
        }
    }

}
int reqForGet(int sock, string path, string type)
{
    FILE *file, *mime;
    int n;
    long fsize, save;
    char buf[1024];
    char *content = NULL;
    string rest;
    string typeOfMsg;
    time_t now;
    struct tm tm;

//    rest.append(path);
    //  rest.append(typeOfMsg);

    now = time(0);
    tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    rest = "Date: ";
    rest.append(buf);
    //todo content type in case of folder - txt?
    rest += "\n\rContent-Type: ";

    if(fileOrFolder(path) == 0 && type == "folder")
    {
        rest += "text/plain\n\rContent-Length: 0\n\rContent-Encoding: identity\n\r";

        bzero(buf, 1024);

        string a = "ls " + path;
        if ((mime = popen(a.c_str(), "r")) == NULL) {
            errMsg(42, "ERROR popen neziskal mime");
        }

        char b[200];
        string mi;
        while(fgets(b, 200, mime)) {
            mi += b;
        }

        //cout << mi << endl;
        n = (int) write(sock, ok.c_str(), 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");
        n = (int) write(sock, rest.c_str(), 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");
        n = (int) write(sock, mi.data(), 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");


    } else if (fileOrFolder(path) == 1 && type == "file"){
        cout << "ahoj" << endl;
        file = fopen(path.c_str(), "rb");
        // typeOfMsg = "?type=file HTTP/1.1";
        if ( file == NULL ) errMsg(42, "ERROR otevirani souboru");
        string a = "file -b --mime-type " + path;
        if ((mime = popen(a.c_str(), "r")) == NULL) {
            errMsg(42, "ERROR popen neziskal mime");
        }

        char b[200];
        string mi;
        while(fgets(b, 200, mime)) {
            mi += b;
        }
        rest += mi + "Content-Length: ";
        pclose(mime);

        if (fseek(file, 0, SEEK_END) == -1) errMsg(42, "ERROR nelze dojit na konec souboru");

        fsize = ftell(file);
        save = fsize;
        //  cout << fsize << endl;
        if (fsize == -1) errMsg(42, "ERROR zadna velikost souboru");

        stringstream ss;
        ss << fsize;
        string str = ss.str();
        rest += str + "\n\rAccept-Encoding: identity\n\r";

        rewind(file);

        content = (char*) malloc((size_t) fsize);
        if (!content) errMsg(42, "ERROR nenaalokoval se prostor pro file");
        if (fread(content, fsize, 1, file) != 1) errMsg(42, "ERROR soubor nebyl nacten");
        fclose(file);
        //cout << rest <<endl;
        n = (int) write (sock, ok.c_str(), 1024);
        if ( n < 0)errMsg(42, "ERROR psani na socket");
        n = (int) write(sock, rest.data(), 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");

        bzero(buf, 1024);

        while (fsize > 0) {
            int sent = (int) send(sock, content, fsize, 0);
            if (sent <= 0) {
                if (sent == 0) errMsg(42, "ERROR disconnected");
                else errMsg(42, "ERROR nic nebylo zapsano");
            }
            content += sent;
            fsize -= sent;
        }
        free(content-save);
        //cout << fsize << endl;
        if (fsize == 0)
        {

            n = (int) write(sock, ok.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR psani na socket");
        }
    }
    else
    {
        cout << "ahoj" << endl;

        if (fileOrFolder(path) == 0 && type == "file")
        {
            n = (int) write(sock, badR.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR cteni ze socketu");
            n = (int) write(sock, "Not a file.", 1024);
            if (n < 0) errMsg(42, "ERROR psani do socketu");
        }
        else if (fileOrFolder(path) == 1 && type == "folder")
        {
            n = (int) write(sock, badR.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR cteni ze socketu");
            n = (int) write (sock, "Not a directory.", 1024);
            if (n < 0) errMsg(42, "ERROR psani do socketu");
        }
        else if (fileOrFolder(path) == 2 && type == "folder")
        {
            n = (int) write(sock, notF.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR cteni ze socketu");
            n = (int) write(sock, "Directory not found.", 1024);
            if (n < 0) errMsg(42, "ERROR psani do socketu");
        }
        else if (fileOrFolder(path) == 2 && type == "file")
        {
            n = (int) write(sock, notF.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR cteni ze socketu");
            n = (int) write (sock, "File not found.", 1024);
            if (n < 0) errMsg(42, "ERROR psani do socketu");
        }

    }

    //free(content);
    close(sock);
}

int reqForDel(int soc, string path, string op)
{
    int n;
    char buf[1024];
    string rest;
    time_t  now;
    tm tm;

    if (op == "file")
    {
       if (fileOrFolder(path) == 1){
            cout << path << endl;
            remove(path.c_str());
            rest = ok;
            n = (int) write(soc, ok.c_str(), 1024);
            if (n < 0) {
                errMsg(42, "ERROR neodeslano");
            }
            now = time(0);
            tm = *gmtime(&now);
            strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

            rest = "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\r\nContent-Length: 0\n\rContent-Encoding: identity\n\r";

            n = (int) write (soc, rest.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR chyba pri psano do socketu");

            return OK;
        } else {
            //cout << op << endl;

            n = (int) write(soc, badR.c_str(), 1024);
            if (n < 0) {
                errMsg(42, "ERROR neodeslano");
            }
            n = (int) write(soc, "Not a file.", 1024);//todo send head
            if (n < 0) {
                errMsg(42, "ERROR neodeslano");
            }
            close(soc);
            return OK;
        }

    }
    else if (op == "folder")
    {
        //cout << "ahoj" <<endl;
        bzero(buf, 1024);

        if (rmdir(path.c_str()) != 0)
        {
            n = (int) write(soc, badR.c_str(), 1024);
            if (n < 0) {
                errMsg(42, "ERROR neodeslano");
            }
            if (fileOrFolder(path) == 2)
            {
                n = (int ) write (soc, "Directory not found.", 1024);
                if (n < 0 ) errMsg(42, "ERROR neodeslano");
            }
            else {
                n = (int) write(soc, "Directory not empty.", 1024);//todo send head
                if (n < 0) {
                    errMsg(42, "ERROR neodeslano");
                }
            }
            close(soc);
            return 1;
        }
        else
        {
            n = (int) write(soc, ok.c_str(), 1024);
            if (n < 0) {
                errMsg(42, "ERROR neodeslano");
            }
            now = time(0);
            tm = *gmtime(&now);
            strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

            rest = "Date: ";
            rest.append(buf);
            rest += "\n\rContent-Type: text/plain\r\nContent-Length: 0\n\rContent-Encoding: identity\n\r";

            n = (int) write (soc, rest.c_str(), 1024);
            if (n < 0) errMsg(42, "ERROR chyba pri psano do socketu");

            return OK;
        }
    }
}


void serveMe(int newsockfd, struct arg * arg) {
    int n;
    string operation;
    string path = arg->rootDir; //todo ?
   // cout << path << endl;
    string type;
    string fsize;
    FILE *file = NULL;
    long size = 0;

    char buff[1024];
    bzero(buff, 1024);
    n = (int) read(newsockfd, buff, 1024);


    if (n < 0) errMsg(42, "ERROR cteni ze socketu");
    string rest = buff;
    //cout << rest << endl;
    int flag = 0;
    int countOfN = 0;
    for (string::size_type i = 0; i < rest.size(); i++) {
        if (i < 3) {

            operation += rest[i];
            if (operation == "DEL")
            {
                i += 4;
            }
        }
        else if (i != 3 && rest[i] != '?' && flag == 0) path += rest[i]; //todo \t atd.
        else if (rest[i] == '?') {
            flag = 1;
        } else if (flag == 1 && rest[i] == '=') flag = 2;
        else if (flag == 2 && rest[i] != ' ') type += rest[i];
        else if (rest[i] == ' ' && flag == 2) flag = 3;
        else if (rest[i] == '\n') countOfN++;
        else if (countOfN == 5) {
            if (isdigit(rest[i]))
                fsize += rest[i];
        }
    }

    //cout << operation << endl;
    //cout << type << endl;
    //cout << path << endl;
    //cout << fsize << endl;

    if (operation == "PUT") {
        //path += "/text.txt";
        reqForPut(newsockfd, type, path, fsize);
    } else if (operation == "GET") {
        //path += "/k.deb";
        reqForGet(newsockfd, path, type);
    } else if (operation == "DEL") {
        //path += "/k.deb";
        reqForDel(newsockfd, path, type);
    } else if (operation == "MKD") {
        reqForPut(newsockfd, type, path, fsize);
    } else if (operation == "RMD")
    {
        reqForDel(newsockfd, path, type);
    }
    else if ( operation == "LST" )
    {
        reqForGet(newsockfd, path, type);
    }
    close(newsockfd);
    exit(0);
}
void createSock(struct arg * str)
{
    //close(12345);
    int sockfd, newsockfd, pid, n;

    socklen_t clientLeng;
    sockaddr_in servAddr, clientAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        errMsg(42, "ERROR otevirani socketu");

    bzero((char*) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port   =   htons(str->portNum);

    if (bind(sockfd, (sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        errMsg(42, "ERROR binding");

    listen (sockfd, 5);

    clientLeng = sizeof(clientAddr);

    while (42)
    {
        newsockfd = accept(sockfd, (sockaddr * ) &clientAddr, &clientLeng );

        if (newsockfd < 0) errMsg(42, "ERRR accept");

       pid = fork();
       if (pid == 0)
       {
            serveMe(newsockfd, str);
       } else close(newsockfd); //todo SIGCHILD
    }
}



int main(int argc, char *argv[]) {
    arg * str = getParams(argc, argv);

    createSock(str);

    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
