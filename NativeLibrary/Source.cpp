#include <iostream>

extern "C" {
typedef int (*FillBuffer)();
}

namespace details
{
    class dotnetstream
    {
        FillBuffer fillbuffer_;
        char* buffer_;
        int size_;

    public:
        explicit dotnetstream(FillBuffer fillbuffer, char *buffer,  int size) : fillbuffer_(fillbuffer), buffer_(buffer), size_(size)
        {
        }

        char* buffer() { return buffer_; }
        int read()
        {
            const int ret = (*fillbuffer_)();
            return ret;
        }
    };

    // https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
    class customstreambuf : public std::streambuf
    {
        dotnetstream* stream_;

    public:
        explicit customstreambuf(dotnetstream* stream) : stream_(stream)
        {
        }

        int underflow() override
        {
            if (this->gptr() == this->egptr())
            {
                const int size = this->stream_->read();
                if (size < 0)
                {
                    // throw std::runtime_error("oops");
                    throw "socket stream error";
                }
                this->setg(this->stream_->buffer(), this->stream_->buffer(), this->stream_->buffer() + size);
            }
            return this->gptr() == this->egptr()
                       ? std::char_traits<char>::eof()
                       : std::char_traits<char>::to_int_type(*this->gptr());
        }
    };
}

extern "C" {

__declspec(dllexport) struct dotnetstream* create_dotnetstream(FillBuffer fillBuffer, char* buffer, int size)
{
    return reinterpret_cast<struct dotnetstream*>(new details::dotnetstream(fillBuffer, buffer, size));
}

__declspec(dllexport) void delete_dotnetstream(struct dotnetstream* stream)
{
    delete reinterpret_cast<details::dotnetstream*>(stream);
}

__declspec(dllexport) void play(struct dotnetstream* stream)
{
    details::dotnetstream* s = reinterpret_cast<details::dotnetstream*>(stream);

    // play
    {
        details::customstreambuf sbuf(s);
        std::istream in(&sbuf);
        std::cout << in.rdbuf() << std::endl;
    }
}
}
