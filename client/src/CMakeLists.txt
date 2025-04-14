idf_component_register(
    SRCS 
        "main.cpp"
        "network.cpp"
        "set_config.cpp"
        "set_leds.cpp"
        "redraw.cpp"
        "get_status.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi led_strip arduino-esp32 protocol)
