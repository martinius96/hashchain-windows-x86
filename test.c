#include <stdio.h>
#include <stdlib.h>

void testGetFile() {
    FILE *fp = fopen("chains", "r");
    size_t len = 255;
    char *line = malloc(sizeof(char) * len);
    if (fp == NULL) {
        printf("cant open file!");
        return;
    }
    int riadok = 0; //counter
    while(fgets(line, len, fp) != NULL) {
        riadok++;
       printf("[+]"); 
        printf("%s\n", line);
    }
     int count = 0;
     rewind(fp);
    while(fgets(line, len, fp) != NULL) {
        count++;
        if (count == riadok){
        printf("SERVER STORED HASH: "); 
        printf("%s\n", line);
        }
        if (count+1 == riadok){
        printf("Client HASH: "); 
        printf("%s\n", line);
        }
    }
        free(line);
}
int main(){
    testGetFile();
    return 0;
}
