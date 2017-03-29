
#include <stdio.h>

int main ()
{
    FILE *fp;
    int c;
    int sz;
    fp = fopen("file.txt","w+");
    fputs("987654321", fp);

    fseek( fp, -4, SEEK_END );
    fputs("_", fp);
    fclose(fp);

    fp = fopen("file.txt","r");
    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fclose(fp);
    fp = fopen("file.txt","r");
    fseek(fp, -sz, SEEK_END);
    while(1)
    {
        c = fgetc(fp);
        if( feof(fp) )
        {
            break;
        }
        printf("%c", c);
    }
    fclose(fp);
    return(0);
}