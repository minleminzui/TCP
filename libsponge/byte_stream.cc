// #include "byte_stream.hh"
// #include<iostream>
// using namespace std;
// // Dummy implementation of a flow-controlled in-memory byte stream.

// // For Lab 0, please replace with a real implementation that passes the
// // automated checks run by `make check_lab0`.

// // You will need to add private members to the class declaration in `byte_stream.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

// using namespace std;

// ByteStream::ByteStream(const size_t capacity) : byteQueue(capacity + 1), cap(capacity + 1){ }

// size_t ByteStream::write(const string &data) {
//     // Write a string of bytes into the stream. Write as many
//     // as will fit, and return the number of bytes written.
//     uint32_t preWrite = totWrite;
//     for(auto c : data){
//         if((tail + 1) % cap != head){
//             totWrite++;
//             byteQueue[tail] = c; 
//             tail = (tail + 1) % cap;
//         }else break;
//     }
//     // DUMMY_CODE(data);
//     return totWrite - preWrite ; 
// }

// //! \param[in] len bytes will be copied from the output side of the buffer
// string ByteStream::peek_output(const size_t len) const {
//     string s;
//     size_t temHead = head;
//     for(size_t i = 0; i < len && temHead != tail; ++i){//注意这里是temHead != tail
//         s += byteQueue[temHead];
//         temHead = (temHead + 1) % cap;
//     }
//     // DUMMY_CODE(len);
//     return s;
// }

// //! \param[in] len bytes will be removed from the output side of the buffer
// void ByteStream::pop_output(const size_t len) { 
//     size_t bufferLen = (tail - head + cap) % cap;
//     if(bufferLen >= len){
//         bufferLen = len;
//     }
//     head = (head + bufferLen) % cap;
//     totRead += bufferLen;
//  }

// //! Read (i.e., copy and then pop) the next "len" bytes of the stream
// //! \param[in] len bytes will be popped and returned
// //! \returns a string
// std::string ByteStream::read(const size_t len) {
//     // DUMMY_CODE(len);
//     string s = peek_output(len);
//     pop_output(len);
//     return s;
// }

// void ByteStream::end_input() {isEnd = true;}

// bool ByteStream::input_ended() const { return isEnd; }

// size_t ByteStream::buffer_size() const { return (tail - head + cap) % cap; }

// bool ByteStream::buffer_empty() const { 
//     return head == tail; 
// }

// bool ByteStream::eof() const { return isEnd && head == tail; }

// size_t ByteStream::bytes_written() const { return totWrite; }

// size_t ByteStream::bytes_read() const { return totRead; }

// size_t ByteStream::remaining_capacity() const { return cap - ((tail - head + cap) % cap) - 1; }

#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t len = data.length();
    if (len > _capacity - _buffer.size()) {
        len = _capacity - _buffer.size();
    }
    _write_count += len;
    string s;
    s.assign(data.begin(), data.begin() + len);
    _buffer.append(BufferList(move(s)));
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length = len;
    if (length > _buffer.size()) {
        length = _buffer.size();
    }
    string s = _buffer.concatenate();
    return string().assign(s.begin(), s.begin() + length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t length = len;
    if (length > _buffer.size()) {
        length = _buffer.size();
    }
    _read_count += length;
    _buffer.remove_prefix(length);
    return;
}

void ByteStream::end_input() { _input_ended_flag = true; }

bool ByteStream::input_ended() const { return _input_ended_flag; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.size() == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _write_count; }

size_t ByteStream::bytes_read() const { return _read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffer.size(); }