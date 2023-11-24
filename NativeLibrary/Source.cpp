#include <iostream>

extern "C" {
typedef int (*FillBuffer)();
typedef int (*WriteBuffer)(int);
}

namespace details
{
    class dotnetstream2
    {
        WriteBuffer writebuffer_;
        char* buffer_;
        int size_;

    public:
        explicit dotnetstream2(WriteBuffer writebuffer, char* buffer, int size) : writebuffer_(writebuffer), buffer_(buffer), size_(size)
        {
        }
        int size() { return size_; }
        char* buffer() { return buffer_; }
        int write(int count)
        {
            const int ret = (*writebuffer_)(count);
            return ret;
        }
    };

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

    // https://stackoverflow.com/questions/22116158/whats-wrong-with-this-stream-buffer
    class customstreambuf2 : public std::streambuf
    {
        dotnetstream2* stream_;
    public:
        explicit customstreambuf2(dotnetstream2* stream) : stream_(stream)
        {
            // -1 trick:
            this->setp(this->stream_->buffer(), this->stream_->buffer() + this->stream_->size() - 1);
        }
    private:
        int_type overflow(int_type i) override
        {
            if (!traits_type::eq_int_type(i, traits_type::eof()))
            {
                // see -1 trick in cstor:
                *pptr() = traits_type::to_char_type(i);
                pbump(1);

                if (flush())
                {
                    //pbump(-(pptr() - pbase()));
                    pbump(-this->stream_->size());
                    return i;
                }
                else
                    return traits_type::eof();
            }
            return traits_type::not_eof(i);
        }

        int sync() override
        {
            return flush() ? 0 : -1;
        }

        // helper:
        bool flush()
        {
            const int num = pptr() - pbase();
            const int size = this->stream_->write(num);
            return size == num;
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

__declspec(dllexport) struct dotnetstream2* create_dotnetstream2(WriteBuffer writeBuffer, char* buffer, int size)
{
    return reinterpret_cast<struct dotnetstream2*>(new details::dotnetstream2(writeBuffer, buffer, size));
}

__declspec(dllexport) void delete_dotnetstream2(struct dotnetstream2* stream)
{
    delete reinterpret_cast<details::dotnetstream2*>(stream);
}


__declspec(dllexport) void play2(struct dotnetstream2* stream)
{
    details::dotnetstream2* s = reinterpret_cast<details::dotnetstream2*>(stream);

    // play
    {
        details::customstreambuf2 sbuf(s);
        std::ostream out(&sbuf);
        out << "ABCDEF";
        out.flush();
    }
}
}
