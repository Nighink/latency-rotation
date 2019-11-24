#ifndef DIODE_HPP
#define DIODE_HPP

#include <mbed.h>

class Diode {

  /**
   * The pin which the photodiode connects to
   */
  AnalogIn m_pin;
  /**
   * Counter how many black readings there were in a row
   */
  uint8_t m_black_count;

  /**
   * Counter how many readings above the black
   * threshold there were to sort out misreadings
   */
  uint8_t m_above_black_count;

  /**
   * Max brightness since the last black
   */
  uint16_t m_brightness;

  /**
   * Second biggest brightness since the last black
   * Used to filter the signal and get more reliable
   * readings.
   */
  uint16_t m_brightness_second;

  /**
   * Number of the Diode to find the saved
   * calibration values
   */
  uint8_t m_num;

public:
  Diode(PinName pin, uint8_t num);

  /**
   * Reads the current value of the photodiode
   * and updates the brightness information
   *
   * @return the brightness value of the frame
   *    or -1 if the frame hasn't ended yet
   */
  int16_t update();


  /**
   * The amount of times `update` was called since
   * the last detected frame brightness.
   *
   * Used to associate brightness readings
   * of the different photodiodes with one another
   * to be sure they are from the same frame.
   */
  uint8_t time_since_last_frame;

  /**
   * Max brightness of previous light wave
   */
  uint16_t m_last_brightness;

  /**
   * Calibration values
   *
   * The brightness values to distinguish numbers
   */
  uint16_t m_calibration[4];

// #ifdef DIODE_CALIBRATION
#pragma region calibration
  /**
   * Tells the diode that the current brightness
   * value is to be interpreted as black.
   */
  void calibrate_black();

  /**
   * Resets some values so that previous calibrated
   * numbers don't influence the next measurement
   */
  void next_calibration();

  /**
   * Resets the calibration to allow another calibration round
   */
  void calibrate_reset();

  /**
   * Minimum brightness value found for a flank for the respective number
   */
  uint16_t m_calibration_min[4];
  /**
   * Maximum brightness value found for a flank for the respective number
   */
  uint16_t m_calibration_max[4];
  /**
   * Write calibration to the EEPROM
   */
  void save_calibration();
  /**
   * waiting for black to prevent using the last part of a frame 
   * to count if the measurement for a new number starts
   */
  bool m_wait_for_black;
  /**
   * Calibrates the diode by telling which number should
   * be encoded with the shown brightness
   */
  void calibrate(uint8_t number);
  /**
   * Prints the calibration to the serial
   */
  void print_calibration(Serial& pc);
  /**
   * Prints the calibration to the serial
   * in a reduced way
   */
  void print_calibration_plain(Serial& pc);
#pragma endregion calibration
// #endif

  /**
   * Loads calibration from the EEPROM
   */
  void load_calibration();

  /**
   * Prints the calibration that is used in the proper runs.
   * Needs to be called after `load_calibration()`
   */
  void print_loaded_calibration(Serial& pc);
};

class DiodeNumber {

  Diode m_diodes[4];
  /**
   * Bitfield to save which diode
   * has reported a value in the last time
   */
  uint8_t m_read_ok;

  /**
   * Helper function to translate the brightness readings
   * into a number
   */
  int16_t calculate_number();

public:
  DiodeNumber();
  /**
   * Reads the current photodiode setting
   * Assumes to get called every 0.1ms
   *
   * @return [0, 255] if a number was read else -1
   */
  int16_t read();

};

#endif