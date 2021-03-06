/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    cdcuser.c
 *      Purpose: USB Communication Device Class User module 
 *      Version: V1.10
 *----------------------------------------------------------------------------
*      This software is supplied "AS IS" without any warranties, express,
 *      implied or statutory, including but not limited to the implied
 *      warranties of fitness for purpose, satisfactory quality and
 *      noninfringement. Keil extends you a royalty-free right to reproduce
 *      and distribute executable files created using this software for use
 *      on NXP Semiconductors LPC microcontroller devices only. Nothing else 
 *      gives you the right to use this software.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include "projectconfig.h"

#include "usb.h"
#include "usbhw.h"
#include "usbcfg.h"
#include "usbcore.h"
#include "cdc.h"
#include "cdcuser.h"
#include "cdc_buf.h"

unsigned char BulkBufIn  [64];            // Buffer to store USB IN  packet
unsigned char BulkBufOut [64];            // Buffer to store USB OUT packet
unsigned char NotificationBuf [10];

CDC_LINE_CODING CDC_LineCoding  = {CFG_USBCDC_BAUDRATE, 0, 0, 8};
unsigned short  CDC_SerialState[3] = { 0x0,0x0,0x0 };
unsigned short  CDC_DepInEmpty[3]  = { 1,1,1 };                   // Data IN EP is empty

/*----------------------------------------------------------------------------
  We need a buffer for incomming data on USB port because USB receives
  much faster than  UART transmits
 *---------------------------------------------------------------------------*/
/* Buffer masks */
#define CDC_BUF_SIZE               (64)               // Output buffer in bytes (power 2)
                                                       // large enough for file transfer
#define CDC_BUF_MASK               (CDC_BUF_SIZE-1ul)

/* Buffer read / write macros */
#define CDC_BUF_RESET(cdcBuf)      (cdcBuf.rdIdx = cdcBuf.wrIdx = 0)
#define CDC_BUF_WR(cdcBuf, dataIn) (cdcBuf.data[CDC_BUF_MASK & cdcBuf.wrIdx++] = (dataIn))
#define CDC_BUF_RD(cdcBuf)         (cdcBuf.data[CDC_BUF_MASK & cdcBuf.rdIdx++])   
#define CDC_BUF_EMPTY(cdcBuf)      (cdcBuf.rdIdx == cdcBuf.wrIdx)
#define CDC_BUF_FULL(cdcBuf)       (cdcBuf.rdIdx == cdcBuf.wrIdx+1)
#define CDC_BUF_COUNT(cdcBuf)      (CDC_BUF_MASK & (cdcBuf.wrIdx - cdcBuf.rdIdx))


// CDC output buffer
typedef struct __CDC_BUF_T {
  unsigned char data[CDC_BUF_SIZE];
  volatile unsigned int wrIdx;
  volatile unsigned int rdIdx;
} CDC_BUF_T;

CDC_BUF_T  CDC_OutBuf[3];                                 // buffer for all CDC Out data

/*----------------------------------------------------------------------------
  read data from CDC_OutBuf
 *---------------------------------------------------------------------------*/
int CDC_RdOutBuf(int port, char *buffer, const int *length) {
  int bytesToRead, bytesRead;
  
  /* Read *length bytes, block if *bytes are not avaialable	*/
  bytesToRead = *length;
  bytesToRead = (bytesToRead < (*length)) ? bytesToRead : (*length);
  bytesRead = bytesToRead;


  // ... add code to check for underrun

  while (bytesToRead--) {
    *buffer++ = CDC_BUF_RD(CDC_OutBuf[port]);
  }
  return (bytesRead);  
}

/*----------------------------------------------------------------------------
  write data to CDC_OutBuf
 *---------------------------------------------------------------------------*/
int CDC_WrOutBuf (int port, const char *buffer, int *length) {
  int bytesToWrite, bytesWritten;

  // Write *length bytes
  bytesToWrite = *length;
  bytesWritten = bytesToWrite;


  // ... add code to check for overwrite

  while (bytesToWrite) {
      CDC_BUF_WR(CDC_OutBuf[port], *buffer++);           // Copy Data to buffer  
      bytesToWrite--;
  }     

  return (bytesWritten); 
}

/*----------------------------------------------------------------------------
  check if character(s) are available at CDC_OutBuf
 *---------------------------------------------------------------------------*/
int CDC_OutBufAvailChar (int port, int *availChar) {

  *availChar = CDC_BUF_COUNT(CDC_OutBuf[port]);

  return (0);
}
/* end Buffer handling */


