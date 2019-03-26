set(COMPONENT_ADD_INCLUDEDIRS .)

file(GLOB component_files "*.c")
set(COMPONENT_SRCS ${component_files})

set(COMPONENT_PRIV_REQUIRES http)

file(GLOB txt_files "*.html" "*.css" "*.js")
set(COMPONENT_EMBED_TXTFILES ${txt_files})

register_component()
