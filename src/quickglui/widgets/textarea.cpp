/****
 * QuickGLUI - Quick Graphical LVLGL-based User Interface development library
 * Copyright  2020  Skurydin Alexey
 * http://github.com/anakod
 * All QuickGLUI project files are provided under the MIT License.
 ****/

#include "textarea.h"
#include "../internal/widgetmanager.h"
#include <gui/keyboard.h>

TextArea::TextArea(lv_obj_t* handle) {
  assign(handle);
}

TextArea::TextArea(const Widget* parent, const char* txt) {
  createObject(parent->handle());
  
  if (txt != NULL)
    lv_textarea_set_text(native, txt);
}

void TextArea::createObject(lv_obj_t* parent) {
    assign(lv_textarea_create(parent));
    lv_textarea_set_password_mode(native, false);
    lv_textarea_set_one_line(native, true);
    myCb = NULL;
//GDV    lv_textarea_set_cursor_hidden(native, true);
}

void TextArea::assign(lv_obj_t* newHandle)
{
    Widget::assign(newHandle);
    if (myCb == NULL)
        lv_obj_add_event_cb(native, myCb, LV_EVENT_ALL, &TextArea::Action);
}

TextArea& TextArea::text(const char * txt) {
  lv_textarea_set_text(native, txt);
  return *this;
}
const char* TextArea::text() {
  return lv_textarea_get_text(native);
}
/*GDV not allowed in LVGL >= v8
TextArea& TextArea::alignText(lv_label_align_t mode) {
  lv_label_set_align(native, mode);
  return *this;
}
*/
TextArea& TextArea::autoKeyboard(bool enable) {
  auto handle = DefaultWidgetManager.GetOrCreate(native);
  handle->SetFlag(IsAutoKeyboardDisabled, !enable);
  return *this;
}

TextArea& TextArea::digitsMode(bool onlyDigits, const char* filterDigitsList) {
  auto handle = DefaultWidgetManager.GetOrCreate(native);
  handle->SetFlag(IsDigitsOnlyMode, onlyDigits);
  if (onlyDigits)
    lv_textarea_set_accepted_chars(native, filterDigitsList);
  return *this;
}

void TextArea::Action(lv_obj_t * obj, lv_event_code_t event) {
    auto handle = DefaultWidgetManager.GetIfExists(obj);
    bool autoKeyboard = (handle == NULL /*enabled by default*/) || !handle->IsFlagSet(IsAutoKeyboardDisabled);
    bool digitsOnlyMode = (handle != NULL) && handle->IsFlagSet(IsDigitsOnlyMode);
    
    if (!autoKeyboard) return; //Nothing to do for now

    switch (event) {
        case LV_EVENT_CLICKED:
            if (digitsOnlyMode)
                num_keyboard_set_textarea(obj);
            else
                keyboard_set_textarea(obj);
            break;
        case LV_EVENT_DEFOCUSED:
            keyboard_hide();
            break;
    }
}