/*----------------------------------------------------------------------------
  CDC Initialisation
  Initializes the data structures and serial port
  Parameters:   None 
  Return Value: None
 *---------------------------------------------------------------------------*/
void CDC_Init (int port) {

//  ser_OpenPort ();
//  ser_InitPort (CDC_LineCoding.dwDTERate,
//                CDC_LineCoding.bDataBits, 
//                CDC_LineCoding.bParityType,
//                CDC_LineCoding.bCharFormat);

  CDC_DepInEmpty[port]  = 1;
  CDC_SerialState[port] = CDC_GetSerialState(port);

  CDC_BUF_RESET(CDC_OutBuf[port]);

  // Initialise the CDC buffer.   This is required to buffer outgoing
  // data (MCU to PC) since data can only be sent 64 bytes per frame
  // with at least 1ms between frames.  To see how the buffer is used,
  // see 'puts' in systeminit.c
  cdcBufferInit(port);
}


/*----------------------------------------------------------------------------
  CDC SendEncapsulatedCommand Request Callback
  Called automatically on CDC SEND_ENCAPSULATED_COMMAND Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendEncapsulatedCommand (void) {

  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC GetEncapsulatedResponse Request Callback
  Called automatically on CDC Get_ENCAPSULATED_RESPONSE Request
  Parameters:   None                          (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetEncapsulatedResponse (void) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SetCommFeature Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetCommFeature (unsigned short wFeatureSelector) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC GetCommFeature Request Callback
  Called automatically on CDC Get_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetCommFeature (unsigned short wFeatureSelector) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC ClearCommFeature Request Callback
  Called automatically on CDC CLEAR_COMM_FATURE Request
  Parameters:   FeatureSelector
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_ClearCommFeature (unsigned short wFeatureSelector) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SetLineCoding Request Callback
  Called automatically on CDC SET_LINE_CODING Request
  Parameters:   none                    (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetLineCoding (void) {

  CDC_LineCoding.dwDTERate   =   (EP0Buf[0] <<  0)
                               | (EP0Buf[1] <<  8)
                               | (EP0Buf[2] << 16)
                               | (EP0Buf[3] << 24); 
  CDC_LineCoding.bCharFormat =  EP0Buf[4];
  CDC_LineCoding.bParityType =  EP0Buf[5];
  CDC_LineCoding.bDataBits   =  EP0Buf[6];

//  ser_ClosePort();
//  ser_OpenPort ();
//  ser_InitPort (CDC_LineCoding.dwDTERate,
//                CDC_LineCoding.bDataBits, 
//                CDC_LineCoding.bParityType,
//                CDC_LineCoding.bCharFormat);    
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC GetLineCoding Request Callback
  Called automatically on CDC GET_LINE_CODING Request
  Parameters:   None                         (global SetupPacket and EP0Buf)
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_GetLineCoding (void) {

  EP0Buf[0] = (CDC_LineCoding.dwDTERate >>  0) & 0xFF;
  EP0Buf[1] = (CDC_LineCoding.dwDTERate >>  8) & 0xFF;
  EP0Buf[2] = (CDC_LineCoding.dwDTERate >> 16) & 0xFF;
  EP0Buf[3] = (CDC_LineCoding.dwDTERate >> 24) & 0xFF;
  EP0Buf[4] =  CDC_LineCoding.bCharFormat;
  EP0Buf[5] =  CDC_LineCoding.bParityType;
  EP0Buf[6] =  CDC_LineCoding.bDataBits;

  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SetControlLineState Request Callback
  Called automatically on CDC SET_CONTROL_LINE_STATE Request
  Parameters:   ControlSignalBitmap 
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SetControlLineState (unsigned short wControlSignalBitmap) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC SendBreak Request Callback
  Called automatically on CDC Set_COMM_FATURE Request
  Parameters:   0xFFFF  start of Break 
                0x0000  stop  of Break
                0x####  Duration of Break
  Return Value: TRUE - Success, FALSE - Error
 *---------------------------------------------------------------------------*/
uint32_t CDC_SendBreak (unsigned short wDurationOfBreak) {

  /* ... add code to handle request */
  return (TRUE);
}


/*----------------------------------------------------------------------------
  CDC_BulkIn call on DataIn Request
  Parameters:   none
  Return Value: none
 *---------------------------------------------------------------------------*/
