set(COMPONENT_ADD_INCLUDEDIRS .)

file(GLOB component_files "*.c")

set(COMPONENT_SRCS ${component_files})

set(COMPONENT_PRIV_REQUIRES message datetime nvs_flash mqtt_connection radio elastic esp_adc_cal)

register_component()
