#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024
#define MAX_DATA_SIZE 10000

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
int finish = 0;
int size = sizeof(profile) - sizeof(char*);

void parse_line(char *line);
void print_profile(int num);    //%P
void find(char *word);          //%F
void read(char *filename);      //%R
void write(char *filename);     //%W
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
    finish = 1;
}

void cmd_check(){
    fprintf(stdout,"%d profile(s)\nEnable to add %d data(s)\n"
           ,profile_data_nitems, MAX_DATA_SIZE - profile_data_nitems);
}

void cmd_print(int num){
    print_profile(num); 
}

void cmd_read(char *param){
    read(param);
}

void cmd_write(char *param){
    write(param);
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
        fprintf(stderr, "Please input the correct Data-format\n");
        return NULL;
    }
    if(split(profile_element[2], date_element_add, '-', 3) != 3){
        fprintf(stderr, "Please input the correct BirthDay-format\n");
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
    fprintf(stdout,"Id    : %d\n", profile_data_store[num].ID);
    fprintf(stdout,"Name  : %s\n", profile_data_store[num].Name);
    fprintf(stdout,"Birth : %04d-%02d-%02d\n",
           profile_data_store[num].Date.year,
           profile_data_store[num].Date.month,
           profile_data_store[num].Date.day);
    fprintf(stdout,"Addr. : %s\n", profile_data_store[num].Address);
    fprintf(stdout,"Comm. : %s\n\n", profile_data_store[num].Comment);
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
        fprintf(stdout, "[No.%d]\n", profile_num + 1);
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

void read(char *filename){
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

void write(char *filename){
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
    }
    else{
        new_profile(&profile_data_store[profile_data_nitems], line);
    }
}

//--------------------[ main ]-----------------------------

int main(){
    char line[MAX_LINE_LEN + 1];
    // profile p;
    // printf("ID:%d\nBirth:%d\nComment:%d\nName:%d\nAddress:%d\n",
    // sizeof(p.ID), sizeof(p.Date), sizeof(p.Comment), sizeof(p.Name), sizeof(p.Address));
    // printf("profile:%d\n\n", sizeof(profile));
    // printf("ID:%p\nBirth:%p\nComment:%p\nName:%p\nAddress:%p\n", 
    // &p.ID, &p.Date, &p.Comment, &p.Name, &p.Address);

    while(get_line(stdin, line)){
        parse_line(line);
        if(finish) break;
    }
    
    return 0;
}
