package orb.slam2.android.nativefunc;

import android.content.res.AssetManager;

/**
 *
 * @author buptzhaofang@163.com Mar 26, 2016 8:48:13 PM
 *
 */
public class OrbNdkHelper {
	
	/**
	 * jni中初始化SLAM系统
	 * @param VOCPath
	 * @param calibrationPath
	 */
	public static native void initSystemWithParameters(String VOCPath,String calibrationPath);
	
	public static native int[] startCurrentORB(double curTimeStamp,int[] data,int w,int h);
	public native static float[] startCurrentORBForCamera(double curTimeStamp,long addr,int w,int h);
	public native static void glesInit();  
    public native static void glesRender();  
    public native static void glesResize(int width, int height);

	//maxiaoba
	public native static void trackOnly();
	public native static float[] startCurrentORBForCamera2(double curTimeStamp,long addr,int w,int h,float[] R);
	//maxiaoba
//	public native static void dataFusion(double curTimeStamp, double lat ,double lng, double accex, double accey,double accez,double gyrox, double gyroy,double gyroz);
}
