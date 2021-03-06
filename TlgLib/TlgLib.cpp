#include <iostream>
#include <string>
#include <sstream>
#include "libtlg\TLG.h"
#include "libtlg\stream.h"
#include "TlgLib.h"

using namespace System::Drawing;
using namespace System::Drawing::Imaging;
using namespace System::Runtime::InteropServices;
extern tTJSBinaryStream *GetMemoryStream();

public struct BmpData
{
	int Stride;
	int Width;
	int Height;
	unsigned char* Data;
};

static void* scanLineCallback(void *callbackdata, int y)
{
	if (y == -1)
	{
		return NULL;
	}
	BmpData* data = (BmpData*)(callbackdata);
	return data->Data + data->Stride * y;
}

static bool sizeCallback(void *callbackdata, unsigned int w, unsigned int h, unsigned int colors)
{
	BmpData* data = (BmpData*)(callbackdata);
	data->Stride = w * colors;
	data->Width = w;
	data->Height = h;
	data->Data = new unsigned char[data->Stride * data->Height];
	return true;
}


array<Byte>^ SaveTlg(Bitmap^ bmp, int type = 1)
{
	tTJSBinaryStream* output = GetMemoryStream();
	//1:8bit 3:RGB 4:RGBA
	int pxFormat = 4;
	switch (bmp->PixelFormat)
	{
	case PixelFormat::Format8bppIndexed:
		pxFormat = 1;
		break;
	case PixelFormat::Format24bppRgb:
		pxFormat = 3;
		break;
	default:
		break;
	}

	BitmapData^ bmpData = bmp->LockBits(Rectangle(0, 0, bmp->Width, bmp->Height),
		ImageLockMode::ReadOnly, bmp->PixelFormat);
	int stride = bmpData->Stride;
	IntPtr scan0 = bmpData->Scan0;
	int scanBytes = stride * bmpData->Height;
	BmpData data;
	data.Width = bmpData->Width;
	data.Height = bmpData->Height;
	data.Stride = bmpData->Stride;
	data.Data = new unsigned char[scanBytes];
	memcpy(data.Data, scan0.ToPointer(), scanBytes);
	bmp->UnlockBits(bmpData);

	std::map<std::string, std::string> tags;
	tags.insert(std::pair<std::string, std::string>("Software", "FreeMote"));

	//Demonstrating Tag building
	//std::stringstream ss;
	//std::map<std::string, std::string>::const_iterator it = tags.begin();
	//while (it != tags.end()) {
	//	ss << it->first.length() << ":" << it->first << "=" << it->second.length() << ":" << it->second << ",";
	//	it++;
	//}
	//std::string s = ss.str();

	int state = TVPSaveTLG(output, type, bmp->Width, bmp->Height, pxFormat, &data, scanLineCallback, &tags);
	delete data.Data;
	if (state == 0)
	{
		const int len = output->Seek(0, TJS_BS_SEEK_END);
		output->Seek(0, TJS_BS_SEEK_SET);
		array<Byte>^ buf = gcnew array<Byte>(len);
		pin_ptr<unsigned char> nativeBuf = &buf[0];
		output->ReadBuffer(nativeBuf, len);
		delete output;
		return buf;
	}

	return nullptr;
}

array<Byte>^ FreeMote::Tlg::TlgNative::ToTlg6(Bitmap^ bmp)
{
	return SaveTlg(bmp, 1);
}

array<Byte>^ FreeMote::Tlg::TlgNative::ToTlg5(Bitmap^ bmp)
{
	return SaveTlg(bmp, 0);
}

array<Byte>^ FreeMote::Tlg::TlgNative::ToBitmapBytes(array<Byte>^ tlgBytes, Int32% bitWidth)
{
	bitWidth = 4;
	BmpData data;
	std::map<std::string, std::string> tags;
	pin_ptr<unsigned char> btsPtr = &tlgBytes[0];
	tTJSBinaryStream* input = GetMemoryStream();
	input->WriteBuffer(btsPtr, tlgBytes->Length);
	int state = TVPLoadTLG(&data, sizeCallback, scanLineCallback, &tags, input);
	delete input;
	if (state == 0)
	{
		bitWidth = data.Stride / data.Width;
		array<Byte>^ buf = gcnew array<Byte>(data.Stride*data.Height);
		Marshal::Copy(IntPtr(data.Data), buf, 0, buf->Length);
		delete data.Data;
		return buf;
	}
	return nullptr;
}

