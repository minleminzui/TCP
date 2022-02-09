#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    const TCPHeader &hdr = seg.header();
    if(!hdr.syn && !_syn) return;//如果当前segment没置位syn，且之前syn也没置位，那么错误返回
    if(stream_out().eof() && _fin) return;//字节流达到了结尾，那么该segment无效

    if(hdr.syn && !_syn){
        _isn = hdr.seqno;
        _syn = true;
    }

    uint64_t seq_start = unwrap(hdr.seqno + (hdr.syn ? 1 : 0), _isn, checkpoint);//获取当前序列号对应的absolute sequno，hdr.seqno + (hdr.syn ? 1 : 0)表示syn占据了一个序列号
    _reassembler.push_substring(std::string(seg.payload().str()), seq_start - 1, hdr.fin);//seq——start - 1，由于syn占据了一个序列号，absolute sequno与stream index相差一
    checkpoint = _reassembler.first_unassembled_byte();
    if(hdr.fin && !_fin && _reassembler.is_eof()){//需要fin没有被丢弃，才能把_fin置位
        _fin = true;
    }
    if(_reassembler.empty() && _fin){//只有所有的字节都是reassembled，且_fin被置位了，才需要考虑fin占据了一个序列号
        _ackno = wrap(_reassembler.first_unassembled_byte() + 2, _isn);
        // std::cout<<stream_out().bytes_written()<<endl;
        // printf("一共读了%lu\n", _reassembler.first_unassembled_byte());
    }else{
        _ackno = wrap(_reassembler.first_unassembled_byte() + 1, _isn);
    }
    
}


optional<WrappingInt32> TCPReceiver::ackno() const 
{ 
    if(!_syn) return nullopt;
    else  return _ackno; 
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }//实际上，字节流index可以有64bit
