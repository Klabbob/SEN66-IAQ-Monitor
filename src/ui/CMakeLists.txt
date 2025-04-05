SET(SOURCES ui_comp_tile.c
    ui_comp.c
    ui_MainScreen.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    fonts/ui_font_Arial_Black_22.c
    fonts/ui_font_Arial_Bold_13.c)

add_library(ui ${SOURCES})
