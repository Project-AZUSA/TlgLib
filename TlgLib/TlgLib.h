#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Runtime::CompilerServices;

namespace FreeMote
{
	namespace Tlg
	{
		[ExtensionAttribute]
		static public ref class TlgNative
		{
		public:
			[ExtensionAttribute]
			array<Byte>^ ToTlg6(Bitmap^ bmp);
			[ExtensionAttribute]
			array<Byte>^ ToTlg5(Bitmap^ bmp);
		};
	}

}
