#include <iostream>

extern "C" __declspec(dllexport) void HelloWorld();

void HelloWorld()
{
	std::cout << "Hello world2 from c++" << std::endl;
}

extern "C" {
    // System.IO.Stream
    // public override int Read(byte[] buffer, int offset, int count)
    typedef int (*ReadFunction)(char** buffer, int size);
}

namespace details {

    class dotnetstream {
    public:
        ReadFunction readfunction_;
        int buffersize;
        int read(char* buffer) {
            const int ret = (*readfunction_)(&buffer, buffersize);
            return ret;
        }
    };

    // https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
    class compressbuf
        : public std::streambuf {
        dotnetstream* stream_;
        char* buffer_;
        // context for the compression
    public:
        compressbuf(dotnetstream* stream)
            : stream_(stream), buffer_(new char[stream->buffersize]) {
            // initialize compression context
        }
        ~compressbuf() { delete[] this->buffer_; }
        int underflow() {
            if (this->gptr() == this->egptr()) {
                // decompress data into buffer_, obtaining its own input from
                // this->sbuf_; if necessary resize buffer
                // the next statement assumes "size" characters were produced (if
                // no more characters are available, size == 0.
                const int size = this->stream_->read(this->buffer_);
                this->setg(this->buffer_, this->buffer_, this->buffer_ + size);
            }
            return this->gptr() == this->egptr()
                ? std::char_traits<char>::eof()
                : std::char_traits<char>::to_int_type(*this->gptr());
        }
    };

}

extern "C" {
 //   typedef void (*ReadFunction)(char* buffer, int size);

    __declspec(dllexport) struct dotnetstream *create_dotnetstream() {
        return reinterpret_cast<struct dotnetstream *>(new details::dotnetstream);
    }
    __declspec(dllexport) void delete_dotnetstream(struct dotnetstream* stream) {
        delete reinterpret_cast<details::dotnetstream*>(stream);
    }
    __declspec(dllexport) void setup_dotnetstream(struct dotnetstream* stream, ReadFunction readFunction, int buffering) {
        details::dotnetstream* s = reinterpret_cast<details::dotnetstream*>(stream);
        s->readfunction_ = readFunction;
        s->buffersize = buffering;

        // play
        if (readFunction) {
            
            details::compressbuf   sbuf(s);
            std::istream  in(&sbuf);
            std::cout << in.rdbuf() << std::endl;
        }
    }
}