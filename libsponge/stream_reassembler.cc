#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), first_unacceptable(capacity) {}

// ! \details This function accepts a substring (aka a segment) of bytes,
// ! possibly out-of-order, from the logical stream, and assembles any newly
// ! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t len = data.size();
    first_unread = _output.bytes_read();//注意first_unread并不是永远是0
    // if(index )
    string to_write;// 攒到一起写到ByteStream中
    // printf("进入put——subtring data:%s  index: %lu eof:%d \n\n", data.c_str(), index, eof);
    for(size_t i = 0; i < len ; ++i){
        /*每个分段的字符分为有重叠与没有重叠，其中没重叠的又分为可以立即push的与不可以立即push的
        */
    //    printf("打印%c\n",data[i]);
       size_t cur = i + index;
        if(cur < first_unread + _capacity){
            auto& exist = m_exist[cur];// 减少unordered_map索引的时间
            // if(!exist || cur >= first_unassembled){
                if(!exist){
                    exist = true;//将对应index的存在位置位
                    
                    if(cur == first_unassembled){//当取data的字符时，注意这里需要将i与cur区分清楚，一定是data[i]
                        // _output.write(data.substr(i, 1));//将data下标为i的字符传入ByteStream
                        to_write += data.substr(i, 1);
                        ++first_unassembled;
                    }else{
                        m_char[cur] = data[i];//记录index对应字符
                        ++unassembled;
                    }
                }else if(cur == first_unassembled){//如果该substring存在重叠部分且可以放入ByteStream中
                        // _output.write(string(1,m_char[cur]));
                        to_write += m_char[cur];
                        ++first_unassembled;
                        --unassembled;
                    }
                }
            // }       
        // }
        else{//如果当前字节的index超出了或等于capacity，直接丢弃
            break;
        }
    }
    size_t cur = index + len;
    while(cur == first_unassembled && m_exist[cur]){//这里判断是否还有unassembled的字节
        // _output.write(string(1,m_char[cur]));
        to_write += m_char[cur];
        ++first_unassembled;
        --unassembled;
        ++cur;
    }
    _output.write(move(to_write));
    if(eof == true && _eof == false && index + len <=  first_unread + _capacity)
         _eof = true;//如果最后一段字节提前到达，需要将_eof标记为true，而且一定得是这个段的最后一个字节push进了ByteStream,也就是没有被丢弃
    if (empty() && _eof)//判断是否需要置位ByteStreams的eof信号
        _output.end_input();
    // printf("调试数据 empty():%d _eof:%d first_unassembled:%lu _output.eof():%d\n\n", 
    //             empty(), _eof, first_unassembled, _output.eof());
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled; }

bool StreamReassembler::empty() const { return unassembled == 0; }

// #include "stream_reassembler.hh"

// // Dummy implementation of a stream reassembler.

// // For Lab 1, please replace with a real implementation that passes the
// // automated checks run by `make check_lab1`.

// // You will need to add private members to the class declaration in `stream_reassembler.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

// using namespace std;

// StreamReassembler::StreamReassembler(const size_t capacity)
//     : _output(capacity), _Unassembled(), _firstUnassembled(0), _nUnassembled(0), _capacity(capacity), _eof(0) {}

// //! \details This function accepts a substring (aka a segment) of bytes,
// //! possibly out-of-order, from the logical stream, and assembles any newly
// //! contiguous substrings and writes them into the output stream in order.

// void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
//     if(index + data.size() <= _output.bytes_read() + _capacity) //如果最后一段字节提前到达，需要将_eof标记为true，而且一定得是这个段的最后一个字节push进了ByteStream,也就是没有被丢弃
//         _eof = _eof | eof;
//     if (data.size() > _capacity) {
//         _eof = 0;
//     }
//     if (data.empty() || index + data.size() <= _firstUnassembled) {
//         if (_eof)
//             _output.end_input();
//         return;
//     }
//     size_t firstUnacceptable = _firstUnassembled + (_capacity - _output.buffer_size());

//     //不能直接影响_output的情形，储存的数据提前优化处理
//     std::set<typeUnassembled>::iterator iter2;
//     size_t resIndex = index;
//     auto resData = std::string(data);
//     if (resIndex < _firstUnassembled) {
//         resData = resData.substr(_firstUnassembled - resIndex);
//         resIndex = _firstUnassembled;
//     }
//     if (resIndex + resData.size() > firstUnacceptable)
//         resData = resData.substr(0, firstUnacceptable - resIndex);
    
//     //           | resData |
//     //        <---|iter|
//     iter2=_Unassembled.lower_bound(typeUnassembled(resIndex,resData));
//     while(iter2!=_Unassembled.begin()){  
//         //resIndex > _firstUnassembled
//         if(iter2==_Unassembled.end())
//             iter2--;
//         if (size_t deleteNum = merge_substring(resIndex, resData, iter2)) {  //返回值是删掉重合的bytes数
//             _nUnassembled -= deleteNum;
//             if(iter2 !=_Unassembled.begin()){
//                 _Unassembled.erase(iter2--);
//             }
//             else{
//                 _Unassembled.erase(iter2);
//                 break;
//             }
//         } 
//         else
//             break;
//     }

//     //         ｜resData |
//     //          | iter2 ... | --->
//     iter2 = _Unassembled.lower_bound(typeUnassembled(resIndex, resData));
//     while(iter2 != _Unassembled.end()){
//         if (size_t deleteNum = merge_substring(resIndex, resData, iter2)) {  //返回值是删掉重合的bytes数
//             _Unassembled.erase(iter2++);
//             _nUnassembled -= deleteNum;
//         } else
//             break;
//     }

//     if (resIndex <= _firstUnassembled) {
//         size_t wSize = _output.write(string(resData.begin() + _firstUnassembled - resIndex, resData.end()));
//         if (wSize == resData.size() && eof)
//             _output.end_input();
//         _firstUnassembled += wSize;
//     } else {
//         _Unassembled.insert(typeUnassembled(resIndex, resData));
//         _nUnassembled += resData.size();
//     }

//     if (empty() && _eof) {
//         _output.end_input();
//     }
//     return;
// }
// int StreamReassembler::merge_substring(size_t &index, std::string &data, std::set<typeUnassembled>::iterator iter2) {
//     // return value: 1:successfully merge; 0:fail to merge
//     std::string data2 = (*iter2).data;
//     size_t l2 = (*iter2).index, r2 = l2 + data2.size() - 1;
//     size_t l1 = index, r1 = l1 + data.size() - 1;
//     if (l2 > r1 + 1 || l1 > r2 + 1)
//         return 0;
//     index = min(l1, l2);
//     size_t deleteNum = data2.size();
//     if (l1 <= l2) {
//         if (r2 > r1)
//             data += std::string(data2.begin() + r1 - l2 + 1, data2.end());
//     } else {
//         if (r1 > r2)
//             data2 += std::string(data.begin() + r2 - l1 + 1, data.end());
//         data.assign(data2);
//     }
//     return deleteNum;
// }

// size_t StreamReassembler::unassembled_bytes() const { return _nUnassembled; }

// bool StreamReassembler::empty() const { return _nUnassembled == 0; }