#include "touch.h"
#include <Wire.h>
#include "HWCDC.h"
#include "power.h"

extern HWCDC USBSerial;

std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
  std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void Arduino_IIC_Touch_Interrupt(void);

std::unique_ptr<Arduino_IIC> FT3168(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS,
                                                       DRIVEBUS_DEFAULT_VALUE, TP_INT, Arduino_IIC_Touch_Interrupt));

void Arduino_IIC_Touch_Interrupt(void) {
  FT3168->IIC_Interrupt_Flag = true;
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  if (FT3168->IIC_Interrupt_Flag == true) {
    // Read touch coordinates only when touch is detected
    int32_t touchX = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    int32_t touchY = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);

    FT3168->IIC_Interrupt_Flag = false;
    power_reset_inactivity();
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

bool touch_has_activity() {
  if (FT3168->IIC_Interrupt_Flag) {
    FT3168->IIC_Interrupt_Flag = false;
    return true;
  }
  return false;
}

void touch_init() {
  Wire.begin(IIC_SDA, IIC_SCL);

  // Retry up to 5 times instead of infinite loop
  int retries = 5;
  while (FT3168->begin() == false && retries > 0) {
    USBSerial.println("FT3168 init fail");
    delay(500);  // Reduced delay
    retries--;
  }

  if (retries > 0) {
    USBSerial.println("FT3168 OK");
    FT3168->IIC_Write_Device_State(FT3168->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
                                   FT3168->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR);
  } else {
    USBSerial.println("FT3168 FAIL - continuing without touch");
  }
}
