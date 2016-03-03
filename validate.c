#include "main.h"
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

bool validate_file(const char *file) {
    CURL *curl;
    CURLcode response;
    bool res = false;

    FILE *fd = fopen(file, "r");
    curl = curl_easy_init();
    if(curl) {

        struct curl_httppost* post = NULL;
        struct curl_httppost* last = NULL;
        struct curl_forms forms[2];

        struct MemoryStruct chunk;

        chunk.memory = (char *)malloc(1);
        chunk.size = 0;


        forms[0].option = CURLFORM_FILE;
        forms[0].value  = file;
        forms[1].option  = CURLFORM_END;

        curl_easy_setopt(curl, CURLOPT_URL, "http://vali.fai-civl.org/cgi-bin/vali-igc.cgi");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "igcfile", CURLFORM_FILE, file, CURLFORM_CONTENTTYPE, "text/plain", CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        response = curl_easy_perform(curl);
        if (response == CURLE_OK && strstr(chunk.memory, "OK") != NULL) {
            res = true;
        }

        curl_easy_cleanup(curl);
    }
    return res;
}