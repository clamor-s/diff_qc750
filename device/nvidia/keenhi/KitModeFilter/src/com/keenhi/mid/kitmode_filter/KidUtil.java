package com.keenhi.mid.kitmode_filter;

import android.os.SystemProperties;
/**
 * created by densy.
 * @hide
 */
public class KidUtil {
  public final static int EXTRA_MODE_INVALID =0;
  public final static int EXTRA_MODE_KID =1;
  public final static int EXTRA_MODE_ANDROID =2;
  
  public final static String KID_SERVICE_MODE_PROPERTY ="sys.keenhi.kit_mode";
  public final static String KIT_SERVICE_DEFAULT_MODE_PROPERTY ="persist.sys.kh.def_is_parent";
  
  public final static String PRODUCT_MODE_PROPERTY = "sys.kh.production_mode";
  
  public static boolean isKidMode(){
    return (SystemProperties.getInt(KID_SERVICE_MODE_PROPERTY, EXTRA_MODE_ANDROID) !=EXTRA_MODE_ANDROID &&
            !SystemProperties.getBoolean(PRODUCT_MODE_PROPERTY, false));
  }
  
  public static boolean defaultIsParentMode(){
      return SystemProperties.getBoolean(KIT_SERVICE_DEFAULT_MODE_PROPERTY, false);
    }
}
