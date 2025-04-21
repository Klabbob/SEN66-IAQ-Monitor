SET(SOURCES ui_comp_tile.c
    ui_comp.c
    ui_FRCScreen.c
    ui_AltitudeScreen.c
    ui_ChartTimeScreen.c
    ui_BrightnessScreen.c
    ui_theme_manager.cpp
    ui_themes.cpp
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_816914973.c
    ui_img_810620936.c
    ui_img_718623388.c
    ui_img_1346862293.c
    ui_img_blank_png.c
    fonts/ui_font_Arial_Black_18.c
    fonts/ui_font_Arial_Black_22.c
    fonts/ui_font_Arial_Bold_13.c)

add_library(ui ${SOURCES})
