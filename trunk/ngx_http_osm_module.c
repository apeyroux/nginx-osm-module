// thx http://www.evanmiller.org/nginx-modules-guide.html

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <jansson.h>

#define true 1
#define false 0

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

    { ngx_string("osm_nb_mapnik_th"),
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

    osmlcf->enable = true;

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
    ngx_int_t		rc;
    ngx_buf_t 		*b = NULL;
    ngx_chain_t		out;
	double 			loadav[3];

	struct rusage	*usage = NULL;

	char 			*sjson_p = NULL;
	json_t			*json_p = NULL;

	if(NULL == (json_p = malloc(sizeof(json_p)))) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate json obj. No json, no chocolate !");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

	// lycos vas chercher la conf
    ngx_http_osm_loc_conf_t  *osmlcf;
    osmlcf = ngx_http_get_module_loc_conf(r, ngx_http_osm_module);

	/*
	 * check du load av du sys
	 */
	if(-1 == getloadavg(loadav, 3)) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to get loadav.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	// get du sys usage TODO: Faire le get de errno en cas d'erreur cf man getrusage
	// TODO: mettre du test sur le malloc
	usage = (struct rusage *) malloc(sizeof(struct rusage));
	if(-1 == getrusage(RUSAGE_SELF, usage)) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to get rusage.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	//TODO:Ajouter du test
	json_p = json_pack("{s{s{sfsfsf}s{si}}s{si}}", 
							"sys_info", 
							"loadav", 
							"1", 
							loadav[0], 
							"5", 
							loadav[1], 
							"15", 
							loadav[2], 
							"usage",
							"max_mem",
							usage->ru_maxrss, // Taille maximale de mémoire résidente utilisée (en kilooctets). Pour RUSAGE_CHILDREN, il s'agit de la taille résidente du fils le plus grand, et non de la taille résidente maximale du processus.
							"module_config", 
							"osm_nb_mapnik_th", 
							osmlcf->nb_mapnik_th);

	//TODO:Ajouter du test
	sjson_p = json_dumps(json_p, JSON_ENSURE_ASCII|JSON_INDENT(4)|JSON_PRESERVE_ORDER);
	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s", sjson_p);

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = strlen(sjson_p);
	r->headers_out.content_type.len = sizeof("application/json") - 1;
    r->headers_out.content_type.data = (u_char *) "application/json";
                          
	b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;

	u_char *result = NULL;
	result = ngx_palloc(r->pool, strlen(sjson_p) + 1);
	//TODO:Ajouter du test
	ngx_cpymem(result, sjson_p, strlen(sjson_p) + 1);

	b->pos = result;
	b->last = result + strlen(result) + 1;
    b->memory = 1;
    b->last_buf = 1;

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

	free(usage);
	usage = NULL;
	free(json_p);
	json_p = NULL;

    return ngx_http_output_filter(r, &out);
}
