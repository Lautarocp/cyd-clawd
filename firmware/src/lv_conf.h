#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Color depth: 16 = RGB565 */
#define LV_COLOR_DEPTH 16

/* Memoria interna LVGL (~48 KB) */
#define LV_USE_STDLIB_MALLOC    0   /* 0 = LV_STDLIB_BUILTIN */
#define LV_USE_STDLIB_STRING    0
#define LV_USE_STDLIB_SPRINTF   0
#define LV_MEM_SIZE (48 * 1024U)
#define LV_MEM_ADR  0

/* Refresh y lectura de input */
#define LV_DEF_REFR_PERIOD      16
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_DPI_DEF 130

/* Sin logging ni profiler */
#define LV_USE_LOG      0
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*==========================
 *  FUENTES MONTSERRAT
 *==========================*/
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*==========================
 *  WIDGETS (nombres LVGL 9.x)
 *==========================*/
#define LV_USE_ANIMIMAGE    0   /* requiere LV_USE_IMAGE — lo deshabilitamos */
#define LV_USE_ARC          0
#define LV_USE_BAR          1
#define LV_USE_BUTTON       0
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       0
#define LV_USE_CHART        0
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     0
#define LV_USE_IMAGE        1   /* base widget, necesario para deps internas */
#define LV_USE_IMAGEBUTTON  0
#define LV_USE_KEYBOARD     0
#define LV_USE_LABEL        1
#define LV_USE_LED          0
#define LV_USE_LINE         0
#define LV_USE_LIST         0
#define LV_USE_MENU         0
#define LV_USE_METER        0
#define LV_USE_MSGBOX       0
#define LV_USE_ROLLER       0
#define LV_USE_SCALE        0
#define LV_USE_SLIDER       0
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      0
#define LV_USE_SWITCH       0
#define LV_USE_TABLE        0
#define LV_USE_TABVIEW      0
#define LV_USE_TEXTAREA     0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0

/*==========================
 *  TEMAS
 *==========================*/
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_SIMPLE  1
#define LV_USE_THEME_MONO    0

/*==========================
 *  LAYOUTS
 *==========================*/
#define LV_USE_FLEX 0
#define LV_USE_GRID 0

/*==========================
 *  MISC — todo apagado
 *==========================*/
#define LV_USE_SNAPSHOT     0
#define LV_USE_SYSMON       0
#define LV_USE_PROFILER     0
#define LV_USE_MONKEY       0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0
#define LV_USE_OBSERVER     0
#define LV_USE_IME_PINYIN   0
#define LV_USE_FILE_EXPLORER 0
#define LV_USE_LODEPNG      0
#define LV_USE_LIBPNG       0
#define LV_USE_BMP          0
#define LV_USE_TJPGD        0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF          0
#define LV_USE_QRCODE       0
#define LV_USE_BARCODE      0
#define LV_USE_FFMPEG       0
#define LV_USE_FREETYPE     0
#define LV_USE_TINY_TTF     0
#define LV_USE_RLOTTIE      0
#define LV_USE_VECTOR_GRAPHIC 0

#endif /* LV_CONF_H */
