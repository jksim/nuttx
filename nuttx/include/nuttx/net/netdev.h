/****************************************************************************
 * include/nuttx/net/netdev.h
 * Defines architecture-specific device driver interfaces to the uIP network.
 *
 *   Copyright (C) 2007, 2009, 2011-2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Derived largely from portions of uIP with has a similar BSD-styple license:
 *
 *   Copyright (c) 2001-2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_NET_NETDEV_H
#define __INCLUDE_NUTTX_NET_NETDEV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdint.h>
#include <net/if.h>

#include <nuttx/net/uip.h>
#ifdef CONFIG_NET_IGMP
#  include <nuttx/net/igmp.h>
#endif

#include <nuttx/net/netconfig.h>
#include <net/ethernet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure collects information that is specific to a specific network
 * interface driver.  If the hardware platform supports only a single instance
 * of this structure.
 */

struct net_driver_s
{
  /* This link is used to maintain a single-linked list of ethernet drivers.
   * Must be the first field in the structure due to blink type casting.
   */

#if CONFIG_NSOCKET_DESCRIPTORS > 0
  FAR struct net_driver_s *flink;

  /* This is the name of network device assigned when netdev_register was called.
   * This name is only used to support socket ioctl lookups by device name
   * Examples: "eth0"
   */

  char d_ifname[IFNAMSIZ];
#endif

  /* Drivers interface flags.  See IFF_* definitions in include/net/if.h */

  uint8_t d_flags;

  /* Ethernet device identity */

#ifdef CONFIG_NET_ETHERNET
  struct ether_addr d_mac;  /* Device MAC address */
#endif

  /* Network identity */

  net_ipaddr_t d_ipaddr;  /* Host IP address assigned to the network interface */
  net_ipaddr_t d_draddr;  /* Default router IP address */
  net_ipaddr_t d_netmask; /* Network subnet mask */

  /* The d_buf array is used to hold incoming and outgoing packets. The device
   * driver should place incoming data into this buffer. When sending data,
   * the device driver should read the link level headers and the TCP/IP
   * headers from this buffer. The size of the link level headers is
   * configured by the UIP_LLH_LEN define.
   *
   * uIP will handle only a single buffer for both incoming and outgoing
   * packets.  However, the drive design may be concurrently send and
   * filling separate, break-off buffers if CONFIG_NET_MULTIBUFFER is
   * defined.  That buffer management must be controlled by the driver.
   */

#ifdef CONFIG_NET_MULTIBUFFER
  uint8_t *d_buf;
#else
  uint8_t d_buf[CONFIG_NET_BUFSIZE + CONFIG_NET_GUARDSIZE];
#endif

  /* d_appdata points to the location where application data can be read from
   * or written into a packet.
   */

  uint8_t *d_appdata;

  /* This is a pointer into d_buf where a user application may append
   * data to be sent.
   */

  uint8_t *d_snddata;

#ifdef CONFIG_NET_TCPURGDATA
  /* This pointer points to any urgent TCP data that has been received. Only
   * present if compiled with support for urgent data (CONFIG_NET_TCPURGDATA).
   */

  uint8_t *d_urgdata;

  /* Length of the (received) urgent data */

  uint16_t d_urglen;
#endif

/* The length of the packet in the d_buf buffer.
 *
 * Holds the length of the packet in the d_buf buffer.
 *
 * When the network device driver calls the uIP input function,
 * d_len should be set to the length of the packet in the d_buf
 * buffer.
 *
 * When sending packets, the device driver should use the contents of
 * the d_len variable to determine the length of the outgoing
 * packet.
 */

  uint16_t d_len;

  /* When d_buf contains outgoing xmit data, d_sndlen is non-zero and represents
   * the amount of application data after d_snddata
   */

  uint16_t d_sndlen;

  /* IGMP group list */

#ifdef CONFIG_NET_IGMP
  sq_queue_t grplist;
#endif

  /* Driver callbacks */

  int (*d_ifup)(struct net_driver_s *dev);
  int (*d_ifdown)(struct net_driver_s *dev);
  int (*d_txavail)(struct net_driver_s *dev);
#ifdef CONFIG_NET_RXAVAIL
  int (*d_rxavail)(struct net_driver_s *dev);
#endif
#ifdef CONFIG_NET_IGMP
  int (*d_addmac)(struct net_driver_s *dev, FAR const uint8_t *mac);
  int (*d_rmmac)(struct net_driver_s *dev, FAR const uint8_t *mac);
#endif
#ifdef CONFIG_NETDEV_PHY_IOCTL
  int (*d_ioctl)(int cmd, struct mii_ioctl_data *req);
#endif

  /* Drivers may attached device-specific, private information */

  void *d_private;
};

