#/*
# * Licensed to the EPYSYS SCIENCE (EpiSci) under one or more
# * contributor license agreements.
# * The EPYSYS SCIENCE (EpiSci) licenses this file to You under
# * the Episys Science (EpiSci) Public License (Version 1.1) (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy/blob/master/LICENSE
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about EPYSYS SCIENCE (EpiSci):
# *      bo.ryu@episci.com
# */

#include <sys/stat.h>
#include <sstream>
#include "lte_proxy.h"
#include "nfapi_pnf.h"
#include "proxy_ss_interface.h"

namespace
{
    Multi_UE_Proxy *instance;
}

Multi_UE_Proxy::Multi_UE_Proxy(int num_of_ues,  std::string enb_ip, std::string proxy_ip, std::string ue_ip)
{
    printf("config\n");
    assert(instance == NULL);
    instance = this;
    num_ues = num_of_ues ;

    configure(enb_ip, proxy_ip, ue_ip);
//Print different values, are they saved somewhere?
    oai_subframe_init();
}

void Multi_UE_Proxy::start(softmodem_mode_t softmodem_mode)
{
    printf("start\n");
    pthread_t thread;

    configure_nfapi_pnf(vnf_ipaddr.c_str(), vnf_p5port, pnf_ipaddr.c_str(), pnf_p7port, vnf_p7port);

    if (NULL != ss_cfg_g && ss_cfg_g->cfg.vt_enabled) {
        if (pthread_create(&thread, NULL, &oai_subframe_task_vt, (void *)softmodem_mode) != 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_create failed for calling oai_subframe_task_vt");
        }
    } else {
        if (pthread_create(&thread, NULL, &oai_subframe_task, (void *)softmodem_mode) != 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_create failed for calling oai_subframe_task");
        }
    }

    for (int i = 0; i < num_ues; i++)
    {
        threads.push_back(std::thread(&Multi_UE_Proxy::receive_message_from_ue, this, i));
    }
    for (auto &th : threads)
    {

        if(th.joinable())
        {
            th.join();
        }
    }
}

