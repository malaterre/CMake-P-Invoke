using Microsoft.Win32.SafeHandles;
using System;
using System.Diagnostics;
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
        // Returns the SafeHandle instead of IntPtr
        [DllImport("NativeLibrary")]
        internal extern static MySafeHandle create_dotnetstream();

        [DllImport("NativeLibrary")]
        internal extern static void delete_dotnetstream(IntPtr handle);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        internal delegate int ReadFunctionDelegate(ref IntPtr buffer, int count);

        [DllImport("NativeLibrary")]
        internal extern static void setup_dotnetstream(MySafeHandle handle, IntPtr callback, int buffering);
    }

    public sealed class MyFileWrapper : IDisposable
    {
        private readonly Stream _input; // keep reference to avoid garabage collection
        private readonly MySafeHandle _handle;

        // System.IO.Stream expects an input byte[] to read into,
        // since we do not want to go the `unsafe` road, create one here:
        // also define the buffering implicitely:
        // https://stackoverflow.com/questions/1862982/c-sharp-filestream-optimal-buffer-size-for-writing-large-files
        private readonly byte[] _buffer = new byte[4096];

        public MyFileWrapper(Stream input)
        {
            _input = input;
            _handle = NativeMethods.create_dotnetstream();
            NativeMethods.ReadFunctionDelegate rf = this.MyRead;
            var ptr = Marshal.GetFunctionPointerForDelegate(rf);
            NativeMethods.setup_dotnetstream(_handle, ptr, _buffer.Length);
        }

        private int MyRead(ref IntPtr outbuffer, int count)
        {
            // programmer error:
            Debug.Assert(count == _buffer.Length);
            try
            {
                int ret = _input.Read(_buffer, 0, _buffer.Length);
                Marshal.Copy(_buffer, 0, outbuffer, ret);
                return ret;
            }
            catch
            {
            // https://www.mono-project.com/docs/advanced/pinvoke/#runtime-exception-propagation
            // https://github.com/dotnet/runtime/issues/4756

                return -1;
            }
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
        [DllImport("NativeLibrary.dll")]
        public static extern void HelloWorld();

        [DllImport("NativeLibrary.dll")]
        public static extern void SetStream(IntPtr stream);

        static void Main(string[] args)
        {
            HelloWorld();
            var fs = new FileStream("README.md", FileMode.Open);
            var w = new MyFileWrapper(fs);
        }
    }
}
