/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef FLB_OUT_RPZ
#define FLB_OUT_RPZ

#include <fluent-bit/flb_output_plugin.h>
#include <fluent-bit/flb_pack.h>
#include <fluent-bit/flb_sds.h>
#include <fluent-bit/flb_upstream.h>

/* default values */
#define RPZ_HTTP_CONT_TYPE "application/json"
#define RPZ_HTTP_USR_AGNT  "Fluent-Bit"
#define RPZ_DEFAULT_PORT   443

/* types of output formats supported */
#define RPZ_OUT_JSON 1
#define RPZ_OUT_XML  2
#define RPZ_OUT_PROP 3

struct flb_out_rpz {
    /* DEVNOTE: To avoid mysterious SIGSEGVs place the configuration parameters
     * at the beginning of the structure. Furthermore, order them in the exact
     * order of their appearance in the configuration map */

    /* in which format to transmit data */
    int format;

    /* Timestamp format */
    int       time_format;
    flb_sds_t time_key;

    flb_sds_t webhook;

    /* webhook is parsed into following components as, protocol://host:port/uri */
    flb_sds_t protocol;
    flb_sds_t host;
    flb_sds_t uri;
    int       port;

    /* upstream connection */
    struct flb_upstream *u;

    struct flb_output_instance *ins;
};

/* parse the configurations and create a flb_out_rpz context from
 * the output instance `ins`. `config` is used to create the upstream connection
 * with `flb_upstream_create`
 */
struct flb_out_rpz *get_rpz_context(struct flb_output_instance *ins,
                                    struct flb_config *config);

#endif
