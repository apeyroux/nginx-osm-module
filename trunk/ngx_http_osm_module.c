

// http://www.evanmiller.org/nginx-modules-guide.html

/*
 * Modules can define up to three configuration structs, one for the main, server, and location contexts. 
 * Most modules just need a location configuration. 
 * The naming convention for these is ngx_http_<module name>_(main|srv|loc)_conf_t. Here's an example, taken from the dav module
 *
 * Notice that Nginx has special data types (ngx_uint_t and ngx_flag_t); these are just aliases for the primitive 
 * data types you know and love (cf. core/ngx_config.h if you're curious).
 *
 * The elements in the configuration structs are populated by module directives.
 *
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
    
static ngx_str_t  ngx_http_osm_text = ngx_string("OSM Module is under construction ...");

typedef struct {
    ngx_uint_t   nb_mapnik_th;
    ngx_flag_t  enable;
} ngx_http_osm_loc_conf_t;

static void *ngx_http_osm_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_osm_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_osm(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_osm_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_osm_init(ngx_http_osm_loc_conf_t *cglcf);

static ngx_command_t ngx_http_osm_commands[] = {
    { ngx_string("osm"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_osm,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("nb_mapnik_th"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_osm_loc_conf_t, nb_mapnik_th),
      NULL },

    ngx_null_command
};

static ngx_http_module_t  ngx_http_osm_module_ctx = {
    NULL,                         /* preconfiguration */
    NULL,                         /* postconfiguration */
    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */
    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */
    ngx_http_osm_create_loc_conf,  /* create location configuration */
    ngx_http_osm_merge_loc_conf /* merge location configuration */
};


/*
 * def du module
 */
ngx_module_t ngx_http_osm_module = {
    NGX_MODULE_V1,
    &ngx_http_osm_module_ctx,      /* module context */
    ngx_http_osm_commands,         /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static char *ngx_http_osm(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;
    ngx_http_osm_loc_conf_t *osmlcf = conf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_osm_handler;

    osmlcf->enable = 1;

    return NGX_CONF_OK;
}

static void *ngx_http_osm_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_osm_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_osm_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->nb_mapnik_th = NGX_CONF_UNSET_UINT;
    conf->enable = NGX_CONF_UNSET;
    return conf;
}

static char *ngx_http_osm_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_osm_loc_conf_t *prev = parent;
    ngx_http_osm_loc_conf_t *conf = child;

    ngx_conf_merge_uint_value(conf->nb_mapnik_th, prev->nb_mapnik_th, 8);
    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    if (conf->nb_mapnik_th < 1) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "nb_mapnik_th must be equal or more than 1"); 
        return NGX_CONF_ERROR;
    }

    if(conf->enable)
        ngx_http_osm_init(conf);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_osm_init(ngx_http_osm_loc_conf_t *cglcf)
{
    ngx_int_t i = 0;
    return i;
}

static ngx_int_t ngx_http_osm_handler(ngx_http_request_t *r)
{
    ngx_int_t     rc;
    ngx_buf_t    *b;
    ngx_chain_t   out;

    //ngx_http_osm_loc_conf_t  *osmlcf;
    //osmlcf = ngx_http_get_module_loc_conf(r, ngx_http_osm_module);
    //osmlcf->nb_mapnik_th;

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = ngx_http_osm_text.len;
                           
    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    b = ngx_calloc_buf(r->pool);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;

    b->start = b->pos = ngx_http_osm_text.data;
    b->end = b->last = ngx_http_osm_text.data + ngx_http_osm_text.len;
    b->memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "OSM module is under construction.");

    return ngx_http_output_filter(r, &out);
}
