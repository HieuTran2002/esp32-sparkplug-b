idf_component_register( 
    SRCS    "main.c" 
            "sparkplug_b.pb.c"
            "pb_decode.c"
            "pb_encode.c"
            "pb_common.c"
            "mqtt_helper.c"
            "wifi_helper.c"
            "simple_SNTP.c"
            "payload.c"
            "node.c"
    INCLUDE_DIRS "."
)

