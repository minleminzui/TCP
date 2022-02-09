#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), first_unacceptable(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t len = data.size();
    first_unread = _output.bytes_read();//注意first_unread并不是永远是0
    // printf("进入put——subtring data:%s  index: %lu eof:%d \n\n", data.c_str(), index, eof);
    for(size_t i = 0; i < len ; ++i){
        /*每个分段的字符分为有重叠与没有重叠，其中没重叠的又分为可以立即push的与不可以立即push的
        */
    //    printf("打印%c\n",data[i]);
       size_t cur = i + index;
        if(cur < first_unread + _capacity){//如果当前字节的index超出了或等于capacity，直接丢弃
            
            if(!m_exist[cur] || cur >= first_unassembled){
                if(!m_exist[cur]){
                    m_exist[cur] = true;//将对应index的存在位置位
                    m_char[cur] = data[i];//记录index对应字符
                    if(cur == first_unassembled){//当取data的字符时，注意这里需要将i与cur区分清楚，一定是data[i]
                        _output.write(data.substr(i, 1));//将data下标为i的字符传入ByteStream
                        ++first_unassembled;
                    }else{
                        ++unassembled;
                    }
                }else{
                    if(cur == first_unassembled){//如果该substring存在重叠部分且可以放入ByteStream中
                        _output.write(string(1,m_char[cur]));
                        ++first_unassembled;
                        --unassembled;
                    }
                }
            }       
        }
    }
    size_t cur = index + len;
    while(m_exist[cur] && cur == first_unassembled){//这里判断是否还有unassembled的字节
        _output.write(string(1,m_char[cur]));
        ++first_unassembled;
        --unassembled;
        ++cur;
    }

    if(eof == true && _eof == false && index + len <=  first_unread + _capacity)
         _eof = true;//如果最后一段字节提前到达，需要将_eof标记为true，而且一定得是这个段的最后一个字节push进了ByteStream,也就是没有被丢弃
    if (empty() && _eof)//判断是否需要置位ByteStreams的eof信号
        _output.end_input();
    // printf("调试数据 empty():%d _eof:%d first_unassembled:%lu _output.eof():%d\n\n", 
    //             empty(), _eof, first_unassembled, _output.eof());
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled; }

bool StreamReassembler::empty() const { return unassembled == 0; }
