#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "utils.h"

#define NET 1
#define ORIGIN 0

#define TMP_BUF_SIZE 10
#define BUF_SIZE 10
#define MAX_LINE_LEN 1024
#define MAX_DATA_SIZE 10000

int s_sock;
int dst_sock;
int finish_flag = 0;
int dst_connection = 0;

char recv_buf[MAX_LINE_LEN];
char send_buf[BUF_SIZE];

int send_msg(char* msg);
void send_signal(uint8_t signal);

typedef struct{
    int year;
    int month;
    int day;
}date;

typedef struct{
    int ID;
    char Name[70];
    date Date;
    char Address[70];
    char *Comment;
}profile;

profile profile_data_store[MAX_DATA_SIZE];

int profile_data_nitems = 0;
int size = sizeof(profile) - sizeof(char*);

void parse_line(char *line);
void print_profile(int num);    //%P
void find(char *word);          //%F
void m_read(char *filename);      //%R
void m_write(char *filename);     //%W
void sort(int num);             //%S
void delete_profile(int num);   //%D

/*-----------------[ basic functions ]-----------------------*/
int subst(char *str, char c1, char c2){
    int count = 0;

    while(*str != '\0'){
        if(*str == c1){
            *str = c2;
            count++;
        }
        str++;
    }
    return count;
}

int split(char *str, char *ret[], char sep, int max){
    int count = 1, num = 0;

    ret[num] = str;

    while(*str != '\0' && count < max){
        if(*str == sep){
            *str = '\0';
            ret[++num] = (str + 1);
            count++;
        }
        str++;
    }

    while(*str != '\0'){
        if(*str == sep){
            *str = ' ';
        }
        str++;
    }
    return count;
}

int convert(char *param){
    int num = 0, sign = 1;

    if(*param == '-'){
        sign = -1;
        param++;
    }

    while(*param != '\0'){
        if('0' <= *param && *param <= '9'){
            num += *param - '0';
            num *= 10;
            param++;
        }
        else{
            return 0;
        }
    }

    return (num / 10) * sign;
}

int check_convert(int num, char *param){
    if(num == 0 && *param != '0') return 0;
    else return 1;
}

//----------------------[ get_line() ]-------------------------

int get_line(FILE *fp, char *line){
    if(fgets(line, MAX_LINE_LEN + 1, fp) == NULL){
        //fprintf(stdout,"Can't get line (fail or finish).\n");
        return 0;
    }
    
    subst(line, '\n', '\0');

    return 1;
}

/*-------------[execute command functions]---------*/

void cmd_quit(){
  int send_size = send_msg("See you!\n");
  send_signal(SIGNAL_END_CONNECTION);

  if (send_size < 0) {
    fprintf(stderr, "send() failed\n");
    dst_connection = 0;
    finish_flag = 1;
  }

  dst_connection = 0;
}

void cmd_check(){
    // fprintf(stdout,"%d profile(s)\nEnable to add %d data(s)\n"
    //        ,profile_data_nitems, MAX_DATA_SIZE - profile_data_nitems);

  char msg[MAX_LINE_LEN];
  sprintf(msg, "%d profile(s)\nEnable to add %d data(s)\n"
           ,profile_data_nitems, MAX_DATA_SIZE - profile_data_nitems);

  int send_size = send_msg(msg);
  if (send_size < 0) {
    fprintf(stderr, "send() failed\n");
  }
  send_signal(SIGNAL_END_MSG);
}

void cmd_print(int num){
    print_profile(num); 
}

void cmd_read(char *param){
    m_read(param);
}

void cmd_write(char *param){
    m_write(param);
}

void cmd_find(char *param){
    find(param);
}

void cmd_sort(int num){
    sort(num);
}

void exec_command(char cmd, char *param){
    int num;

    switch(cmd){
    case 'Q':
        cmd_quit();
        break;
    case 'C':
        cmd_check();
        break;
    case 'P':
        num = convert(param);
        if(check_convert(num, param)) cmd_print(num);
        break;
    case 'R':
        cmd_read(param);
        break;
    case 'W':
        cmd_write(param);
        break;
    case 'F':
        if(*param) cmd_find(param);
        break;
    case 'S':
        num = convert(param);
        if(check_convert(num, param)) cmd_sort(num);
        break;
    case 'D':
        if(*param) num = convert(param);
        else break;
        if(check_convert(num, param)) delete_profile(num);
        break;
    default:
        fprintf(stderr, "No such command '%%%c'\n", cmd);
    }
}

/*------------------[create new profile data]--------------------*/