void Multi_UE_Proxy::configure(std::string enb_ip, std::string proxy_ip, std::string ue_ip)
{
    //for (i = 0; i< sizeof(ue_ip); i++){
    //    printf("%d : %s \n", i, ue_ip[])
    //}

    oai_ue_ipaddr =  "172.18.0.3";//"127.0.0.1"; //ue_ip;
    std::string oai_ue_ipaddr2 = "172.18.0.2"; //ue_ip2;
    std::string oai_ue_ipaddr3 = "172.18.0.4"; //ue_ip2;
    vnf_ipaddr = enb_ip;
    pnf_ipaddr = proxy_ip;
    vnf_p5port = 50001;
    vnf_p7port = 50011;
    pnf_p7port = 50010;
    std::string ues[3] = {oai_ue_ipaddr, oai_ue_ipaddr3, oai_ue_ipaddr2};
    //std::string ues[2] = {oai_ue_ipaddr2, oai_ue_ipaddr2};
    //std::string ues[2] = {oai_ue_ipaddr, oai_ue_ipaddr};


    std::cout<<"VNF is on IP Address "<<vnf_ipaddr<<std::endl;
    std::cout<<"PNF is on IP Address "<<pnf_ipaddr<<std::endl;
    std::cout<<"OAI-UE is on IP Address "<<oai_ue_ipaddr<<std::endl;
    std::cout<<"OAI-UE2 is on IP Address "<<oai_ue_ipaddr2<<std::endl;

    
    for (int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        //printf ("test: %d \n Secondtest : %d\n", oai_ue_ipaddr.c_str() == oai_ue_ipaddr2.c_str(), ue_ip.c_str() == ues[ue_idx].c_str() );
        //printf("%s :  %s : bool %d : test %d \n", ues[ue_idx].c_str() , oai_ue_ipaddr.c_str(), ues[ue_idx].c_str() == oai_ue_ipaddr.c_str() );
        //printf("%d : %s \n", ue_idx, ues[ue_idx].c_str());
        //printf("%d \n", ues[ue_idx].c_str()== oai_ue_ipaddr.c_str());
        printf("%d\n",ue_idx);
        int oai_rx_ue_port = 3211 + ue_idx * port_delta;
        int oai_tx_ue_port = 3212 + ue_idx * port_delta;
        // if (ue_idx == 0){
        //     results = init_oai_socket(ues[0].c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
        //     printf("first \n");
        // }
        // else{
        //     printf("second\n");
        //     results = init_oai_socket(ues[1].c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
            
        // }
        printf("starting for %s\n", ues[ue_idx].c_str());
        int result = init_oai_socket(ues[ue_idx].c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
        if (result == -1)
            printf("failed rx\n");
    }

    // for (int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    // {
    //     int oai_rx_ue_port = 3211; //+ ue_idx * port_delta;
    //     int oai_tx_ue_port = 3212; //+ ue_idx * port_delta;
    //     init_oai_socket(oai_ue_ipaddr.c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
    // }
}

int Multi_UE_Proxy::init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx)
{
    {   //Setup Rx Socket
        //printf("Rxstart\n");
        memset(&address_rx_[ue_idx], 0, sizeof(address_rx_[ue_idx]));
        address_rx_[ue_idx].sin_family = AF_INET;
        address_rx_[ue_idx].sin_addr.s_addr = INADDR_ANY;
        address_rx_[ue_idx].sin_port = htons(rx_port);

        ue_rx_socket_ = socket(address_rx_[ue_idx].sin_family, SOCK_DGRAM, 0);
        //printf("rxsocket %d\n", ue_rx_socket_);
        ue_rx_socket[ue_idx] = ue_rx_socket_;
        if (ue_rx_socket_ < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
            return -1;
        }
        if (bind(ue_rx_socket_, (struct sockaddr *)&address_rx_[ue_idx], sizeof(address_rx_[ue_idx])) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "bind failed in init_oai_socket: %s\n", strerror(errno));
            printf("failed rx bind \n");
            close(ue_rx_socket_);
            ue_rx_socket_ = -1;
            return -1;
        }
    }
    
    {   //Setup Tx Socket
        //printf("Txstart\n");
        memset(&address_tx_[ue_idx], 0, sizeof(address_tx_[ue_idx]));
        address_tx_[ue_idx].sin_family = AF_INET;
        address_tx_[ue_idx].sin_port = htons(tx_port);

        if (inet_aton(addr, &address_tx_[ue_idx].sin_addr) == 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "addr no good %s", addr);
            return -1;
        }

        ue_tx_socket_ = socket(address_tx_[ue_idx].sin_family, SOCK_DGRAM, 0);
        ue_tx_socket[ue_idx] = ue_tx_socket_;
        //printf("txsocket %d\n", ue_tx_socket_);

        if (ue_tx_socket_ < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
            return -1;
        }

        if (connect(ue_tx_socket_, (struct sockaddr *)&address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
        {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "tx connection failed in init_oai_socket: %s\n", strerror(errno));
          close(ue_tx_socket_);
          return -1;
        }
    }
    return 0;
}

void Multi_UE_Proxy::receive_message_from_ue(int ue_idx)
{
    //printf("thread %d\n",ue_idx);
    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    socklen_t addr_len = sizeof(address_rx_[ue_idx]);
    while(true)
    {
        //if(ue_idx !=0){
        printf("pre id: %d\n", ue_idx);
            //printf("%d\n",ue_rx_socket[ue_idx]);
        //}

        int buflen = recvfrom(ue_rx_socket[ue_idx], buffer, sizeof(buffer), 0, (sockaddr *)&address_rx_[ue_idx], &addr_len); // dies here?

        if(ue_idx !=0){
            printf("post id: %d : %d\n", ue_idx, buflen);
        }
        if (buflen == -1)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Recvfrom failed %s", strerror(errno));
            printf("failed\n");
            return ;
        }
        if (buflen == 4)
        {
            //NFAPI_TRACE(NFAPI_TRACE_INFO , "Dummy frame");
            continue;

        }
        else
        {
            printf("else\n");
            nfapi_p7_message_header_t header;
            if (nfapi_p7_message_header_unpack(buffer, buflen, &header, sizeof(header), NULL) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Header unpack failed for standalone pnf");
                return ;
            }
            uint16_t sfn_sf = nfapi_get_sfnsf(buffer, buflen);
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(Proxy) Proxy has received %d uplink message from OAI UE at socket. Frame: %d, Subframe: %d",
                    header.message_id, NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf));
        }
        oai_subframe_handle_msg_from_ue(buffer, buflen, ue_idx + 2);

    }

}

