idf_component_register(
    SRCS 
        "main.cpp"
        "network.cpp"
        "commands/set_config.cpp"
        "commands/set_leds.cpp"
        "commands/redraw.cpp"
        "commands/get_status.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi led_strip arduino-esp32 protocol)
