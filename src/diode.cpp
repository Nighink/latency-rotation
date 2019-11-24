#include "diode.hpp"
#include "constants.hpp"
#include <stm32l1xx_hal_flash.h>

DiodeNumber::DiodeNumber()
    : m_diodes({Diode(A0, 0), Diode(A1, 1), Diode(A2, 2), Diode(A3, 3)}) {}

int16_t DiodeNumber::read()
{
  for (uint8_t i = 0; i < 4; i++)
  {
    int16_t brightness = m_diodes[i].update();
    if (brightness >= 0)
    {
      m_read_ok |= 1 << i;
    }
    else if (m_diodes[i].time_since_last_frame > 2 * 1000 / DIODE_READ_US)
    {
      // reading becomes invalid after 2ms
      m_read_ok &= ~(1 << i);
    }
  }
  if (m_read_ok == 0xF)
  {
    // got a successful read from all photodiodes
    m_read_ok = 0;
    return calculate_number();
  }
  return -1;
}

Diode::Diode(PinName pin, uint8_t num)
    : m_pin(pin), m_black_count(0), m_above_black_count(0), m_brightness(0),
      m_brightness_second(0), m_num(num), time_since_last_frame(0),
      m_last_brightness(0)
{
#ifdef DIODE_CALIBRATION
  for (uint8_t i = 0; i < 4; i++)
  {
    m_calibration[i] = 0;
    m_calibration_min[i] = -1;
    m_calibration_max[i] = 0;
  }
#else
  load_calibration();
#endif
}

int16_t Diode::update()
{
  uint16_t brightness = m_pin.read_u16();
  time_since_last_frame += 1;
  if (brightness < m_calibration[0])
  {
    m_black_count += 1;
    if (m_black_count == 3)
    {
      if (m_above_black_count > 2)
      {
        time_since_last_frame = 0;
        m_last_brightness =
            static_cast<uint16_t>((m_brightness + m_brightness_second) / 2);
        m_brightness = 0;
        m_brightness_second = 0;
        return m_last_brightness;
      }
      m_brightness = 0;
      m_brightness_second = 0;
    }
    else if (m_black_count == 110)
    {
      time_since_last_frame = 0;
      m_black_count = 0;
      return 0;
    }
  }
  else
  {
    m_black_count = 0;
    m_above_black_count += 1;
    // m_brightness = max(m_brightness, brightness);
    if (brightness > m_brightness)
    {
      m_brightness_second = m_brightness;
      m_brightness = brightness;
    }
    else if (brightness > m_brightness_second)
    {
      m_brightness_second = brightness;
    }
  }
  return -1;
}

// #ifdef DIODE_CALIBRATION
void Diode::calibrate(uint8_t number)
{
  uint16_t val = m_pin.read_u16();
  if (val < m_calibration[0])
  {
    m_black_count += 1;
    if (m_black_count == 3 && !m_wait_for_black)
    {
      // light wave has ended
      if (m_above_black_count > 2)
      {
        m_calibration_min[number] = min(
            m_calibration_min[number],
            static_cast<uint16_t>((m_brightness + m_brightness_second) / 2));
        m_calibration_max[number] = max(
            m_calibration_max[number],
            static_cast<uint16_t>((m_brightness + m_brightness_second) / 2));
      }
      m_brightness = 0;
      m_brightness_second = 0;
      m_above_black_count = 0;
      // }
    }
    else if (m_black_count == 12 * 1000 / DIODE_READ_US)
    {
      m_black_count = 0;
    }
    m_wait_for_black = false;
  }
  else if (!m_wait_for_black)
  {
    // in the light wave
    m_black_count = 0;
    m_above_black_count += 1;
    if (val > m_brightness)
    {
      m_brightness_second = m_brightness;
      m_brightness = val;
    }
    else if (val > m_brightness_second)
    {
      m_brightness_second = val;
    }
  }
}

void Diode::next_calibration()
{
  m_wait_for_black = true;
  m_brightness = 0;
  m_brightness_second = 0;
  m_above_black_count = 0;
}

void Diode::calibrate_black()
{
  uint16_t new_black =
      static_cast<uint16_t>(static_cast<float>(m_pin.read_u16()) * 1.15f);
  m_calibration[0] = max(new_black, m_calibration[0]);
}

void Diode::calibrate_reset()
{
  for (uint8_t i = 0; i < 4; i++)
  {
    m_calibration[i] = 0;
    m_calibration_min[i] = -1;
    m_calibration_max[i] = 0;
  }
}

// see https://os.mbed.com/forum/mbed/topic/4912/?page=1#comment-27406
void Diode::save_calibration()
{
  m_calibration[1] = (m_calibration_min[1] + m_calibration_max[0]) / 2;
  m_calibration[2] = (m_calibration_min[2] + m_calibration_max[1]) / 2;
  m_calibration[3] = (m_calibration_min[3] + m_calibration_max[2]) / 2;
  HAL_StatusTypeDef status;
  uint32_t address = 0x08080000 + m_num * 2 * sizeof(uint32_t);
  uint32_t *data = reinterpret_cast<uint32_t *>(m_calibration);
  HAL_FLASHEx_DATAEEPROM_Unlock(); // Unprotect the EEPROM to allow writing
  status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, address,
                                          data[0]);
  status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD,
                                          address + sizeof(uint32_t), data[1]);
  HAL_FLASHEx_DATAEEPROM_Lock(); // Reprotect the EEPROM
}

void Diode::print_calibration(Serial &pc)
{
  pc.printf(
      "{\"min\": [%d, %d, %d, %d], \"max\":[%d, %d, %d, %d], \"black\": %d}",
      m_calibration_min[0], m_calibration_min[1], m_calibration_min[2],
      m_calibration_min[3], m_calibration_max[0], m_calibration_max[1],
      m_calibration_max[2], m_calibration_max[3], m_calibration[0]);
}

void Diode::print_calibration_plain(Serial &pc)
{
  for (uint8_t i = 0; i < 4; i++)
  {
    pc.printf("cal%d%d-%d-%d\r\n", m_num, i, m_calibration_min[i],
              m_calibration_max[i]);
  }
}
// #endif

void Diode::load_calibration()
{
  uint32_t address = 0x08080000 + m_num * 2 * sizeof(uint32_t);
  __IO uint16_t *data = reinterpret_cast<__IO uint16_t *>(address);
  for (uint8_t i = 0; i < 4; i++)
  {
    m_calibration[i] = data[i];
  }
}

void Diode::print_loaded_calibration(Serial &pc)
{
  pc.printf("{\"num\":%d,\"0\":%d,\"1\":%d,\"2\":%d,\"3\":%d}\r\n", m_num,
            m_calibration[0], m_calibration[1], m_calibration[2],
            m_calibration[3]);
}

int16_t DiodeNumber::calculate_number()
{
  uint16_t num = 0;
  for (uint8_t i = 0; i < 4; i++)
  {
    uint16_t brightness = m_diodes[i].m_last_brightness;
    if (brightness <= m_diodes[i].m_calibration[1])
    {
      // 0 = nothing to add
    }
    else if (brightness <= m_diodes[i].m_calibration[2])
    {
      // 1
      num += 1 << (i * 2);
    }
    else if (brightness <= m_diodes[i].m_calibration[3])
    {
      // 2
      num += 2 << (i * 2);
    }
    else
    {
      // 3
      num += 3 << (i * 2);
    }
  }
  return num;
}
