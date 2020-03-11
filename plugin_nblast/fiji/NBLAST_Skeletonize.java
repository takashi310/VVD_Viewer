import ij.*;
import ij.process.*;
import ij.gui.*;
import ij.io.*;

import ij.plugin.PlugIn;
import ij.plugin.filter.*;

public class NBLAST_Skeletonize implements PlugInFilter 
{
  ImagePlus imp;
  static final int NGRAY=256;
  

  public int setup(String arg, ImagePlus imp) 
  {
    this.imp = imp;
    return DOES_8G + NO_CHANGES;
  }

  public void run(ImageProcessor ip) 
  {
    int[] dims = imp.getDimensions();
	int width  = dims[0];
	int height = dims[1];
	int nCh    = dims[2];
	int depth  = dims[3];
	int nFrame = dims[4];

	int target_size = 512*256*128;
	double scale_limit = 1.0/3.0;

	double scalefac = (double)target_size/(double)(width*height*depth);
	scalefac = Math.pow(scalefac, 1.0/3.0);
	IJ.log(""+scalefac);
	if (scalefac < scale_limit)
		scalefac = scale_limit;
	if (scalefac > 1.0)
		scalefac = 1.0;
	
	String newtitle = imp.getTitle() + ".skeleton";
	if (scalefac < 1.0 && scalefac > 0.0)
	{
		IJ.run(imp, "Scale...", "x="+scalefac+" y="+scalefac+" z="+scalefac+" interpolation=Bilinear average processcreate title="+newtitle);
		IJ.selectWindow(newtitle);
	}
	IJ.setThreshold(64.0, 255.0, "Black & White");
	IJ.run("Convert to Mask", "method=Default background=Dark black");
	IJ.run("Skeletonize (2D/3D)", "");

  }

}
