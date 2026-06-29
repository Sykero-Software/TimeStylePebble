#ifndef PBL_PLATFORM_APLITE
#include <pebble.h>

#include "clock_area.h"
#include "settings.h"
#include "sidebar.h"
#include "clock_analog.h"

#include <pebble-fctx/fctx.h>
#include <pebble-fctx/fpath.h>
#include <pebble-fctx/ffont.h>

#define ROUND_VERTICAL_PADDING 15

// Digital time line below the analog circle. Hidden when the slack band under the
// circle is shorter than MIN_BAND (also what makes it vanish under a status strip /
// notification — apply_twt_layout shortens the clock frame, collapsing the band).
#define ANALOG_DIGITAL_MIN_BAND 10
#define ANALOG_DIGITAL_MAX_EM   28

char time_hours[3];
char time_minutes[3];

Layer* clock_area_layer;
FFont* hours_font;
FFont* minutes_font;

static int s_clock_hours;
static int s_clock_minutes;

// just allocate all the fonts at startup because i don't feel like
// dealing with allocating and deallocating things
FFont* avenir;
FFont* avenir_bold;
FFont* leco;

GRect screen_rect;

// "private" functions
void update_fonts() {
  switch(settings.clockFontId) {
    case FONT_SETTING_DEFAULT:
        hours_font = avenir;
        minutes_font = avenir;
      break;
    case FONT_SETTING_BOLD:
        hours_font = avenir_bold;
        minutes_font = avenir_bold;
      break;
    case FONT_SETTING_BOLD_H:
        hours_font = avenir_bold;
        minutes_font = avenir;
      break;
    case FONT_SETTING_BOLD_M:
        hours_font = avenir;
        minutes_font = avenir_bold;
      break;
    case FONT_SETTING_LECO:
        hours_font = leco;
        minutes_font = leco;
      break;
  }
}

// Draw a single-line HH:MM in the band [band_top, band_top+band_h) of the clock
// layer's local coordinate space, centred. FCTX rasterises in absolute screen
// coordinates and ignores the layer frame origin, so add it back manually.
static void draw_digital_below(GContext* ctx, Layer* l, GRect bounds, int band_top, int band_h) {
  // Build "H:MM" / "HH:MM"; trim the leading space %l/%k emit without leading zero.
  const char* h = time_hours;
  while (*h == ' ') { h++; }
  char buf[8];
  snprintf(buf, sizeof(buf), "%s:%s", h, time_minutes);

  FContext fctx;
  fctx_init_context(&fctx, ctx);
  fctx_set_color_bias(&fctx, 0);
  fctx_set_fill_color(&fctx, settings.timeColor);

  #ifdef PBL_COLOR
    fctx_enable_aa(settings.clockFontId != FONT_SETTING_LECO);
  #endif

  int em = band_h - 2;
  if (em > ANALOG_DIGITAL_MAX_EM) { em = ANALOG_DIGITAL_MAX_EM; }

  int h_adjust = layer_get_frame(l).origin.x;
  int v_adjust = layer_get_frame(l).origin.y;

  FPoint pos;
  fctx_begin_fill(&fctx);
  fctx_set_text_em_height(&fctx, hours_font, em);
  pos.x = INT_TO_FIXED(bounds.size.w / 2 + h_adjust);
  pos.y = INT_TO_FIXED(band_top + band_h / 2 + v_adjust);
  fctx_set_offset(&fctx, pos);
  fctx_draw_string(&fctx, buf, hours_font, GTextAlignmentCenter, FTextAnchorMiddle);
  fctx_end_fill(&fctx);

  fctx_deinit_context(&fctx);
}

