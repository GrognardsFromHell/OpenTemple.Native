
function (exclude_qt_plugins)
    qt_import_plugins(${ARGV0}
            EXCLUDE_BY_TYPE imageformats
            EXCLUDE_BY_TYPE platforms
            EXCLUDE_BY_TYPE iconengines
            EXCLUDE_BY_TYPE qmltooling
            EXCLUDE_BY_TYPE generic
            EXCLUDE_BY_TYPE bearer
            )
endfunction()
