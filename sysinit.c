/**************************************************************************/
/*! 
    @file     sysinit.c
    @author   K. Townsend (microBuilder.eu)
    @date     22 March 2010
    @version  0.10

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2010, microBuilder SARL
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sysinit.h"

#include "core/cpu/cpu.h"
#include "core/pmu/pmu.h"

#ifdef CFG_PRINTF_UART
  #include "core/uart/uart.h"
#endif

#ifdef CFG_INTERFACE
  #include "core/cmd/cmd.h"
#endif

#ifdef CFG_CHIBI
  #include "drivers/rf/chibi/chb.h"
#endif

#ifdef CFG_USBHID
  #include "core/usbhid-rom/usbhid.h"
#endif

#ifdef CFG_USBCDC
  volatile unsigned int lastTick;
  #include "core/usbcdc/usb.h"
  #include "core/usbcdc/usbcore.h"
  #include "core/usbcdc/usbhw.h"
  #include "core/usbcdc/cdcuser.h"
  #include "core/usbcdc/cdc_buf.h"
#endif

#ifdef CFG_ST7565
  #include "drivers/displays/bitmap/st7565/st7565.h"
  #include "drivers/displays/smallfonts.h"
#endif

#ifdef CFG_SSD1306
  #include "drivers/displays/bitmap/ssd1306/ssd1306.h"
  #include "drivers/displays/smallfonts.h"
#endif

#ifdef CFG_TFTLCD
  #include "drivers/displays/tft/lcd.h"
  #include "drivers/displays/tft/touchscreen.h"
  #include "drivers/displays/tft/drawing.h"  
#endif

#ifdef CFG_I2CEEPROM
  #include "drivers/storage/eeprom/mcp24aa/mcp24aa.h"
  #include "drivers/storage/eeprom/eeprom.h"
#endif

#ifdef CFG_PWM
  #include "core/pwm/pwm.h"
#endif

#ifdef CFG_SDCARD
  #include "core/ssp/ssp.h"
  #include "drivers/fatfs/diskio.h"
  #include "drivers/fatfs/ff.h"

  DWORD get_fattime ()
  {
    DWORD tmr = 0;

    // tmr =  (((DWORD)rtcYear - 80) << 25)
	//      | ((DWORD)rtcMon << 21)
    //      | ((DWORD)rtcMday << 16)
    //      | (WORD)(rtcHour << 11)
    //      | (WORD)(rtcMin << 5)
    //      | (WORD)(rtcSec >> 1);

    return tmr;
  }
#endif

/**************************************************************************/
/*! 
    Configures the core system clock and sets up any mandatory
    peripherals like the systick timer, UART for printf, etc.

    This function should set the HW to the default state you wish to be
    in coming out of reset/startup, such as disabling or enabling LEDs,
    setting specific pin states, etc.
*/
/**************************************************************************/
void systemInit()
{
  cpuInit();                                // Configure the CPU
  systickInit(CFG_SYSTICK_DELAY_IN_MS);     // Start systick timer
  gpioInit();                               // Enable GPIO
  pmuInit();                                // Configure power management

  // Set LED pin as output and turn LED off
  int c=0;
  for(c=0;c<4;c++) {
	  gpioSetDir(CFG_LED_PORT, c, 1);
	  gpioSetValue(CFG_LED_PORT, c, CFG_LED_OFF);
  }
  // Config alt reset pin if requested (really only relevant to LPC1343 LCD Board)
  #ifdef CFG_ALTRESET
    gpioSetDir (CFG_ALTRESET_PORT, CFG_ALTRESET_PIN, gpioDirection_Input);
    gpioSetInterrupt (CFG_ALTRESET_PORT, CFG_ALTRESET_PIN, gpioInterruptSense_Level, gpioInterruptEdge_Single, gpioInterruptEvent_ActiveHigh);
    gpioIntEnable (CFG_ALTRESET_PORT, CFG_ALTRESET_PIN); 
  #endif

  // Initialise EEPROM
  #ifdef CFG_I2CEEPROM
    mcp24aaInit();
  #endif

  // Initialise UART with the default baud rate
  #ifdef CFG_PRINTF_UART
    #ifdef CFG_I2CEEPROM
      uint32_t uart = eepromReadU32(CFG_EEPROM_UART_SPEED);
      if ((uart == 0xFFFFFFFF) || (uart > 115200))
      {
        uartInit(CFG_UART_BAUDRATE);  // Use default baud rate
      }
      else
      {
        uartInit(uart);               // Use baud rate from EEPROM
      }
    #else
      uartInit(CFG_UART_BAUDRATE);
    #endif
  #endif

  // Initialise PWM (requires 16-bit Timer 1 and P1.9)
  #ifdef CFG_PWM
    pwmInit();
  #endif

  // Initialise USB HID
  #ifdef CFG_USBHID
    usbHIDInit();
  #endif

  // Initialise USB CDC
  #ifdef CFG_USBCDC
    lastTick = systickGetTicks();   // Used to control output/printf timing
    CDC_Init(CDC_SERIAL_PORT_1);    // Initialise VCOM
    CDC_Init(CDC_SERIAL_PORT_2);
    CDC_Init(CDC_SERIAL_PORT_3);
    USB_Init();                     // USB Initialization
    USB_Connect(TRUE);              // USB Connect
    // Wait until USB is configured or timeout occurs
    uint32_t usbTimeout = 0; 
    while ( usbTimeout < CFG_USBCDC_INITTIMEOUT / 10 )
    {
      if (USB_Configuration) break;
      systickDelay(10);             // Wait 10ms
      usbTimeout++;
    }
  #endif

  // Printf can now be used with UART or USBCDC

  // Initialise the ST7565 128x64 pixel display
  #ifdef CFG_ST7565
    st7565Init();
    st7565ClearScreen();    // Clear the screen  
    st7565Backlight(1);     // Enable the backlight
  #endif

  // Initialise the SSD1306 OLED display
  #ifdef CFG_SSD1306
    ssd1306Init(SSD1306_INTERNALVCC);
    ssd1306ClearScreen();   // Clear the screen  
  #endif

  // Initialise TFT LCD Display
  #ifdef CFG_TFTLCD
    lcdInit();
    // You may need to call the tsCalibrate() function to calibrate 
    // the touch screen is this has never been done.  This only needs
    // to be done once and the values are saved to EEPROM.  This 
    // function can also be called from tsInit if it's more
    // convenient
    /*
    #ifdef CFG_I2CEEPROM
    if (eepromReadU8(CFG_EEPROM_TOUCHSCREEN_CALIBRATED) != 1)
    {
      tsCalibrate();
    }
    #endif
    */
  #endif

  // Initialise Chibi
  // Warning: CFG_CHIBI must be disabled if no antenna is connected,
  // otherwise the SW will halt during initialisation
  #ifdef CFG_CHIBI
    // Write addresses to EEPROM for the first time if necessary
    // uint16_t addr_short = 0x0025;
    // uint64_t addr_ieee =  0x0000000000000025;
    // mcp24aaWriteBuffer(CFG_EEPROM_CHIBI_SHORTADDR, (uint8_t *)&addr_short, 2);
    // mcp24aaWriteBuffer(CFG_EEPROM_CHIBI_IEEEADDR, (uint8_t *)&addr_ieee, 8);
    chb_init();
    // chb_pcb_t *pcb = chb_get_pcb();
    // printf("%-40s : 0x%04X%s", "Chibi Initialised", pcb->src_addr, CFG_PRINTF_NEWLINE);
  #endif

  // Start the command line interface
  #ifdef CFG_INTERFACE
    cmdInit();
  #endif
}

