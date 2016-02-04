#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include "include/sds.h"
#include "include/lgtv.h"


#define TRIES 2
#define TRUE 1


int main(void)
{
    int i;
    char buff_code[20];
    char buff_cmd[20];
    sds session;
    sds buff_t;
    sds tvIp;

    tvIp = getip();
    if (strcmp(tvIp, "") == 0) goto exit;

    printf("Lg TV ip : %s\n", tvIp);

    buff_t  = sdsnew("123456");
    session = getsession(tvIp, buff_t);

    if((strcmp(session, "Unauthorized") == 0 )) {
        for ( i = 0 ; i < TRIES ; i ++) {
            sdsfree(session);
            sdsfree(buff_t);
            printf("Enter Tv Code: ");
            scanf("%s", &buff_code);
            buff_t= sdsnew(buff_code);
            session = getsession(tvIp, buff_t);
            if((strcmp(session, "OK") == 0 )) break;
        }
        if ( i == TRIES ) goto exit;
    }

    while(TRUE){
        printf("Enter Tv Command: ");
        scanf("%s", &buff_cmd);
        if (strcmp(buff_cmd, "exit") == 0 ) break;
        buff_t=sdscatprintf(sdsempty(), "%s", buff_cmd);
        handleCommand(tvIp, buff_t);
    }

    goto exit;

exit:
    if(session) sdsfree(session);
    if(buff_t) sdsfree(buff_t);
    if(tvIp) sdsfree(tvIp);

    return 0;
}
