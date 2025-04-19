idf_component_register(
    SRCS
        "main.cpp"
        "network.cpp"
        "wifi.cpp"
        "log.cpp"
        "commands/set_config.cpp"
        "commands/set_leds.cpp"
        "commands/redraw.cpp"
        "commands/get_logs.cpp"
    INCLUDE_DIRS "."
    REQUIRES led_strip protocol esp_wifi esp_event nvs_flash esp_ringbuf)
