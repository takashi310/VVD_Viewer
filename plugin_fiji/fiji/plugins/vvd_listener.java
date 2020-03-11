import ij.*;
import ij.gui.*;
import ij.plugin.PlugIn;
import ij.io.*;
import ij.process.*;

import java.lang.*;
import java.io.*;
import java.nio.*;
import java.nio.charset.*;
import java.net.*;
import java.awt.*;
import ij.macro.Interpreter;

import org.apache.commons.lang3.RandomStringUtils;


public class vvd_listener implements PlugIn {

	final public String version = "1.23";
	final public byte IPC_EXEC = (byte)1;
	final public byte IPC_POKE = (byte)3;
	final public byte IPC_CONNECT = (byte)10;
	final public byte IPC_DISCONNECT = (byte)11;
	final public byte wxIPC_TEXT = (byte)1;
	final public byte wxIPC_PRIVATE = (byte)20;
	final public byte IPC_FAIL = (byte)9;
	
	static final String PID =
    java.lang.management.ManagementFactory.getRuntimeMXBean().getName().split("@")[0];

	public void sendInitMessage(String topic, DataOutputStream out) throws IOException {
		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			norder = ByteOrder.LITTLE_ENDIAN;
		}
		
		byte[] b, tb;
		b = topic.getBytes(Charset.forName("UTF-8"));
		ByteBuffer outsentence = ByteBuffer.allocate(1+4+b.length);
		outsentence.order(norder);
		outsentence.put(IPC_CONNECT);
		outsentence.putInt(b.length);
		outsentence.put(b);
		outsentence.flip();
		tb = outsentence.array();
		out.write(tb, 0, tb.length);
		out.flush();
	}

	public void setServerTimeout(int sec, DataOutputStream out) throws IOException {
		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			norder = ByteOrder.LITTLE_ENDIAN;
		}
		
		byte[] b, b2, tb;
		b = "settimeout".getBytes(Charset.forName("UTF-8"));
		ByteBuffer outsentence = ByteBuffer.allocate(1+4+b.length+1+4+4);
		outsentence.order(norder);
		outsentence.put(IPC_POKE);
		outsentence.putInt(b.length);
		outsentence.put(b);
		outsentence.put(wxIPC_PRIVATE);
		outsentence.putInt(4);
		outsentence.putInt(sec);
		outsentence.flip();
		tb = outsentence.array();
		out.write(tb, 0, tb.length);
		out.flush();
	}

	public void setServerRcvBufSize(int size, DataOutputStream out) throws IOException {
		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			norder = ByteOrder.LITTLE_ENDIAN;
		}
		
		byte[] b, b2, tb;
		b = "setrcvbufsize".getBytes(Charset.forName("UTF-8"));
		ByteBuffer outsentence = ByteBuffer.allocate(1+4+b.length+1+4+4);
		outsentence.order(norder);
		outsentence.put(IPC_POKE);
		outsentence.putInt(b.length);
		outsentence.put(b);
		outsentence.put(wxIPC_PRIVATE);
		outsentence.putInt(4);
		outsentence.putInt(size);
		outsentence.flip();
		tb = outsentence.array();
		out.write(tb, 0, tb.length);
		out.flush();
	}

	public void sendTextMessage(String item_name, String message, DataOutputStream out) throws IOException {
		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			norder = ByteOrder.LITTLE_ENDIAN;
		}
		
		byte[] b, b2, tb;
		b = item_name.getBytes(Charset.forName("UTF-8"));
		b2 = message.getBytes(Charset.forName("UTF-8"));
		ByteBuffer outsentence = ByteBuffer.allocate(1+4+b.length+1+4+b2.length);
		outsentence.order(norder);
		outsentence.put(IPC_POKE);
		outsentence.putInt(b.length);
		outsentence.put(b);
		outsentence.put(wxIPC_TEXT);
		outsentence.putInt(b2.length);
		outsentence.put(b2);
		outsentence.flip();
		tb = outsentence.array();
		out.write(tb, 0, tb.length);
		out.flush();
	}

	public int getImageSizeByte(ImagePlus imp) {
		int[] dims = imp.getDimensions();
		int imageW = dims[0];
		int imageH = dims[1];
		int nCh    = dims[2];
		int imageD = dims[3];
		int nFrame = dims[4];
		int bdepth = imp.getBitDepth();
		int imagesize = imageW*imageH*imageD*nCh*(bdepth/8);

		return imagesize;
	}

	public void sendImage(ImagePlus imp, DataOutputStream out, DataInputStream in) throws IOException {
		if (imp == null) return;
		
		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			norder = ByteOrder.LITTLE_ENDIAN;
		}

		//setServerTimeout(1, out);

		FileInfo finfo = imp.getFileInfo();
		if(finfo == null) return;

		int[] dims = imp.getDimensions();
		int imageW = dims[0];
		int imageH = dims[1];
		int nCh    = dims[2];
		int imageD = dims[3];
		int nFrame = dims[4];
		int bdepth = imp.getBitDepth();
		int imagesize = imageW*imageH*imageD*(bdepth/8);
		double xspc = finfo.pixelWidth;
		double yspc = finfo.pixelHeight;
		double zspc = finfo.pixelDepth;

		if (bdepth == 24)
			return;
		
		LUT[] luts = imp.getLuts();

		ImageStack stack = imp.getStack();

		byte[] b, b2, tb;
		
		for(int ch = 0; ch < nCh; ch++){
			b = "volume".getBytes(Charset.forName("UTF-8"));
			if (nCh > 1) b2 = (imp.getTitle()+"_Ch"+ch+"\0").getBytes(Charset.forName("UTF-8"));
			else b2 = (imp.getTitle()+"\0").getBytes(Charset.forName("UTF-8"));
			ImageProcessor tp = stack.getProcessor(imp.getStackIndex(ch+1, 1, 1));
			Color col;
			if (ch < luts.length) col = new Color(luts[ch].getRGB(255));
			else col = new Color(255, 255, 255);
			String label = stack.getSliceLabel(imp.getStackIndex(ch+1, 1, 1));
			if (label != null) {
				if      (label.equals("Red"))   col = new Color(255, 0, 0);
				else if (label.equals("Green")) col = new Color(0, 255, 0);
				else if (label.equals("Blue"))  col = new Color(0, 0, 255);
			}

			IJ.log(""+col.getRed()+" "+col.getGreen()+" "+col.getBlue()+" "+bdepth);

			int margin = 0;
			int bufsize = 1+4+b.length+1+4+4+b2.length+4+4+4+4+4+4+4+8+8+8+imagesize+margin;
			int datasize = 4+b2.length+4+4+4+4+4+4+4+8+8+8+imagesize;
			ByteBuffer outsentence = ByteBuffer.allocate(bufsize);

			outsentence.order(norder);
			outsentence.put(IPC_POKE);
			outsentence.putInt(b.length);
			outsentence.put(b);
			outsentence.put(wxIPC_PRIVATE);
			outsentence.putInt(datasize);
			outsentence.putInt(b2.length);
			outsentence.put(b2);
			outsentence.putInt(imageW);
			outsentence.putInt(imageH);
			outsentence.putInt(imageD);
			outsentence.putInt(bdepth);
			outsentence.putInt(col.getRed());
			outsentence.putInt(col.getGreen());
			outsentence.putInt(col.getBlue());
			outsentence.putDouble(xspc);
			outsentence.putDouble(yspc);
			outsentence.putDouble(zspc);
			
			for(int s = 0; s < imageD; s++) {
				ImageProcessor ip = stack.getProcessor(imp.getStackIndex(ch+1, s+1, 1));
				if (bdepth == 8) {
					byte[] data = (byte[])ip.getPixels();
					outsentence.put(data);
				} else if (bdepth == 16) {
					short[] data = (short[])ip.getPixels();
					if (norder == ByteOrder.BIG_ENDIAN) {
						outsentence.asShortBuffer().put(data);
					} else {
						for(short e : data)outsentence.putShort(e);
					}
				}
			}

			for(int n=0; n<margin; n++) outsentence.put((byte)0);
			
			outsentence.flip();

			IJ.log("Sending...");
			
			tb = outsentence.array();
			out.write(tb, 0, tb.length);
			out.flush();

			IJ.log("Volume "+imageW+" "+imageH+" "+imageD);
		}
		//setServerTimeout(1, out);
	}

	public ImagePlus LoadReceivedImage(byte[] data)
	{
		ByteBuffer inbuf = ByteBuffer.wrap(data);

		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			norder = ByteOrder.LITTLE_ENDIAN;
		}
		
		inbuf.order(norder);

		int namelen = inbuf.getInt();
		byte[] namebuf = new byte[namelen];
		inbuf.get(namebuf);
		String name = new String(namebuf, Charset.forName("UTF-8"));
		int width = inbuf.getInt();
		int height = inbuf.getInt();
		int depth = inbuf.getInt();
		int bdepth = inbuf.getInt();
		int r = inbuf.getInt();
		int g = inbuf.getInt();
		int b = inbuf.getInt();
		double xspc = inbuf.getDouble();
		double yspc = inbuf.getDouble();
		double zspc = inbuf.getDouble();

		int imagesize = width*height*depth*(bdepth/8);

		IJ.log("RECEIVED: "+name);
		
		ImagePlus rcvimp = null;
		if (bdepth == 8)
			rcvimp = NewImage.createByteImage(name, width, height, depth, NewImage.FILL_BLACK);
		else if (bdepth == 16)
			rcvimp = NewImage.createShortImage(name, width, height, depth, NewImage.FILL_BLACK);
		else if (bdepth == 32)
			rcvimp = NewImage.createFloatImage(name, width, height, depth, NewImage.FILL_BLACK);

		if (rcvimp == null)
			return null;

		FileInfo finfo_r = rcvimp.getFileInfo();
		finfo_r.pixelWidth = xspc;
		finfo_r.pixelHeight = yspc;
		finfo_r.pixelDepth = zspc;
		rcvimp.setFileInfo(finfo_r);

		rcvimp.show();
		IJ.run("Properties...", "unit=micron pixel_width="+xspc+" pixel_height="+yspc+" voxel_depth="+zspc);
		IJ.log("xspc: "+xspc+"  yspc: "+yspc+"  zspc: "+zspc);
		
		try {
			Color col = new Color(r,g,b);
			rcvimp.setLut(LUT.createLutFromColor(col));
			rcvimp.setColor(col);
		} catch (IllegalArgumentException e) {
			Color col = new Color(255,255,255);
			rcvimp.setLut(LUT.createLutFromColor(col));
			rcvimp.setColor(col);
			IJ.log("IllegalArgumentException (Color)");
		}

		ImageStack stack = rcvimp.getStack();
		if (bdepth == 8) {
			for (int s = 1; s <= depth; s++){
				ImageProcessor ips = stack.getProcessor(s);
				byte[] slicebuf = new byte[width*height];
				inbuf.get(slicebuf);
				ips.setPixels(slicebuf);
				stack.setProcessor(ips, s);
			}
		} else if (bdepth == 16) {
			ShortBuffer sinbuf = inbuf.asShortBuffer();
			for (int s = 1; s <= depth; s++){
				ImageProcessor ips = stack.getProcessor(s);
				short[] slicebuf = new short[width*height];
				sinbuf.get(slicebuf);
				ips.setPixels(slicebuf);
				stack.setProcessor(ips, s);
			}
		} else if (bdepth == 32) {
			FloatBuffer finbuf = inbuf.asFloatBuffer();
			for (int s = 1; s <= depth; s++){
				ImageProcessor ips = stack.getProcessor(s);
				float[] slicebuf = new float[width*height];
				finbuf.get(slicebuf);
				ips.setPixels(slicebuf);
				stack.setProcessor(ips, s);
			}
		}

		IJ.log("LOADED: "+name);

		return rcvimp;
	}

	@Override
	public void run(String arg) {
		IJ.getInstance().setAlwaysOnTop(true); 
		long t0 = System.currentTimeMillis();
		String topic = "FijiVVDPlugin";
		String item = "TestTEXT";
		String pass = RandomStringUtils.randomAlphabetic(10);
		
		ByteOrder norder;
		if (ByteOrder.nativeOrder().equals(ByteOrder.BIG_ENDIAN)) {
			IJ.log("Big-endian");
			norder = ByteOrder.BIG_ENDIAN;
		} else {
			IJ.log("Little-endian");
			norder = ByteOrder.LITTLE_ENDIAN;
		}
		
		try {
			Socket clientSocket = new Socket("localhost", 8002);
			clientSocket.setReceiveBufferSize(100*1024*1024);
			clientSocket.setSendBufferSize(100*1024*1024);
			byte type;
			
			DataInputStream inFromServer = new DataInputStream(clientSocket.getInputStream());
			DataOutputStream outToServer = new DataOutputStream(clientSocket.getOutputStream());

			sendInitMessage(topic, outToServer);

			byte con = inFromServer.readByte();
			IJ.log("Connected "+ con);

			setServerTimeout(10, outToServer);
			
			//send plugin version
			sendTextMessage("version", version+"\0", outToServer);
			IJ.log("Version Check");

			//send PID
			sendTextMessage("pid", PID+"\0", outToServer);
			IJ.log("PID: "+PID);
			
			//send single instance confirmation
			sendTextMessage("confirm", pass, outToServer);
			IJ.log("Single Instance Check");

			byte format;
			int size, size2;
			byte[] intbuf = new byte[4];
			while (true) {	
				type = inFromServer.readByte();

				if (type == IPC_FAIL) {
					IJ.log("IPC_FAIL");
					sendTextMessage("confirm", pass, outToServer);
					continue;
				}

				if (type == IPC_CONNECT) {
					IJ.log(""+(int)type);
					continue;
				}
		
				if (type != IPC_POKE) {
					IJ.log("INVALID SIGNAL: "+(int)type);
					break;
				}
				
				inFromServer.read(intbuf);
				if (norder == ByteOrder.LITTLE_ENDIAN)
					size = (intbuf[0] & 0xFF) | (intbuf[1] & 0xFF) << 8 | (intbuf[2] & 0xFF) << 16 | (intbuf[3] & 0xFF) << 24;
				else
					size = (intbuf[0] & 0xFF << 24) | (intbuf[1] & 0xFF) << 16 | (intbuf[2] & 0xFF) << 8 | (intbuf[3] & 0xFF);
				IJ.log("size: "+size);
				
				byte[] data = new byte[size];
				String str = "";
				int count;
				int sum = 0;
				while (sum < size && (count = inFromServer.read(data)) > 0) {
					String s = new String(data, 0, count, Charset.forName("UTF-8"));
					str += s; sum += count;
				}
				IJ.log(str);
				
				format = inFromServer.readByte();
				IJ.log("format: "+format);
				
				inFromServer.read(intbuf);
				if (norder == ByteOrder.LITTLE_ENDIAN)
					size2 = (intbuf[0] & 0xFF) | (intbuf[1] & 0xFF) << 8 | (intbuf[2] & 0xFF) << 16 | (intbuf[3] & 0xFF) << 24;
				else
					size2 = (intbuf[0] & 0xFF << 24) | (intbuf[1] & 0xFF) << 16 | (intbuf[2] & 0xFF) << 8 | (intbuf[3] & 0xFF);
				IJ.log("size2: "+size2);
				
				byte[] data2 = new byte[size2];
				int tmpbufsize = size2 > 100*1024*1024 ? 100*1024*1024 : size2;
				byte[] rcvbuf = new byte[tmpbufsize];
				ByteBuffer inbuf = ByteBuffer.wrap(data2);
				sum = 0;
				int loopcnt = 0;
				while (sum < size2 && (count = inFromServer.read(rcvbuf)) > 0) {
					inbuf.put(rcvbuf, 0, count);
					sum += count; loopcnt++;
				}
				IJ.log("read_num: "+sum+"  loop_count: "+loopcnt);

				if (str.equals("confirm")) {
					String pass_server = new String(data2, Charset.forName("UTF-8"));
					IJ.log(pass+"  "+pass_server);
					if (!pass_server.equals(pass)) {
						IJ.log("dupe");
						break;
					} else {
						IJ.log("OK");
					}
				} else if (str.equals("setrcvbufsize") && size2 == 4) {
					int bufsize = 0;
					if (norder == ByteOrder.LITTLE_ENDIAN)
						bufsize = (data2[0] & 0xFF) | (data2[1] & 0xFF) << 8 | (data2[2] & 0xFF) << 16 | (data2[3] & 0xFF) << 24;
					else
						bufsize = (data2[0] & 0xFF << 24) | (data2[1] & 0xFF) << 16 | (data2[2] & 0xFF) << 8 | (data2[3] & 0xFF);
					clientSocket.setReceiveBufferSize(bufsize);
					IJ.log("RcvBufSize; "+bufsize);
				} else if (str.equals("volume")) {
					ImagePlus rcvimp = LoadReceivedImage(data2);
					rcvimp.show();
				} else if (str.equals("com")) {
					String command = new String(data2, Charset.forName("UTF-8"));
					IJ.log("Command; "+command);
					String ret = IJ.runMacroFile("vvd_run.txt", command);
					if (ret == null || !ret.equals("[aborted]")) {
						IJ.log("command finished");
						ImagePlus imp = WindowManager.getCurrentImage();
						if (imp != null) {
							int bd = imp.getBitDepth();
							if (bd == 24)
								IJ.runMacro("run(\"RGB Stack\");");
							if (bd == 32)
								IJ.runMacro("run(\"16-bit\");");
							imp = WindowManager.getCurrentImage();
							int isize = getImageSizeByte(imp);
							clientSocket.setSendBufferSize(isize+10*1024*1024);
							sendImage(imp, outToServer, inFromServer);
						}
						else IJ.log("There is no active image.");
					}
					else IJ.log("command aborted");
					
					IJ.runMacro("run(\"Close All\");");
					IJ.freeMemory();
					sendTextMessage("com_finish", command+"\0", outToServer);
					IJ.log("done");
				} else {
					IJ.log("Skip");
					byte[] b = str.getBytes(Charset.forName("UTF-8"));
					String strdata = "";
					for (int i=0; i<b.length; i++)
						strdata = strdata + " " + b[i];
					IJ.log(strdata);
				}
			}
			outToServer.writeByte(IPC_DISCONNECT);
			clientSocket.close();
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
	}
}