int check_profile(char *line){
    while(*line != '\0'){
        if(!(0 <= *line && *line <= 127)){
            fprintf(stderr,"There is a char that can't be include\n");
            return 0;
        }
        line++;
    }
    return 1;
}

profile *new_profile(profile *new_profile_data, char *line){
    if(!(check_profile(line))) return NULL;

    int a;
    char *profile_element[5]; 
    char *date_element_add[3]; 

    if(split(line, profile_element, ',', 5) != 5){
      // fprintf(stderr, "Please input the correct Data-format\n");
      char *msg = "Please input the correct Data-format\n"; 
      send_msg(msg);
      send_signal(SIGNAL_END_MSG);
      return NULL;
    }
    if(split(profile_element[2], date_element_add, '-', 3) != 3){
      char *msg = "Please input the correct BirthDay-format\n";
      send_msg(msg);
      send_signal(SIGNAL_END_MSG);
      return NULL;
    }

    if(check_convert(convert(profile_element[0]), profile_element[0]) == 0)
        return NULL;
    for(a = 0; a < 3; a++){
        if(check_convert(convert(date_element_add[a]), date_element_add[a])
           == 0) 
            return NULL;
    }
    
    new_profile_data->ID = convert(profile_element[0]);
    strcpy(new_profile_data->Name, profile_element[1]);
    new_profile_data->Date.year = convert(date_element_add[0]);
    new_profile_data->Date.month = convert(date_element_add[1]);
    new_profile_data->Date.day = convert(date_element_add[2]);
    strcpy(new_profile_data->Address, profile_element[3]);
    new_profile_data->Comment = (char *)malloc(strlen(profile_element[4]) + 1);
    strcpy(new_profile_data->Comment, profile_element[4]);
    subst(new_profile_data->Comment, ',', ' ');

    profile_data_nitems++;
    return new_profile_data;
}

/*-----------------[command functions]--------------------*/

//====================[ %P command ]=======================

void print_profile_element(int num){
    // fprintf(stdout,"Id    : %d\n", profile_data_store[num].ID);
    // fprintf(stdout,"Name  : %s\n", profile_data_store[num].Name);
    // fprintf(stdout,"Birth : %04d-%02d-%02d\n",
    //        profile_data_store[num].Date.year,
    //        profile_data_store[num].Date.month,
    //        profile_data_store[num].Date.day);
    // fprintf(stdout,"Addr. : %s\n", profile_data_store[num].Address);
    // fprintf(stdout,"Comm. : %s\n\n", profile_data_store[num].Comment);

  char msg[MAX_LINE_LEN];
  clear_buf(msg, MAX_LINE_LEN);

  sprintf(msg, "Id    : %d\nName  : %s\nBirth : %04d-%02d-%02d\nAddr. : %s\nComm. : %s\n\n",
  profile_data_store[num].ID, profile_data_store[num].Name,
  profile_data_store[num].Date.year, profile_data_store[num].Date.month,
  profile_data_store[num].Date.day,
  profile_data_store[num].Address,
  profile_data_store[num].Comment);

  printf("%s", msg);

  send_msg(msg);
}

void print_profile(int num){
    int profile_num, end_num;

    if(profile_data_nitems == 0){
        fprintf(stdout,"No datas.\n");
        return;
    }
    if(num > 0){
        profile_num = 0;
        end_num = num;
        if(num > profile_data_nitems) end_num = profile_data_nitems;
    }
    else if(num < 0){
        profile_num = profile_data_nitems + num;
        end_num = profile_data_nitems;
        if(-num > profile_data_nitems) profile_num = 0;
    }
    else{
        profile_num = 0;
        end_num = profile_data_nitems;
    } 

    for(; profile_num < end_num; profile_num++){
        // fprintf(stdout, "[No.%d]\n", profile_num + 1);
        print_profile_element(profile_num);
    }
}

//========================[ %F command ]========================

int check_Birth(char *ret[], profile p){
    if(p.Date.year == convert(ret[0]) &&
       p.Date.month == convert(ret[1]) && 
       p.Date.day == convert(ret[2]))
    {
        return 1;
    }
    return 0;
}

int check_word(char *word, profile p, int word_state_birth, char *ret[]){
    
    if(convert(word) == p.ID) return 1;
    if(strcmp(word, p.Name) == 0 ||
       strcmp(word, p.Address) == 0 ||
       strcmp(word, p.Comment) == 0){
        return 1;
    }
    if(word_state_birth){
        if(check_Birth(ret, p)) return 1;
    }
    
    return 0;
}