void update_clock_area_layer(Layer *l, GContext* ctx) {
  // check layer bounds
  GRect bounds = layer_get_unobstructed_bounds(l);

  #ifdef PBL_ROUND
    bounds = GRect(0, ROUND_VERTICAL_PADDING, screen_rect.size.w, screen_rect.size.h - ROUND_VERTICAL_PADDING * 2);
  #endif

  #ifndef PBL_ROUND
  if (settings.clockStyle == CLOCK_STYLE_ANALOG) {
    ClockAnalog_draw(ctx, bounds, s_clock_hours, s_clock_minutes,
                     settings.timeColor, settings.timeBgColor, settings.analogTickStyle);

    // Optional single-line digital time in the unused band below the circle.
    // Circle diameter is min(w,h); below the circle there are (h/2 - half) px of
    // slack (0 when the area is square or wider than tall). Drawn only when that
    // band is tall enough to be legible; otherwise omitted (auto-hides under a
    // status strip / notification, which shrinks `bounds`).
    if (settings.analogDigitalClock) {
      int half = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2;
      int band_top = bounds.size.h / 2 + half;     // circle bottom, layer-local y
      int band_h = bounds.size.h - band_top;        // == h/2 - half
      if (band_h >= ANALOG_DIGITAL_MIN_BAND) {
        draw_digital_below(ctx, l, bounds, band_top, band_h);
      }
    }
    return;
  }
  #endif

  // initialize FCTX, the fancy 3rd party drawing library that all the cool kids use
  FContext fctx;

  fctx_init_context(&fctx, ctx);
  fctx_set_color_bias(&fctx, 0);
  fctx_set_fill_color(&fctx, settings.timeColor);


  // calculate font size
  int font_size = 4 * bounds.size.h / 7;

  // avenir + avenir bold metrics
  int v_padding = bounds.size.h / 16;
  int h_adjust = 0;
  int v_adjust = 0;

  // alternate metrics for LECO
  if(settings.clockFontId == FONT_SETTING_LECO) {
    font_size = 4 * bounds.size.h / 7 + 6;
    v_padding = bounds.size.h / 20;
    h_adjust = -4;
    v_adjust = 0;

    // leco looks awful with antialiasing
    #ifdef PBL_COLOR
      fctx_enable_aa(false);
    #endif
  } else {
    #ifdef PBL_COLOR
      fctx_enable_aa(true);
    #endif
  }

  // if it's a round watch, EVERYTHING CHANGES
  #ifdef PBL_ROUND
    v_adjust = ROUND_VERTICAL_PADDING;

    if(settings.clockFontId != FONT_SETTING_LECO) {
      h_adjust = -1;
    }
  #else
    // apply_twt_layout() owns the clock frame on rectangular (non-aplite)
    // platforms, insetting it horizontally between the sidebar(s). FCTX ignores
    // the layer frame origin, so add it back manually (mirrors the vertical
    // shift below). No per-sidebar nudge is needed — the frame already excludes
    // the sidebar(s), including the optional secondary panel.
    h_adjust += layer_get_frame(l).origin.x;
  #endif

  // FCTX rasterises directly into the framebuffer using absolute screen
  // coordinates and ignores the layer's frame origin, so the clock would not
  // follow the layer when apply_twt_layout() moves it down to reserve a top
  // strip for the date header (it would only shrink, then overlap the date).
  // Shift drawing down by the layer's vertical offset manually.
  v_adjust += layer_get_frame(l).origin.y;

  FPoint time_pos;
  fctx_begin_fill(&fctx);
  fctx_set_text_em_height(&fctx, hours_font, font_size);
  fctx_set_text_em_height(&fctx, minutes_font, font_size);

  // draw hours
  time_pos.x = INT_TO_FIXED(bounds.size.w / 2 + h_adjust);
  time_pos.y = INT_TO_FIXED(v_padding + v_adjust);
  fctx_set_offset(&fctx, time_pos);
  fctx_draw_string(&fctx, time_hours, hours_font, GTextAlignmentCenter, FTextAnchorTop);

  //draw minutes
  time_pos.y = INT_TO_FIXED(bounds.size.h - v_padding + v_adjust);
  fctx_set_offset(&fctx, time_pos);
  fctx_draw_string(&fctx, time_minutes, minutes_font, GTextAlignmentCenter, FTextAnchorBaseline);
  fctx_end_fill(&fctx);

  fctx_deinit_context(&fctx);
}


void ClockArea_init(Window* window) {
  // record the screen size, since we NEVER GET IT AGAIN
  screen_rect = layer_get_bounds(window_get_root_layer(window));

  GRect bounds;
  bounds = GRect(0, 0, screen_rect.size.w, screen_rect.size.h);

  // init the clock area layer
  clock_area_layer = layer_create(bounds);
  layer_add_child(window_get_root_layer(window), clock_area_layer);
  layer_set_update_proc(clock_area_layer, update_clock_area_layer);

  // allocate fonts
  avenir =      ffont_create_from_resource(RESOURCE_ID_AVENIR_REGULAR_FFONT);
  avenir_bold = ffont_create_from_resource(RESOURCE_ID_AVENIR_BOLD_FFONT);
  leco =        ffont_create_from_resource(RESOURCE_ID_LECO_REGULAR_FFONT);

  // select fonts based on settings
  update_fonts();
}

void ClockArea_deinit() {
  layer_destroy(clock_area_layer);

  ffont_destroy(avenir);
  ffont_destroy(avenir_bold);
  ffont_destroy(leco);
}

void ClockArea_redraw() {
  // check if the fonts need to be switched
  update_fonts();

  layer_mark_dirty(clock_area_layer);
}

void ClockArea_update_time(struct tm* time_info) {

  // hours
  if (clock_is_24h_style()) {
    strftime(time_hours, sizeof(time_hours), (settings.showLeadingZero) ? "%H" : "%k", time_info);
  } else {
    strftime(time_hours, sizeof(time_hours), (settings.showLeadingZero) ? "%I" : "%l", time_info);
  }

  // minutes
  strftime(time_minutes, sizeof(time_minutes), "%M", time_info);

  // raw values for the analog renderer (analog uses hours % 12, ignores 12/24h)
  s_clock_hours = time_info->tm_hour;
  s_clock_minutes = time_info->tm_min;
}

#endif