void Multi_UE_Proxy::oai_enb_downlink_nfapi_task(void *msg_org)
{
    lock_guard_t lock(mutex);

    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    int encoded_size = nfapi_p7_message_pack(msg_org, buffer, sizeof(buffer), nullptr);
    if (encoded_size <= 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message pack failed");
        return;
    }

    union
    {
        nfapi_p7_message_header_t header;
        nfapi_dl_config_request_t dl_config_req;
        nfapi_tx_request_t tx_req;
        nfapi_hi_dci0_request_t hi_dci0_req;
        nfapi_ul_config_request_t ul_config_req;
        vendor_nfapi_cell_search_indication_t cell_info;
    } msg;

    if (nfapi_p7_message_unpack((void *)buffer, encoded_size, &msg, sizeof(msg), NULL) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p7_message_unpack failed NEM ID: %d", 1);
        return;
    }

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        address_tx_[ue_idx].sin_port = htons(3212 + ue_idx * port_delta);
        uint16_t id_=1;
        switch (msg.header.message_id)
        {

        case NFAPI_DL_CONFIG_REQUEST:
        {
            int dl_sfn = NFAPI_SFNSF2SFN(msg.dl_config_req.sfn_sf);
            int dl_sf = NFAPI_SFNSF2SF(msg.dl_config_req.sfn_sf);
            uint16_t dl_numPDU = msg.dl_config_req.dl_config_request_body.number_pdu;
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(UE) Prior to sending dl_config_req to OAI UE. Frame: %d,"
                       " Subframe: %d, Number of PDUs: %u",
                       dl_sfn, dl_sf, dl_numPDU);
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_DL_CONFIG_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "DL_CONFIG_REQ forwarded to UE from UE NEM: %u", id_);
            }
            break;
        }
        case NFAPI_TX_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_TX_CONFIG_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "TX_REQ forwarded to UE from UE NEM: %u", id_);
            }
            break;

        case NFAPI_UL_CONFIG_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_UL_CONFIG_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "UL_CONFIG_REQ forwarded to UE from UE NEM: %u", id_);
            }
            break;

        case NFAPI_HI_DCI0_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_HI_DCI0_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "NFAPI_HI_DCI0_REQ forwarded to UE from UE NEM: %u", id_);
            }
            break;
        case P7_CELL_SEARCH_IND:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send P7_CELL_SRCH_IND to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "P7_CELL_SRCH_IND forwarded to UE from UE NEM: %u", id_);
            }
            break;


        default:
            NFAPI_TRACE(NFAPI_TRACE_INFO , "Unhandled message at UE NEM: %d message_id: %u", id_, msg.header.message_id);
            break;
        }
    }
}

void Multi_UE_Proxy::pack_and_send_downlink_sfn_sf_msg(uint16_t sfn_sf)
{
    lock_guard_t lock(mutex);

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        address_tx_[ue_idx].sin_port = htons(3212 + ue_idx * port_delta);
        //printf(" assert error  %d\n", ue_tx_socket[ue_idx]);
        assert(ue_tx_socket[ue_idx] > 2);
        if (sendto(ue_tx_socket[ue_idx], &sfn_sf, sizeof(sfn_sf), 0, (const struct sockaddr *) &address_tx_[ue_idx], sizeof(address_tx_[ue_idx])) < 0)
        {
            int sfn = NFAPI_SFNSF2SFN(sfn_sf);
            int sf = NFAPI_SFNSF2SF(sfn_sf);
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send sfn_sf_tx to OAI UE FAIL Frame: %d,Subframe: %d", sfn, sf);
        }
    }
}

void transfer_downstream_nfapi_msg_to_proxy(void *msg)
{
    instance->oai_enb_downlink_nfapi_task(msg);
}
void transfer_downstream_sfn_sf_to_proxy(uint16_t sfn_sf)
{
    instance->pack_and_send_downlink_sfn_sf_msg(sfn_sf);
}
