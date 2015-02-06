using System;
using System.Net;
using System.Net.Sockets;

class ServerMain
{
	public static void Main(string[] args)
	{
		ServerMain sm = new ServerMain();
		sm.Run(args);
	}

	public void OnTurboInfo(netki.CubeTurboInfo info)
	{
		Console.WriteLine("Cube: turbo version is ["+ info.Version + "]");
	}

	public void OnTurboClientConnect(netki.CubeClientConnect connect)
	{
		Console.WriteLine("Cube: client connected with id " + connect.ConnectionId);
	}

	public void OnTurboClientDisconnect(netki.CubeClientDisconnect connect)
	{
		Console.WriteLine("Cube: client disconnected with id " + connect.ConnectionId);
	}

	public void OnTurboClientData(netki.CubeClientData data)
	{
		Console.WriteLine("Cube: client id got data " + data.ConnectionId + " (" + data.Data.Length + ")");
	}

	public void Run(string[] args)
	{
		string sockfile = "/tmp/" + System.IO.Path.GetRandomFileName();
		Console.WriteLine("Waiting for turbo at [" + sockfile + "]");
	
		Socket s = new Socket(AddressFamily.Unix, SocketType.Stream, ProtocolType.Unspecified);
		s.Bind(new Mono.Unix.UnixEndPoint(sockfile));
		s.Listen(1);

		System.Diagnostics.Process.Start("/tmp/turbonet", sockfile);

		Socket conn = s.Accept();
		Console.WriteLine("Cube connected to turbo process.");

		byte[] buf = new byte[256 * 1024];
		int readpos = 0, parsepos = 0;

		netki.Bitstream.Buffer bufwrap = new netki.Bitstream.Buffer();
		bufwrap.buf = buf;

		while (true)
		{
			int rd = conn.Receive(buf, readpos, buf.Length - readpos, SocketFlags.None);
			if (rd <= 0)
			{
				Console.WriteLine("Socket closed!");
				break;
			}

			readpos += rd;

			// Start parsing from the start.
			bufwrap.bitpos = 0;
			bufwrap.bytepos = parsepos;
			bufwrap.error = 0;
			bufwrap.bufsize = readpos;

			int parsed = 0;
			while (true)
			{
				if (bufwrap.bufsize - bufwrap.bytepos < 2)
					break;

				System.UInt32 id = netki.Bitstream.ReadBits(bufwrap, 16);
				if (bufwrap.error != 0)
				{
					Console.WriteLine("read id error");
					break;
				}

				bool decoded = false;
				switch (id)
				{
					case netki.CubeTurboInfo.TYPE_ID:
						{
							netki.CubeTurboInfo res = new netki.CubeTurboInfo();
							if (netki.CubeTurboInfo.ReadFromBitstream(bufwrap, res))
							{
								OnTurboInfo(res);
								decoded = true;
							}
							break;
						}
					case netki.CubeClientConnect.TYPE_ID:
						{
							netki.CubeClientConnect res = new netki.CubeClientConnect();
							if (netki.CubeClientConnect.ReadFromBitstream(bufwrap, res))
							{
								OnTurboClientConnect(res);
								decoded = true;
							}
							break;
						}
					case netki.CubeClientDisconnect.TYPE_ID:
						{
							netki.CubeClientDisconnect res = new netki.CubeClientDisconnect();
							if (netki.CubeClientDisconnect.ReadFromBitstream(bufwrap, res))
							{
								OnTurboClientDisconnect(res);
								decoded = true;
							}
							break;
						}
					case netki.CubeClientData.TYPE_ID:
						{
							netki.CubeClientData res = new netki.CubeClientData();
							if (netki.CubeClientData.ReadFromBitstream(bufwrap, res))
							{
								OnTurboClientData(res);
								decoded = true;
							}
							break;
						}
					default:
						Console.WriteLine("Turbo: unknown packet id " + id);
						break;
				}

				if (decoded)
				{
					netki.Bitstream.SyncByte(bufwrap);
					parsed = bufwrap.bytepos;
				}
				else
				{
					break;
				}
			}

			if (parsed > 0)
			{
				parsepos = parsed;
			}

			if (readpos == buf.Length)
			{
				Console.WriteLine("Turbo: read buffer exhausted!");
				break;
			}

			if (readpos == parsepos)
			{
				readpos = 0;
				parsepos = 0;
			}
			else if (parsepos > 65536)
			{
				for (int i=parsepos;i<readpos;i++)
					buf[i-parsepos] = buf[i];
				readpos -= parsepos;
				parsepos = 0;
			}
		}
	}
}

