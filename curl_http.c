//
//  curl_http.c
//  mod_hacking
//
//  Created by zhanwang-sky on 2025/3/12.
//

#include <string.h>
#include <curl/curl.h>
#include "curl_http.h"

typedef struct {
  char* buf;
  size_t buf_sz;
  size_t curr_sz;
} cb_param_t;

static size_t
on_resp_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
  cb_param_t* cb_param = (cb_param_t*) userdata;
  size_t realsize = size * nmemb;
  size_t remain = cb_param->buf_sz - cb_param->curr_sz;
  size_t copy_sz = realsize < remain ? realsize : remain;
  if (copy_sz) {
    memcpy(cb_param->buf, ptr, copy_sz);
    cb_param->curr_sz += copy_sz;
  }
  return copy_sz;
}

int curl_http_request(const char* url,
                      int timeout_ms,
                      const char* headers_buf, int nb_headers,
                      const char* data_buf, int data_len,
                      char* resp_buf, int* resp_sz,
                      bool verbose) {
  CURL* curl = NULL;
  struct curl_slist* slist = NULL;
  CURLcode res = CURLE_OK;
  long response_code = 0;
  int ret = -1;

  curl = curl_easy_init();
  if (!curl) {
    goto exit;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);

  timeout_ms = timeout_ms < 0 ? 0 : timeout_ms;
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long) timeout_ms);

  if (headers_buf && nb_headers > 0) {
    struct curl_slist* tmp = NULL;
    int pos = 0;
    for (int i = 0; i != nb_headers; ++i) {
      tmp = curl_slist_append(slist, headers_buf + pos);
      if (!tmp) {
        goto exit;
      }
      pos += 1 + strlen(headers_buf + pos);
      slist = tmp;
    }
  }
  if (slist) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
  }

  if (data_buf && data_len > 0) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) data_len);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_buf);
  }

  if (resp_buf && resp_sz && *resp_sz > 0) {
    cb_param_t cb_param = {
      .buf = resp_buf,
      .buf_sz = (size_t) *resp_sz,
      .curr_sz = 0
    };
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &on_resp_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &cb_param);
  }

  if (verbose) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    goto exit;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  ret = (int) response_code;

exit:
  if (slist) {
    curl_slist_free_all(slist);
  }
  if (curl) {
    curl_easy_cleanup(curl);
  }
  return ret;
}
