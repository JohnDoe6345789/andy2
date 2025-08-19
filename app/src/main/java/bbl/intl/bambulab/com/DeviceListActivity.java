
package bbl.intl.bambulab.com;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;
import com.bambulab.iotc.IOTCManager;
import java.util.ArrayList;
import java.util.List;

public class DeviceListActivity extends Activity {
    private static final String TAG = "DeviceListActivity";
    
    private ListView deviceListView;
    private Button refreshButton;
    private TextView statusText;
    private ArrayAdapter<String> deviceAdapter;
    private List<String> deviceList;
    private IOTCManager iotcManager;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Create layout
        setContentView(createLayout());
        
        // Initialize IOTC manager
        iotcManager = new IOTCManager();
        int result = iotcManager.initialize();
        
        if (result < 0) {
            Toast.makeText(this, "Failed to initialize IOTC: " + result, Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
        // Initialize device list
        deviceList = new ArrayList<>();
        deviceAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, deviceList);
        deviceListView.setAdapter(deviceAdapter);
        
        // Set up list item click listener
        deviceListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String selectedDevice = deviceList.get(position);
                openCameraActivity(selectedDevice);
            }
        });
        
        // Perform initial scan
        refreshDevices();
        
        Log.d(TAG, "DeviceListActivity created");
    }
    
    private View createLayout() {
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        layout.setPadding(20, 20, 20, 20);
        
        // Title
        TextView titleText = new TextView(this);
        titleText.setText("Bambu Lab Printers");
        titleText.setTextSize(20);
        titleText.setPadding(0, 0, 0, 20);
        layout.addView(titleText);
        
        // Status text
        statusText = new TextView(this);
        statusText.setText("Scanning for devices...");
        statusText.setPadding(0, 0, 0, 10);
        layout.addView(statusText);
        
        // Refresh button
        refreshButton = new Button(this);
        refreshButton.setText("Refresh");
        refreshButton.setOnClickListener(v -> refreshDevices());
        layout.addView(refreshButton);
        
        // Device list
        deviceListView = new ListView(this);
        deviceListView.setLayoutParams(new android.widget.LinearLayout.LayoutParams(
            android.widget.LinearLayout.LayoutParams.MATCH_PARENT, 0, 1.0f));
        layout.addView(deviceListView);
        
        return layout;
    }
    
    private void refreshDevices() {
        refreshButton.setEnabled(false);
        statusText.setText("Scanning for devices...");
        
        new Thread(() -> {
            try {
                // Perform LAN search
                String[] devices = iotcManager.lanSearch(10000); // 10 second timeout
                
                runOnUiThread(() -> {
                    deviceList.clear();
                    
                    if (devices != null && devices.length > 0) {
                        for (String device : devices) {
                            deviceList.add(device);
                        }
                        statusText.setText("Found " + devices.length + " device(s)");
                    } else {
                        statusText.setText("No devices found");
                    }
                    
                    deviceAdapter.notifyDataSetChanged();
                    refreshButton.setEnabled(true);
                });
                
            } catch (Exception e) {
                runOnUiThread(() -> {
                    statusText.setText("Error scanning: " + e.getMessage());
                    refreshButton.setEnabled(true);
                });
                Log.e(TAG, "Error during device scan", e);
            }
        }).start();
    }
    
    private void openCameraActivity(String deviceInfo) {
        // Extract UID from device info (assuming format: "UID:IP:PORT")
        String[] parts = deviceInfo.split(":");
        if (parts.length > 0) {
            String deviceUID = parts[0];
            
            Intent intent = new Intent(this, CameraActivity.class);
            intent.putExtra("device_uid", deviceUID);
            startActivity(intent);
        } else {
            Toast.makeText(this, "Invalid device format", Toast.LENGTH_SHORT).show();
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (iotcManager != null) {
            iotcManager.deinitialize();
        }
    }
}
