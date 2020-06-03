// Full-featured library for Si5351
// version 1.0
// (c) Andrew Bilokon, UR5FFR
// mailto:ban.relayer@gmail.com
// http://dspview.com
// https://github.com/andrey-belokon

#ifndef SI5351A_H
#define SI5351A_H

#include <inttypes.h>

#define SI5351_CLK_DRIVE_2MA  0
#define SI5351_CLK_DRIVE_4MA  1
#define SI5351_CLK_DRIVE_6MA  2
#define SI5351_CLK_DRIVE_8MA  3

/*
 * Feequency plan:
 * CLK0 - PLL_A, multisynth integer
 * CLK1 - PLL_B, multisynth integer
 * CLK2 - PLL_B, multisynth integer or fractional
 * if CLK1 == 0 --> CLK2 - PLL_B, multisynth integer
 */
 
class Si5351 {
  private:
    uint16_t freq_div[3] = {0,0,0};
    uint8_t freq_rdiv[3] = {0,0,0};
    uint8_t power[3] = {SI5351_CLK_DRIVE_8MA,SI5351_CLK_DRIVE_8MA,SI5351_CLK_DRIVE_8MA};
    uint32_t freq[3] = {0,0,0};
    uint32_t xtal_freq, freq_pll_b;
    uint8_t need_reset_pll;

    static uint32_t VCOFreq_Mid; 
    
    void si5351_setup_msynth(uint8_t synth, uint32_t pll_freq);
    void update_freq(uint8_t clk_num);
    void update_freq12(uint8_t freq1_changed);
    void update_freq_quad();
    void disable_out(uint8_t clk_num); // 0,1,2
    void set_control(uint8_t clk_num, uint8_t ctrl); // 0,1,2
  public:
    static uint32_t VCOFreq_Max; // == 900000000
    static uint32_t VCOFreq_Min; // == 600000000

    Si5351() { xtal_freq=250000000; }
    
    // power 0=2mA, 1=4mA, 2=6mA, 3=8mA
    void setup(uint8_t power1 = 3, uint8_t power2 = 3, uint8_t power3 = 3);

    // out xtal freq to CLK0
    void out_calibrate_freq();
    
    // change out power
    void set_power(uint8_t clk_num, uint8_t value);
    void set_power(uint8_t power1, uint8_t power2, uint8_t power3);
    
    // set xtal freq 
    void set_xtal_freq(uint32_t freq);
    
    // pass zero frequency for disable out
    // return true if PLL was reset
    uint8_t set_freq(uint32_t f0, uint32_t f1, uint32_t f2);
    uint8_t set_freq(uint32_t f0, uint32_t f1);
    uint8_t set_freq(uint32_t f0);
    
    // CLK0,CLK1 in qudrature, CLK2 = f2
    // return true if PLL was reset
    uint8_t set_freq_quadrature(uint32_t f01, uint32_t f2);
    
    // check that freq set corrected
    uint8_t is_freq_ok(uint8_t clk_num);
};

#endif
