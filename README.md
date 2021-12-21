# RPZ Fluent Bit plugin

Fluent Bit plugin for Rapidomize ICS (Intelligent Connected Services) platform.
The **rpz** output plugin allows to flush your records into a Rapidomize Application
Currently output is limited to JSON, and support for RPZ specific output formats 
are under development. 

## Configuration Parameters

| Key | Description | Default |
|:----|:------------|:--------|
|format| Payload format, currently supports `json` only. | `json` |
|webhook| Webhook string for the IC application | |
|time_format| Format of timestamp. Supports: `double`, `iso8601`, `epoch`| `double`|
|time_key | JSON key for the timestamp | `date` |

## Example Configuration File

__plugins.conf__ file for loading external plugins:

```
[PLUGINS]
    # rpz output plugin
    Path /path/to/flb-out_rpz.so
```

__fluent-bit.conf__ , main configuration file:

```
[SERVICE]
    Grace 0
    Flush 5
    Plugins_file plugins.conf
    Log_level debug

[INPUT]
    Name forward
    Listen 0.0.0.0
    Port 24224

[OUTPUT]
    Name rpz
    Match test.*
    Format json
    Time_key timestamp
    Time_format epoch
    Webhook http://127.0.0.1:21847/flb
```

## TLS/SSL

rpz output plugin supports tls/ssl, configurations for these properties are same
as fluent-bit's. See [TLS/SSL](https://docs.fluentbit.io/manual/v/0.12/configuration/tls_ssl)
section of Fluent-bit documentation.

## Development

This plugin is based on [fluent/fluent-bit-plugin](https://github.com/fluent/fluent-bit-plugin).
Refer it for instructions on building the plugin.

## Testing

Currently only manual testing is available. To test the plugin, you need two things:

1. plugin build as a shared library. See [Development](#development). 
2. you need to create an IC app on Rapidomize cloud and get the _webhook_ connection string for the application.

To test the plugin, run following command:

``` sh
$ fluent-bit -vv -e path/to/flb-out_rpz.so \
             -i cpu -o rpz \
             -p format=json \
             -p webhook='https://webhook/connection/string' \
             -p time_key="tm" \
             -p tls=on 
```

*OR*

Spin a fluent-bit with the example configuration:

``` sh
$ fluent-bit -c fluent-bit.conf
```

and send a message to forward plugin from another shell:

``` sh
$ echo '{"json": "message"}' | fluent-cat test.tag
```

## Status - preview
version 0.7.5 - 'Dugong Weasel'

## Contributions?
Contributions are highly welcome. If you're interested in contributing leave a note with your username.

## License
Apache 2.0

