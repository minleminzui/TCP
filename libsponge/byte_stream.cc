#include "byte_stream.hh"
#include<iostream>
using namespace std;
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : byteQueue(capacity + 1), cap(capacity + 1){ }

size_t ByteStream::write(const string &data) {
    // Write a string of bytes into the stream. Write as many
    // as will fit, and return the number of bytes written.
    uint32_t preWrite = totWrite;
    for(auto c : data){
        if((tail + 1) % cap != head){
            totWrite++;
            byteQueue[tail] = c; 
            tail = (tail + 1) % cap;
        }else break;
    }
    // DUMMY_CODE(data);
    return totWrite - preWrite ; 
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string s;
    size_t temHead = head;
    for(size_t i = 0; i < len && temHead != tail; ++i){//注意这里是temHead != tail
        s += byteQueue[temHead];
        temHead = (temHead + 1) % cap;
    }
    // DUMMY_CODE(len);
    return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t bufferLen = (tail - head + cap) % cap;
    if(bufferLen >= len){
        bufferLen = len;
    }
    head = (head + bufferLen) % cap;
    totRead += bufferLen;
 }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    string s = peek_output(len);
    pop_output(len);
    return s;
}

void ByteStream::end_input() {isEnd = true;}

bool ByteStream::input_ended() const { return isEnd; }

size_t ByteStream::buffer_size() const { return (tail - head + cap) % cap; }

bool ByteStream::buffer_empty() const { 
    return head == tail; 
}

bool ByteStream::eof() const { return isEnd && head == tail; }

size_t ByteStream::bytes_written() const { return totWrite; }

size_t ByteStream::bytes_read() const { return totRead; }

size_t ByteStream::remaining_capacity() const { return cap - ((tail - head + cap) % cap) - 1; }