/**************************************************************************/
/*! 
    @brief Sends a single byte to a pre-determined peripheral (UART, etc.).

    @param[in]  byte
                Byte value to send
*/
/**************************************************************************/
void __putchar(const char c) 
{
  #ifdef CFG_PRINTF_UART
    // Send output to UART
    uartSendByte(c);
  #endif
}

int puts(const char * str) {
	return 0;
}
/**************************************************************************/
/*! 
    @brief Sends a string to a pre-determined end point (UART, etc.).

    @param[in]  str
                Text to send

    @note This function is only called when using the GCC-compiler
          in Codelite or running the Makefile manually.  This function
          will not be called when using the C library in Crossworks for
          ARM.
*/
/**************************************************************************/
int ser_puts(int port, const char * str)
{
  // There must be at least 1ms between USB frames (of up to 64 bytes)
  // This buffers all data and writes it out from the buffer one frame
  // and one millisecond at a time
  int dep = CDC_DEP1_IN;
  switch(port) {
	case CDC_SERIAL_PORT_1:
		dep = CDC_DEP1_IN;
		break;
	case CDC_SERIAL_PORT_2:
		dep = CDC_DEP2_IN;
		break;
	case CDC_SERIAL_PORT_3:
		dep = CDC_DEP3_IN;
		break;
  }

  #ifdef CFG_PRINTF_USBCDC
    if (USB_Configuration) 
    {
      while(*str)
        cdcBufferWrite(port, *str++);
      // Check if we can flush the buffer now or if we need to wait
      unsigned int currentTick = systickGetTicks();
      if (currentTick != lastTick)
      {
        uint8_t frame[64];
        uint32_t bytesRead = 0;
        while (cdcBufferDataPending(port))
        {
          // Read up to 64 bytes as long as possible
          bytesRead = cdcBufferReadLen(port, frame, 64);
          USB_WriteEP (dep, frame, bytesRead);
          systickDelay(1);
        }
        lastTick = currentTick;
      }
    }
  #else
    // Handle output character by character in __putchar
    while(*str) __putchar(*str++);
  #endif

  return 0;
}

// Override printf here if we're using Crossworks for ARM
// so that we can still use the custom libc libraries.
// For Codelite and compiling from the makefile (Yagarto, etc.)
// this is done in /core/libc

#ifdef __CROSSWORKS_ARM

/**************************************************************************/
/*! 
    @brief  Outputs a formatted string on the DBGU stream. Format arguments
            are given in a va_list instance.

    @param[in]  pFormat
                Format string
    @param[in]  ap
                Argument list
*/
/**************************************************************************/
signed int vprintf(const char *pFormat, va_list ap)
{
  char pStr[CFG_PRINTF_MAXSTRINGSIZE];
  char pError[] = "stdio.c: increase CFG_PRINTF_MAXSTRINGSIZE\r\n";
  
  // Write formatted string in buffer
  if (vsprintf(pStr, pFormat, ap) >= CFG_PRINTF_MAXSTRINGSIZE) {
    
    puts(pError);
    while (1); // Increase CFG_PRINTF_MAXSTRINGSIZE
  }
  
  // Display string
  return puts(pStr);
}

/**************************************************************************/
/*! 
    @brief  Outputs a formatted string on the DBGU stream, using a 
            variable number of arguments

    @param[in]  pFormat
                Format string
*/
/**************************************************************************/
signed int printf(const char *pFormat, ...)
{
    va_list ap;
    signed int result;

    // Forward call to vprintf
    va_start(ap, pFormat);
    result = vprintf(pFormat, ap);
    va_end(ap);

    return result;
}

#endif
