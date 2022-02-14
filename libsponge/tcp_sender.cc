#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if(!_syn){//如果没有发送SYN报文,也就是第一次握手
        _syn = true;
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = _isn;
        _segments_out.push(seg);
        _segments_outstanding.push(seg);
        _next_seqno = 1;
        _bytes_in_flight = seg.length_in_sequence_space();
        if(!_timer_running){
            _timer_running = true;
            _rto = _initial_retransmission_timeout;
            _tick = 0;
        }
        return;// 这里需要return，等到第二次握手的到来，再发送新数据
    }
    
    uint64_t remain = _receiver_window_size - _bytes_in_flight;
    if(_receiver_window_size == 0) remain += 1;// 对应文档 If the receiver has announced window size of zero, the fill window method should act like the window size is one. 
    // printf("tcp_sender _receiver_window_size %d remain %lu _stream.buffer_size() %lu\n\n", _receiver_window_size, remain, _stream.buffer_size());
    while(remain > 0 && _stream.buffer_size() > 0){
        TCPSegment seg;
        
        uint64_t payload_size = min(remain, TCPConfig::MAX_PAYLOAD_SIZE);// 一个报文段允许读取字节数的最大值
        string payload = _stream.read(payload_size);
        // printf("remain  %lu _stream.buffer_size() %lu _receiver_window_size %d payload %s\n\n", remain, _stream.buffer_size(), _receiver_window_size, payload.c_str());
        remain -= payload.size();
        seg.payload() = move(payload);//转化为右值引用调用移动构造
        
        seg.header().seqno = wrap(_next_seqno, _isn);//这里需要修改下一个可以发送的序列号
        _next_seqno  = unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space();
        
        
        if(_stream.eof() && remain >= payload.size() + 1){
            seg.header().fin = true;
            _fin = true;
            _next_seqno++;// 即使流eof了，fin置位了，还需要发送正确的 _next_seqno
        }
        _segments_outstanding.push(seg);
        _segments_out.push(seg);
        
        _bytes_in_flight += seg.length_in_sequence_space();
        /*对应Every time a segment containing data (nonzero length in sequence space) is sent
            (whether it’s the first time or a retransmission), if the timer is not running, start it
            running so that it will expire after RTO milliseconds (for the current value of RTO).*/
        if(!_timer_running){
            _timer_running = true;
            _rto = _initial_retransmission_timeout;
            _tick = 0;
        }
    }
    /* 即使_stream没有字节了，这里需要特判TCPSegment中只有fin标志的情况, 因为Remember that the SYN and
        FIN flags also occupy a sequence number each, which means that they occupy space in
        the window.*/
    if(!_fin && _stream.eof() && remain){
        TCPSegment seg;
        _fin = true;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().fin = true;
        _segments_outstanding.push(seg);
        _segments_out.push(seg);
        _bytes_in_flight += seg.length_in_sequence_space();
        _next_seqno++;
        if(!_timer_running){
            _timer_running = true;
            _rto = _initial_retransmission_timeout;
            _tick = 0;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t unwrap_ackno = unwrap(ackno, _isn, _next_seqno);
    if(unwrap_ackno == _next_seqno && window_size != _receiver_window_size)// 这里有个特例就是如果发送了多次相同的ackno，但是窗口值不同，需要更新窗口值
        _receiver_window_size = window_size;
    if(!_segments_outstanding.size()) return;// 重复ackno无效，需要判断不然下一句的t会引发runtime error
    auto t = _segments_outstanding.front();
    if(unwrap_ackno > _next_seqno || // the ackno is greater than all of the sequence numbers in the segment, 由前可知，如果ackno不能够使得_segments_outstanding第一个元素出队并且不能够额外发送fin包，那么也是一个错误的确认号
        unwrap_ackno + window_size - 1 + (window_size ? 0 : 1) < unwrap(t.header().seqno, _isn, _next_seqno) + t.length_in_sequence_space()) return; // 如果确认号大于了_next_seqno,那么是错误的确认号
    // window_size为零的时候需要特判，并且RTO不能指数退避， 对应文档 If the receiver has announced window size of zero, the fill window method should act like the window size is one. 
    _receiver_window_size = window_size;
    // printf("_receiver_window_size %d\n\n", _receiver_window_size);
    do{
        t = _segments_outstanding.front();
        auto seqno = t.header().seqno;
        auto len = t.length_in_sequence_space();
        if(unwrap(seqno, _isn, _next_seqno) + len - 1 < unwrap_ackno){// 更新收到确认的TCPSegment，把相应Segemnt弹出缓存
            _bytes_in_flight -= len;
            _segments_outstanding.pop();
            _tick = 0;
            _rto = _initial_retransmission_timeout;// When the receiver gives the sender an ackno that acknowledges the successful receipt of new data
            _consecutive_retransmissions = 0;
        }else{
            break;
        }
    }while(_segments_outstanding.size());

    if(!_bytes_in_flight) _timer_running = false;// 对应When all outstanding data has been acknowledged, stop the retransmission timer.
    fill_window();//对应于文档"The TCPSender should fill the window again if new space has opened up."
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { //由于tick只能够被动调用，也就是我不会去调用，onwer回去调用
    
    _tick += ms_since_last_tick;
    if(_tick >= _rto && _segments_outstanding.size()){
        _segments_out.push(_segments_outstanding.front());
        if(_receiver_window_size || _segments_outstanding.front().header().syn){// 对于第一次握手发送的报文，window size一开始也是0，这里需要特判
            _consecutive_retransmissions++;//Keep track of the number of consecutive retransmissions, and increment it because you just retransmitted something
            _rto <<= 1;// Double the value of RTO.
        }
        _tick = 0;// Reset the retransmission timer and start it such that it expires after RTO milliseconds 
    }

 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);// 这里的空报文的发送，并不会改变_next_seqno,这个报文只是用来回应keep-alive报文的
    _segments_out.push(seg);
}