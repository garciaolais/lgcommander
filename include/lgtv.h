#ifndef LGTV_H_INCLUDED
#define LGTV_H_INCLUDED
#include "sds.h"

#define PORT 1900
#define SERVER "239.255.255.250"
#define BUFLEN 512



void die(char *s);
sds getip();
sds getsession(sds tvIp, sds key);
void handleCommand(sds tvIp, sds cmdcode);
size_t callbackFunction(char *ptr, size_t size, size_t nmemb, void *userdata);


#endif // LGTV_H_INCLUDED
