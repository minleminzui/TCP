#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    
    // 查询arp表
    const auto& arp_iter = _arp_table.find(next_hop_ip);

    // 如果 arp 表中没查到
    if(arp_iter == _arp_table.end()){
        // 如果5s内没有发送对于该next_hop_ip的arp-request,则需要发送
        if(_waiting_arp_response_ip_addr.find(next_hop_ip) == _waiting_arp_response_ip_addr.end()){
            ARPMessage arp_request;// 这个是arp-request的payload
            arp_request.opcode = ARPMessage::OPCODE_REQUEST;// 这个字段用来反应是arp-request还是arp-reply
            arp_request.sender_ethernet_address = _ethernet_address;// 发送者的以太网地址
            arp_request.sender_ip_address = _ip_address.ipv4_numeric();// 发送者的ip地址
            arp_request.target_ethernet_address = {};// 目前不知道对方的以太网地址，所以留空
            arp_request.target_ip_address = next_hop_ip;// 目的ip地址

            EthernetFrame eth_frame;
            eth_frame.header() = {ETHERNET_BROADCAST,// 目的以太网地址
                                    _ethernet_address,// 源以太网地址
                                    EthernetHeader::TYPE_ARP// 如果是arp-request包，那么类型是arp
                                    };
            eth_frame.payload() = arp_request.serialize();// 将序列化的arp_message作为arp_request的payload
            _frames_out.emplace(eth_frame);
            _waiting_arp_response_ip_addr[next_hop_ip] = _arp_response_default_ttl;
        }
        // 这里需要把还不知道以太网地址的Internet数据报加入等待队列，直到收到arp响应
        //  and queue the IP datagram so it can be sent after the ARP reply is received.
        _waiting_arp_internet_datagrams.emplace_back(next_hop, dgram);
    }else{
        // 否则，需要把Internet包放入以太网帧中发送
        EthernetFrame eth_frame;
        eth_frame.header() = {  arp_iter->second.eth_addr,// 目的以太网帧地址
                                _ethernet_address,// 源以太网弟子
                                EthernetHeader::TYPE_IPv4// 类型
                                };
        eth_frame.payload() = dgram.serialize();
        _frames_out.emplace(eth_frame);
    }
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);
    // 对于收到错误的目的以太网地址，The code should ignore any frames not destined for the network interface
    if(frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST)
        return nullopt;
    // 如果收到IP包    
    if(frame.header().type == EthernetHeader::TYPE_IPv4){
        InternetDatagram datagram;
        /*If the inbound frame is IPv4, parse the payload as an InternetDatagram and,
            if successful (meaning the parse() method returned ParseResult::NoError),
            return the resulting InternetDatagram to the caller.*/
        if(datagram.parse(frame.payload()) != ParseResult::NoError)
            return nullopt;
        return datagram;
    }
    //如果是arp包
    else if(frame.header().type == EthernetHeader::TYPE_ARP){
        ARPMessage arp_msg;
        if(arp_msg.parse(frame.payload()) != ParseResult::NoError)
            return nullopt;

        // 获取目的与源地址
        const uint32_t& src_ip_addr = arp_msg.sender_ip_address;
        const uint32_t& dst_ip_addr = arp_msg.target_ip_address;
        const EthernetAddress& src_eth_addr = arp_msg.sender_ethernet_address;
        const EthernetAddress& dst_eth_addr = arp_msg.target_ethernet_address;
        
        // 判断是否是发送给自己的arp-request
        bool is_vaild_arp_request = 
            arp_msg.opcode == ARPMessage::OPCODE_REQUEST && dst_ip_addr == _ip_address.ipv4_numeric();
        
        // 判断是否是一个对于自己发送的arp-request的reply报文
        bool is_vaild_arp_reply = 
            arp_msg.opcode == ARPMessage::OPCODE_REPLY && dst_eth_addr == _ethernet_address;
        /* In addition, if it’s
            an ARP request asking for our IP address, 
            send an appropriate ARP reply*/
        if(is_vaild_arp_request){
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = _ethernet_address;  
            arp_reply.target_ethernet_address = src_eth_addr;
            arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
            arp_reply.target_ip_address = src_ip_addr;

            EthernetFrame eth_frame;
            eth_frame.header() = {  src_eth_addr,// 目的以太网地址
                                    _ethernet_address,// 源以太网地址
                                    EthernetHeader::TYPE_ARP// 类型是arp
                                    };
            eth_frame.payload() = arp_reply.serialize();
            
            _frames_out.emplace(eth_frame);
        }

        // 对于收到reply或request，我们需要更新arp表，并发送_waiting_arp_internet_datagrams中的报文
        if(is_vaild_arp_reply || is_vaild_arp_request){
            _arp_table[src_ip_addr] = {src_eth_addr, _arp_entry_deafult_ttl};

            for(auto iter = _waiting_arp_internet_datagrams.begin(); iter != _waiting_arp_internet_datagrams.end();){
                if(iter->first.ipv4_numeric() == src_ip_addr){
                    send_datagram(iter -> second, iter -> first);
                    iter = _waiting_arp_internet_datagrams.erase(iter);//迭代器不能在for()中增加,list中erase的用法
                }else{
                    ++iter;
                }
            }
            _waiting_arp_response_ip_addr.erase(src_ip_addr);
        }
    }
    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    //  DUMMY_CODE(ms_since_last_tick); 
    // 将ARP表中的过期条目删除
    for(auto iter = _arp_table.begin(); iter != _arp_table.end(); /* nop */) {
        if (iter->second.ttl <= ms_since_last_tick)
            iter = _arp_table.erase(iter);
        else {
            iter->second.ttl -= ms_since_last_tick;
            ++iter;
        }
    }

    // 将 ARP 等待队列中过期的条目删除
    for (auto iter = _waiting_arp_response_ip_addr.begin(); iter != _waiting_arp_response_ip_addr.end(); /* nop */) {
        // 如果 ARP 等待队列中的 ARP 请求过期
        if (iter->second <= ms_since_last_tick) {
            // 重新发送 ARP 请求
            // 注意这里，不能把ARP等待队列中的数据删除，需要把过期arp-request重发就够了
            // 并且把_waiting_arp_response_ip_addr中的ttl重新置为5s
            ARPMessage arp_request;
            arp_request.opcode = ARPMessage::OPCODE_REQUEST;
            arp_request.sender_ethernet_address = _ethernet_address;
            arp_request.sender_ip_address = _ip_address.ipv4_numeric();
            arp_request.target_ethernet_address = {/* 这里应该置为空*/};
            arp_request.target_ip_address = iter->first;

            EthernetFrame eth_frame;
            eth_frame.header() = {/* dst  */ ETHERNET_BROADCAST,
                                  /* src  */ _ethernet_address,
                                  /* type */ EthernetHeader::TYPE_ARP};
            eth_frame.payload() = arp_request.serialize();
            _frames_out.push(eth_frame);

            iter->second = _arp_response_default_ttl;
        } else {
            iter->second -= ms_since_last_tick;
            ++iter;
        }
    }
}

