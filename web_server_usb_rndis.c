// PicoWi Web server, see https://iosoft.blog/picowi
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

// v0.38 JPB 1/5/23  Adapted for iosoft.blog post

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lib/picowi_defs.h"
#include "lib/picowi_pico.h"
#include "lib/picowi_wifi.h"
#include "lib/picowi_init.h"
#include "lib/picowi_ioctl.h"
#include "lib/picowi_event.h"
#include "lib/picowi_join.h"
#include "lib/picowi_ip.h"
#include "lib/picowi_dhcp.h"
#include "lib/picowi_net.h"
#include "lib/picowi_tcp.h"
#include "lib/picowi_web.h"

#include "usbd_core.h"
#include "usbd_rndis.h"
#include "cdc_rndis_device.h"
#include "lwip.h"

// The hard-coded password is for test purposes only!!!
#define SSID                "test"
#define PASSWD              "testpass"
#define EVENT_POLL_USEC     100000
#define PING_RESP_USEC      200000
#define PING_DATA_SIZE      32

#define TEST_BLOCK_COUNT 100

extern MACADDR my_mac;

int simple_eth_event_handler(EVENT_INFO *eip)
{
    if (eip->chan == SDPCM_CHAN_DATA &&
        eip->dlen >= sizeof(ETHERHDR))
    {
		struct pbuf *out_pkt;
		out_pkt = pbuf_alloc(PBUF_RAW, eip->dlen, PBUF_POOL);
		//memcpy(out_pkt->payload, buf, len);
		pbuf_take(out_pkt, eip->data, eip->dlen);
        usbd_rndis_eth_tx(out_pkt);
    }
    return(1);
}

int main()
{
    uint32_t led_ticks;
    bool ledon = false;
    int server_sock;
    
    //set_display_mode(DISP_INFO | DISP_JOIN | DISP_TCP);
    set_display_mode(DISP_INFO | DISP_JOIN | DISP_TCP_STATE);
    io_init();
    usdelay(1000);
	add_event_handler(simple_eth_event_handler);
    if (net_init() && net_join(SSID, PASSWD))
    {
		cdc_rndis_init(my_mac);
        ustimeout(&led_ticks, 0);
        while (1)
        {
            // Get any events, poll the network-join state machine
            net_event_poll();
            net_state_poll();
            // Toggle LED at 0.5 Hz if joined, 5 Hz if not
            if (ustimeout(&led_ticks, link_check() > 0 ? 1000000 : 100000))
                wifi_set_led(ledon = !ledon);
        }
    }
}
