#pragma once
//
// Page-cycling sensor dashboard UI for a 128x32 1-bit SSD1306.
// Pure drawing + layout only: no id()/millis() in here. The display lambda
// reads the sensors and passes plain values in via Readings. Included via
// `esphome: includes:`.
//
#include "esphome/components/display/display.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace dash {

// One screen's worth of sensor values. NaN = "not available yet".
struct Readings {
  float temp_c;
  float humidity;       // %
  float pressure_hpa;   // hPa
  float iaq;            // BSEC2 IAQ index 0..500
  const char *iaq_accuracy;  // BSEC2 calibration status text ("" if none)
  float co2_eq;         // eCO2 ppm
  float voc_eq;         // bVOC ppm
  float gas_ohm;        // raw gas resistance, ohms
  float people;         // radar target count
  float distance_mm;    // radar target-1 distance, mm
};

static const int PAGES = 8;

// IAQ index -> short quality word (kept short so it fits next to the number).
inline const char *iaq_word(float q) {
  if (q <= 50)  return "Top";
  if (q <= 100) return "Good";
  if (q <= 150) return "Fair";
  if (q <= 200) return "Poor";
  if (q <= 300) return "Bad";
  return "Sev";
}

// Draw the whole dashboard for the current millis() tick: picks the page,
// formats label + value, and renders label (top-left), page number
// (top-right), big value, and the bottom progress bar.
inline void draw(esphome::display::Display &it, uint32_t now,
                 const Readings &r,
                 esphome::display::BaseFont *big,
                 esphome::display::BaseFont *small) {
  const uint32_t PAGE_MS = 3000;                 // ~3s per metric
  int   page = (now / PAGE_MS) % PAGES;
  float prog = (now % PAGE_MS) / (float) PAGE_MS;  // 0..1 within page

  char label[24];
  char val[24];
  switch (page) {
    case 0:  // temperature
      snprintf(label, sizeof(label), "TEMPERATURE");
      if (std::isnan(r.temp_c)) snprintf(val, sizeof(val), "-- C");
      else                      snprintf(val, sizeof(val), "%.1f C", r.temp_c);
      break;
    case 1:  // humidity
      snprintf(label, sizeof(label), "HUMIDITY");
      if (std::isnan(r.humidity)) snprintf(val, sizeof(val), "-- %%");
      else                        snprintf(val, sizeof(val), "%.0f %%", r.humidity);
      break;
    case 2:  // pressure
      snprintf(label, sizeof(label), "PRESSURE");
      if (std::isnan(r.pressure_hpa)) snprintf(val, sizeof(val), "-- hPa");
      else                            snprintf(val, sizeof(val), "%.0f hPa", r.pressure_hpa);
      break;
    case 3: {  // air quality (BSEC2 IAQ) + word + calibration status
      const char *acc = r.iaq_accuracy;
      if (acc == nullptr || acc[0] == '\0') snprintf(label, sizeof(label), "AIR QUALITY");
      else                                  snprintf(label, sizeof(label), "AIR %s", acc);
      bool stabilizing = (acc != nullptr && std::strcmp(acc, "Stabilizing") == 0);
      if (stabilizing || std::isnan(r.iaq)) {
        // BSEC2 not calibrated yet -> IAQ is a fixed placeholder, not real.
        snprintf(val, sizeof(val), "warming");
      } else {
        snprintf(val, sizeof(val), "%.0f %s", r.iaq, iaq_word(r.iaq));
      }
      break;
    }
    case 4:  // CO2 equivalent
      snprintf(label, sizeof(label), "CO2 EQUIV");
      if (std::isnan(r.co2_eq)) snprintf(val, sizeof(val), "-- ppm");
      else                      snprintf(val, sizeof(val), "%.0f ppm", r.co2_eq);
      break;
    case 5:  // breath VOC equivalent
      snprintf(label, sizeof(label), "VOC EQUIV");
      if (std::isnan(r.voc_eq)) snprintf(val, sizeof(val), "-- ppm");
      else                      snprintf(val, sizeof(val), "%.1f ppm", r.voc_eq);
      break;
    case 6:  // raw gas resistance
      snprintf(label, sizeof(label), "GAS RESIST");
      if (std::isnan(r.gas_ohm)) snprintf(val, sizeof(val), "-- k");
      else                       snprintf(val, sizeof(val), "%.0f k", r.gas_ohm / 1000.0f);
      break;
    default:  // presence / radar
      snprintf(label, sizeof(label), "PRESENCE");
      if (std::isnan(r.people) || r.people <= 0) snprintf(val, sizeof(val), "clear");
      else if (std::isnan(r.distance_mm))        snprintf(val, sizeof(val), "%.0f ppl", r.people);
      else snprintf(val, sizeof(val), "%.0f @ %.1fm", r.people, r.distance_mm / 1000.0f);
      break;
  }

  it.printf(2, 0, small, "%s", label);                  // label, top-left
  it.printf(127, 0, small, esphome::display::TextAlign::TOP_RIGHT,
            "%d/%d", page + 1, PAGES);                  // page number, top-right
  it.printf(4, 11, big, "%s", val);                     // big value
  it.filled_rectangle(0, 31, (int) (prog * 128), 1);    // bottom progress bar
}

}  // namespace dash