void find(char *word){
    int i, word_state_birth;
    char *ret[3];

    fprintf(stdout, "word:[%s]\n\n", word);

    if(split(word, ret, '-', 3) == 3) word_state_birth = 1;
    else word_state_birth = 0;

    for(i = 0; i < profile_data_nitems; i++){
        if(check_word(word, profile_data_store[i], word_state_birth, ret)){
            fprintf(stdout, "[No.%d]\n", i + 1);
            print_profile_element(i);
        }
    }
}

//===================[ %R command ]============================

void m_read(char *filename){
    int count = 0;
    char buf[MAX_LINE_LEN + 1];

    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL){
        fprintf(stderr, "Failed to open file %s\n", filename);
        return;
    }

    while(get_line(fp, buf)){
        parse_line(buf);
        count++;
    }

    fclose(fp);
    
    return;
}

//=====================[ %W command ]==========================

int fprintf_profile_csv(FILE *fp, profile p){

    return (fprintf(fp, "%d,%s,%d-%d-%d,%s,%s\n",
            p.ID, p.Name, p.Date.year, p.Date.month,
            p.Date.day, p.Address, p.Comment));
}

void m_write(char *filename){
    int i;
    FILE *fp = fopen(filename, "w");

    if(fp == NULL){
        fprintf(stderr, "Failed to open file %s\n", filename);
        return;
    }

    for(i = 0; i < profile_data_nitems; i++){
        if(!fprintf_profile_csv(fp, profile_data_store[i])){
            fprintf(stderr, "Failed to write\n");
        }
    }

    fclose(fp);
}

//=======================[%S command]==========================

void swap_profile(profile *p1, profile *p2){
    profile p;
    p = *p1;
    *p1 = *p2;
    *p2 = p;
}

int compare_date(date d1, date d2){
    if(d1.year != d2.year) return (d1.year - d2.year);
    if(d1.month != d2.month) return (d1.month - d2.month);
    if(d1.day != d2.day) return (d1.day - d2.day);

    return 0;
}

int compare_profile(profile *p1, profile *p2, int num){
    
    switch(num){
    case 1: return (p1->ID - p2->ID);
    case 2: return strcmp(p1->Name, p2->Name);
    case 3: return compare_date(p1->Date, p2->Date);
    case 4: return strcmp(p1->Address, p2->Address);
    case 5: return strcmp(p1->Comment, p2->Comment);
    default: return 0;
    }
}

void buble_sort(int num){
    int i, a;
    for(i = 0; i < profile_data_nitems; i++){
        for(a = 0; a < profile_data_nitems - (i + 1); a++){
            if(compare_profile(&profile_data_store[a], &profile_data_store[a + 1], num) > 0)
                swap_profile(&profile_data_store[a], &profile_data_store[a + 1]);
        }
    }
}

void sort(int num){
    if(!(1 <= num && num <= 5)){
        fprintf(stderr, "No column No.%d\n", num);
        return;
    }
    
    buble_sort(num);
}
/*-----------------[Original command functions]----------------*/

//=========================[ %D command ]========================= 

//-------------------------[clear_comment()]-------------------------
void clear_comment(profile p){
    free(p.Comment);
}

//----------------------[ delete_profile() ]----------------------

void delete_profile(int num){
    int i, end;
    
    if(num > 0){
        i = 0;

        if(num > profile_data_nitems){
            end = profile_data_nitems; 
        }
        else{
            end = num;
        }

        for(; i < end; i++){
            clear_comment(profile_data_store[i]);
        }
        for(i = num; i < profile_data_nitems; i++){
           profile_data_store[i - num] = profile_data_store[i];
        }

        profile_data_nitems -= num;
    }
    else if(num < 0){
        end = profile_data_nitems;

        if(abs(num) > profile_data_nitems){
            i = 0;
        }
        else{
            i = profile_data_nitems + num;
        }

        for(; i < end; i++){
            clear_comment(profile_data_store[i]);
        }
        profile_data_nitems += num;
    }
    else{
        for(i = 0; i < profile_data_nitems; i++){
            clear_comment(profile_data_store[i]);
        }
        profile_data_nitems = 0;
    }
}

//-----------------[ parse_line() ]-------------------------

void parse_line(char *line){
    
    if(*line == '%'){
      exec_command(line[1], &line[3]);
      // printf("command!\n");
    }
    else{
      new_profile(&profile_data_store[profile_data_nitems], line);
      printf("registered! %d\n", profile_data_nitems);

    }
}


// [ Network functions ]

