using System.Runtime.InteropServices;

namespace ConsoleApplication
{
    class Program
    {
        [DllImport("NativeLibrary.dll")]
        public static extern void HelloWorld();

        static void Main(string[] args)
        {
            HelloWorld();
        }
    }
}
