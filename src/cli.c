#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include <jansson.h>
#include <curl/curl.h>

#define ERR(...) fprintf(stderr, __VA_ARGS__)
#define LOG(...) fprintf(stdout, __VA_ARGS__)
#define BUFFER_SIZE  (256 * 1024)  /* 256 KB */

#define URL_FORMAT "http://data.fixer.io/api/latest?access_key=%s"
#define URL_SIZE   256

typedef enum err_type {
    ERR_NONE,
    ERR_ARGUMENT,
    ERR_API_KEY,
    ERR_JSON_PARSE,
    ERR_JSON_BAD_FORMAT,
    ERR_API_CALL,
    ERR_RATE_NOT_FOUND
} err_type_t;

struct write_result {
    char *data;
    int pos;
};

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
    struct write_result *result = (struct write_result *)stream;

    if(result->pos + size * nmemb >= BUFFER_SIZE - 1)
    {
        fprintf(stderr, "error: too small buffer\n");
        return 0;
    }

    memcpy(result->data + result->pos, ptr, size * nmemb);
    result->pos += size * nmemb;

    return size * nmemb;
}

static char *request(const char *url) {
    CURL *curl;
    CURLcode status;
    char *data;
    long code;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(!curl)
        goto error;

    data = malloc(BUFFER_SIZE);
    if(!data)
        goto error;

    struct write_result write_result = {
        .data = data,
        .pos = 0
    };

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

    status = curl_easy_perform(curl);
    if(status != 0) {
        ERR("error: unable to request data from %s:\n", url);
        ERR("%s\n", curl_easy_strerror(status));
        goto error;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code != 200) {
        ERR("error: server responded with code %ld\n", code);
        goto error;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    /* zero-terminate the result */
    data[write_result.pos] = '\0';

    return data;

error:
    if(data)
        free(data);
    if(curl)
        curl_easy_cleanup(curl);
    curl_global_cleanup();
    return NULL;
}

double convert(const int amount, const double rate_from, const double rate_to) {
    if (rate_from == rate_to) return (float)amount;
    return rate_to / rate_from * amount;
}

void strnupp(char *dest, const char *src, const size_t count) {
    size_t i;
    for (i = 0; i < count; i++)
        *(dest + i) = toupper(*(src + i));
    *(dest + count) = 0;
}

void usage(const char* program) {
    ERR("usage:\n");
    ERR("   %s <amount> <from> <to>\n",     program);
    ERR("env variables\n");
    ERR("   API_KEY Your api key from fixer.io\n");
}

int main(int argc, char** argv) {
    int amount_value;
    char *text;
    char *api_key, *amount, *from, *to;
    char url[URL_SIZE];
    char buf[4];

    json_t *root, *success, *base, *rates, *rate_from, *rate_to;
    json_error_t error;

    api_key = getenv("API_KEY");

    if (!api_key) {
        ERR("Error: API_KEY variable not found\n");
        return ERR_API_KEY;
    }

    if (argc != 4) {
        ERR("Error: not enough arguments\n");
        usage(*argv);
        return ERR_ARGUMENT;
    }

    amount = *(argv + 1);
    from = *(argv + 2);
    to = *(argv + 3);
    snprintf(url, URL_SIZE, URL_FORMAT, api_key);

    text = request(url);
    if (!text)
        return 1;

    root = json_loads(text, 0, &error);
    free(text);

    if (!root) {
        ERR("error: on line %d: %s\n", error.line, error.text);
        return ERR_JSON_PARSE;
    }

    if (!json_is_object(root)) {
        ERR("Error: json bad format\n");
        json_decref(root);
        return ERR_JSON_BAD_FORMAT;
    }

    success = json_object_get(root, "success");
    if (!json_is_true(success)) {
        ERR("Error: api call failed\n");
        json_decref(root);
        return ERR_API_CALL;
    }

    base = json_object_get(root, "base");
    if (!json_is_string(base)) {
        ERR("Error: api conversion failed\n");
        json_decref(root);
        return ERR_API_CALL;
    }

    rates = json_object_get(root, "rates");
    if (!json_is_object(rates)) {
        ERR("Error: api conversion failed\n");
        json_decref(root);
        return ERR_API_CALL;
    }

    strnupp(buf, from, 3);
    rate_from = json_object_get(rates, buf);

    strnupp(buf, to, 3);
    rate_to = json_object_get(rates, buf);

    if (!json_is_real(rate_from) || !json_is_real(rate_to)) {
        ERR("Error: one of the rates not found\n");
        json_decref(root);
        return ERR_RATE_NOT_FOUND;
    }

    amount_value = atoi(amount);

    LOG("%s %s --> %.4f %s\n", amount, from, convert(amount_value, json_real_value(rate_from), json_real_value(rate_to)), to);

    json_decref(root);
    return ERR_NONE;
}
