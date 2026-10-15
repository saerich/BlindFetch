#define _POSIX_C_SOURCE 200809L
#include "esp_http_client.h"
#include <curl/curl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <strings.h>
#include <unistd.h>

#define RX_CAPACITY (64u * 1024u)
#define TX_CAPACITY (64u * 1024u)

typedef struct header { char *key, *value; struct header *next; } header_t;
struct esp_http_client {
    CURL *curl;
    char *url, *cert_file;
    char *post_field;
    size_t post_len;
    header_t *headers;
    struct curl_slist *curl_headers;
    esp_http_client_method_t method;
    int timeout_ms;
    int expected_upload;
    size_t uploaded;
    long status;
    int64_t content_length;
    unsigned char rx[RX_CAPACITY], tx[TX_CAPACITY];
    size_t rx_head, rx_size, tx_head, tx_size;
    int headers_ready, done, cancelled, running;
    CURLcode result;
    pthread_t worker;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    esp_http_client_event_cb_t event_handler;
    void *user_data;
};

static int wait_locked(struct esp_http_client *c) {
    if (c->timeout_ms <= 0) return pthread_cond_wait(&c->cond, &c->mutex);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += c->timeout_ms / 1000;
    ts.tv_nsec += (long)(c->timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
    return pthread_cond_timedwait(&c->cond, &c->mutex, &ts);
}
static void wake(struct esp_http_client *c) { pthread_cond_broadcast(&c->cond); }

static size_t on_write(char *ptr, size_t size, size_t nmemb, void *userdata) {
    struct esp_http_client *c = userdata;
    size_t n = size * nmemb, off = 0;
    pthread_mutex_lock(&c->mutex);
    while (off < n && !c->cancelled) {
        while (c->rx_size == RX_CAPACITY && !c->cancelled) pthread_cond_wait(&c->cond, &c->mutex);
        if (c->cancelled) break;
        size_t tail = (c->rx_head + c->rx_size) % RX_CAPACITY;
        size_t amount = RX_CAPACITY - c->rx_size;
        if (amount > RX_CAPACITY - tail) amount = RX_CAPACITY - tail;
        if (amount > n - off) amount = n - off;
        memcpy(c->rx + tail, ptr + off, amount);
        c->rx_size += amount; off += amount; wake(c);
    }
    pthread_mutex_unlock(&c->mutex);
    return off;
}
static size_t on_read(char *ptr, size_t size, size_t nmemb, void *userdata) {
    struct esp_http_client *c = userdata;
    size_t cap = size * nmemb;
    pthread_mutex_lock(&c->mutex);
    while (!c->tx_size && !c->cancelled && c->uploaded < (size_t)c->expected_upload)
        pthread_cond_wait(&c->cond, &c->mutex);
    if (c->cancelled) { pthread_mutex_unlock(&c->mutex); return CURL_READFUNC_ABORT; }
    if (!c->tx_size) { pthread_mutex_unlock(&c->mutex); return 0; }
    size_t amount = c->tx_size;
    if (amount > cap) amount = cap;
    if (amount > TX_CAPACITY - c->tx_head) amount = TX_CAPACITY - c->tx_head;
    memcpy(ptr, c->tx + c->tx_head, amount);
    c->tx_head = (c->tx_head + amount) % TX_CAPACITY;
    c->tx_size -= amount;
    c->uploaded += amount;
    wake(c);
    pthread_mutex_unlock(&c->mutex);
    return amount;
}
static size_t on_header(char *ptr, size_t size, size_t nmemb, void *userdata) 
{
    struct esp_http_client *c = userdata;
    size_t n = size * nmemb;
    pthread_mutex_lock(&c->mutex);
    if (n >= 5 && !memcmp(ptr, "HTTP/", 5)) 
    {
        c->headers_ready = 0;
        c->content_length = -1;
        long status = 0;
        sscanf(ptr, "HTTP/%*s %ld", &status);
        c->status = status;
    } 
    else if (n == 2 && ptr[0] == '\r' && ptr[1] == '\n') 
    {
        if (c->status >= 200 || c->status == 101) c->headers_ready = 1;
        wake(c);
    } 
    else if (n > 15 && !strncasecmp(ptr, "Content-Length:", 15)) { c->content_length = strtoll(ptr + 15, NULL, 10); }
    pthread_mutex_unlock(&c->mutex);
    return n;
}
static void *run_request(void *arg)
{
    struct esp_http_client *c = arg;
    char errorBuffer[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(c->curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    // curl_easy_setopt(c->curl, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(c->curl, CURLOPT_HTTPGET, 1L);
    CURLcode result = curl_easy_perform(c->curl);

    if (result != CURLE_OK) { fprintf(stderr, "libcurl error %d: %s (%s)\n", (int)result, curl_easy_strerror(result), errorBuffer); }
    pthread_mutex_lock(&c->mutex);
    c->result = result; c->done = 1; wake(c);
    pthread_mutex_unlock(&c->mutex);
    return NULL;
}
static char *dupstr(const char *s) { return s ? strdup(s) : NULL; }
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *cfg) 
{
    if (!cfg || !cfg->url) return NULL;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    struct esp_http_client *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->curl = curl_easy_init(); c->url = dupstr(cfg->url);
    if (!c->curl || !c->url) { if (c->curl) curl_easy_cleanup(c->curl); free(c->url); free(c); return NULL; }
    pthread_mutex_init(&c->mutex, NULL); pthread_cond_init(&c->cond, NULL);
    c->method = cfg->method; 
    c->timeout_ms = cfg->timeout_ms > 0 ? cfg->timeout_ms : 30000;
    c->event_handler = cfg->event_handler;
    c->user_data = cfg->user_data;
    c->content_length = -1;
    curl_easy_setopt(c->curl, CURLOPT_URL, c->url);
    curl_easy_setopt(c->curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c->curl, CURLOPT_CONNECTTIMEOUT_MS, (long)c->timeout_ms);
    curl_easy_setopt(c->curl, CURLOPT_FOLLOWLOCATION, cfg->disable_auto_redirect ? 0L : 1L);
    curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, on_write);
    curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, c);
    curl_easy_setopt(c->curl, CURLOPT_HEADERFUNCTION, on_header);
    curl_easy_setopt(c->curl, CURLOPT_HEADERDATA, c);
    curl_easy_setopt(c->curl, CURLOPT_READFUNCTION, on_read);
    curl_easy_setopt(c->curl, CURLOPT_READDATA, c);
    if (cfg->cert_pem) 
    {
        /* libcurl expects a CA file path. Persist the PEM in a temporary file. */
        char name[] = "/tmp/esp-http-ca-XXXXXX";
        int fd = mkstemp(name);
        if (fd < 0) { esp_http_client_cleanup(c); return NULL; }
        FILE *f = fdopen(fd, "w");
        if (!f) { close(fd); unlink(name); esp_http_client_cleanup(c); return NULL; }
        size_t len = strlen(cfg->cert_pem);
        if (fwrite(cfg->cert_pem, 1, len, f) != len || fclose(f) != 0) { unlink(name); esp_http_client_cleanup(c); return NULL; }
        c->cert_file = dupstr(name);
        curl_easy_setopt(c->curl, CURLOPT_CAINFO, c->cert_file);
    }
    if (cfg->skip_cert_common_name_check) curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    return c;
}
esp_err_t esp_http_client_set_method(esp_http_client_handle_t c, esp_http_client_method_t method) {
    if (!c || c->running) return ESP_FAIL;
    c->method = method; return ESP_OK;
}
esp_err_t esp_http_client_set_header(esp_http_client_handle_t c, const char *key, const char *value) {
    if (!c || !key || !value || c->running) return ESP_FAIL;
    header_t *h = calloc(1, sizeof(*h)); if (!h) return ESP_FAIL;
    h->key = dupstr(key); h->value = dupstr(value);
    if (!h->key || !h->value) { free(h->key); free(h->value); free(h); return ESP_FAIL; }
    h->next = c->headers; c->headers = h; return ESP_OK;
}
esp_err_t esp_http_client_set_post_field(esp_http_client_handle_t c, const char *data, int len) {
    if (!c || !data || len < 0 || c->running) return ESP_FAIL;
    char *copy = malloc((size_t)len + 1); if (!copy) return ESP_FAIL;
    memcpy(copy, data, (size_t)len); copy[len] = 0;
    free(c->post_field); c->post_field = copy; c->post_len = (size_t)len; return ESP_OK;
}
esp_err_t esp_http_client_open(esp_http_client_handle_t c, int write_len) {
    if (!c || c->running || write_len < 0) return ESP_FAIL;
    c->expected_upload = write_len;
    c->headers_ready = c->done = c->cancelled = 0;
    c->rx_head = c->rx_size = c->tx_head = c->tx_size = c->uploaded = 0;
    c->content_length = -1; c->status = 0;
    c->result = CURLE_OK;
    curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, NULL);
    curl_slist_free_all(c->curl_headers); c->curl_headers = NULL;
    for (header_t *h = c->headers; h; h = h->next) {
        size_t n = strlen(h->key) + strlen(h->value) + 3;
        char *line = malloc(n); if (!line) return ESP_FAIL;
        snprintf(line, n, "%s: %s", h->key, h->value);
        struct curl_slist *next = curl_slist_append(c->curl_headers, line);
        free(line); if (!next) return ESP_FAIL; c->curl_headers = next;
    }
    curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->curl_headers);
    curl_easy_setopt(c->curl, CURLOPT_HTTPGET, 1L);
    switch (c->method) {
        case HTTP_METHOD_POST:
            curl_easy_setopt(c->curl, CURLOPT_POST, 1L);
            curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)write_len);
            break;
        case HTTP_METHOD_PUT:
            curl_easy_setopt(c->curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(c->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)write_len);
            break;
        case HTTP_METHOD_HEAD: curl_easy_setopt(c->curl, CURLOPT_NOBODY, 1L); break;
        default: if (c->method != HTTP_METHOD_GET) return ESP_FAIL; break;
    }
    c->running = 1;
    if (pthread_create(&c->worker, NULL, run_request, c)) { c->running = 0; return ESP_FAIL; }
    return ESP_OK;
}
int esp_http_client_write(esp_http_client_handle_t c, const char *buf, int len) {
    if (!c || !buf || len < 0 || !c->running) return -1;
    int off = 0;
    pthread_mutex_lock(&c->mutex);
    while (off < len && !c->done && !c->cancelled) {
        while (c->tx_size == TX_CAPACITY && !c->done && !c->cancelled) {
            if (wait_locked(c) == ETIMEDOUT) goto out;
        }
        if (c->done || c->cancelled) break;
        size_t tail = (c->tx_head + c->tx_size) % TX_CAPACITY;
        size_t amount = TX_CAPACITY - c->tx_size;
        if (amount > TX_CAPACITY - tail) amount = TX_CAPACITY - tail;
        if (amount > (size_t)(len - off)) amount = (size_t)(len - off);
        if (amount > (size_t)c->expected_upload - c->uploaded - c->tx_size)
            amount = (size_t)c->expected_upload - c->uploaded - c->tx_size;
        if (!amount) break;
        memcpy(c->tx + tail, buf + off, amount);
        c->tx_size += amount; off += (int)amount; wake(c);
    }
out:
    pthread_mutex_unlock(&c->mutex);
    return off ? off : (len == 0 ? 0 : -1);
}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t c) {
    if (!c || !c->running) return -1;
    pthread_mutex_lock(&c->mutex);
    while (!c->headers_ready && !c->done && !c->cancelled) {
        if (wait_locked(c) == ETIMEDOUT) break;
    }
    int64_t n = c->headers_ready ? (c->content_length >= 0 ? c->content_length : 0) : -1;
    pthread_mutex_unlock(&c->mutex); return n;
}
int esp_http_client_read(esp_http_client_handle_t c, char *buf, int len) {
    if (!c || !buf || len < 0 || !c->running) return -1;
    if (!len) return 0;
    pthread_mutex_lock(&c->mutex);
    while (!c->rx_size && !c->done && !c->cancelled) {
        if (wait_locked(c) == ETIMEDOUT) { pthread_mutex_unlock(&c->mutex); return -1; }
    }
    if (!c->rx_size) {
        int result = (c->done && c->result == CURLE_OK) ? 0 : -1;
        pthread_mutex_unlock(&c->mutex); return result;
    }
    size_t n = c->rx_size;
    if (n > (size_t)len) n = (size_t)len;
    if (n > RX_CAPACITY - c->rx_head) n = RX_CAPACITY - c->rx_head;
    memcpy(buf, c->rx + c->rx_head, n);
    c->rx_head = (c->rx_head + n) % RX_CAPACITY;
    c->rx_size -= n; wake(c);
    pthread_mutex_unlock(&c->mutex); return (int)n;
}
int esp_http_client_read_response(esp_http_client_handle_t c, char *buf, int len) {
    if (!c || !buf || len < 0) return -1;
    int total = 0;
    while (total < len) {
        int n = esp_http_client_read(c, buf + total, len - total);
        if (n <= 0) return total ? total : n;
        total += n;
    }
    return total;
}
int esp_http_client_get_status_code(esp_http_client_handle_t c) { return c ? (int)c->status : -1; }
int64_t esp_http_client_get_content_length(esp_http_client_handle_t c) { return c ? c->content_length : -1; }
esp_err_t esp_http_client_close(esp_http_client_handle_t c) {
    if (!c) return ESP_FAIL;
    if (c->running) {
        pthread_mutex_lock(&c->mutex);
        c->cancelled = 1; wake(c);
        pthread_mutex_unlock(&c->mutex);
        pthread_join(c->worker, NULL);
        c->running = 0;
    }
    return ESP_OK;
}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t c) {
    if (!c) return ESP_FAIL;
    esp_http_client_close(c);
    curl_easy_cleanup(c->curl);
    curl_slist_free_all(c->curl_headers);
    while (c->headers) { header_t *h = c->headers; c->headers = h->next; free(h->key); free(h->value); free(h); }
    if (c->cert_file) { unlink(c->cert_file); free(c->cert_file); }
    free(c->url); free(c->post_field);
    pthread_cond_destroy(&c->cond); pthread_mutex_destroy(&c->mutex); free(c);
    return ESP_OK;
}
esp_err_t esp_http_client_perform(esp_http_client_handle_t c) 
{
    if (!c) { return ESP_FAIL; }

    size_t len = c->post_field ? c->post_len : 0;
    if (len > INT_MAX) { return ESP_FAIL; }
    if (esp_http_client_open(c, (int)len) != ESP_OK) { return ESP_FAIL; }
    if (len && esp_http_client_write(c, c->post_field, (int)len) != (int)len) { return ESP_FAIL; }
    if (esp_http_client_fetch_headers(c) < 0) { return ESP_FAIL; }

    char tmp[4096];
    int n;

    while ((n = esp_http_client_read(c, tmp, sizeof(tmp))) > 0)
    {
        if (c->event_handler)
        {
            esp_http_client_event_t evt = 
            {
                .event_id = HTTP_EVENT_ON_DATA,
                .client = c,
                .data = tmp,
                .data_len = n,
                .user_data = c->user_data
            };

            if (c->event_handler(&evt) != ESP_OK) { return ESP_FAIL; }
        }
    }

    return n == 0 ? ESP_OK : ESP_FAIL;
}
esp_err_t esp_http_client_flush_response(
    esp_http_client_handle_t client,
    int *len
)
{
    if (!client) {
        return ESP_ERR_INVALID_ARG;
    }

    char buffer[4096];
    int total = 0;
    int received;

    while ((received = esp_http_client_read(
        client, buffer, sizeof(buffer))) > 0)
    {
        total += received;
    }

    if (len) {
        *len = total;
    }

    return received < 0 ? ESP_FAIL : ESP_OK;
}