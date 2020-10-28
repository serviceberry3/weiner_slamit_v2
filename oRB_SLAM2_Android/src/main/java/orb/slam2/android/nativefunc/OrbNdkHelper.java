package orb.slam2.android.nativefunc;

import android.content.res.AssetManager;

import org.opencv.core.Mat;

/**
 *
 * @author buptzhaofang@163.com Mar 26, 2016 8:48:13 PM
 * 			MOD noah.weiner@yale.edu Oct 27, 2020 20:23 PM
 *
 */
public class OrbNdkHelper {
	
	//initialize the ORB SLAM system
	public static native void initSystemWithParameters(String VOCPath,String calibrationPath);
	
	public static native int[] startCurrentORB(double curTimeStamp, int[] data, int w, int h);
	public native static float[] startCurrentORBForCamera(double curTimeStamp,long addr,int w,int h);
	public native static void glesInit();  
    public native static void glesRender();  
    public native static void glesResize(int width, int height);


	public native static void trackOnly();


	public native static float[] startCurrentORBForCamera2(double curTimeStamp, long addr, int w, int h, float[] R);

	//get a double float points containing Posenet pts to be drawn
	public native static float[][] getRawPosenetPts(long addr);



	//public native static void dataFusion(double curTimeStamp, double lat ,double lng, double accex, double accey,double accez,double gyrox, double gyroy,double gyroz);
}
