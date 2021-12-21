/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "rpz.h"

/* Implcit inclusions */
//#include <fluent-bit/flb_config.h>
//#include <fluent-bit/flb_http_client.h>
//#include <fluent-bit/flb_io.h>
//#include <fluent-bit/flb_log.h>
//#include <fluent-bit/flb_macros.h>
//#include <fluent-bit/flb_mem.h>
//#include <fluent-bit/flb_output_plugin.h>
//#include <fluent-bit/flb_task.h>
//#include <fluent-bit/flb_upstream.h>
//#include <fluent-bit/flb_utils.h>

/*
 * TODO: Handle output formats specific to RPZ, current support is limited to FLB_PACK_JSON_*
 * TODO: Intergrate the librpz.
 */

static int cb_rpz_init(struct flb_output_instance *ins,
                   struct flb_config *config, void *data)
{
    struct flb_out_rpz *ctx = get_rpz_context(ins, config);
    if (!ctx) {
        return -1;
    }

    flb_output_set_context(ins, (void *) ctx);
    return 0;
}

static void cb_rpz_flush(const void *data, size_t bytes,
                    const char *tag, int tag_len,
                    struct flb_input_instance *i_ins,
                    void *out_context,
                    struct flb_config *config)
{
    int ret;
    flb_sds_t json;
    size_t bytes_sent;
    int out_ret = FLB_OK;
    struct flb_http_client *client;
    struct flb_out_rpz *ctx = out_context;
    struct flb_upstream *u = ctx->u;
    struct flb_upstream_conn *u_conn = flb_upstream_conn_get(u);

    if (!u_conn) {
        flb_plg_error(ctx->ins, "no upstream connections available to %s:%i",
                      u->tcp_host, u->tcp_port);
        FLB_OUTPUT_RETURN(FLB_RETRY);
    }

    switch (ctx->format) {
    case FLB_PACK_JSON_FORMAT_NONE:
    case FLB_PACK_JSON_FORMAT_LINES:
    case FLB_PACK_JSON_FORMAT_STREAM:
        /* These formats are not supported by RPZ, but used for debugging purposes
         * of the plugin */
        json = flb_pack_msgpack_to_json_format(data, bytes, FLB_PACK_JSON_FORMAT_JSON,
                                               ctx->time_format, ctx->time_key);
        write(STDOUT_FILENO, json, flb_sds_len(json));
        flb_sds_destroy(json);
        fflush(stdout);
        printf("\n");
        break;
    case FLB_PACK_JSON_FORMAT_JSON:
        json = flb_pack_msgpack_to_json_format(data, bytes, ctx->format,
                                               ctx->time_format,
                                               ctx->time_key);
        if (!json) {
            flb_plg_error(ctx->ins, "error formatting JSON payload");
            flb_upstream_conn_release(u_conn);
            FLB_OUTPUT_RETURN(FLB_ERROR);
        }

        client = flb_http_client(u_conn, FLB_HTTP_POST, ctx->uri, json,
                                 flb_sds_len(json), ctx->host, ctx->port, NULL, 0);
        flb_http_add_header(client,
                            "Content-Type", 12,
                            RPZ_HTTP_CONT_TYPE, sizeof(RPZ_HTTP_CONT_TYPE) - 1);
        flb_http_add_header(client,
                            "User-Agent", 10,
                            RPZ_HTTP_USR_AGNT, sizeof(RPZ_HTTP_USR_AGNT) - 1);

        ret = flb_http_do(client, &bytes_sent);
        if (ret == 0) {
            if (client->resp.status < 200 || client->resp.status > 205) {
                flb_plg_error(ctx->ins, "%s:%i, HTTP status=%i",
                              ctx->host, ctx->port, client->resp.status);
                out_ret = FLB_RETRY;
            }
            else {
                if (client->resp.payload) {
                    flb_plg_info(ctx->ins, "%s:%i, HTTP status=%i\n%s",
                                 ctx->host, ctx->port,
                 client->resp.status, client->resp.payload);
                }
                else {
                    flb_plg_info(ctx->ins, "%s:%i, HTTP status=%i",
                                 ctx->host, ctx->port,
                                 client->resp.status);
                }
            }
        } else {
            flb_plg_error(ctx->ins, "could not flush records to %s:%i (http_do=%i)",
                          ctx->host, ctx->port, ret);
            out_ret = FLB_RETRY;
        }

        flb_sds_destroy(json);
        flb_upstream_conn_release(u_conn);
        flb_http_client_destroy(client);
        break;
    }

    FLB_OUTPUT_RETURN(out_ret);
}

static int cb_rpz_exit(void *data, struct flb_config *config)
{
    struct flb_out_rpz *ctx = data;
    if (!ctx) {
        return 0;
    }

    if (ctx->u)
        flb_upstream_destroy(ctx->u);

    flb_free(ctx);

    return 0;
}

static struct flb_config_map config_map[] = {
    {
     FLB_CONFIG_MAP_STR, "format", "json",
     0, FLB_FALSE, 0,
     "Specify the payload format, supported formats: json"
    },

    {
     FLB_CONFIG_MAP_STR, "time_format", "double",
     0, FLB_FALSE, 0,
     "Specify the format of the date, supported formats: double, iso8601 "
     "(e.g: 2018-05-30T09:39:52.000681Z) and epoch."
    },

    {
     FLB_CONFIG_MAP_STR, "time_key", "date",
     0, FLB_FALSE, 0,
     "Specify the name of the date field in output."
    },

    /* offset of the final parameter is manadatory */
    {
     FLB_CONFIG_MAP_STR, "webhook", NULL,
     0, FLB_TRUE, offsetof(struct flb_out_rpz, webhook),
     "Specify the RPZ connection string \'proto://host:port/uri\'"
    },

    /* EOF */
    {0}
};

/* Plugin registration */
struct flb_output_plugin out_rpz_plugin = {
    .name         = "rpz",
    .description  = "Transmit data to Rapidomize cloud",
    .cb_init      = cb_rpz_init,
    .cb_flush     = cb_rpz_flush,
    .cb_exit      = cb_rpz_exit,
    .config_map   = config_map,
    .flags        = FLB_OUTPUT_NET | FLB_IO_OPT_TLS,
};
