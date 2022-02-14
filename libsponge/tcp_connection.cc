#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 用于计算可以塞到_segments_out队列的字节数
size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    if(!_active) return;
    _time_since_last_segment_received = 0;// 接收到报文，需要重置零_time_since_last_segment_received
    auto header = seg.header();
    
    // LISTEN状态， 三次握手，接收 第一次握手，也就是被动打开，需要己方的确认号为0且可以发送的下一个序列号为0
    if(_sender.next_seqno_absolute() == 0 && !_receiver.ackno().has_value()){
        if(header.rst) return;// listen状态的rst需要无视
        if(!header.syn) return;
        _receiver.segment_received(seg);
        connect();
        return;
    }

    if(header.rst){
        // if(_sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute()
        //     && !header.ack) return;
        unclean_shutdown(false);// 收到rst不需要发送rst
        return;
    }

    // SYN_SENT 主动打开状态
    if(_sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute()){
        if(seg.payload().size() > 0) return;// 三次握手，接收 第二次握手，需要对方发送的对于第一次握手SYN包的ACK不能有数据
        if(!header.ack){// 处理两个peer同时发出第一次握手
            if(header.syn){
                _receiver.segment_received(seg);// 这边使得ackno不再是0了
                _sender.send_empty_segment();// 只需要回应ACK置位就可以了，不需要置位SYN
                send_sender_segments();
            }
            return;
        }
    }
    // if(_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 
    //     && header.seqno == _receiver.ackno().value() - 1){
    //         _sender.send_empty_segment();
    // }
    /* gives the segment to the TCPReceiver so it can inspect the fields it cares about on
        incoming segments: seqno, syn , payload, and fin .*/
    _receiver.segment_received(seg);
    /* if the ack flag is set, tells the TCPSender about the fields it cares about on incoming
    segments: ackno and window size. */
    if(header.ack){
        _sender.ack_received(header.ackno, header.win);
        // printf("Seg ACK %d win %d ackno %u\n\n", header.ack, header.win, header.ackno.raw_value());
        /* 或者说是对应于"if the incoming segment occupied any sequence numbers, the TCPConnection makes
            sure that at least one segment is sent in reply, to reflect an update in the ackno and
            window size."*/
        if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty())// 对于第三次握手，需要发送ACK报文
            _sender.send_empty_segment();

        send_sender_segments();
    }
        
    
    /*if the incoming segment occupied any sequence numbers, the TCPConnection makes
        sure that at least one segment is sent in reply, to reflect an update in the ackno and
        window size.*/
    
 }

bool TCPConnection::active() const { return _active; }// 一个TCPConnection的active只会从true到false

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    // if(!_active) return; 
    if(data.empty()) return 0;
    auto ret = _sender.stream_in().write(data);// 将数据写到_sender的ByteStream中
    // 第一次握手的 发送，会发生在这里
    /* any time the TCPSender has pushed a segment onto its outgoing queue, having set the
        fields it’s responsible for on outgoing segments: (seqno, syn , payload, and fin ).*/
    _sender.fill_window();
    // printf("TCPConnection::write the window size %lu\n\n", _receiver.window_size());
    /* Before sending the segment, the TCPConnection will ask the TCPReceiver for the fields
        it’s responsible for on outgoing segments: ackno and window size. If there is an ackno,
        it will set the ack flag and the fields in the TCPSegment */
    send_sender_segments();
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick);
    if(!_active) return;
    _time_since_last_segment_received += ms_since_last_tick;/* tell the TCPSender about the passage of time.*/
    _sender.tick(ms_since_last_tick);
    /* abort the connection, and send a reset segment to the peer (an empty segment with
        the rst flag set), if the number of consecutive retransmissions is more than an upper
        limit TCPConfig::MAX RETX ATTEMPTS.*/
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS)
        unclean_shutdown(true);

    send_sender_segments();// 依旧需要把重传的报文加上ackno与window size
}

void TCPConnection::end_input_stream() {// 结束输入报文，也就是_sender的ByteStream，_sender发字节输入到这个input接口
    _sender.stream_in().end_input();
    _sender.fill_window();// outbound stream结束，需要发送FIN包
    send_sender_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();// connect函数用于初始化连接，fill_window中会发送SYN包
    send_sender_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            unclean_shutdown(true);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

// bool TCPConnection::in_syn_sent(){
//     return _sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute();
// }

// 这个函数用来给需要发送出去的报文(也就是outbound stream)添加ackno与window size
void TCPConnection::send_sender_segments(){
    TCPSegment seg;
    // TCPSender与TCPConnection的_segments_out不同，TCPConnection的发出的报文需要添加window size与ackno
    if(_sender.segments_out().empty() && _need_send_rst){// 如果_sender中没有报文，那么我们需要添加一个空报文，用来置位rst
        _sender.send_empty_segment();
    }
    while(_sender.segments_out().size()){
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        // printf("seg payload %s\n\n", seg.payload().copy().c_str());
        // cerr<<"seg payload"<< seg.payload().copy().c_str() <<"\n\n";
        if(_receiver.ackno().has_value()){// 发送的第一次握手没有ackno，那么不需要加上ackno与window size
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        if(_need_send_rst){
            _need_send_rst = false;
            seg.header().rst = true; 
        }
        _segments_out.push(seg);
    }
    

    clean_shutdown();// 每次发完数据，就可能需要进入closed状态
} 

/* In an unclean shutdown, the TCPConnection
either sends or receives a segment with the rst flag set. In this case, the outbound and
inbound ByteStreams should both be in the error state, and active() can return false
immediately */
void TCPConnection::unclean_shutdown(bool rst){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
    _need_send_rst = rst;
    send_sender_segments();
    // TCPSegment seg = _sender.segments_out().front();
    // // auto header = seg.header();
    // seg.header().ack = true;
    // seg.header().rst = true;
    // _segments_out.push(seg);

}

void TCPConnection::clean_shutdown(){// shutdown，也就是CLOSED状态，是指什么也不发送了，包括ack
    if(_receiver.stream_out().input_ended()){
        /* At any point where prerequisites #1 through #3 are satisfied, the connection is “done” (and
        active() should return false) if linger after streams finish is false. Otherwise you
        need to linger: the connection is only done after enough time (10 × cfg.rt_timeout) has
        elapsed since the last segment was received.当接收到的字节流在发送的字节流还未发送FIN之前就完全被上层接收了
        那么就不用linger了*/
        if(!_sender.is_fin()){
            _linger_after_streams_finish = false;
        }else if(_sender.bytes_in_flight() == 0){
            if(!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout)
                _active = false;
        }
    }
}