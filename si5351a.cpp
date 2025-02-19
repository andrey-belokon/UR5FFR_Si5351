// Full-featured library for Si5351
// version 1.0
// (c) Andrew Bilokon, UR5FFR
// mailto:ban.relayer@gmail.com
// http://dspview.com
// https://github.com/andrey-belokon

#include <inttypes.h>
#include "si5351a.h"
#include "i2c.h"

#define SI_CLK0_CONTROL 16      // Register definitions
#define SI_CLK1_CONTROL 17
#define SI_CLK2_CONTROL 18
#define SI_SYNTH_PLL_A  26
#define SI_SYNTH_PLL_B  34
#define SI_SYNTH_MS_0   42
#define SI_SYNTH_MS_1   50
#define SI_SYNTH_MS_2   58
#define SI_PLL_RESET    177

#define SI_PLL_RESET_A   0x20
#define SI_PLL_RESET_B   0x80

#define SI_CLK0_PHASE  165
#define SI_CLK1_PHASE  166
#define SI_CLK2_PHASE  167

#define R_DIV(x) ((x) << 4)

#define SI_CLK_SRC_PLL_A  0b00000000
#define SI_CLK_SRC_PLL_B  0b00100000

#define SI5351_I2C_ADDR 0x60

// 1048575
#define FRAC_DENOM 0xFFFFF

// for fast rdiv shift 
uint8_t power2[8] = {1,2,4,8,16,32,64,128};

uint32_t Si5351Base::VCOFreq_Max = 900000000;
uint32_t Si5351Base::VCOFreq_Min = 600000000;
uint32_t Si5351Base::VCOFreq_Mid = 750000000;

bool Si5351::_i2c_begin_write(uint8_t addr)
{
  return i2c_begin_write(addr);
}

void Si5351::_i2c_end()
{
  i2c_end();
}

bool Si5351::_i2c_write(uint8_t data)
{
  return i2c_write(data);
}

bool Si5351Soft::_i2c_begin_write(uint8_t addr)
{
  return i2c.i2c_begin_write(addr);
}

void Si5351Soft::_i2c_end()
{
  i2c.i2c_end();
}

bool Si5351Soft::_i2c_write(uint8_t data)
{
  return i2c_write(data);
}

void Si5351Base::si5351_write_reg(uint8_t reg, uint8_t data)
{
  _i2c_begin_write(SI5351_I2C_ADDR);
  _i2c_write(reg);
  _i2c_write(data);
  _i2c_end();
}

