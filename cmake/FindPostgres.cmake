execute_process(
        COMMAND pg_config --pkglibdir
        OUTPUT_VARIABLE PostgreSQL_PKGLIBDIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND pg_config --sharedir
        OUTPUT_VARIABLE PostgreSQL_SHAREDIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND pg_config --includedir-server
        OUTPUT_VARIABLE PostgreSQL_INCLUDE_SERVERDIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)