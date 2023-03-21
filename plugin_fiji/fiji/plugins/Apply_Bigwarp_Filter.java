import java.io.File;
import ij.io.FileInfo;
import java.io.IOException;
import bdv.ij.ApplyBigwarpPlugin;
import bdv.viewer.Interpolation;
import bigwarp.landmarks.LandmarkTableModel;
import net.imglib2.realtransform.BoundingBoxEstimation;
import ij.IJ;
import ij.ImagePlus;
import ij.ImageStack;
import ij.process.ImageProcessor;
import ij.plugin.PlugIn;
import ij.plugin.filter.*;
import ij.gui.*;
import java.util.*;

 
public class Apply_Bigwarp_Filter implements PlugInFilter {
	
	private String[] transform_type_  = { "Thin Plate Spline", "Affine", "Similarity", "Rotation", "Translation" };
	private String[] interpolation_type_  = { "Linear", "Nearest Neighbor" };
	
	private String landmarksPath = "";
	private String movingPath = "";
	private String resultPath = "";
	private int sizeX = 512;
	private int sizeY = 512;
	private int sizeZ = 512;
	private double resX = 1.0;
	private double resY = 1.0;
	private double resZ = 1.0;
	private double offsetX = 0.0;
	private double offsetY = 0.0;
	private double offsetZ = 0.0;
	private int transformType = 0;
	private int interpType = 0;
	private int nThreads = 1;
	
	ImagePlus imp;
	static final int NGRAY=256;
  

	public int setup(String arg, ImagePlus imp) 
	{
		this.imp = imp;
		return DOES_8G + DOES_16 + NO_CHANGES;
	}
	
	private boolean showDialog() {
		GenericDialog gd = new GenericDialog("Apply Bigwarp");
		gd.addStringField("Landmark_file", "");
		gd.addNumericField("x_size",  sizeX, 0);
		gd.addNumericField("y_size",  sizeY, 0);
		gd.addNumericField("z_size",  sizeZ, 0);
		gd.addNumericField("x_spacing",  resX, 5);
		gd.addNumericField("y_spacing",  resY, 5);
		gd.addNumericField("z_spacing",  resZ, 5);
		gd.addNumericField("x_offset",  0, 0);
		gd.addNumericField("y_offset",  0, 0);
		gd.addNumericField("z_offset",  0, 0);
		gd.addChoice("Transform_type", transform_type_, transform_type_[0]);
		gd.addChoice("Interpolation", interpolation_type_, interpolation_type_[0]);
		gd.addNumericField("Threads",  nThreads, 0);
		gd.showDialog();
	
		if (gd.wasCanceled()) return false;
		landmarksPath = gd.getNextString();
		sizeX = (int)gd.getNextNumber();
		sizeY = (int)gd.getNextNumber();
		sizeZ = (int)gd.getNextNumber();
		resX = (double)gd.getNextNumber();
		resY = (double)gd.getNextNumber();
		resZ = (double)gd.getNextNumber();
		offsetX = (double)gd.getNextNumber();
		offsetY = (double)gd.getNextNumber();
		offsetZ = (double)gd.getNextNumber();
		transformType = gd.getNextChoiceIndex();
		interpType = gd.getNextChoiceIndex();
		nThreads = (int)gd.getNextNumber();
		
		return true;
	}

	@Override
	public void run(ImageProcessor ip) {
		
		FileInfo finfo = imp.getFileInfo();
		if(finfo == null) return;

		int[] dims = imp.getDimensions();
		int w = dims[0];
		int h = dims[1];
		int nCh    = dims[2];
		int d = dims[3];
		int nFrame = dims[4];
		int bdepth = imp.getBitDepth();
		
		sizeX = w;
		sizeY = h;
		sizeZ = d;
		
		resX = finfo.pixelWidth;
		resY = finfo.pixelHeight;
		resZ = finfo.pixelDepth;

		if (!showDialog()) return;
		ImagePlus movingIp = imp;
		
		int nd = 2;
		if ( movingIp.getNSlices() > 1 )
			nd = 3;

		LandmarkTableModel ltm = new LandmarkTableModel( nd );
		try {
			ltm.load( new File(landmarksPath) );
		} catch ( IOException e ) {
			e.printStackTrace();
			return;
		}
		
		IJ.log("nThreads "+nThreads);

		double[] res = new double[3];
		res[0] = resX;
		res[1] = resY;
		res[2] = resZ;
		
		double[] fov = new double[3];
		fov[0] = sizeX;
		fov[1] = sizeY;
		fov[2] = sizeZ;
		
		double[] offsetSpec = new double[3];
		offsetSpec[0] = offsetX;
		offsetSpec[1] = offsetY;
		offsetSpec[2] = offsetZ;

		Interpolation interp = Interpolation.NLINEAR;
		if( interpType == 1 )
			interp = Interpolation.NEARESTNEIGHBOR;
		
		final BoundingBoxEstimation bboxEst = new BoundingBoxEstimation(BoundingBoxEstimation.Method.CORNERS);
		ApplyBigwarpPlugin.WriteDestinationOptions emptyWriteOpts = new ApplyBigwarpPlugin.WriteDestinationOptions( "", "", null, null );
		List<ImagePlus> warpedIpList = ApplyBigwarpPlugin.apply(
			movingIp, movingIp, ltm, transform_type_[transformType],
			ApplyBigwarpPlugin.SPECIFIED_PIXEL, "", bboxEst, ApplyBigwarpPlugin.SPECIFIED,
			res, fov, offsetSpec,
			interp, false, nThreads, true, emptyWriteOpts );

		ImagePlus outimp = warpedIpList.get(0);
		int[] out_dims = outimp.getDimensions();
		int w2 = out_dims[0];
		int h2 = out_dims[1];
		int d2 = out_dims[3];
		
		if (w2 > w) w2 = w;
		if (h2 > h) h2 = h;
		if (d2 > d) d2 = d;
		
		if (w != w2 || h != h2 || d != d2)
		{
			ImageStack ostack = outimp.getStack();
			final ImageProcessor[] oiplist = new ImageProcessor[d2];
			for(int s = 0; s < d2; s++)
				oiplist[s] =  ostack.getProcessor(s+1);
			
			ImagePlus rimp = NewImage.createImage(outimp.getTitle(), w, h, d, bdepth, NewImage.FILL_BLACK);
			ImageStack rstack = rimp.getStack();
			final ImageProcessor[] riplist = new ImageProcessor[d];
			for(int s = 0; s < d; s++)
				riplist[s] =  rstack.getProcessor(s+1);
			int i_offsetX = (int)(offsetX + 0.5);
			int i_offsetY = (int)(offsetY + 0.5);
			int i_offsetZ = (int)(offsetZ + 0.5);
			for(int s = 0; s < d2; s++) {
				for(int y = 0; y < h2; y++) {
					for(int x = 0; x < w2; x++) {
						riplist[s+i_offsetZ].set(x+i_offsetX, y+i_offsetY, oiplist[s].get(x, y));
					}
				}
			}
			IJ.run(rimp, "Properties...", "unit=microns pixel_width="+resX+" pixel_height="+resY+" voxel_depth="+resZ);
			rimp.show();
		}
		else
			outimp.show();
	}
}
