/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "rpz.h"
#include <fluent-bit/flb_utils.h>

/* Implicit inclusions */
//#include <fluent-bit/flb_macros.h>
//#include <fluent-bit/flb_output.h>
//#include <fluent-bit/flb_output_plugin.h>
//#include <fluent-bit/flb_sds.h>
//#include <fluent-bit/flb_upstream.h>

/**
 * allocate memory for flb_out_rpz and populate it with config map
 * return a pointer to flb_out_rpz struct or NULL if failed.
 */
static struct flb_out_rpz *create_rpz_ctx(struct flb_output_instance *ins)
{
    int ret;
    struct flb_out_rpz *ctx;

    ctx = flb_calloc(1, sizeof *ctx);
    if (!ctx) {
        flb_errno();
        return NULL;
    }

    /* set plugin context */
    ctx->ins = ins;

    /* Create and validate config map */
    ret = flb_output_config_map_set(ins, ctx);
    if (ret == -1) {
        flb_free(ctx);
        return NULL;
    }

    return ctx;
}

/**
 * populate the upstream member of `ctx`. `config` is simply passed to
 * `flb_upstream_create` function. return 1 on success, 0 on failure
 */
static int create_rpz_upstream(struct flb_out_rpz *ctx,
                               struct flb_config *config)
{
    int ret = 0;
    int flags = 0;
    char *protocol = NULL;
    char *host = NULL;
    char *port = NULL;
    char *uri = NULL;
    struct flb_upstream *u;

    /* Nothing to do with a NULL ctx */
    if (!ctx) {
        return 0;
    }

    /* determine the flags for upstream connection */
#ifdef FLB_HAVE_TLS
    if (ctx->ins->use_tls == FLB_TRUE) {
        flags = FLB_IO_TLS;
    }
    else {
        flags = FLB_IO_TCP;
    }
#else
    flags = FLB_IO_TCP;
#endif

    /* start processing the webhook string */
    if (!ctx->webhook) {
        flb_plg_error(ctx->ins, "Please specify the 'webhook' parameter");
        return 0;
    }

    ret = flb_utils_url_split(ctx->webhook, &protocol, &host, &port, &uri);
    if (!host) {
        flb_plg_error(ctx->ins, "Could not parse HOSTNAME");
        goto err_free_optionals;
    }
    if (!uri) {
        flb_plg_error(ctx->ins, "Could not parse URI");
        goto err_free_optionals;
    }

    ctx->host = flb_sds_create(host);
    flb_free(host);
    ctx->uri = flb_sds_create(uri);
    flb_free(uri);

    if (protocol) {
        ctx->protocol = flb_sds_create(protocol);
        flb_free(protocol);
    }
    else {
        /* default protocol */
        ctx->protocol = flb_sds_create("https");
    }

    flb_plg_info(ctx->ins, "port = %s", port);
    if (port) {
        ctx->port = atoi(port);
        flb_free(port);
    }

    if (ctx->port <= 0) {
        ctx->port = RPZ_DEFAULT_PORT;
    }

    u = flb_upstream_create(config, ctx->host, ctx->port,
                            flags, (void *) &ctx->ins->tls);

    if (!u) {
        flb_plg_error(ctx->ins, "Unable to create the upstream");
        return 0;
    }

    ctx->u = u;
    flb_output_upstream_set(ctx->u, ctx->ins);

    return 1;

err_free_optionals:
    if (port) {
        flb_free(port);
    }

    if (protocol) {
        flb_free(protocol);
    }

    return 0;
}

struct flb_out_rpz *get_rpz_context(struct flb_output_instance *ins,
                                    struct flb_config *config)
{
    int ret;
    const char *tmp;
    struct flb_out_rpz *ctx;

    ctx = create_rpz_ctx(ins);
    if (!ctx) {
        return NULL;
    }

    if (create_rpz_upstream(ctx, config) == 0) {
        flb_free(ctx);
        return NULL;
    }

    /*-----------------------------------------------------------------*/
    /*  OUTPUT FORMAT: one of FLB_PACK_JSON_FORMAT_*, RPZ_PACK         */
    /*-----------------------------------------------------------------*/
    /* if (!ctx->format) { */
    /*     ctx->format = FLB_PACK_JSON_FORMAT_NONE; */
    /* } */
    tmp = flb_output_get_property("format", ctx->ins);
    if (tmp) {
        ret = flb_pack_to_json_format_type(tmp);
        if (ret == -1) {
            flb_plg_error(ctx->ins, "Unrecognized format ('%s')", tmp);
        }
        else {
            ctx->format = ret;
        }
    }

    /*-----------------------------------------------------------------*/
    /*  DATE KEY: key to use for timestamp in output                   */
    /*-----------------------------------------------------------------*/
    /* if (!ctx->time_key) { */
    /*     ctx->time_key = flb_sds_create("date"); */
    /* } */
    tmp = flb_output_get_property("time_key", ins);
    if (tmp) {
        /* check if we have to disable it */
        if (flb_utils_bool(tmp) == FLB_FALSE) {
            ctx->time_key = NULL;
        }
        else {
            ctx->time_key = flb_sds_create(tmp);
        }
    }

    /*-----------------------------------------------------------------*/
    /*  DATE FORMAT: one of FLB_PACK_JSON_DATE_*                       */
    /*-----------------------------------------------------------------*/
    /* if (!ctx->time_format) { */
    /*     ctx->time_format = FLB_PACK_JSON_DATE_DOUBLE; */
    /* } */
    tmp = flb_output_get_property("time_format", ins);
    if (tmp) {
        ret = flb_pack_to_json_date_type(tmp);
        if (ret == -1) {
            flb_plg_error(ctx->ins,
                          "unrecognized date format (%s); default to DOUBLE",
                          tmp);
        }
        else {
            ctx->time_format = ret;
        }
    }

    return ctx;
}
