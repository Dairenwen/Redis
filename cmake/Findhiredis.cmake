find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_HIREDIS REQUIRED IMPORTED_TARGET hiredis)

add_library(hiredis::hiredis ALIAS PkgConfig::PC_HIREDIS)

set(hiredis_FOUND TRUE)