void Si5351Base::si5351_write_regs(uint8_t synth, uint32_t P1, uint32_t P2, uint32_t P3, uint8_t rDiv, bool divby4)
{
  _i2c_begin_write(SI5351_I2C_ADDR);
  _i2c_write(synth);
  _i2c_write(((uint8_t*)&P3)[1]);
  _i2c_write((uint8_t)P3);
  _i2c_write((((uint8_t*)&P1)[2] & 0x3) | rDiv | (divby4 ? 0x0C : 0x00));
  _i2c_write(((uint8_t*)&P1)[1]);
  _i2c_write((uint8_t)P1);
  _i2c_write(((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
  _i2c_write(((uint8_t*)&P2)[1]);
  _i2c_write((uint8_t)P2);
  _i2c_end();
}

// Set up MultiSynth with mult, num and denom
// mult is 15..90
// num is 0..1,048,575 (0xFFFFF)
// denom is 0..1,048,575 (0xFFFFF)
//
void Si5351Base::si5351_setup_msynth_abc(uint8_t synth, uint8_t a, uint32_t b, uint32_t c, uint8_t rDiv)
{
  uint32_t t = 128*b / c;
  si5351_write_regs(
    synth,
    (uint32_t)(128 * (uint32_t)(a) + t - 512),
    (uint32_t)(128 * b - c * t),
    c,
    rDiv,
    false
  );
}

//
// Set up MultiSynth with integer divider and R divider
// R divider is the bit value which is OR'ed onto the appropriate register, it is a #define in si5351a.h
//
void Si5351Base::si5351_setup_msynth_int(uint8_t synth, uint32_t divider, uint8_t rDiv)
{
  // P2 = 0, P3 = 1 forces an integer value for the divider
  si5351_write_regs(
    synth,
    128 * divider - 512,
    0,
    1,
    rDiv,
    divider == 4
  );
}

void Si5351Base::si5351_setup_msynth(uint8_t synth, uint32_t pll_freq)
{
  uint8_t a = pll_freq / xtal_freq;
  uint32_t b = (pll_freq % xtal_freq) >> 5;
  uint32_t c = xtal_freq >> 5;
  uint32_t t = 128*b / c;
  si5351_write_regs(
    synth,
    (uint32_t)(128 * (uint32_t)(a) + t - 512),
    (uint32_t)(128 * b - c * t),
    c,
    0,
    false
  );
}

void Si5351Base::setup(uint8_t power0, uint8_t power1, uint8_t power2)
{
  power[0] = power0;
  power[1] = power1;
  power[2] = power2;
  si5351_write_reg(SI_CLK0_CONTROL, 0x80);
  si5351_write_reg(SI_CLK1_CONTROL, 0x80);
  si5351_write_reg(SI_CLK2_CONTROL, 0x80);
  VCOFreq_Mid = (VCOFreq_Min+VCOFreq_Max) >> 1;
}

void Si5351Base::set_power(uint8_t clk_num, uint8_t value)
{
  power[clk_num] = value;
  // for force update
  freq[clk_num] = 0; 
  freq_div[clk_num] = 0;
}

void Si5351Base::set_power(uint8_t power1, uint8_t power2, uint8_t power3)
{
  set_power(0,power1);
  set_power(0,power2);
  set_power(0,power3);
}

void Si5351Base::set_xtal_freq(uint32_t freq)
{
  xtal_freq = freq;
  freq_div[0] = freq_div[1] = freq_div[2] = freq_rdiv[0] = freq_rdiv[1] = freq_rdiv[2] = 0;
}

uint8_t Si5351Base::set_freq(uint32_t f0, uint32_t f1, uint32_t f2)
{
  need_reset_pll = 0;
  uint8_t freq1_changed = f1 != freq[1];
  if (f0 != freq[0]) {
    freq[0] = f0;
    update_freq(0);
  }
  if (freq1_changed || f2 != freq[2]) {
    freq[1] = f1;
    freq[2] = f2;
    update_freq12(freq1_changed);
  }
  if (need_reset_pll) 
    si5351_write_reg(SI_PLL_RESET, need_reset_pll);
  return need_reset_pll;
}

uint8_t Si5351Base::set_freq(uint32_t f0, uint32_t f1)
{
  need_reset_pll = 0;
  if (f0 != freq[0]) {
    freq[0] = f0;
    update_freq(0);
  }
  if (f1 != freq[1]) {
    freq[1] = f1;
    update_freq(1);
  }
  if (need_reset_pll) 
    si5351_write_reg(SI_PLL_RESET, need_reset_pll);
  return need_reset_pll;
}

uint8_t Si5351Base::set_freq(uint32_t f0)
{
  need_reset_pll = 0;
  if (f0 != freq[0]) {
    freq[0] = f0;
    update_freq(0);
  }
  if (need_reset_pll) 
    si5351_write_reg(SI_PLL_RESET, need_reset_pll);
  return need_reset_pll;
}

void Si5351Base::disable_out(uint8_t clk_num)
{
 si5351_write_reg(SI_CLK0_CONTROL+clk_num, 0x80);
 freq_div[clk_num] = 0;
}

uint8_t Si5351Base::is_freq_ok(uint8_t clk_num)
{
 return freq_div[clk_num] != 0;
}

void Si5351Base::out_calibrate_freq()
{
  si5351_write_reg(SI_CLK0_CONTROL, power[0]);
  si5351_write_reg(SI_CLK1_CONTROL, power[1]);
  si5351_write_reg(SI_CLK2_CONTROL, power[2]);
  si5351_write_reg(SI_SYNTH_MS_0+2,0);
  si5351_write_reg(SI_SYNTH_MS_1+2,0);
  si5351_write_reg(SI_SYNTH_MS_2+2,0);
  si5351_write_reg(187, 0xD0);
  freq[0]=freq[1]=freq[2]=xtal_freq;
}

void Si5351Base::update_freq(uint8_t clk_num)
{
  uint32_t divider,pll_freq;
  uint8_t rdiv = 0;

  if (freq[clk_num] == 0) {
    disable_out(clk_num);
    return;
  }

  // try to use last divider
  divider = freq_div[clk_num];
  rdiv = freq_rdiv[clk_num];
  pll_freq = divider * freq[clk_num] * power2[rdiv]; //(1 << rdiv);
  
  if (pll_freq < VCOFreq_Min || pll_freq > VCOFreq_Max) {
    divider = VCOFreq_Mid / freq[clk_num];
    if (divider < 4) 
    {
      disable_out(clk_num);
      return;
    }
    
    if (divider < 6) 
      divider = 4;

    rdiv =  0;
    while (divider > 300) {
      rdiv++;
      divider >>= 1;
    }
    if (rdiv == 0) divider &= 0xFFFFFFFE;
    pll_freq = divider * freq[clk_num] * power2[rdiv]; //(1 << rdiv);
  }

  si5351_setup_msynth((clk_num ? SI_SYNTH_PLL_B : SI_SYNTH_PLL_A), pll_freq);

  if (divider != freq_div[clk_num] || rdiv != freq_rdiv[clk_num]) {
    si5351_setup_msynth_int(SI_SYNTH_MS_0+clk_num*8, divider, R_DIV(rdiv));
    si5351_write_reg(SI_CLK0_CONTROL+clk_num, 0x4C | power[clk_num] | (clk_num ? SI_CLK_SRC_PLL_B : SI_CLK_SRC_PLL_A));
    freq_div[clk_num] = divider;
    freq_rdiv[clk_num] = rdiv;
    need_reset_pll |= (clk_num ? SI_PLL_RESET_B : SI_PLL_RESET_A);
  }
}

void Si5351Base::update_freq12(uint8_t freq1_changed)
{
  uint32_t pll_freq,divider,num;
  uint8_t rdiv = 0;

  if (freq[1] == 0) {
    disable_out(1);
  }
  
  if (freq[2] == 0) {
    disable_out(2);
  }

  if (freq[1]) {
    if (freq1_changed) {
      // try to use last divider
      divider = freq_div[1];
      rdiv = freq_rdiv[1];
      pll_freq = divider * freq[1] * power2[rdiv]; //(1 << rdiv);
      
      if (pll_freq < VCOFreq_Min || pll_freq > VCOFreq_Max) {
        divider = VCOFreq_Mid / freq[1];
        if (divider < 4) {
          disable_out(1);
          return;
        }
        if (divider < 6) 
          divider = 4;
    	rdiv =  0;
        while (divider > 300) {
          rdiv++;
          divider >>= 1;
        }
        if (rdiv == 0) divider &= 0xFFFFFFFE;
        
        pll_freq = divider * freq[1] * power2[rdiv]; //(1 << rdiv);
      }
    
      si5351_setup_msynth(SI_SYNTH_PLL_B, pll_freq);
      if (divider != freq_div[1] || rdiv != freq_rdiv[1]) {
        si5351_setup_msynth_int(SI_SYNTH_MS_1, divider, R_DIV(rdiv));
        si5351_write_reg(SI_CLK1_CONTROL, 0x4C | power[1] | SI_CLK_SRC_PLL_B);
        freq_div[1] = divider;
        freq_rdiv[1] = rdiv;
        need_reset_pll |= SI_PLL_RESET_B;
      }
      freq_pll_b = pll_freq;
    }

    if (freq[2]) {
      // CLK2 --> PLL_B with fractional or integer multisynth 
      divider = freq_pll_b / freq[2];
      if (divider < 8) {
        disable_out(2);
        return;
      }
      rdiv = 0;
      uint32_t ff = freq[2];
      while (divider > 64) {
        rdiv++;
        ff <<= 1;
        divider >>= 1;
      }
      divider = freq_pll_b / ff;
      num = (uint64_t)(freq_pll_b % ff) * FRAC_DENOM / ff;
        
      si5351_setup_msynth_abc(SI_SYNTH_MS_2,divider, num, (num?FRAC_DENOM:1), R_DIV(rdiv));
      si5351_write_reg(SI_CLK2_CONTROL, (num?0x0C:0x4C) | power[2] | SI_CLK_SRC_PLL_B);
      freq_div[2] = 1; // non zero for correct enable/disable CLK2
    }
  } else if (freq[2]) {
    // PLL_B --> CLK2, multisynth integer
    // try to use last divider
    divider = freq_div[2];
    rdiv = freq_rdiv[2];
    pll_freq = divider * freq[2] * power2[rdiv]; //(1 << rdiv);
    
    if (pll_freq < VCOFreq_Min || pll_freq > VCOFreq_Max) {
      divider = VCOFreq_Mid / freq[2];
      if (divider < 4) {
        disable_out(2);
        return;
      }
      if (divider < 6) 
        divider = 4;
      rdiv =  0;
      while (divider > 300) {
        rdiv++;
        divider >>= 1;
      }
      if (rdiv == 0) divider &= 0xFFFFFFFE;
    
      pll_freq = divider * freq[2] * power2[rdiv]; //(1 << rdiv);
    }
  
    si5351_setup_msynth(SI_SYNTH_PLL_B, pll_freq);
  
    if (divider != freq_div[2] || rdiv != freq_rdiv[2]) {
      si5351_setup_msynth_int(SI_SYNTH_MS_2, divider, R_DIV(rdiv));
      si5351_write_reg(SI_CLK2_CONTROL, 0x4C | power[2] | SI_CLK_SRC_PLL_B);
      freq_div[2] = divider;
      freq_rdiv[2] = rdiv;
      need_reset_pll |= SI_PLL_RESET_B;
    }
  }
}

void Si5351Base::update_freq_quad(bool inverse_phase)
{
  uint32_t pll_freq,divider;

  if (freq[0] == 0) {
    disable_out(0);
    disable_out(1);
    return;
  }

  if (freq[0] >= 7000000) {
    divider = (VCOFreq_Max / freq[0]);
  } else if (freq[0] >= 4000000) {
    divider = (VCOFreq_Min / freq[0]);
  } else if (freq[0] >= 2000000) {
    // VCO run on freq less than 600MHz. possible unstable
    // comment this for disable operation below 600MHz VCO (4MHz on out)
    divider = 0x7F;
  } else {
    divider = 0; // disable out on invalid freq
  }
  if (divider < 4 || divider > 0x7F) {
    disable_out(0);
    disable_out(1);
    return;
  }

  if (divider < 6) 
    divider = 4;

  pll_freq = divider * freq[0];

  si5351_setup_msynth(SI_SYNTH_PLL_A, pll_freq);

  if (divider != freq_div[0]) {
    uint8_t phase = divider & 0x7F;
    si5351_setup_msynth_int(SI_SYNTH_MS_0, divider, 0);
    si5351_write_reg(SI_CLK0_CONTROL, 0x4C | power[0] | SI_CLK_SRC_PLL_A);
    si5351_write_reg(SI_CLK0_PHASE, (inverse_phase ? phase : 0));
    si5351_setup_msynth_int(SI_SYNTH_MS_1, divider, 0);
    si5351_write_reg(SI_CLK1_CONTROL, 0x4C | power[0] | SI_CLK_SRC_PLL_A);
    si5351_write_reg(SI_CLK1_PHASE, (inverse_phase ? 0 : phase));
    freq_div[0] = freq_div[1] = divider;
    need_reset_pll |= SI_PLL_RESET_A;
  }
}

uint8_t Si5351Base::set_freq_quadrature(uint32_t f01, uint32_t f2, bool inverse_phase)
{
  need_reset_pll = 0;
  if (f01 != freq[0]) {
    freq[0] = f01;
    update_freq_quad(inverse_phase);
  }
  if (f2 != freq[2]) {
    freq[2] = f2;
    update_freq(2);
  }
  if (need_reset_pll) 
    si5351_write_reg(SI_PLL_RESET, need_reset_pll);
  return need_reset_pll;
}
