/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "lwip/igmp.h"
#include "lwip/mld6.h"

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
#include "FreeRTOS.h"
#include "event_groups.h"
#endif

#include "ethernetif.h"

#include "fsl_enet.h"
#include "fsl_phy.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#define ENET_ALIGN(x) ((unsigned int)((x) + ((ENET_BUFF_ALIGNMENT)-1)) & (unsigned int)(~(unsigned int)((ENET_BUFF_ALIGNMENT)-1)))

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 */
struct ethernetif{
    ENET_Type           *base;
    enet_handle_t       handle;
    uint32_t            phyAddr;
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    EventGroupHandle_t  enetTransmitAccessEvent;
    EventBits_t         txFlag;
#endif
    uint8_t RxBuffDescrip[ENET_RXBD_NUM * sizeof(enet_rx_bd_struct_t) + ENET_BUFF_ALIGNMENT];
    uint8_t TxBuffDescrip[ENET_TXBD_NUM * sizeof(enet_tx_bd_struct_t) + ENET_BUFF_ALIGNMENT];
    uint8_t RxDataBuff[ENET_RXBD_NUM * ENET_ALIGN(ENET_RXBUFF_SIZE) + ENET_BUFF_ALIGNMENT];
    uint8_t TxDataBuff[ENET_TXBD_NUM * ENET_ALIGN(ENET_TXBUFF_SIZE) + ENET_BUFF_ALIGNMENT];
};

/*******************************************************************************
 * Variables
 ******************************************************************************/
static struct ethernetif ethernetif_0; 

/*******************************************************************************
 * Code
 ******************************************************************************/
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
void ethernet_callback(ENET_Type *base, enet_handle_t *handle, enet_event_t event, void *param)
{
    struct netif *netif = (struct netif *)param;
    struct ethernetif *ethernetif = netif->state;

    switch (event)
    {
        case kENET_RxEvent:
            ethernetif_input(netif);
            break;
        case kENET_TxEvent:
        {
            portBASE_TYPE taskToWake = pdFALSE;

            if (__get_IPSR())
            {
                xEventGroupSetBitsFromISR(ethernetif->enetTransmitAccessEvent, ethernetif->txFlag, &taskToWake);
                if (pdTRUE == taskToWake)
                {
                    taskYIELD();
                }
            }
            else
            {
                xEventGroupSetBits(ethernetif->enetTransmitAccessEvent, ethernetif->txFlag);
            }
        }
        break;
        default:
            break;
    }
}
#endif

#if LWIP_IPV4 && LWIP_IGMP
static err_t ethernetif_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, u8_t action) {
  struct ethernetif *ethernetif = netif->state;
  uint8_t multicastMacAddr[6];
  err_t result;  

  multicastMacAddr[0] = 0x01U;
  multicastMacAddr[1] = 0x00U;
  multicastMacAddr[2] = 0x5EU;
  multicastMacAddr[3] = (group->addr>>8) & 0x7FU;
  multicastMacAddr[4] = (group->addr>>16)& 0xFFU;
  multicastMacAddr[5] = (group->addr>>24)& 0xFFU;

  switch (action) {
    case IGMP_ADD_MAC_FILTER: 
      /* Adds the ENET device to a multicast group.*/
      ENET_AddMulticastGroup(ethernetif->base, multicastMacAddr);
      result = ERR_OK;
      break;
    case IGMP_DEL_MAC_FILTER:
      /* Moves the ENET device from a multicast group.*/
    #if 0
      ENET_LeaveMulticastGroup(ethernetif->base, multicastMacAddr);
    #endif
      result = ERR_OK;
      break;
    default:
      result = ERR_IF;
      break;
  }

  return result;
}
#endif