void CDC_BulkIn(int port) {
  //int numBytesRead, numBytesAvail;

//  uint8_t frame[64];
//  uint32_t bytesRead = 0;
//  if (cdcBufferDataPending())
//  {
//    // Read up to 64 bytes
//    bytesRead = cdcBufferReadLen(frame, 64);
//    if (bytesRead > 0)
//    {
//      USB_WriteEP (CDC_DEP_IN, frame, bytesRead);
//    }
//  }


//  ser_AvailChar (&numBytesAvail);
//
//  // ... add code to check for overwrite
//
//  numBytesRead = ser_Read ((char *)&BulkBufIn[0], &numBytesAvail);
//
//  // send over USB
//  if (numBytesRead > 0) {
//	USB_WriteEP (CDC_DEP_IN, &BulkBufIn[0], numBytesRead);
//  }
//  else {
//    CDC_DepInEmpty = 1;
//  }
} 


/*----------------------------------------------------------------------------
  CDC_BulkOut call on DataOut Request
  Parameters:   none
  Return Value: none
 *---------------------------------------------------------------------------*/
void CDC_BulkOut(int port) {
  int numBytesRead;
  int dep = CDC_DEP1_OUT;
  switch(port) {
	case CDC_SERIAL_PORT_1:
		dep = CDC_DEP1_OUT;
		break;
	case CDC_SERIAL_PORT_2:
		dep = CDC_DEP2_OUT;
		break;
	case CDC_SERIAL_PORT_3:
		dep = CDC_DEP3_OUT;
		break;
  }
  // get data from USB into intermediate buffer
  numBytesRead = USB_ReadEP(dep, &BulkBufOut[0]);

  // ... add code to check for overwrite

  // store data in a buffer to transmit it over serial interface
  CDC_WrOutBuf (port, (char *)&BulkBufOut[0], &numBytesRead);

}


/*----------------------------------------------------------------------------
  Get the SERIAL_STATE as defined in usbcdc11.pdf, 6.3.5, Table 69.
  Parameters:   none
  Return Value: SerialState as defined in usbcdc11.pdf
 *---------------------------------------------------------------------------*/
unsigned short CDC_GetSerialState (int port) {
//  unsigned short temp;

  CDC_SerialState[port] = 0;
//  ser_LineState (&temp);
//
//  if (temp & 0x8000)  CDC_SerialState |= CDC_SERIAL_STATE_RX_CARRIER;
//  if (temp & 0x2000)  CDC_SerialState |= CDC_SERIAL_STATE_TX_CARRIER;
//  if (temp & 0x0010)  CDC_SerialState |= CDC_SERIAL_STATE_BREAK;
//  if (temp & 0x4000)  CDC_SerialState |= CDC_SERIAL_STATE_RING;
//  if (temp & 0x0008)  CDC_SerialState |= CDC_SERIAL_STATE_FRAMING;
//  if (temp & 0x0004)  CDC_SerialState |= CDC_SERIAL_STATE_PARITY;
//  if (temp & 0x0002)  CDC_SerialState |= CDC_SERIAL_STATE_OVERRUN;

  return (CDC_SerialState[port]);
}


/*----------------------------------------------------------------------------
  Send the SERIAL_STATE notification as defined in usbcdc11.pdf, 6.3.5.
 *---------------------------------------------------------------------------*/
void CDC_NotificationIn (int port) {

  NotificationBuf[0] = 0xA1;                           // bmRequestType
  NotificationBuf[1] = CDC_NOTIFICATION_SERIAL_STATE;  // bNotification (SERIAL_STATE)
  NotificationBuf[2] = 0x00;                           // wValue
  NotificationBuf[3] = 0x00;
  NotificationBuf[4] = 0x00;                           // wIndex (Interface #, LSB first)
  NotificationBuf[5] = 0x00;
  NotificationBuf[6] = 0x02;                           // wLength (Data length = 2 bytes, LSB first)
  NotificationBuf[7] = 0x00; 
  NotificationBuf[8] = (CDC_SerialState[port] >>  0) & 0xFF; // UART State Bitmap (16bits, LSB first)
  NotificationBuf[9] = (CDC_SerialState[port] >>  8) & 0xFF;

  int cep = CDC_CEP1_IN;
  switch(port) {
	case CDC_SERIAL_PORT_1:
		cep = CDC_CEP1_IN;
		break;
	case CDC_SERIAL_PORT_2:
		cep = CDC_CEP2_IN;
		break;
	case CDC_SERIAL_PORT_3:
		cep = CDC_CEP3_IN;
		break;
  }
  USB_WriteEP (cep, &NotificationBuf[0], 10);   // send notification
}