Bitmap^ FreeMote::Tlg::TlgNative::ToBitmap(array<Byte>^ tlgBytes, Int32% version, bool useUnmanagedScan0)
{
	BmpData data;
	std::map<std::string, std::string> tags;
	pin_ptr<unsigned char> btsPtr = &tlgBytes[0];
	tTJSBinaryStream* input = GetMemoryStream();
	input->WriteBuffer(btsPtr, tlgBytes->Length);
	int v = 0;
	int state = TVPLoadTLG(&data, sizeCallback, scanLineCallback, &tags, input, &v);
	delete input;
	version = v;

	if (state == 0)
	{
		int colors = data.Stride / data.Width;
		PixelFormat pxFormat;
		switch (colors)
		{
		case 1:
			pxFormat = PixelFormat::Format8bppIndexed;
			break;
		case 3:
			pxFormat = PixelFormat::Format24bppRgb;
			break;
		default:
			pxFormat = PixelFormat::Format32bppArgb;
			break;
		}
		if (useUnmanagedScan0)
		{
			//No copy but can leak native memory (data.Data can not be deleted before Bitmap is disposed)!
			//To avoid this, use TlgLoader
			//https://msdn.microsoft.com/en-us/library/zy1a2d14(v=vs.110).aspx
			//The caller is responsible for allocating and freeing the block of memory specified by the scan0 parameter. However, the memory should not be released until the related Bitmap is released.
			Bitmap^ bmp = gcnew Bitmap(data.Width, data.Height, data.Stride, pxFormat, IntPtr(data.Data));
			//delete data.Data;
			return bmp;
		}
		else
		{
			//Copy!
			Bitmap^ bmp = gcnew Bitmap(data.Width, data.Height, pxFormat);
			BitmapData^ bmpData = bmp->LockBits(Rectangle(0, 0, bmp->Width, bmp->Height),
				ImageLockMode::ReadOnly, bmp->PixelFormat);
			memcpy(bmpData->Scan0.ToPointer(), data.Data, bmpData->Stride * bmpData->Height);
			bmp->UnlockBits(bmpData);
			delete data.Data;
			return bmp;
		}

	}
	return nullptr;
}

Tuple<Bitmap^, Int32>^ FreeMote::Tlg::TlgNative::ToBitmap(array<Byte>^ tlgBytes)
{
	Int32 ver = 0;
	Bitmap^ bmp = ToBitmap(tlgBytes, ver, false);
	return gcnew Tuple<Bitmap^, Int32>(bmp, ver);
}

bool FreeMote::Tlg::TlgNative::CheckTlg(array<Byte>^ tlgBytes)
{
	pin_ptr<unsigned char> btsPtr = &tlgBytes[0];
	tTJSBinaryStream* input = GetMemoryStream();
	input->WriteBuffer(btsPtr, tlgBytes->Length);
	bool r = TVPCheckTLG(input);
	delete input;
	return r;
}

bool FreeMote::Tlg::TlgNative::GetInfoTlg(array<Byte>^ tlgBytes, Int32 % width, Int32 % height, Int32% version)
{
	pin_ptr<unsigned char> btsPtr = &tlgBytes[0];
	tTJSBinaryStream* input = GetMemoryStream();
	input->WriteBuffer(btsPtr, tlgBytes->Length);
	int w, h, v;
	bool result = TVPGetInfoTLG(input, &w, &h, &v);
	delete input;
	width = w;
	height = h;
	version = v;
	return result;
}

FreeMote::Tlg::TlgLoader::TlgLoader(array<Byte>^ tlgBytes)
{
	BmpData data;
	std::map<std::string, std::string> tags;
	pin_ptr<unsigned char> btsPtr = &tlgBytes[0];
	tTJSBinaryStream* input = GetMemoryStream();
	input->WriteBuffer(btsPtr, tlgBytes->Length);
	int v = 0;
	int state = TVPLoadTLG(&data, sizeCallback, scanLineCallback, &tags, input, &v);
	delete input;
	_version = v;
	_data = data.Data;
	if (state == 0)
	{
		_width = data.Width;
		_height = data.Height;
		_colors = data.Stride / data.Width;
		GetBitmap();
	}
	else
	{
		throw gcnew BadImageFormatException("Not a valid TLG image");
	}
}

Bitmap^ FreeMote::Tlg::TlgLoader::GetBitmap()
{
	PixelFormat pxFormat;
	switch (_colors)
	{
	case 1:
		pxFormat = PixelFormat::Format8bppIndexed;
		break;
	case	 3:
		pxFormat = PixelFormat::Format24bppRgb;
		break;
	default:
		pxFormat = PixelFormat::Format32bppArgb;
		break;
	}
	_bmp = gcnew System::Drawing::Bitmap(_width, _height, _colors * _width, pxFormat, IntPtr(_data));
	return _bmp;
}