int make_server_sock(char *port) {
    int sock;
    struct sockaddr_in addr;

    memset((char *)&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons((unsigned int)atoi(port));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int yes = 1;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    return sock;
}


int recv_msg(int sock, char *buf, int size) {
  int recv_size = recv(sock, buf, size, 0);
  if (recv_size < 0) {
    fprintf(stderr, "recv() failed\n");
    close(sock);
    close(sock);
    return -1;
  }

  return recv_size;
}

int send_msg(char *msg) {
  // 改行文字も含めて送信するため
  int len = strlen(msg) + 1;

  int send_sum = 0;
  int send_count = (len / BUF_SIZE) + 1;
  int count = 0;
  printf("send count:%d\n", send_count);

  while (count < send_count) {
    char tmp_buf[BUF_SIZE];
    clear_buf(tmp_buf, BUF_SIZE);
    int copy_size;

    if (len > BUF_SIZE) {
      copy_size = BUF_SIZE;
    }
    else {
      copy_size = len;
    }

    strncpy(tmp_buf, msg, copy_size);

    // printf("copy size: %d\n", copy_size);
    // printf("send msg: %s\n", send_buf);

    int send_size = send(dst_sock, tmp_buf, BUF_SIZE, 0);

    if (send_size < 0) {
      fprintf(stderr, "send() failed\n");
      return -1;
    }

    send_sum += send_size;
    msg += BUF_SIZE;
    len -= BUF_SIZE;
    count++;
  }

  return send_sum;
}

void send_signal(uint8_t signal) {
  char tmp_buf[BUF_SIZE];
  tmp_buf[0] = signal;
  // clear_buf(send_buf, BUF_SIZE);
  // send_buf[0] = signal;
  send(dst_sock, tmp_buf, BUF_SIZE, 0); 

  // printf("send singal-end\n");
}

//--------------------[ main ]-----------------------------


int main(int argc, char *argv[]){

  #if NET
  if (argc < 2) {
    fprintf(stderr, "Args not enough!\n");
    return 1;
  }

  s_sock = make_server_sock(argv[1]);
  if (s_sock < 0) {
    printf("socket() failed\n");
    return 1;
  }

  int ret = listen(s_sock, 1);
  if (ret < 0) {
    printf("listen() failed\n");
    return 1;
  }


  while (finish_flag != 1) {
    int yes = 1;

    // client 接続待受
    struct sockaddr_in dst_addr;
    socklen_t addr_len;

    dst_sock = accept(s_sock, (struct sockaddr *)&dst_addr, &addr_len);
    setsockopt(dst_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (dst_sock < 0) {
      fprintf(stderr, "accept() falied\n");
      close(s_sock);
      return 1;
    }

    char *dst_ip = addr_ip(&dst_addr);
    int dst_port = addr_port(&dst_addr);

    fprintf(stdout, "<connect> \tIP [%s] PORT [%d] SOCKET: [%d]\n", 
            dst_ip, dst_port, dst_sock);

    // main routines
  
    dst_connection = 1;
    while (dst_connection) {

      // int recv_size = recv_msg(dst_sock);
      // printf("recv size: %d\n", recv_size);
      // if (recv_size < 0) {
      //   fprintf(stderr, "recv_msg() failed\n");
      //   break;
      // }

      int msg_finish = 0;
      char *frame_ptr = recv_buf;
      clear_buf(recv_buf, MAX_LINE_LEN);

      while (1) {
        char tmp_buf[BUF_SIZE];
        clear_buf(tmp_buf, BUF_SIZE);

        int recv_size = recv(dst_sock, tmp_buf, TMP_BUF_SIZE, 0);
        // printf("recv size: %d\n", recv_size);
        // printf("msg: %s\n", tmp_buf);
        if (is_contain(tmp_buf, recv_size, SIGNAL_END_MSG) > 0) {
          // printf("end.\n");
          tmp_buf[recv_size] = 0;
          msg_finish = 1;
        }

        strncpy(frame_ptr, tmp_buf, recv_size);
        frame_ptr += recv_size;

        if (msg_finish) break; 
      }

      printf("msg: [%s]\n", recv_buf);
      parse_line(recv_buf);
 
      // send_msg("accept message\n");
      send_signal(SIGNAL_END_MSG);
    }

    fprintf(stdout, "<disconnect> \tIP [%s] PORT [%d] SOCKET: [%d]\n", 
            dst_ip, dst_port, dst_sock);
    close(dst_sock);
  }

  close(s_sock);

  #endif

  #if ORIGIN

  // char line[MAX_LINE_LEN + 1];

  // while(get_line(stdin, line)){
  //   parse_line(line);
  //   if(finish) break;
  // }

  #endif

  return 0;
}
