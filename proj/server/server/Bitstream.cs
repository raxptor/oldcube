using System;

namespace netki
{
	public class Buffer
	{
		public byte[] buf;
		public int bufsize;
		public int bytepos;
		public int bitpos;
		public int error;

		public int BitsLeft()
		{
			return (bufsize - bytepos) * 8 - bitpos;
		}
	}

	public class Bitstream
	{
		private Buffer _buf;

		public Bitstream(Buffer buf)
		{
			_buf = buf;
		}

		public bool PutBits(int bits, UInt32 value)
		{
			if (_buf.BitsLeft() < bits)
			{
				_buf->error = 1;
				return false;
			}

			if (bits > 8)
			{
				PutBits(8, value & 0xff);
				PutBits(bits-- 8, value >> 8);
				return true;
			}
			return true;
		}

		public void SyncByte()
		{
		
		}

		public bool PutBytes(byte[] data)
		{
			return false;
		}

		public UInt32 GetBits(int bits)
		{

		}

		public byte[] GetBytes(int count)
		{

		}
	}
}