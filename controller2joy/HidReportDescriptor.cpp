#include "HidReportDescriptor.h"

namespace {

constexpr uint16_t PAGE_GENERIC_DESKTOP = 0x01;
constexpr uint16_t PAGE_BUTTON = 0x09;
constexpr uint16_t USAGE_X = 0x30;  // X, Y, Z, Rx, Ry, Rz are consecutive
constexpr uint16_t USAGE_RZ = 0x35;
constexpr uint16_t USAGE_HAT = 0x39;

constexpr uint8_t MAX_USAGES = 16;
constexpr uint8_t MAX_REPORT_IDS = 8;
constexpr uint8_t MAX_PUSH_DEPTH = 4;

enum : uint8_t { D_X, D_Y, D_Z, D_RX, D_RY, D_RZ, D_COUNT };

int32_t signExtend(uint32_t v, uint8_t bytes) {
  if (bytes == 1) return static_cast<int8_t>(v);
  if (bytes == 2) return static_cast<int16_t>(v);
  return static_cast<int32_t>(v);
}

struct GlobalState {
  uint16_t usagePage = 0;
  int32_t logicalMin = 0;
  uint32_t logicalMaxRaw = 0;
  uint8_t logicalMaxBytes = 0;
  uint8_t reportSize = 0;
  uint8_t reportId = 0;
  uint16_t reportCount = 0;

  // Logical Maximum is only signed when Logical Minimum is negative.
  int32_t logicalMax() const {
    return logicalMin < 0 ? signExtend(logicalMaxRaw, logicalMaxBytes)
                          : static_cast<int32_t>(logicalMaxRaw);
  }
};

class Parser {
 public:
  explicit Parser(GamepadLayout &out) : out_(out) {}

  bool run(const uint8_t *desc, uint16_t len) {
    uint16_t i = 0;
    while (i < len) {
      const uint8_t prefix = desc[i++];
      if (prefix == 0xFE) {  // long item: skip
        if (i >= len) break;
        i += 2 + desc[i];
        continue;
      }
      uint8_t size = prefix & 0x03;
      if (size == 3) size = 4;
      if (i + size > len) break;
      uint32_t data = 0;
      for (uint8_t k = 0; k < size; ++k) data |= static_cast<uint32_t>(desc[i + k]) << (8 * k);
      i += size;
      item(prefix & 0xFC, data, size);
    }
    finish();
    return out_.usable();
  }

 private:
  void item(uint8_t tag, uint32_t data, uint8_t size) {
    switch (tag) {
      // Main items
      case 0x80: input(data); clearLocals(); break;
      case 0x90:  // Output
      case 0xB0:  // Feature
      case 0xA0:  // Collection
      case 0xC0:  // End Collection
        clearLocals();
        break;
      // Global items
      case 0x04: g_.usagePage = static_cast<uint16_t>(data); break;
      case 0x14: g_.logicalMin = signExtend(data, size); break;
      case 0x24: g_.logicalMaxRaw = data; g_.logicalMaxBytes = size; break;
      case 0x74: g_.reportSize = static_cast<uint8_t>(data); break;
      case 0x84: g_.reportId = static_cast<uint8_t>(data); out_.usesReportIds = true; break;
      case 0x94: g_.reportCount = static_cast<uint16_t>(data); break;
      case 0xA4: if (depth_ < MAX_PUSH_DEPTH) stack_[depth_++] = g_; break;
      case 0xB4: if (depth_ > 0) g_ = stack_[--depth_]; break;
      // Local items
      case 0x08: if (usageCount_ < MAX_USAGES) usages_[usageCount_++] = fullUsage(data, size); break;
      case 0x18: usageMin_ = fullUsage(data, size); haveMin_ = true; break;
      case 0x28: usageMax_ = fullUsage(data, size); haveMax_ = true; break;
      default: break;
    }
  }

  uint32_t fullUsage(uint32_t data, uint8_t size) const {
    return size == 4 ? data : (static_cast<uint32_t>(g_.usagePage) << 16) | data;
  }

  void clearLocals() {
    usageCount_ = 0;
    haveMin_ = haveMax_ = false;
  }

  uint32_t usageAt(uint16_t index) const {
    if (haveMin_ && haveMax_) {
      const uint32_t u = usageMin_ + index;
      return u <= usageMax_ ? u : 0;
    }
    if (usageCount_ == 0) return 0;
    return usages_[index < usageCount_ ? index : usageCount_ - 1];
  }

  uint16_t *offsetFor(uint8_t id) {
    for (uint8_t k = 0; k < offsetCount_; ++k)
      if (offsets_[k].id == id) return &offsets_[k].bits;
    if (offsetCount_ == MAX_REPORT_IDS) return nullptr;
    offsets_[offsetCount_] = {id, 0};
    return &offsets_[offsetCount_++].bits;
  }

  void input(uint32_t flags) {
    uint16_t *offset = offsetFor(g_.reportId);
    if (!offset) return;
    const bool constant = flags & 0x01;
    const bool variable = flags & 0x02;
    for (uint16_t n = 0; n < g_.reportCount; ++n) {
      if (!constant && variable && g_.reportSize <= 32) {
        HidField f;
        f.valid = true;
        f.reportId = g_.reportId;
        f.bitOffset = *offset;
        f.bitSize = g_.reportSize;
        f.logicalMin = g_.logicalMin;
        f.logicalMax = g_.logicalMax();
        assign(usageAt(n), f);
      }
      *offset += g_.reportSize;
    }
  }

