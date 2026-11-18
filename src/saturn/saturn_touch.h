#ifndef SATURN_TOUCH_H
#define SATURN_TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pc/controller/controller_api.h"

extern struct ControllerAPI controller_saturn_touch;
void saturn_touch_render_imgui(void);

#ifdef __cplusplus
}
#endif

#endif
