

//Just some notes.  This is based on:
//	 https://github.com/andysworkshop/stm32plus
/*
	Copyright (c) 2011-2015 Andrew Brown All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

    Neither the name of stm32plus nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ANDREW BROWN BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <usb.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_rcc.h>
#include <systems.h>
#include <usb_defs.h>
#include "usbconfig.h"


#define USBR_BDT ((volatile struct EndpointBufferDescription *)BTABLE_BASE)

void sendData( uint8_t endpointIndex, uint16_t pmaInAddress, uint16_t maxPacketSize, const void *data, uint16_t length);

struct UsbInEndpointData {
	const uint8_t *ptr;
	uint16_t remaining;
	uint16_t total;
	uint16_t maxPacketSize;
	uint16_t pmaAddress;
};

struct UsbInEndpointData _inEndpointData[2]; // 2 user endpoints in this implementation

volatile int usbirqcalls;
volatile int usbirqlast;

int _address;
int _remoteWakeup;
int _deviceState;
int _savedState;
int _remoteWakeup;
int _setupDataLength;
int _controlEndpointState;
int _hidDataRemain;
volatile int usbDataOkToSend;
int _tx_toggle;
void setDeviceState( int newstate )
{
	_deviceState = newstate;
}


void setInterruptMask()
{

    USBR->CNTR=USB_CNTR_CTRM          // correct transfer
              | USB_CNTR_WKUPM        // wakeup
              | USB_CNTR_SUSPM        // suspend
            //  | USB_CNTR_ERRM         // error
           //   | USB_CNTR_ESOFM        // (un)expected start of frame
			  | USB_CNTR_RESETM; // reset
}

void setAddress( int addy )
{
    if(addy==0)
	      USBR->DADDR = USB_DADDR_EF;
    else
		_address=addy;
}

void init_usb()
{

	ConfigureGPIO( 11, PUPD_NONE|INOUT_OUT );
	ConfigureGPIO( 12, PUPD_NONE|INOUT_OUT );

	GPIO_PinAFConfig(GPIOOf(11), 11, GPIO_AF_2);
	GPIO_PinAFConfig(GPIOOf(12), 12, GPIO_AF_2);

	_deviceState = USB_DS_NONE;
    _address=0;
    setDeviceState(USB_DS_DEFAULT);
    _remoteWakeup=0;


    USBR->BTABLE=0;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB,ENABLE);

    NVIC_InitTypeDef nit;
    nit.NVIC_IRQChannel=USB_IRQn;
    nit.NVIC_IRQChannelPriority=0;
    nit.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&nit);

	//Reset the device.
    USBR->CNTR=USB_CNTR_FRES;
    USBR->CNTR=0;

	// Clear interrupts
	USBR->ISTR=0; 

	setInterruptMask();

	USBR->BCDR |= USB_BCDR_DPPU;

	//Fill in serial #
	int i;
	for( i = 0; i < 6; i++ )
	{
		uint8_t id = ((const int8_t*)0x1FFFF7AC)[i+5];
		string3.wString[i] = id;
	}

	for( i = 0; i < 8; i++ )
	{
	    USBR_BDT[i].rx.addr=GET_PMA(i,1);
	    USBR_BDT[i].tx.addr=GET_PMA(i,0);
	}
}

void usb_data( uint8_t * data, int len )
{
	usbDataOkToSend = 0;
	if( (IN_ENDPOINT_ADDRESS & 0x7F) > 1 ) _tx_toggle = !_tx_toggle;
    sendData(IN_ENDPOINT_ADDRESS & 0x7F, GET_PMA(IN_ENDPOINT_ADDRESS & 0x7F,_tx_toggle), ENDPOINT1_SIZE, data, len);
}

uint16_t setTxStatus(uint16_t reg,uint16_t status) {

    // if current reg bit is not equal to the desired reg bit then
    // set 1 in the reg to toggle it

    if((USB_EPTX_DTOG1 & status)!=0)
		reg^=USB_EPTX_DTOG1;

    if((USB_EPTX_DTOG2 & status)!=0)
		reg^=USB_EPTX_DTOG2;

    return reg;
}


//////////////////////////////////////////////////////////////
//  Rewrite this section /////////////////////////////////////
//////////////////////////////////////////////////////////////


void setTxEndpointStatus(volatile uint16_t *epreg,uint16_t status) {

    uint16_t value;

    value=*epreg & USB_EPTX_DTOGMASK;
    *epreg=setTxStatus(value,status) | USB_EP_CTR_RX | USB_EP_CTR_TX;
  }


int setRxStatus(uint16_t reg,uint16_t status) {

    // if current reg bit is not equal to the desired reg bit then
    // set 1 in the reg to toggle it

    if((USB_EPRX_DTOG1 & status)!=0)
      reg^=USB_EPRX_DTOG1;

    if((USB_EPRX_DTOG2 & status)!=0)
      reg^=USB_EPRX_DTOG2;

    return reg;
  }

void setRxEndpointStatus(volatile uint16_t *epreg,uint16_t status) {

    uint16_t value;

    value=*epreg & USB_EPRX_DTOGMASK;
    *epreg=setRxStatus(value,status) | USB_EP_CTR_RX | USB_EP_CTR_TX;
}

void openRxEndpoint(volatile uint16_t *reg,uint8_t addr,uint16_t type,uint16_t maxPacketSize)
{
    // set up the endpoint type and address
    *reg=(*reg & USB_EP_T_MASK) | type;
    *reg=(*reg & USB_EPREG_MASK) | addr | USB_EP_CTR_RX;

    // set up the RX address in the BTABLE and initialise first packet read
    UBsetRxCount( &USBR_BDT[addr].rx, maxPacketSize);

//    USBR_BDT[addr].rx.addr=GET_PMA(addr,1);

    // if DTOG_RX is 1 then we need to write 1 to toggle it to zero
    if((*reg & USB_EP_DTOG_RX)!=0)
      *reg=(*reg & USB_EPREG_MASK) | USB_EP_CTR_RX | USB_EP_DTOG_RX;

    // inititate reception of the first packet
    setRxEndpointStatus(reg,USB_EP_RX_VALID);
}

void openTxEndpoint(volatile uint16_t *reg,uint8_t addr,uint16_t type)
{
	// set up the endpoint type and address
	*reg=(*reg & USB_EP_T_MASK) | type;
	*reg=(*reg & USB_EPREG_MASK) | addr | USB_EP_CTR_TX;

//    USBR_BDT[addr].tx.addr=GET_PMA(addr,0);

	if( addr > 1 && type == USB_EP_BULK )
	{
		*reg |= USB_EP_KIND;		//Double-buffer
	}

	// if DTOG_TX is 1 then we need to write 1 to toggle it to zero
	if((*reg & USB_EP_DTOG_TX)!=0)
		*reg=(*reg & USB_EPREG_MASK) | USB_EP_CTR_TX | USB_EP_DTOG_TX;

	// NAK the TX endpoint (nothing to go yet)
	setTxEndpointStatus(reg,USB_EP_TX_NAK);
}

void openControlEndpoints()
{
    openTxEndpoint(&USBR->EP0R,0,USB_EP_CONTROL);
	openRxEndpoint(&USBR->EP0R,0,USB_EP_CONTROL,CONTROL_MAX_PACKET_SIZE);
}


void continueSendData(uint8_t endpointIndex) {

    uint32_t i,n;
    uint16_t *pdwVal,word;
    uint16_t length;
    const uint8_t *dataBytes;
    struct UsbInEndpointData * ep = &_inEndpointData[endpointIndex];

    // cut down the length if this will be a multi-packet transfer

    if((length=ep->remaining)>ep->maxPacketSize)
      length=ep->maxPacketSize;
    else
      length=ep->remaining;

    n=(length+1)/2;
    pdwVal=(uint16_t*)(BTABLE_BASE+ep->pmaAddress);
    dataBytes=ep->ptr;

    for(i=n;i!=0;i--) {
      word=dataBytes[0] | ((uint16_t)dataBytes[1] << 8);
      *pdwVal++=word;
      dataBytes+=2;
    }

    // update status

    ep->ptr+=length;
    ep->remaining-=length;

    // now that the PMA memory is prepared, set the length and tell the peripheral to send it

    USBR_BDT[endpointIndex].tx.count=length;

	volatile uint16_t * epreg = &USBR->EP0R+endpointIndex*2;
	if( endpointIndex < 2 )
	{
	    setTxEndpointStatus(epreg,USB_EP_TX_VALID);
	}
	else
	{
	    USBR_BDT[endpointIndex].rx.count=length;
	    setTxEndpointStatus(epreg,USB_EP_TX_VALID);
//		  uint16_t value=*epreg & USB_EPTX_DTOGMASK;
//  		*epreg=setTxStatus(value,USB_EP_TX_VALID) | USB_EP_CTR_RX | USB_EP_CTR_TX   | USB_EPTX_STAT;
//		send_text_value( "   ",epreg );
		//*epreg|= USB_EPTX_STAT  | USB_EP_CTR_RX | USB_EP_CTR_TX;
		//*((uint16_t*)USB_EP3R) ^= USB_EP_DTOG_TX;

	}
  }

void sendData( uint8_t endpointIndex, uint16_t pmaInAddress, uint16_t maxPacketSize, const void *data, uint16_t length)
{
    // set up the IN endpoint structure for the transmission

    _inEndpointData[endpointIndex].total=length;
    _inEndpointData[endpointIndex].remaining=length;
    _inEndpointData[endpointIndex].ptr=(const uint8_t *)(data);
    _inEndpointData[endpointIndex].pmaAddress=pmaInAddress;
    _inEndpointData[endpointIndex].maxPacketSize=maxPacketSize;

    // continue from the beginning

    continueSendData(endpointIndex);

}


void sendControlData(const void *data,uint16_t length) {
    _controlEndpointState=USB_EST_DATA_IN;
    sendData(0,GET_PMA(0,0),CONTROL_MAX_PACKET_SIZE,data,length);
  }


void sendControlZeroLengthPacket() {
    _controlEndpointState=USB_EST_STATUS_IN;
    sendData(0,GET_PMA(0,0),CONTROL_MAX_PACKET_SIZE,0,0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


void handleControlOutTransfer( )
{
	uint16_t epr0 = USBR->EP0R;
	int i;


	struct USBSetupHeader * hdr = (struct USBSetupHeader*)(BTABLE_BASE+GET_PMA(0,1));

	if( epr0 & USB_EP_SETUP )
	{
		// get the length from the BTABLE and initialise control state
		_setupDataLength = UBgetCount( USBR_BDT[0].rx );
		_controlEndpointState = USB_EST_SETUP;

		uint8_t mreq = hdr->bmRequest & 0x1f;

		const uint8_t * raddr = 0;
		int rlen = 0;

		switch( mreq  )  //Setup...
		{
		case USB_REQ_RECIPIENT_INTERFACE:
		case USB_REQ_RECIPIENT_DEVICE:
			switch( hdr->bRequest )
			{
			case USB_REQ_GET_DESCRIPTOR:  			//This code can be used for either device or HID descriptors
			{
				const struct descriptor_list_struct * list = descriptor_list;
				for (i=0; i < NUM_DESC_LIST; i++)
				{
					if( list->wValue != hdr->wValue || list->wIndex != hdr->wIndex )
					{
						list++;
						continue;
					}
					raddr = list->addr;
					rlen = list->length;
					rlen = (hdr->wLength < rlen) ? hdr->wLength : rlen; //make sure our packet fits in the return size.
					sendControlData( raddr, rlen );
					goto ctrl_end;
				}
				//If can't find, stall.
				goto stall;
			}
			case USB_REQ_SET_ADDRESS:
				setAddress(hdr->wValue & 0x7F);
				goto ack;
			case USB_REQ_SET_CONFIGURATION:
				if( mreq == USB_REQ_RECIPIENT_DEVICE )
				{
					//XXX Careful: This is where the really key part is, where it opens the interrupt endpoint.
					openTxEndpoint((&USBR->EP0R+(IN_ENDPOINT_ADDRESS&0x7f)*2),IN_ENDPOINT_ADDRESS&0x7f,USB_EP_INTERRUPT);
					//openRxEndpoint((&USBR->EP0R+(OUT_ENDPOINT_ADDRESS&0x7f)*2),OUT_ENDPOINT_ADDRESS&0x7f,USB_EP_INTERRUPT, ENDPOINT2_SIZE);
					usbDataOkToSend = 1;
					send_text( "EP setup\n" );
				}
				else //SET_REPORT
				{
					uint16_t len = _hidDataRemain = hdr->wLength;
					uint16_t value = hdr->wValue;
			        _controlEndpointState = USB_EST_DATA_IN;
					UBsetRxCount( &USBR_BDT[0].rx, len );
			        setRxEndpointStatus( &USBR->EP0R, USB_EP_RX_VALID );
					sendControlZeroLengthPacket();
					USBR->EP0R=USBR->EP0R & (~USB_EP_CTR_RX) & USB_EPREG_MASK;
					CBHIDSetup( len, value );

					return;
				}
				goto ack;
			case 2: //CUSTOM_HID_REQ_GET_IDLE ???
			case 10: // HID_SET_IDLE  ALSO Check to make sure  USB_REQ_RECIPIENT_INTERFACE, not DEVICE
				//XXX Accept idle or something
				goto ack;
			case 1:  //Could be CLEAR_FEATURE or GET_REPOT
			{
				if( mreq == USB_REQ_RECIPIENT_DEVICE )
				{
					//CLEAR_FEATURE
				}
				else
				{
					//GET_REPORT
					//XXX Not sure what I'm supposed to do here.
				}
				goto ack;
			}
			case 0x21: 
				//XXX Not sure why this happens.  Seems random.
				goto ack;
			default:
				send_text_value( "UNKSE: ", hdr->bmRequest );
				send_text_value( "HQ: ", hdr->bRequest );
				send_text_value( "wv: ", hdr->wValue );
				send_text_value( "wi: ", hdr->wIndex );
				send_text_value( "wl: ", hdr->wLength );
				send_text_value( "SDL: ", _setupDataLength );
				goto ack;
				break;
			}
			break;

			//XXX TODO WRONG WRONG WRONG! WHY?
		case 20:
		case 16:
			//XXX Unknown error.
			break;
		default:
			send_text_value( "UKMR: ", hdr->bmRequest );
			send_text_value( "HQ: ", hdr->bRequest );
			send_text_value( "wv: ", hdr->wValue );
			send_text_value( "wi: ", hdr->wIndex );
			send_text_value( "wl: ", hdr->wLength );
			send_text_value( "SDL: ", _setupDataLength );
			break;
		}
	}
	else if( epr0 & USB_EP_CTR_RX )
	{
		//If there is a "control-out" message, then the payload of the control-out will get funneled in here.

		uint16_t count;
		// clear the 'correct transfer for reception' bit for this endpoint

		//Do stuff.
		count = UBgetCount( USBR_BDT[0].rx );
		int gs = 0;

		if( _hidDataRemain )
		{
			CBHIDData( count, (uint8_t*)(BTABLE_BASE+GET_PMA(0,1)) ); // XXX XXX
			_hidDataRemain-=count;
		}

	    UBsetRxCount( &USBR_BDT[0].rx, CONTROL_MAX_PACKET_SIZE);
		setRxEndpointStatus(&USBR->EP0R,USB_EP_RX_VALID);
	}
	else
	{
		//...
		send_text_value( "EPR0: ", epr0 );
		send_text_value( "SEO: ", hdr->bmRequest );
		send_text_value( "HQ: ", hdr->bRequest );
		send_text_value( "wv: ", hdr->wValue );
		send_text_value( "wi: ", hdr->wIndex );
		send_text_value( "wl: ", hdr->wLength );
	}

ctrl_end:
	// clear the 'correct transfer for reception' bit for this endpoint
	USBR->EP0R=USBR->EP0R & (~USB_EP_CTR_RX) & USB_EPREG_MASK;
	return;
ack:
	sendControlZeroLengthPacket();
stall:
	USBR->EP0R=USBR->EP0R & (~USB_EP_CTR_RX) & USB_EPREG_MASK;
	setRxEndpointStatus(&USBR->EP0R,USB_EP_RX_STALL);
}

void handleControlInTransfer()
{

    USBR->EP0R=USBR->EP0R & (~USB_EP_CTR_TX) & USB_EPREG_MASK;

    if(_controlEndpointState==USB_EST_DATA_IN) {
		if(_inEndpointData[0].remaining) {
			// continue sending the next in the multi-packet transfer
			continueSendData(0);
			// prepare for premature end of transfer
			UBsetRxCount( &USBR_BDT[0].rx, 0 );
			setRxEndpointStatus(&USBR->EP0R,USB_EP_RX_VALID);
		}
		else
		{
			// if we're sending a multiple of max packet size then a zero length is required

			if((_inEndpointData[0].total % CONTROL_MAX_PACKET_SIZE)==0 &&
				_inEndpointData[0].total>CONTROL_MAX_PACKET_SIZE &&
				_inEndpointData[0].total<_setupDataLength) {

				// send zero length packet

				sendData(0,GET_PMA(0,1),CONTROL_MAX_PACKET_SIZE,0,0);

				// prepare for premature end of transfer

				UBsetRxCount( &USBR_BDT[0].rx, 0);
				setRxEndpointStatus(&USBR->EP0R,USB_EP_RX_VALID);
			}
			else {
				_controlEndpointState=USB_EST_STATUS_OUT;
				UBsetRxCount( &USBR_BDT[0].rx, 0 );
				setRxEndpointStatus(&USBR->EP0R,USB_EP_RX_VALID);
			}
		}
	}
	if(_address>0 && _inEndpointData[0].remaining==0) {
		USBR->DADDR=_address | USB_DADDR_EF;
		_address=0;
	}
}

void handleEndpointOutTransfer( volatile uint16_t * reg, uint8_t endpointIndex )
{
	//NOTE: Not implemented.
	//send_text_value( "EPO: ", endpointIndex );
}
void handleEndpointInTransfer( volatile uint16_t * reg, uint8_t endpointIndex )
{
	//TODO: Writeme
    *reg=*reg & (~USB_EP_CTR_TX) & USB_EPREG_MASK;
	usbDataOkToSend = 1;
}

void onCorrectTransfer()
{
	uint16_t irq;
	uint8_t endpointIndex;
	uint8_t isOut;
	volatile uint16_t *reg;

	// USB_ISTR_CTR is read only and will be automatically cleared by hardware
	// when we've processed all endpoint results

	while(((irq=USBR->ISTR) & USB_ISTR_CTR)!=0) {
		endpointIndex = irq & USB_ISTR_EP_ID;
		isOut = (irq & USB_ISTR_DIR)!=0;

		if(endpointIndex==0) {
			// control endpoint events
			if( isOut )
				handleControlOutTransfer(  );
			else
				handleControlInTransfer(  );
		} else {
		//	send_text_value( "   ", endpointIndex );
			// normal endpoint events
			reg=&USBR->EP0R+2*endpointIndex;
			if((*reg & USB_EP_CTR_RX)!=0)
				handleEndpointOutTransfer(reg,endpointIndex);
			if((*reg & USB_EP_CTR_TX)!=0)
				handleEndpointInTransfer(reg,endpointIndex);
		}
	}
}


void __attribute__ ((interrupt("IRQ"))) USB_IRQHandler(void)
{
	usbirqlast = USBR->ISTR;

	if( USBR->ISTR & USB_ISTR_CTR )
	{
		onCorrectTransfer();
	}

	if( USBR->ISTR & USB_ISTR_RESET )
	{
	    USBR->ISTR &= ~USB_ISTR_RESET;
    	setAddress(0);
    	openControlEndpoints();
		//May want to notify the rest of the system.
	}

	//Clear out any other interrupt stuff.
	if( USBR->ISTR & ( USB_ISTR_ESOF | USB_ISTR_PMAOVR | USB_ISTR_ERR | USB_ISTR_SOF ) )
	{
		USBR->ISTR &= ~( USB_ISTR_ESOF | USB_ISTR_PMAOVR | USB_ISTR_ERR | USB_ISTR_SOF );
	}

	// wakeup
	if( USBR->ISTR & USB_ISTR_WKUP )
	{
		USBR->CNTR &= ~USB_CNTR_LPMODE; // come out of low power mode
		setInterruptMask();  // set the interrupt mask
		USBR->ISTR &= ~USB_ISTR_WKUP; // clear interrupt flag
		setDeviceState( _savedState ); // notify
	}

	// suspend
	if((USBR->ISTR & USB_ISTR_SUSP)!=0)
	{
		USBR->ISTR &= ~USB_ISTR_SUSP;  // clear interrupt flag
		// suspend and low power mode
		USBR->CNTR |= USB_CNTR_FSUSP;
		USBR->CNTR |= USB_CNTR_LPMODE;
		_savedState = _deviceState;
		setDeviceState( USB_DS_SUSPENDED );

	}
}



void UBsetRxCount(volatile struct UsbBufferDescriptionTableEntry * e, int length)
{
	int nb = length>>5;
	if( !(length & 0x1f) )
	{
		nb--;
	}
	e->count = (nb << 10) | 0x8000;
}

/*  ORIGINAL CODE:
      if((length)>62) {

        if((length & 0x1f)==0)
          wNBlocks--;

        e->count=(wNBlocks << 10) | 0x8000;
      }
      else {

        wNBlocks=length >> 1;

        if((length & 0x1)!=0)
          wNBlocks++;

        e->count=wNBlocks << 10;
      }
    }*/

