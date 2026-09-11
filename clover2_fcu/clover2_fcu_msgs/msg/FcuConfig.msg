# FCU interface configuration distributed by the clover2_fcu bridge.
#
# The bridge is the single configuration point: it holds these values as
# node parameters and serves them to every client. The client loads the
# single factory (protocol) and creates capability providers from it.

# Factory (protocol) name, e.g. "mavros"
string protocol

# Per-capability plugin parameters: YAML map keyed by capability name, e.g.
#   odometry:
#     topic: /clover2/loc/odometry
string plugin_params
