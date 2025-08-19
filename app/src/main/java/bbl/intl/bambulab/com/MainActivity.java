
package bbl.intl.bambulab.com;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.util.Log;
import com.bambulab.iotc.IOTCManager;
import com.bambulab.iotc.IOTCNative;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";
    private IOTCManager iotcManager;
    private TextView statusText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Set up basic UI
        statusText = new TextView(this);
        statusText.setText("Bambu Lab IOTC Client\nInitializing...");
        statusText.setPadding(50, 50, 50, 50);
        setContentView(statusText);
        
        Log.d(TAG, "MainActivity onCreate");
        
        // Initialize IOTC
        initializeIOTC();
    }
    
    private void initializeIOTC() {
        try {
            // Load native library
            IOTCNative.loadLibrary();
            
            // Initialize IOTC manager
            iotcManager = new IOTCManager();
            int result = iotcManager.initialize();
            
            if (result >= 0) {
                statusText.setText("Bambu Lab IOTC Client\nIOTC Initialized Successfully\nTap to scan for devices");
                statusText.setOnClickListener(v -> openDeviceList());
                Log.d(TAG, "IOTC initialized successfully");
            } else {
                statusText.setText("Bambu Lab IOTC Client\nIOTC Initialization Failed\nError: " + result);
                Log.e(TAG, "IOTC initialization failed: " + result);
            }
        } catch (Exception e) {
            statusText.setText("Bambu Lab IOTC Client\nException: " + e.getMessage());
            Log.e(TAG, "Error initializing IOTC", e);
        }
    }
    
    private void openDeviceList() {
        android.content.Intent intent = new android.content.Intent(this, DeviceListActivity.class);
        startActivity(intent);
    }
    
    private void startDeviceDiscovery() {
        new Thread(() -> {
            try {
                // Perform LAN search for devices
                String[] devices = iotcManager.lanSearch(5000); // 5 second timeout
                
                runOnUiThread(() -> {
                    StringBuilder sb = new StringBuilder("Bambu Lab IOTC Client\nIOTC Ready\n\nDevices Found:\n");
                    if (devices != null && devices.length > 0) {
                        for (String device : devices) {
                            sb.append("- ").append(device).append("\n");
                        }
                    } else {
                        sb.append("No devices found");
                    }
                    statusText.setText(sb.toString());
                });
                
            } catch (Exception e) {
                runOnUiThread(() -> {
                    statusText.setText("Bambu Lab IOTC Client\nDevice Discovery Error: " + e.getMessage());
                });
                Log.e(TAG, "Device discovery error", e);
            }
        }).start();
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (iotcManager != null) {
            iotcManager.deinitialize();
        }
        Log.d(TAG, "MainActivity onDestroy");
    }
}
