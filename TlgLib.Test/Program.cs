﻿using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;

namespace FreeMote.Tlg.Tests
{
    class Program
    {
        static void Main(string[] args)
        {
            //MemoryTest();
            
            var target = File.Exists("test.tlg") ? "test.tlg" : "NewGame5.tlg";
            TlgImageConverter converter = new TlgImageConverter();
            var original = File.ReadAllBytes(target);
            byte[] converted = null;
            using (var fs = File.Open(target, FileMode.Open))
            {
                using (var br = new BinaryReader(fs))
                {
                    var bmp = converter.Read(br);
                    converted = bmp.ToTlg6();
                    if (converted == null)
                    {
                        Console.WriteLine("Conversion failed.");
                    }
                    //else
                    //{
                    //    Console.WriteLine($"Totally equal: {(original.SequenceEqual(converted) ? "Yes" : "No")}");
                    //}
                }
            }

            if (converted != null)
            {
                using (var ms = new MemoryStream(converted))
                {
                    using (var br = new BinaryReader(ms))
                    {
                        var bmp = converter.Read(br);
                        bmp.Save("output.png", ImageFormat.Png);
                    }
                }
            }

            Console.WriteLine($"IsTLG: {(TlgNative.CheckTlg(original) ? "Yes" : "No")}");
            if (TlgNative.GetInfoTlg(original, out int w, out int h, out int v))
            {
                Console.WriteLine($"TLGv{v} Size: {w} x {h}");
            }
            //var bmp2 = TlgNative.ToBitmap(original, out _);
            var t = TlgNative.ToBitmap(original);
            Console.WriteLine(t.Item2);
            t.Item1.Save("output2.png", ImageFormat.Png);

            Console.WriteLine("Done.");
            Console.ReadLine();
        }

        static void MemoryTest()
        {
            var target = File.Exists("test.tlg") ? "test.tlg" : "NewGame5.tlg";
            TlgImageConverter converter = new TlgImageConverter();
            var original = File.ReadAllBytes(target);
            Console.WriteLine("TLG bytes loaded.");
            Console.ReadLine();

            //Pure managed!
            using (var ms = new MemoryStream(original))
            {
                using (BinaryReader br = new BinaryReader(ms))
                {
                    Bitmap b = converter.Read(br);
                    b.Dispose();
                }
            }

            Console.WriteLine("Managed done.");
            Console.ReadLine();
            GC.Collect();

            using (TlgLoader ldr = new TlgLoader(original))
            {
                Bitmap b = ldr.Bitmap;
                b.Dispose();
            }
            //You can still access b's properties here (out of `using` scope) if you don't dispose it,
            //but you can not call `Save` or `LockBits` etc. because the data is deleted.
            Console.WriteLine("NativeLoader done.");
            Console.ReadLine();
            GC.Collect();

            //By default `ToBitmap` copies data to managed side so you can call `Save` as you want.
            //Set `useUnmanagedScan0 = true` to make it behave like TlgLoader but no one deletes the data!!
            Bitmap b2 = TlgNative.ToBitmap(original, out _);
            b2.Dispose();
            Console.WriteLine("NativeCopy done.");
            Console.ReadLine();
            GC.Collect();

            Console.WriteLine("All done.");
            Console.ReadLine();
        }
    }
}