  void assign(uint32_t usage, const HidField &f) {
    const uint16_t page = usage >> 16;
    const uint16_t id = usage & 0xFFFF;
    if (page == PAGE_GENERIC_DESKTOP) {
      if (id >= USAGE_X && id <= USAGE_RZ) {
        HidField &d = desktop_[id - USAGE_X];
        if (!d.valid) d = f;
      } else if (id == USAGE_HAT && !out_.hat.valid) {
        out_.hat = f;
      }
    } else if (page == PAGE_BUTTON && id >= 1 && id <= HID_MAX_BUTTONS) {
      HidField &b = out_.button[id - 1];
      if (!b.valid) {
        b = f;
        if (id > out_.buttonCount) out_.buttonCount = static_cast<uint8_t>(id);
      }
    }
  }

  void finish() {
    out_.axis[AXIS_LX] = desktop_[D_X];
    out_.axis[AXIS_LY] = desktop_[D_Y];
    // Right stick is usually Z/Rz (DirectInput) or Rx/Ry (Xbox-style HID).
    const bool zrz = desktop_[D_Z].valid && desktop_[D_RZ].valid;
    out_.axis[AXIS_RX] = zrz ? desktop_[D_Z] : desktop_[D_RX];
    out_.axis[AXIS_RY] = zrz ? desktop_[D_RZ] : desktop_[D_RY];
  }

  GamepadLayout &out_;
  GlobalState g_;
  GlobalState stack_[MAX_PUSH_DEPTH];
  uint8_t depth_ = 0;
  uint32_t usages_[MAX_USAGES] = {};
  uint8_t usageCount_ = 0;
  uint32_t usageMin_ = 0, usageMax_ = 0;
  bool haveMin_ = false, haveMax_ = false;
  struct { uint8_t id; uint16_t bits; } offsets_[MAX_REPORT_IDS] = {};
  uint8_t offsetCount_ = 0;
  HidField desktop_[D_COUNT];
};

bool readField(const HidField &f, const uint8_t *data, uint16_t len, int32_t &value) {
  if ((static_cast<uint32_t>(f.bitOffset) + f.bitSize + 7) / 8 > len) return false;
  uint32_t v = 0;
  for (uint8_t b = 0; b < f.bitSize; ++b) {
    const uint16_t bit = f.bitOffset + b;
    if ((data[bit >> 3] >> (bit & 7)) & 1) v |= 1UL << b;
  }
  if (f.logicalMin < 0 && f.bitSize < 32 && (v & (1UL << (f.bitSize - 1))))
    v |= ~0UL << f.bitSize;
  value = static_cast<int32_t>(v);
  return true;
}

uint8_t normalise(int32_t v, const HidField &f) {
  if (f.logicalMax <= f.logicalMin) return 128;
  if (v < f.logicalMin) v = f.logicalMin;
  if (v > f.logicalMax) v = f.logicalMax;
  return static_cast<uint8_t>((static_cast<int64_t>(v) - f.logicalMin) * 255 /
                              (static_cast<int64_t>(f.logicalMax) - f.logicalMin));
}

const int8_t HAT_X[8] = {0, 1, 1, 1, 0, -1, -1, -1};
const int8_t HAT_Y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

}  // namespace

bool parseHidReportDescriptor(const uint8_t *desc, uint16_t len, GamepadLayout &layout) {
  layout = GamepadLayout{};
  Parser parser(layout);
  return parser.run(desc, len);
}

void decodeHidReport(const GamepadLayout &layout, const uint8_t *report, uint16_t len,
                     GamepadState &state) {
  uint8_t id = 0;
  if (layout.usesReportIds) {
    if (len == 0) return;
    id = report[0];
    ++report;
    --len;
  }
  int32_t v;
  for (uint8_t a = 0; a < AXIS_COUNT; ++a) {
    const HidField &f = layout.axis[a];
    if (f.valid && f.reportId == id && readField(f, report, len, v)) state.axis[a] = normalise(v, f);
  }
  for (uint8_t b = 0; b < layout.buttonCount; ++b) {
    const HidField &f = layout.button[b];
    if (!f.valid || f.reportId != id || !readField(f, report, len, v)) continue;
    if (v) state.buttons |= 1UL << b;
    else state.buttons &= ~(1UL << b);
  }
  const HidField &h = layout.hat;
  if (h.valid && h.reportId == id && readField(h, report, len, v)) {
    const int32_t range = h.logicalMax - h.logicalMin;
    if (v < h.logicalMin || v > h.logicalMax || (range != 7 && range != 3)) {
      state.hatX = state.hatY = 0;  // null state = centred
    } else {
      const uint8_t dir = static_cast<uint8_t>((v - h.logicalMin) * (range == 3 ? 2 : 1));
      state.hatX = HAT_X[dir];
      state.hatY = HAT_Y[dir];
    }
  }
}