#if LWIP_IPV6 && LWIP_IPV6_MLD
static err_t ethernetif_mld_mac_filter(struct netif *netif, const ip6_addr_t *group, u8_t action) {
  struct ethernetif *ethernetif = netif->state;
  uint8_t multicastMacAddr[6];
  err_t result;  

  multicastMacAddr[0] = 0x33U;
  multicastMacAddr[1] = 0x33U;
  multicastMacAddr[2] = (group->addr[3]) & 0xFFU;
  multicastMacAddr[3] = (group->addr[3]>>8) & 0xFFU;
  multicastMacAddr[4] = (group->addr[3]>>16)& 0xFFU;
  multicastMacAddr[5] = (group->addr[3]>>24)& 0xFFU;

  switch (action) {
    case MLD6_ADD_MAC_FILTER: 
      /* Adds the ENET device to a multicast group.*/
      ENET_AddMulticastGroup(ethernetif->base, multicastMacAddr);
      result = ERR_OK;
      break;
    case MLD6_DEL_MAC_FILTER:
      /* Moves the ENET device from a multicast group.*/
    #if 0
      ENET_LeaveMulticastGroup(ethernetif->base, multicastMacAddr);
    #endif
      result = ERR_OK;
      break;
    default:
      result = ERR_IF;
      break;
  }

  return result;
}
#endif

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = configMAC_ADDR0;
  netif->hwaddr[1] = configMAC_ADDR1;
  netif->hwaddr[2] = configMAC_ADDR2;
  netif->hwaddr[3] = configMAC_ADDR3;
  netif->hwaddr[4] = configMAC_ADDR4;
  netif->hwaddr[5] = configMAC_ADDR5;

  /* maximum transfer unit */
  netif->mtu = 1500; // TODO: define a config

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  /* ENET driver initialization.*/
  {
    enet_config_t config;
    uint32_t sysClock;
    bool link = false;
    phy_speed_t speed;
    phy_duplex_t duplex;
    uint32_t count = 0;
    enet_buffer_config_t buffCfg;

    /* prepare the buffer configuration. */
    buffCfg.rxBdNumber =  ENET_RXBD_NUM;                                            /* Receive buffer descriptor number. */
    buffCfg.txBdNumber =  ENET_TXBD_NUM;                                            /* Transmit buffer descriptor number. */
    buffCfg.rxBuffSizeAlign = ENET_ALIGN(ENET_RXBUFF_SIZE);                         /* Aligned receive data buffer size. */
    buffCfg.txBuffSizeAlign = ENET_ALIGN(ENET_TXBUFF_SIZE);                         /* Aligned transmit data buffer size. */
    buffCfg.rxBdStartAddrAlign = (enet_rx_bd_struct_t *)ENET_ALIGN(ethernetif->RxBuffDescrip); /* Aligned receive buffer descriptor start address. */
    buffCfg.txBdStartAddrAlign = (enet_tx_bd_struct_t *)ENET_ALIGN(ethernetif->TxBuffDescrip); /* Aligned transmit buffer descriptor start address. */
    buffCfg.rxBufferAlign = (uint8_t *)ENET_ALIGN(ethernetif->RxDataBuff);          /* Receive data buffer start address. */
    buffCfg.txBufferAlign = (uint8_t *)ENET_ALIGN(ethernetif->TxDataBuff);          /* Transmit data buffer start address. */

    sysClock = CLOCK_GetFreq(kCLOCK_CoreSysClk);

    ENET_GetDefaultConfig(&config);
    PHY_Init(ethernetif->base, ethernetif->phyAddr, sysClock);

    while ((count < ENET_ATONEGOTIATION_TIMEOUT) && (!link))
    {
        PHY_GetLinkStatus(ethernetif->base, ethernetif->phyAddr, &link);
        if (link)
        {
            /* Get the actual PHY link speed. */
            PHY_GetLinkSpeedDuplex(ethernetif->base, ethernetif->phyAddr, &speed, &duplex);
            /* Change the MII speed and duplex for actual link status. */
            config.miiSpeed = (enet_mii_speed_t)speed;
            config.miiDuplex = (enet_mii_duplex_t)duplex;
        }

        count++;
    }

#if 0 /* Disable assert. If initial auto-negation is timeout, 
        the ENET set to default 100Mbs and full-duplex.*/
    if (count == ENET_ATONEGOTIATION_TIMEOUT)
    {
        LWIP_ASSERT("\r\nPHY Link down, please check the cable connection.\r\n", 0);
    }
#endif

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    /* Create the Event for transmit busy release trigger. */
    ethernetif->enetTransmitAccessEvent = xEventGroupCreate();
    ethernetif->txFlag = 0x1;

    config.interrupt |= kENET_RxFrameInterrupt | kENET_TxFrameInterrupt | kENET_TxBufferInterrupt;

    NVIC_SetPriority(ENET_Receive_IRQn, ENET_PRIORITY);
    NVIC_SetPriority(ENET_Transmit_IRQn, ENET_PRIORITY);
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    NVIC_SetPriority(ENET_1588_Timer_IRQn, ENET_1588_PRIORITY);
#endif
#endif

    /* Initialize the ENET module.*/
    ENET_Init(ethernetif->base, &ethernetif->handle, &config, &buffCfg, netif->hwaddr, sysClock);

#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
    ENET_SetCallback(&ethernetif->handle, ethernet_callback, netif);
#endif
    ENET_ActiveRead(ethernetif->base);
  }	
