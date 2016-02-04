#include <stdio.h>
#include <curl/curl.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include "include/lgtv.h"

char header[] = {
            "M-SEARCH * HTTP/1.1\r\n"\
            "HOST: 239.255.255.250:1900\r\n"\
            "MAN: \"ssdp:discover\"\r\n"\
            "MX: 2\r\n"\
            "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n\r\n"};

void die(char *s)
{
    perror(s);
    exit(1);
}

sds getip(){
    //http://www.binarytides.com/programming-udp-sockets-c-linux/
    //http://www.rohitab.com/discuss/topic/41267-ssdpupnp-protocol-example/
    int s, slen;
    sds buff;
    struct sockaddr_in si_other;
    struct timeval timeout;

    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {

        die("socket");
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);

    if (inet_aton(SERVER , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        goto exit;
    }

    buff = sdsnew(header);
    slen = sizeof(si_other);

    //send the message
    if (sendto(s, buff, sdslen(buff), 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        //perror("sendto()");
        goto exit;
    }

    sdsfree(buff);
    buff = sdsnewlen(NULL, BUFLEN);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    /* Receive UDP message */
    int recvlen = recvfrom(s, buff, sdslen(buff), 0, (struct sockaddr *) &si_other, &slen);
    if (recvlen >= 0) {
        //Message Received
        sdsfree(buff);
        close(s);
        return sdsnew(inet_ntoa(si_other.sin_addr));
    } else {
        //Message Receive Timeout or other error
        perror("recvlen()");
        printf("LgTV not found\n");
        //if(buff) sdsfree(buff);
        goto exit;

    }

exit:
    if(buff) sdsfree(buff);
    close(s);
    return sdsempty();
}



sds getsession(const sds tvIp, const sds key){
    CURL *curl;
    CURLcode res;
    sds post;
    sds tvIp_t;
    long rc;

    post = sdsnew("<?xml version=\"1.0\" encoding=\"utf-8\"?><auth><type>AuthReq</type><value>");
    post = sdscat(post, key);
    post = sdscat(post, "</value></auth>");

    curl = curl_easy_init();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if(curl) {
        struct curl_slist *chunk = NULL;
        rc = 0;

        chunk = curl_slist_append(chunk, "Content-Type:application/atom+xml");
        tvIp_t = sdsnew("");
        tvIp_t = sdscat(tvIp_t, tvIp);
        tvIp_t = sdscat(tvIp_t, ":8080/roap/api/auth");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, tvIp_t);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);

        /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
        itself */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)sdslen(post));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackFunction);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if  (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
        curl_slist_free_all(chunk);
    }
    /* always cleanup */
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &rc);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if(tvIp_t) sdsfree(tvIp_t);
    if(post) sdsfree(post);

    if (rc == 200){
        return sdsnew("OK");
    } else if (rc == 401) {
        return sdsnew("Unauthorized");
    } else {
        return sdsnew("Error");
    }
}

void handleCommand(const sds tvIp, const sds cmdcode){
    sds cmdText;
    sds tvIp_t;
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if(curl) {
        struct curl_slist *chunk = NULL;

        cmdText = sdsnew("<?xml version=\"1.0\" encoding=\"utf-8\"?><command>");
        cmdText = sdscat(cmdText, "<name>HandleKeyInput</name><value>");
        cmdText = sdscat(cmdText, cmdcode);
        cmdText = sdscat(cmdText, "</value></command>");
        chunk = curl_slist_append(chunk, "Content-Type:application/atom+xml");
        tvIp_t = sdsnew("");
        tvIp_t = sdscat(tvIp_t, tvIp);
        tvIp_t = sdscat(tvIp_t, ":8080/roap/api/command");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, tvIp_t);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cmdText);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)sdslen(cmdText));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackFunction);
        //curl_easy_setopt(curl, CURLOPT_NOBODY, 1);

        res = curl_easy_perform(curl);
        /* Check for errors */
        if  (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
        curl_slist_free_all(chunk);
    }

    //curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &rc);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    sdsfree(tvIp_t);
    sdsfree(cmdText);
}


size_t callbackFunction(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return size * nmemb;
}


