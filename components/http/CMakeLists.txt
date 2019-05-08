set(COMPONENT_ADD_INCLUDEDIRS .)

file(GLOB component_files "*.c")

set(COMPONENT_SRCS ${component_files})

set(COMPONENT_REQUIRES esp_http_server)
set(COMPONENT_PRIV_REQUIRES wifi esp_http_server http_www http_ssi_nvs http_ssi_functions http_ssi_page)

register_component()
