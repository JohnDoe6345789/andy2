
package bbl.intl.bambulab.com;

import android.app.Application;
import android.content.Context;
import android.util.Log;
import moc.balubmab.ltni.lbb.a;

public class BambuLabApplication extends Application {
    private static final String TAG = "BambuLabApplication";
    private static BambuLabApplication instance;
    
    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        
        Log.d(TAG, "BambuLabApplication onCreate");
        
        // Initialize security checks
        try {
            // Call the obfuscated security check
            if (a.a(this)) {
                Log.d(TAG, "Security check passed");
            } else {
                Log.w(TAG, "Security check failed");
            }
            
            // Initialize other components
            initializeComponents();
            
        } catch (Throwable e) {
            Log.e(TAG, "Error during application initialization", e);
        }
    }
    
    private void initializeComponents() {
        // Initialize crash reporting if enabled
        if (moc.balubmab.ltni.lbb.Configuration.ENABLE_CRASH_REPORT) {
            Log.d(TAG, "Crash reporting enabled");
        }
        
        // Initialize PT if enabled
        if (moc.balubmab.ltni.lbb.Configuration.ENABLE_PT) {
            Log.d(TAG, "PT enabled");
        }
        
        Log.d(TAG, "Application components initialized");
    }
    
    public static BambuLabApplication getInstance() {
        return instance;
    }
    
    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        
        // Perform early initialization
        try {
            // Set up library loading
            a.b(); // Call the Android API warning suppression
        } catch (Throwable e) {
            Log.e(TAG, "Error in attachBaseContext", e);
        }
    }
}
