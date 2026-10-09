#include "HidGamepad.h"

namespace {

constexpr uint16_t REPORT_DESC_MAX = 512;

class DescriptorBuffer : public USBReadParser {
 public:
  uint8_t data[REPORT_DESC_MAX];
  uint16_t length = 0;

  void Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset) override {
    for (uint16_t i = 0; i < len; ++i) {
      const uint32_t pos = static_cast<uint32_t>(offset) + i;
      if (pos >= REPORT_DESC_MAX) return;
      data[pos] = pbuf[i];
      if (pos + 1 > length) length = static_cast<uint16_t>(pos + 1);
    }
  }
};

DescriptorBuffer descriptor;  // static storage keeps 512 bytes off the stack

}  // namespace

uint8_t HidGamepad::OnInitSuccessful() {
  state_ = GamepadState{};
  descriptor.length = 0;

  // USBHID::GetReportDescr() caps the request at 128 bytes, too small for many pads.
  uint8_t chunk[64];
  const uint8_t rcode = pUsb->ctrlReq(bAddress, 0, bmREQ_HID_REPORT, USB_REQUEST_GET_DESCRIPTOR,
                                      0x00, HID_DESCRIPTOR_REPORT, hidInterfaces[0].bmInterface,
                                      REPORT_DESC_MAX, sizeof(chunk), chunk, &descriptor);

  // Not a gamepad (keyboard, mouse, ...): stay attached but report nothing.
  if (rcode || !parseHidReportDescriptor(descriptor.data, descriptor.length, layout_)) return 0;

  state_.connected = true;
  state_.vid = VID;
  state_.pid = PID;
  return 0;
}

void HidGamepad::ParseHIDData(USBHID *, bool, uint8_t len, uint8_t *buf) {
  if (state_.connected && buf) decodeHidReport(layout_, buf, len, state_);
}

uint8_t HidGamepad::Release() {
  state_ = GamepadState{};
  layout_ = GamepadLayout{};
  return HIDUniversal::Release();
}
