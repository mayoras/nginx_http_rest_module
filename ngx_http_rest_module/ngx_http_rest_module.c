#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <stdbool.h>
#include <string.h>

#include "../util/cJSON.h"

#define PING_ENDPOINT "/api/ping"
#define __match(uri, ep)                                                       \
    !strncmp((const char *)uri, ep, strlen(ep)) ? true : false

static char *ngx_http_rest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_rest_handler(ngx_http_request_t *r);

static ngx_int_t dispatch(ngx_http_request_t *r, ngx_chain_t *out);
static ngx_int_t ping_endpoint_handler(ngx_http_request_t *r, ngx_chain_t *out);

/*
 location /api {
    rest;
 }
 */
static ngx_command_t ngx_http_rest_commands[] = {
    {ngx_string("rest"), NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, ngx_http_rest, 0,
     0, NULL},

    ngx_null_command};

static ngx_http_module_t ngx_http_rest_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL  /* merge location configuration */
};

ngx_module_t ngx_http_rest_module = {
    NGX_MODULE_V1,
    &ngx_http_rest_module_ctx, /* module context */
    ngx_http_rest_commands,    /* module directives */
    NGX_HTTP_MODULE,           /* module type */
    NULL,                      /* init master */
    NULL,                      /* init module */
    NULL,                      /* init process */
    NULL,                      /* init thread */
    NULL,                      /* exit thread */
    NULL,                      /* exit process */
    NULL,                      /* exit master */
    NGX_MODULE_V1_PADDING};

static char *ngx_http_rest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_rest_handler;

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_rest_handler(ngx_http_request_t *r) {
    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;

    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        return rc;
    }

    rc = dispatch(r, &out);

    if (rc != NGX_OK) {
        // return rc;
    }

    if (rc == NGX_HTTP_BAD_REQUEST) {
        const u_char *URI = r->uri.data;
        const size_t URI_LEN = r->uri.len;

        r->headers_out.content_type.len = sizeof("text/plain") - 1;
        r->headers_out.content_type.data = (u_char *)"text/plain";
        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = URI_LEN;

        rc = ngx_http_send_header(r);

        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
            return rc;
        }

        b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

        if (b == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        out.buf = b;
        out.next = NULL;

        b->pos = (u_char *)URI;
        b->last = b->pos + URI_LEN;
        b->memory = 1;
        b->last_buf = 1;
    }

    return ngx_http_output_filter(r, &out);
}

static ngx_int_t dispatch(ngx_http_request_t *r, ngx_chain_t *out) {
    const u_char *uri = r->uri.data;

    if (__match(uri, PING_ENDPOINT)) {
        return ping_endpoint_handler(r, out);
    }

    return NGX_HTTP_BAD_REQUEST;
}

static ngx_int_t ping_endpoint_handler(ngx_http_request_t *r,
                                       ngx_chain_t *out) {
    const char *ping_msg = "PONG!\n";

    ngx_int_t rc;
    ngx_buf_t *b;

    // build JSON response
    cJSON *json = cJSON_CreateObject();

    if (json == NULL)
        return NGX_HTTP_INTERNAL_SERVER_ERROR;

    cJSON_AddStringToObject(json, "message", ping_msg);
    cJSON_AddBoolToObject(json, "success", true);

    // stringify object
    char *json_str = cJSON_Print(json);

    // set headers
    r->headers_out.content_type.len = sizeof("application/json") - 1;
    r->headers_out.content_type.data = (u_char *)"application/json";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = strlen(json_str);

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    // set body
    b = ngx_palloc(r->pool, sizeof(ngx_buf_t));

    if (b == NULL) {
        // no memory?
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    // create chain buffer
    out->buf = b;
    out->next = NULL;

    b->pos = (u_char *)json_str;
    b->last = b->pos + strlen(json_str);
    b->memory = 1;
    b->last_buf = 1;

    return NGX_OK;
}
