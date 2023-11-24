using Microsoft.Win32.SafeHandles;
using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ConsoleApplication
{
    // inherits from SafeHandleZeroOrMinusOneIsInvalid, so IsInvalid is already implemented.
    internal sealed class MySafeHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        // A default constructor is required for P/Invoke to instantiate the class
        public MySafeHandle()
            : base(ownsHandle: true)
        {
        }
        protected override bool ReleaseHandle()
        {
            NativeMethods.delete_dotnetstream(handle);
            return true;
        }
    }

    internal static class NativeMethods
    {
        [DllImport("NativeLibrary")]
        internal extern static MySafeHandle create_dotnetstream(IntPtr callback, [In] byte[] buffer, [In] int buffering);

        [DllImport("NativeLibrary")]
        internal extern static void delete_dotnetstream(IntPtr handle);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal delegate int ReadFunctionDelegate();

        [DllImport("NativeLibrary")]
        internal extern static void play(MySafeHandle handle);

    }

    public sealed class BufferedStreamWrapper : IDisposable
    {
        private readonly Stream _input; // keep reference to avoid garabage collection
        private readonly MySafeHandle _handle;
        private readonly NativeMethods.ReadFunctionDelegate _rf;

        // System.IO.Stream expects an input byte[] to read into,
        // since we do not want to go the `unsafe` road, create one here:
        // this also define the buffering (aka `setvbuf`) implicitely:
        // https://stackoverflow.com/questions/1862982/c-sharp-filestream-optimal-buffer-size-for-writing-large-files
        private readonly byte[] _buffer = new byte[4096];

        public BufferedStreamWrapper(Stream input)
        {
            _input = input;
            _rf = this.FillBuffer;
            _handle = NativeMethods.create_dotnetstream(Marshal.GetFunctionPointerForDelegate(_rf), _buffer, _buffer.Length);
        }

        private int FillBuffer()
        {
            try
            {
                return _input.Read(_buffer, 0, _buffer.Length);
            }
            catch
            {
            // https://www.mono-project.com/docs/advanced/pinvoke/#runtime-exception-propagation
            // https://github.com/dotnet/runtime/issues/4756
                return -1;
            }
        }

        public void Play()
        {
            NativeMethods.play(_handle);
        }

        // - There is no need to implement a finalizer, MySafeHandle already has one
        // - You do not need to protect against multiple disposing, MySafeHandle already does
        public void Dispose()
        {
            _handle.Dispose();
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            var fs = new FileStream("C:/Users/mmalaterre/clario/CMake-P-Invoke/README.md", FileMode.Open);
            var w = new BufferedStreamWrapper(fs);
            w.Play();
        }
    }
}
