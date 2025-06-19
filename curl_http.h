//
//  curl_http.h
//  mod_hacking
//
//  Created by zhanwang-sky on 2025/3/12.
//

#ifndef curl_http_h
#define curl_http_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

int curl_http_request(const char* url,
                      int timeout_ms,
                      const char* headers_buf, int nb_headers,
                      const char* data_buf, int data_len,
                      char* resp_buf, int* resp_sz,
                      bool verbose);

#ifdef __cplusplus
}
#endif

#endif /* curl_http_h */