typedef int (*devif_poll_callback_t)(struct net_driver_s *dev);

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * uIP device driver functions
 *
 * These functions are used by a network device driver for interacting
 * with uIP.
 *
 * Process an incoming packet.
 *
 * This function should be called when the device driver has received
 * a packet from the network. The packet from the device driver must
 * be present in the d_buf buffer, and the length of the packet
 * should be placed in the d_len field.
 *
 * When the function returns, there may be an outbound packet placed
 * in the d_buf packet buffer. If so, the d_len field is set to
 * the length of the packet. If no packet is to be sent out, the
 * d_len field is set to 0.
 *
 * The usual way of calling the function is presented by the source
 * code below.
 *
 *     dev->d_len = devicedriver_poll();
 *     if (dev->d_len > 0)
 *       {
 *         devif_input(dev);
 *         if (dev->d_len > 0)
 *           {
 *             devicedriver_send();
 *           }
 *       }
 *
 * Note: If you are writing a uIP device driver that needs ARP
 * (Address Resolution Protocol), e.g., when running uIP over
 * Ethernet, you will need to call the uIP ARP code before calling
 * this function:
 *
 *     #define BUF ((struct eth_hdr_s *)&dev->d_buf[0])
 *     dev->d_len = ethernet_devicedrver_poll();
 *     if (dev->d_len > 0)
 *       {
 *         if (BUF->type == HTONS(UIP_ETHTYPE_IP))
 *           {
 *             arp_ipin();
 *             devif_input(dev);
 *             if (dev->d_len > 0)
 *               {
 *                 arp_out();
 *                 devicedriver_send();
 *               }
 *           }
 *         else if (BUF->type == HTONS(UIP_ETHTYPE_ARP))
 *           {
 *             arp_arpin();
 *             if (dev->d_len > 0)
 *               {
 *                 devicedriver_send();
 *               }
 *           }
 *
 ****************************************************************************/

int devif_input(struct net_driver_s *dev);

/****************************************************************************
 * Polling of connections
 *
 * These functions will traverse each active uIP connection structure and
 * perform appropriate operations:  devif_timer() will perform TCP timer
 * operations (and UDP polling operations); devif_poll() will perform TCP
 * and UDP polling operations. The CAN driver MUST implement logic to
 * periodically call devif_timer(); devif_poll() may be called asynchronously
 * from the network driver can accept another outgoing packet.
 *
 * In both cases, these functions will call the provided callback function
 * for every active connection. Polling will continue until all connections
 * have been polled or until the user-supplied function returns a non-zero
 * value (which it should do only if it cannot accept further write data).
 *
 * When the callback function is called, there may be an outbound packet
 * waiting for service in the uIP packet buffer, and if so the d_len field
 * is set to a value larger than zero. The device driver should then send
 * out the packet.
 *
 * Example:
 *   int driver_callback(struct uip_driver_dev *dev)
 *   {
 *     if (dev->d_len > 0)
 *       {
 *         devicedriver_send();
 *         return 1; <-- Terminates polling if necessary
 *       }
 *     return 0;
 *   }
 *
 *   ...
 *   devif_poll(dev, driver_callback);
 *
 * Note: If you are writing a uIP device driver that needs ARP (Address
 * Resolution Protocol), e.g., when running uIP over Ethernet, you will
 * need to call the arp_out() function in the callback function
 * before sending the packet:
 *
 *   int driver_callback(struct uip_driver_dev *dev)
 *   {
 *     if (dev->d_len > 0)
 *       {
 *         arp_out();
 *         devicedriver_send();
 *         return 1; <-- Terminates polling if necessary
 *       }
 *     return 0;
 *   }
 *
 ****************************************************************************/

int devif_poll(struct net_driver_s *dev, devif_poll_callback_t callback);
int devif_timer(struct net_driver_s *dev, devif_poll_callback_t callback, int hsec);

/****************************************************************************
 * Carrier detection
 *
 * Call netdev_carrier_on when the carrier has become available and the device
 * is ready to receive/transmit packets.
 *
 * Call detdev_carrier_off when the carrier disappeared and the device has moved
 * into non operational state.
 *
 ****************************************************************************/

int netdev_carrier_on(FAR struct net_driver_s *dev);
int netdev_carrier_off(FAR struct net_driver_s *dev);

/****************************************************************************
 * Name: net_chksum
 *
 * Description:
 *   Calculate the Internet checksum over a buffer.
 *
 *   The Internet checksum is the one's complement of the one's complement
 *   sum of all 16-bit words in the buffer.
 *
 *   See RFC1071.
 *
 *   If CONFIG_NET_ARCH_CHKSUM is defined, then this function must be
 *   provided by architecture-specific logic.
 *
 * Input Parameters:
 *
 *   buf - A pointer to the buffer over which the checksum is to be computed.
 *
 *   len - The length of the buffer over which the checksum is to be computed.
 *
 * Returned Value:
 *   The Internet checksum of the buffer.
 *
 ****************************************************************************/

uint16_t net_chksum(FAR uint16_t *data, uint16_t len);

/****************************************************************************
 * Name: net_incr32
 *
 * Description:
 *
 *   Carry out a 32-bit addition.
 *
 *   By defining CONFIG_NET_ARCH_INCR32, the architecture can replace
 *   net_incr32 with hardware assisted solutions.
 *
 * Input Parameters:
 *   op32 - A pointer to a 4-byte array representing a 32-bit integer in
 *          network byte order (big endian).  This value may not be word
 *          aligned. The value pointed to by op32 is modified in place
 *
 *   op16 - A 16-bit integer in host byte order.
 *
 ****************************************************************************/

void net_incr32(FAR uint8_t *op32, uint16_t op16);

/****************************************************************************
 * Name: ip_chksum
 *
 * Description:
 *   Calculate the IP header checksum of the packet header in d_buf.
 *
 *   The IP header checksum is the Internet checksum of the 20 bytes of
 *   the IP header.
 *
 *   If CONFIG_NET_ARCH_CHKSUM is defined, then this function must be
 *   provided by architecture-specific logic.
 *
 * Returned Value:
 *   The IP header checksum of the IP header in the d_buf buffer.
 *
 ****************************************************************************/

uint16_t ip_chksum(FAR struct net_driver_s *dev);

#endif /* __INCLUDE_NUTTX_NET_NETDEV_H */