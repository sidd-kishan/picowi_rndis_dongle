// PicoWi network join example, see http://iosoft.blog/picowi for details
//
// Copyright (c) 2022, Jeremy P Bentham
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>

#include "lib/picowi_defs.h"
#include "lib/picowi_pico.h"
#include "lib/picowi_wifi.h"
#include "lib/picowi_init.h"
#include "lib/picowi_ioctl.h"
#include "lib/picowi_event.h"
#include "lib/picowi_join.h"
#include "lib/picowi_ip.h"

#include "usbd_core.h"
#include "usbd_rndis.h"
#include "cdc_rndis_device.h"
#include "lwip.h"

// The hard-coded password is for test purposes only!!!
#define SSID                "test"
#define PASSWD              "testpass"
#define EVENT_POLL_USEC     1000

uint8_t rndis_mac[6] = { 0x20, 0x89, 0x84, 0x6A, 0x96, 0xAA };
extern MACADDR my_mac;

int simple_eth_event_handler(EVENT_INFO *eip)
{
    if (eip->chan == SDPCM_CHAN_DATA)
    {
		struct pbuf *out_pkt;
		out_pkt = pbuf_alloc(PBUF_RAW, eip->dlen, PBUF_POOL);
		//memcpy(out_pkt->payload, buf, len);
		pbuf_take(out_pkt, eip->data, eip->dlen);
        usbd_rndis_eth_tx(out_pkt);
		pbuf_free(out_pkt);
		out_pkt = NULL;
		return(1);
    }
    return 0;
}

int main() 
{
    uint32_t led_ticks, poll_ticks;
	struct pbuf *p;
    bool ledon=false;
    set_sys_clock_khz(230000, true);
    add_event_handler(join_event_handler);
	add_event_handler(simple_eth_event_handler);
    set_display_mode(DISP_INFO);
    io_init();
    usdelay(1000);
    printf("PicoWi network scan\n");
    if (!wifi_setup())
        printf("Error: SPI communication\n");
    else if (!wifi_init())
        printf("Error: can't initialise WiFi\n");
    else if (!join_start(SSID, PASSWD))
        printf("Error: can't start network join\n");
    else
    {
		//init the usb stack
		cdc_rndis_init(my_mac);
        // Additional diagnostic display
        //set_display_mode(DISP_INFO|DISP_EVENT|DISP_SDPCM|DISP_REG|DISP_JOIN|DISP_DATA);
        set_display_mode(DISP_INFO|DISP_EVENT|DISP_JOIN);
        ustimeout(&led_ticks, 0);
        ustimeout(&poll_ticks, 0);
        while (1)
        {
            // Toggle LED at 1 Hz if joined, 5 Hz if not
            if (ustimeout(&led_ticks, link_check() > 0 ? 500000 : 100000))
                wifi_set_led(ledon = !ledon);
			if(link_check()){
				p = usbd_rndis_eth_rx();
				if (p != NULL) {
					event_net_tx(p->payload,p->len);
					pbuf_free(p);
					p = NULL;
				}
			}
                
            // Get any events, poll the joining state machine
            if (wifi_get_irq() || ustimeout(&poll_ticks, EVENT_POLL_USEC))
            {
                event_poll();
                join_state_poll(SSID, PASSWD);
                ustimeout(&poll_ticks, 0);
            }
        }
    }
}

// EOF