#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, MLD6_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  static unsigned char ucBuffer[ENET_FRAME_MAX_FRAMELEN];
  unsigned char *pucBuffer = ucBuffer;
  unsigned char *pucChar;

  LWIP_ASSERT("Output packet buffer empty", p);

  /* Initiate transfer. */
  
#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif
  
  if (p->len == p->tot_len)
  {
    /* No pbuf chain, don't have to copy -> faster. */
    pucBuffer = (unsigned char *)p->payload;
  }
  else
  {
    /* pbuf chain, copy into contiguous ucBuffer. */
    if (p->tot_len >= ENET_FRAME_MAX_FRAMELEN)
    {
      return ERR_BUF;
    }
    else
    {
      pucChar = ucBuffer;

      for (q = p; q != NULL; q = q->next)
      {
        /* Send the data from the pbuf to the interface, one pbuf at a
        time. The size of the data in each pbuf is kept in the ->len
        variable. */
        /* send data from(q->payload, q->len); */
        if (q == p)
        {
          memcpy(pucChar, q->payload, q->len);
          pucChar += q->len;
        }
        else
        {
          memcpy(pucChar, q->payload, q->len);
          pucChar += q->len;
        }
      }
    }
  }

  /* Send frame. */
#if USE_RTOS && defined(FSL_RTOS_FREE_RTOS)
  {
    status_t result;

    do
    {
      result = ENET_SendFrame(ethernetif->base, &ethernetif->handle, pucBuffer, p->tot_len);

      if (result == kStatus_ENET_TxFrameBusy){
            xEventGroupWaitBits(ethernetif->enetTransmitAccessEvent, ethernetif->txFlag, pdTRUE, (BaseType_t) false, portMAX_DELAY);
      }

    } while (result == kStatus_ENET_TxFrameBusy);
  }
#else
  {
    uint32_t counter;

    for (counter = ENET_TIMEOUT; counter != 0U; counter--){
      if (ENET_SendFrame(ethernetif->base, &ethernetif->handle, pucBuffer, p->tot_len) != kStatus_ENET_TxFrameBusy){
            break;
      }
    }
  }
#endif

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t*)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *p = NULL;
  uint32_t len;
  status_t status;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  status = ENET_GetRxFrameSize(&ethernetif->handle, &len); 

  if(kStatus_ENET_RxFrameEmpty != status)
  {
    /* Call ENET_ReadFrame when there is a received frame. */
    if (len != 0)
    {

    #if ETH_PAD_SIZE
      len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
    #endif

      /* We allocate a pbuf chain of pbufs from the pool. */
      p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

      if (p != NULL) {

      #if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
      #endif

        ENET_ReadFrame(ethernetif->base, &ethernetif->handle, p->payload, p->len);


      MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
      if (((u8_t*)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
      } else {
        /* unicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
      }
      #if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
      #endif

        LINK_STATS_INC(link.recv);
      } else {
        /* drop packet*/
        ENET_ReadFrame(ethernetif->base, &ethernetif->handle, NULL, 0);
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: Fail to allocate new memory space\n"));

        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
      }
    }
    else
    {
      /* Update the received buffer when error happened. */
      if (status == kStatus_ENET_RxFrameError)
      {
      #if 0 /* Error statisctics */
        enet_data_error_stats_t eErrStatic;
        /* Get the error information of the received g_frame. */
        ENET_GetRxErrBeforeReadFrame(&ethernetif->handle, &eErrStatic);
      #endif

        /* Update the receive buffer. */
        ENET_ReadFrame(ethernetif->base, &ethernetif->handle, NULL, 0);
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: RxFrameError\n"));

        LINK_STATS_INC(link.drop);
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
      }
    }

  }
  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void
ethernetif_input(struct netif *netif)
{
  struct pbuf *p;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* move received packet into a new pbuf */
  while((p = low_level_input(netif)) != NULL)
  {
    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (netif->input(p, netif) != ERR_OK) { 
      LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->state = &ethernetif_0;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if LWIP_IPV4
  netif->output = etharp_output;
#endif
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;

#if LWIP_IPV4 && LWIP_IGMP
  netif_set_igmp_mac_filter(netif, ethernetif_igmp_mac_filter);
  netif->flags |= NETIF_FLAG_IGMP;
#endif
#if LWIP_IPV6 && LWIP_IPV6_MLD
  netif_set_mld_mac_filter(netif, ethernetif_mld_mac_filter);
  netif->flags |= NETIF_FLAG_MLD6;
#endif


  /* Init ethernetif parameters.*/  
  ethernetif_0.base = ENET;
  ethernetif_0.phyAddr = ENET_PHY_ADDRESS;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}
