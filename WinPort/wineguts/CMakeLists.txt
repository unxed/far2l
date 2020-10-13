
set(SOURCES
casemap.c
collation.c
compose.c
cpsymbol.c
decompose.c
locale.c
sortkey.c
utf8.c
wctype.c
)

add_library (wineguts ${SOURCES})

